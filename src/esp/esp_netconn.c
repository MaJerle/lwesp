/**
 * \file            esp_netconn.c
 * \brief           API functions for sequential calls
 */

/*
 * Copyright (c) 2017, Tilen MAJERLE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *  * Neither the name of the author nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \author          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "esp_netconn.h"
#include "esp_mem.h"

static uint8_t recv_closed = 0xFF;
static esp_netconn_t* listen_api;              /* Main connection in listening mode */


/**
 * \brief           Callback function for every server connection
 * \param[in]       cb: Pointer to callback structure
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
esp_cb(esp_cb_t* cb) {
    esp_conn_t* conn;
    esp_netconn_t* nc;
    uint8_t close = 0;
    
    switch (cb->type) {
        case ESP_CB_CONN_ACTIVE: {              /* A new connection active as server */
            conn = cb->cb.conn_active_closed.conn;  /* Get connection */
            if (esp_conn_is_client(conn)) {     /* Was connection started by us? */
                nc = conn->arg;                 /* Argument should be ready already */
                if (nc) {                       /* Is netconn set? Should be already by us */
                    nc->conn = conn;            /* Save actual connection */
                } else {
                    close = 1;                  /* Close this connection, invalid netconn */
                }
            } else if (esp_conn_is_server(conn) && listen_api) {    /* Is the connection server type and we have known listening API? */
                nc = esp_netconn_new();         /* Create new API */
                if (nc) {
                    nc->conn = conn;            /* Set connection callback */
                    esp_conn_set_arg(conn, nc); /* Set argument for connection */
                    if (esp_sys_mbox_isvalid(&listen_api->mbox_accept)) {
                        if (!esp_sys_mbox_putnow(&listen_api->mbox_accept, nc)) {
                            ESP_DEBUGF(ESP_DBG_NETCONN, "API: cannot put server connection to accept mbox\r\n");
                            close = 1;
                        }
                    } else {
                        close = 1;
                    }
                } else {
                    close = 1;
                }
            } else {
                close = 1;                      /* Close the connection at this point */
            }
            if (close) {
                esp_conn_close(conn, 0);        /* Close the connection */
                if (nc) {
                    esp_netconn_delete(nc);     /* Free memory for API */
                }
            }
            break;
        }
        
        /**
         * We have a new data received 
         * on a conection which should be of type server
         * and should have netconn structure as argument
         */
        case ESP_CB_DATA_RECV: {
            esp_pbuf_t* pbuf = (esp_pbuf_t *)cb->cb.conn_data_recv.buff;
            conn = cb->cb.conn_data_recv.conn;  /* Get connection */
            nc = conn->arg;                     /* Get API from connection */
            if (!nc || !esp_sys_mbox_isvalid(&nc->mbox_receive) || 
                !esp_sys_mbox_putnow(&nc->mbox_receive, pbuf)) {
                ESP_DEBUGF(ESP_DBG_NETCONN, "API: Ignoring more data for receive\r\n");
                return espOKIGNOREMORE;         /* Return OK to free the memory */
            }
            ESP_DEBUGF(ESP_DBG_NETCONN, "API: written %d bytes to receive mbox\r\n", cb->cb.conn_data_recv.buff->len);
            return espOKMEM;                    /* Return ok but do not release memory */
        }
        
        /**
         * Connection was just closed
         */
        case ESP_CB_CONN_CLOSED: {
            conn = cb->cb.conn_active_closed.conn;  /* Get connection */
            nc = conn->arg;                     /* Get API from connection */
            
            if (nc && esp_sys_mbox_isvalid(&nc->mbox_receive)) {
                esp_sys_mbox_putnow(&nc->mbox_receive, (void *)&recv_closed);
            }
            
            break;
        }
        default:
            return espERR;
    }
    return espOK;
}

/**
 * \brief           Create new netconn connection
 * \return          New netconn connection
 */
esp_netconn_t*
esp_netconn_new(void) {
    esp_netconn_t* a;
    a = esp_mem_calloc(1, sizeof(*a));          /* Allocate memory for core object */
    if (a) {
        if (!esp_sys_mbox_create(&a->mbox_accept, 5)) {
            ESP_DEBUGF(ESP_DBG_NETCONN, "API: cannot create accept MBOX\r\n");
            goto free_ret;
        }
        if (!esp_sys_mbox_create(&a->mbox_receive, 10)) {
            ESP_DEBUGF(ESP_DBG_NETCONN, "API: cannot create receive MBOX\r\n");
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
    if (a) {
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
esp_netconn_delete(esp_netconn_t* nc) {
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
esp_netconn_connect(esp_netconn_t* nc, const char* host, uint16_t port) {
    espr_t res ;
    res = esp_conn_start(NULL, ESP_CONN_TYPE_TCP, host, port, nc, esp_cb, 1);
    return res;
}

/**
 * \brief           Bind a connection to specific port
 * \param[in]       nc: Pointer to netconn structure
 * \param[in]       port: Port used to bind a connection to
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_bind(esp_netconn_t* nc, uint16_t port) {
    if (esp_set_server(port, 1) != espOK) {     /* Enable server on selected port */
        return espERR;
    }
    esp_set_default_server_callback(esp_cb);    /* Set callback function for server connections */
    
    return espOK;
}

/**
 * \brief           Listen on previously binded connection
 * \param[in]       nc: Pointer to netconn used as base connection
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_listen(esp_netconn_t* nc) {
    listen_api = nc;                           /* Set current main API in listening state */
    return espOK;
}

/**
 * \brief           Accept a new connection
 * \param[in]       nc: Pointer to netconn used as base connection
 * \param[in]       new_nc: Pointer to pointer to netconn to save new connection
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_accept(esp_netconn_t* nc, esp_netconn_t** new_nc) {
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
 * \brief           Write data to connection
 * \param[in]       nc: Netconn connection used to write to
 * \param[in]       data: Pointer to data to write
 * \param[in]       btw: Number of bytes to write
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_write(esp_netconn_t* nc, const void* data, size_t btw) {
    return esp_conn_send(nc->conn, data, btw, NULL, 1);
}

/**
 * \brief           Receive data from connection
 * \param[in]       nc: Netconn connection used to receive from
 * \param[in]       pbuf: Pointer to pointer to save new receive buffer to
 * \return          espOK on new data, espCLOSED when connection closed or member of \ref espr_t otherwise
 */
espr_t
esp_netconn_receive(esp_netconn_t* nc, esp_pbuf_t** pbuf) {
    uint32_t time = esp_sys_mbox_get(&nc->mbox_receive, (void **)pbuf, 0);
    if ((uint8_t *)(*pbuf) == (uint8_t *)&recv_closed) {
        //*pbuf = NULL;
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
esp_netconn_close(esp_netconn_t* nc) {
    esp_pbuf_t* pbuf;
    esp_conn_set_arg(nc->conn, NULL);       /* Reset argument */
    esp_conn_close(nc->conn, 1);            /* Close the connection */
    while (1) {
        if (!esp_sys_mbox_getnow(&nc->mbox_receive, (void **)&pbuf)) {
            break;
        }
        if (pbuf && (uint8_t *)pbuf != (uint8_t *)&recv_closed) {
            esp_pbuf_free(pbuf);
        }
    }
    return espOK;
}
