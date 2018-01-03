/**
 * \file            esp_netconn.c
 * \brief           API functions for sequential calls
 */

/*
 * Copyright (c) 2018 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "esp/esp_netconn.h"
#include "esp/esp_private.h"
#include "esp/esp_conn.h"
#include "esp/esp_mem.h"

#if ESP_CFG_NETCONN || __DOXYGEN__

static uint8_t recv_closed = 0xFF;
static esp_netconn_t* listen_api;              /* Main connection in listening mode */

/**
 * \brief           Flush all mboxes and clear possible used memories
 * \param[in]       nc: Pointer to netconn to flush
 */
static void
flush_mboxes(esp_netconn_t* nc) {
    esp_pbuf_t* pbuf;
    esp_netconn_t* new_nc;
    if (esp_sys_sem_isvalid(&nc->mbox_receive)) {
        do {
            if (!esp_sys_mbox_getnow(&nc->mbox_receive, (void **)&pbuf)) {
                break;
            }
            if (pbuf && (uint8_t *)pbuf != (uint8_t *)&recv_closed) {
                esp_pbuf_free(pbuf);
            }
        } while (1);
    }
    if (esp_sys_sem_isvalid(&nc->mbox_accept)) {
        do {
            if (!esp_sys_mbox_getnow(&nc->mbox_accept, (void *)&new_nc)) {
                break;
            }
            if (new_nc) {
                esp_netconn_close(new_nc);      /* Close netconn connection */
            }
        } while (1);
    }
}

/**
 * \brief           Callback function for every server connection
 * \param[in]       cb: Pointer to callback structure
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
esp_cb(esp_cb_t* cb) {
    esp_conn_t* conn;
    esp_netconn_t* nc = NULL;
    uint8_t close = 0;
    
    conn = esp_conn_get_from_evt(cb);           /* Get connection from event */
    switch (cb->type) {
        /*
         * A new connection has been active
         * and should be handled by netconn API
         */
        case ESP_CB_CONN_ACTIVE: {              /* A new connection active is active */
            if (esp_conn_is_client(conn)) {     /* Was connection started by us? */
                nc = esp_conn_get_arg(conn);    /* Argument should be ready already */
                if (nc) {                       /* Is netconn set? Should be already by us */
                    nc->conn = conn;            /* Save actual connection */
                } else {
                    close = 1;                  /* Close this connection, invalid netconn */
                }
            } else if (esp_conn_is_server(conn) && listen_api != NULL) {    /* Is the connection server type and we have known listening API? */
                /*
                 * Create a new netconn structure
                 * and set it as connection argument.
                 */
                nc = esp_netconn_new(ESP_NETCONN_TYPE_TCP); /* Create new API */
                ESP_DEBUGW(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING,
                    nc == NULL, "NETCONN: Cannot create new structure for incoming server connection!\r\n");
                
                if (nc != NULL) {
                    nc->conn = conn;            /* Set connection callback */
                    esp_conn_set_arg(conn, nc); /* Set argument for connection */
#if ECP_CFG_NETCONN_ACCEPT_ON_CONNECT
                    /*
                     * If user wants to write connection to accept mbox,
                     * immediately after connection active,
                     * process it here.
                     *
                     * In case there is no listening connection,
                     * simply close the connection
                     */
                    if (!esp_sys_mbox_isvalid(listen_api->mbox_accept) ||
                        !esp_sys_mbox_putnow(listen_api->mbox_accept, nc)) {
                        close = 1;
                    }
#endif /* ECP_CFG_NETCONN_ACCEPT_ON_CONNECT */
                } else {
                    close = 1;
                }
            } else {
                ESP_DEBUGW(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING, listen_api == NULL,
                    "NETCONN: Closing connection as there is no listening API in netconn!\r\n");
                close = 1;                      /* Close the connection at this point */
            }
            if (close) {
                esp_conn_close(conn, 0);        /* Close the connection */
                if (nc != NULL) {
                    esp_netconn_delete(nc);     /* Free memory for API */
                }
            }
            break;
        }
        
        /*
         * We have a new data received which
         * should have netconn structure as argument
         */
        case ESP_CB_CONN_DATA_RECV: {
            esp_pbuf_p pbuf = cb->cb.conn_data_recv.buff;
            nc = esp_conn_get_arg(conn);        /* Get API from connection */

#if !ECP_CFG_NETCONN_ACCEPT_ON_CONNECT
            /*
             * Write data to listening connection accept mbox,
             * only when first data packet arrives
             */
            if (!nc->rcv_packets) {             /* Is this our first packet? */
                if (esp_sys_mbox_isvalid(&listen_api->mbox_accept)) {
                    if (!esp_sys_mbox_putnow(&listen_api->mbox_accept, nc)) {
                        ESP_DEBUGF(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_DANGER,
                            "NETCONN: Cannot put server connection to accept mbox\r\n");
                        close = 1;
                    }
                } else {
                    ESP_DEBUGF(ESP_CFG_DBG_NETCONN, "NETCONN: Invalid accept mbox\r\n");
                    close = 1;
                }
            }
#endif /* !ECP_CFG_NETCONN_ACCEPT_ON_CONNECT */

            nc->rcv_packets++;                  /* Increase number of received packets */
            if (!close) {
                if (!nc || !esp_sys_mbox_isvalid(&nc->mbox_receive) || 
                    !esp_sys_mbox_putnow(&nc->mbox_receive, pbuf)) {
                    ESP_DEBUGF(ESP_CFG_DBG_NETCONN, "NETCONN: Ignoring more data for receive!\r\n");
                    return espOKIGNOREMORE;     /* Return OK to free the memory and ignore further data */
                } else {
                    esp_pbuf_ref(pbuf);         /* Increase current reference count by 1 as system mbox is referencing our pbuf */
                }
            }
            ESP_DEBUGF(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE, "NETCONN: Written %d bytes to receive mbox\r\n", cb->cb.conn_data_recv.buff->len);
            break;
        }
        
        /*
         * Connection was just closed
         */
        case ESP_CB_CONN_CLOSED: {
            nc = esp_conn_get_arg(conn);        /* Get API from connection */
            
            /*
             * In case we have a netconn available, 
             * simply write pointer to received variable to indicate closed state
             */
            if (nc != NULL && esp_sys_mbox_isvalid(&nc->mbox_receive)) {
                esp_sys_mbox_putnow(&nc->mbox_receive, (void *)&recv_closed);
            }
            
            break;
        }
        default:
            return espERR;
    }
    if (close) {
        esp_conn_close(conn, 0);                /* Close the connection */
        if (nc) {
            esp_netconn_delete(nc);             /* Free memory for API */
        }
    }
    return espOK;
}

/**
 * \brief           Create new netconn connection
 * \param[in]       type: Type of netconn. This parameter can be a value of \ref esp_netconn_type_t enumeration
 * \return          New netconn connection
 */
esp_netconn_p
esp_netconn_new(esp_netconn_type_t type) {
    esp_netconn_t* a;
    
    a = esp_mem_calloc(1, sizeof(*a));          /* Allocate memory for core object */
    if (a != NULL) {
        a->type = type;                         /* Save netconn type */
        if (!esp_sys_mbox_create(&a->mbox_accept, 5)) { /* Allocate memory for accepting message box */
            ESP_DEBUGF(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_DANGER, "NETCONN: Cannot create accept MBOX\r\n");
            goto free_ret;
        }
        if (!esp_sys_mbox_create(&a->mbox_receive, 10)) {   /* Allocate memory for receiving message box */
            ESP_DEBUGF(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_DANGER, "NETCONN: Cannot create receive MBOX\r\n");
            goto free_ret;
        }
    }
    return a;
free_ret:
    if (esp_sys_mbox_isvalid(&a->mbox_accept)) {
        esp_sys_mbox_delete(&a->mbox_accept);
    }
    if (esp_sys_mbox_isvalid(&a->mbox_receive)) {
        esp_sys_mbox_delete(&a->mbox_receive);
    }
    if (a != NULL) {
        esp_mem_free(a);
    }
    return NULL;
}

/**
 * \brief           Delete netconn connection
 * \param[in]       nc: Pointer to netconn structure
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_delete(esp_netconn_p nc) {
    ESP_ASSERT("netconn != NULL", nc != NULL);  /* Assert input parameters */
    
    if (esp_sys_mbox_isvalid(&nc->mbox_accept)) {
        esp_sys_mbox_delete(&nc->mbox_accept);
    }
    if (esp_sys_mbox_isvalid(&nc->mbox_receive)) {
        esp_sys_mbox_delete(&nc->mbox_receive);
    }
    esp_mem_free(nc);
    return espOK;
}

/**
 * \brief           Connect to server as client
 * \param[in]       nc: Pointer to netconn structure
 * \param[in]       host: Pointer to host, such as domain name or IP address in string format
 * \param[in]       port: Target port to use
 * \return          espOK if successfully connected, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_connect(esp_netconn_p nc, const char* host, uint16_t port) {
    espr_t res;
    
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("host != NULL", host != NULL);   /* Assert input parameters */
    ESP_ASSERT("port > NULL", port);            /* Assert input parameters */
    
    /**
     * Start a new connection as client and immediately
     * set current netconn structure as argument
     * and netconn callback function for connection management
     */
    res = esp_conn_start(NULL, (esp_conn_type_t)nc->type, host, port, nc, esp_cb, 1);
    return res;
}

/**
 * \brief           Bind a connection to specific port, can be only used for server connections
 * \param[in]       nc: Pointer to netconn structure
 * \param[in]       port: Port used to bind a connection to
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_bind(esp_netconn_p nc, uint16_t port) {
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    
    /*
     * Enable server on port and set default callback
     */
    return esp_set_server(port, ESP_CFG_MAX_CONNS, 100, esp_cb, 1);
}

/**
 * \brief           Listen on previously binded connection
 * \param[in]       nc: Pointer to netconn used as base connection
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_listen(esp_netconn_p nc) {
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("nc->type must be TCP\r\n", nc->type == ESP_NETCONN_TYPE_TCP);   /* Assert input parameters */
    
    ESP_CORE_PROTECT();
    listen_api = nc;                            /* Set current main API in listening state */
    ESP_CORE_UNPROTECT();
    return espOK;
}

/**
 * \brief           Accept a new connection
 * \param[in]       nc: Pointer to netconn used as base connection
 * \param[in]       new_nc: Pointer to pointer to netconn to save new connection
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_accept(esp_netconn_p nc, esp_netconn_p* new_nc) {
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("new_nc != NULL", new_nc != NULL);   /* Assert input parameters */
    ESP_ASSERT("nc->type must be TCP\r\n", nc->type == ESP_NETCONN_TYPE_TCP);   /* Assert input parameters */
    
    esp_netconn_t* tmp;
    uint32_t time;
    if (nc != listen_api) {                     /* Currently only one API is allowed in listening state */
        return espERR;
    }
    
    *new_nc = NULL;
    time = esp_sys_mbox_get(&nc->mbox_accept, (void **)&tmp, 0);
    if (time == ESP_SYS_TIMEOUT) {
        return espERR;
    }
    *new_nc = tmp;                              /* Set new pointer */
    return espOK;                               /* We have a new connection */
}

/**
 * \brief           Write data to connection output buffers
 * \note            Only you can only use it on TCP or SSL connections
 * \param[in]       nc: Netconn connection used to write to
 * \param[in]       data: Pointer to data to write
 * \param[in]       btw: Number of bytes to write
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_write(esp_netconn_p nc, const void* data, size_t btw) {
    size_t len, sent;
    const uint8_t* d = data;
    espr_t res;
    
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("nc->type must be TCP or SSL\r\n", nc->type == ESP_NETCONN_TYPE_TCP || nc->type == ESP_NETCONN_TYPE_SSL);    /* Assert input parameters */
    
    /*
     * Several steps are done in write process
     *
     * 1. Check if buffer is set and check if there is something to write to it.
     *    1. In case buffer will be full after copy, send it and free memory.
     * 2. Check how many bytes we can write directly without needed to copy
     * 3. Try to allocate a new buffer and copy remaining input data to it
     * 4. In case buffer allocation fails, send data directly (may affect on speed and effectivenes)
     */
    
    /*
     * Step 1
     */
    if (nc->buff != NULL) {                     /* Is there a write buffer ready to be written? */
        len = ESP_MIN(nc->buff_len - nc->buff_ptr, btw);    /* Get number of bytes we can write to buffer */
        if (len) {
            memcpy(&nc->buff[nc->buff_ptr], data, len); /* Copy memory to temporary write buffer */
        }
        d += len;
        nc->buff_ptr += len;
        btw -= len;
        
        /*
         * Step 1.1
         */
        if (nc->buff_ptr == nc->buff_len) {
            res = esp_conn_send(nc->conn, nc->buff, nc->buff_len, &sent, 1);
            
            esp_mem_free(nc->buff);             /* Free memory */
            nc->buff = NULL;                    /* Invalidate buffer */
            if (res != espOK) {
                return res;
            }
        } else {
            return espOK;                       /* Buffer is not yet full yet */
        }
    }
    
    /*
     * Step 2
     */
    if (btw >= ESP_CFG_CONN_MAX_DATA_LEN) {
        size_t rem;
        rem = btw % ESP_CFG_CONN_MAX_DATA_LEN;      /* Get remaining bytes after sending everything */
        res = esp_conn_send(nc->conn, d, btw - rem, &sent, 1);  /* Write data directly */
        if (res != espOK) {
            return res;
        }
        d += sent;                              /* Advance in data pointer */
        btw -= sent;                            /* Decrease remaining data to send */
    }
    
    if (!btw) {                                 /* Sent everything? */
        return espOK;
    }
    
    /*
     * Step 3
     */
    if (nc->buff == NULL) {                     /* Check if we should allocate a new buffer */
        nc->buff = esp_mem_alloc(ESP_CFG_CONN_MAX_DATA_LEN * sizeof(*nc->buff));
        nc->buff_len = ESP_CFG_CONN_MAX_DATA_LEN;   /* Save buffer length */
        nc->buff_ptr = 0;                       /* Save buffer pointer */
    }
    
    /*
     * Step 4
     */
    if (nc->buff != NULL) {                     /* Memory available? */
        memcpy(&nc->buff[nc->buff_ptr], d, btw);    /* Copy data to buffer */
        nc->buff_ptr += btw;
    } else {                                    /* Still no memory available? */
        return esp_conn_send(nc->conn, data, btw, NULL, 1); /* Simply send the blocking way */
    }
    return espOK;
}

/**
 * \brief           Flush buffered data on netconn TCP connection
 * \note            Only you can only use it on TCP or SSL connections
 * \param[in]       nc: Netconn connection to flush data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_flush(esp_netconn_p nc) {
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("nc->type must be TCP or SSL\r\n", nc->type == ESP_NETCONN_TYPE_TCP || nc->type == ESP_NETCONN_TYPE_SSL);    /* Assert input parameters */

    /*
     * In case we have data in write buffer,
     * flush them out to network
     */
    if (nc->buff) {                             /* Check remaining data */
        esp_conn_send(nc->conn, nc->buff, nc->buff_ptr, NULL, 1);   /* Send data */
        esp_mem_free(nc->buff);                 /* Free memory */
        nc->buff = NULL;                        /* Invalid memory */
    }
    return espOK;
}

/**
 * \brief           Send packet to specific IP and port
 * \param[in]       nc: Netconn connection used to send
 * \param[in]       data: Pointer to data to write
 * \param[in]       btw: Number of bytes to write
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_send(esp_netconn_p nc, const void* data, size_t btw) {
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("nc->type must be UDP\r\n", nc->type == ESP_NETCONN_TYPE_UDP);   /* Assert input parameters */
    
    return esp_conn_send(nc->conn, data, btw, NULL, 1);
}

/**
 * \brief           Send packet to specific IP and port
 * \note            Use this function in case of UDP type netconn
 * \param[in]       nc: Netconn connection used to send
 * \param[in]       ip: Pointer to IP address
 * \param[in]       port: Port number used to send data
 * \param[in]       data: Pointer to data to write
 * \param[in]       btw: Number of bytes to write
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_sendto(esp_netconn_p nc, const void* ip, uint16_t port, const void* data, size_t btw) {
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("nc->type must be UDP\r\n", nc->type == ESP_NETCONN_TYPE_UDP);   /* Assert input parameters */
    
    return esp_conn_sendto(nc->conn, ip, port, data, btw, NULL, 1);
}

/**
 * \brief           Receive data from connection
 * \param[in]       nc: Netconn connection used to receive from
 * \param[in]       pbuf: Pointer to pointer to save new receive buffer to
 * \return          espOK on new data, espCLOSED when connection closed or member of \ref espr_t otherwise
 */
espr_t
esp_netconn_receive(esp_netconn_p nc, esp_pbuf_p* pbuf) {
    uint32_t time;
    
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    ESP_ASSERT("pbuf != NULL", pbuf != NULL);   /* Assert input parameters */
    
    time = esp_sys_mbox_get(&nc->mbox_receive, (void **)pbuf, 0);
    if (time == ESP_SYS_TIMEOUT || (uint8_t *)(*pbuf) == (uint8_t *)&recv_closed) {
        *pbuf = NULL;
        return espCLOSED;
    }
    return espOK;
}

/**
 * \brief           Close a netconn connection
 * \param[in]       nc: Netconn connection to close
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_close(esp_netconn_p nc) {
    ESP_ASSERT("nc != NULL", nc != NULL);       /* Assert input parameters */
    
    esp_netconn_flush(nc);                      /* Flush data and ignore result */
    esp_conn_set_arg(nc->conn, NULL);           /* Reset argument */
    esp_conn_close(nc->conn, 1);                /* Close the connection */
    flush_mboxes(nc);                           /* Flush message queues */
    return espOK;
}

/**
 * \brief           Get connection number used for netconn
 * \param[in]       nc: Pointer to netconn for connection
 * \return          -1 on failure, number otherwise
 */
int8_t
esp_netconn_getconnnum(esp_netconn_p nc) {
    if (nc && nc->conn) {
        return esp_conn_getnum(nc->conn);
    }
    return -1;
}

#endif /* ESP_CFG_NETCONN || __DOXYGEN__ */
