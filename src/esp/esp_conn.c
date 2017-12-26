/**	
 * \file            esp_conn.c
 * \brief           Connection API
 */
 
/*
 * Copyright (c) 2017 Tilen Majerle
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "esp/esp_private.h"
#include "esp/esp_conn.h"
#include "esp/esp_mem.h"
#include "esp/esp_timeout.h"

/**
 * \addtogroup      ESP_CONN
 * \{
 *
 * Connection based functions to manage sending and receiving data
 *
 * \note            Functions are in general thread safe. If there is an expection, it is mentioned in function description
 *
 * In the below example, you can find frequent use case how to use connection API in non-blocking callback mode.
 *
 * \par             Example
 *
 * In this example, most useful non-blocking approach is used to handle the connection.
 *
 * \code{c}
//Request data are sent to server once we are connected
uint8_t req_data[] = ""
    "GET / HTTP/1.1\r\n"
    "Host: " CONN_HOST "\r\n"
    "Connection: close\r\n"
    "\r\n";

// Connection callback function
// Called on several connection events, such as connected, closed, data received, data sent, ...
static espr_t
conn_cb(esp_cb_t* cb) {
    esp_conn_p conn = esp_conn_get_from_evt(cb);// Get connection from current event
    if (conn == NULL) {
        return espERR;                          // Return error at this point as this should never happen!
    }
    
    //Check the event type and process accordingly
    switch (cb->type) {
        case ESP_CB_CONN_ACTIVE: {              // Connection just active
            printf("Connection active!\r\n");
            
            // After we are connected,
            // send the HTTP request string in non-blocking way
            esp_conn_send(conn, req_data, sizeof(req_data) - 1, NULL, 0);
            break;
        }
        case ESP_CB_CONN_CLOSED: {              // Connection just closed
            printf("Connection closed!\r\n");
            if (cb->cb.conn_active_closed.forced) {
                printf("Connection closed by user\r\n");
            } else {
                printf("Connection closed by remote host\r\n");
            }
        }
        case ESP_CB_CONN_DATA_RECV: {           // We received connection data
            esp_pbuf_p pbuf = cb->cb.conn_data_recv.buff;
            printf("Connection data received!\r\n");
            if (pbuf != NULL) {                 // Check pbuf with data
                size_t len;
                len = esp_pbuf_length(pbuf, 1); // Get total length of buffer
                printf("Length of data: %d bytes\r\n", (int)len);
            }
        }
        default:
            break;
    }
    return espOK;
}

// Thread function or main function,
// where everything is happening
void
thread_or_main_func(void) {
    // Start the connection in non-blocking way and set the
    // function argument to NULL and callback function to conn_cb
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "example.com", 80, NULL, conn_cb, 0);
    
    // Do other tasks...
}

\endcode
 *
 * \}
 */

/**
 * \brief           Get connection validation ID
 * \param[in]       conn: Connection handle
 * \return          Connection current validation ID
 */
uint8_t
conn_get_val_id(esp_conn_p conn) {
    uint8_t val_id;
    ESP_CORE_PROTECT();
    val_id = conn->val_id;
    ESP_CORE_UNPROTECT();
    
    return val_id;
}

/**
 * \brief           Timeout callback for connection
 * \param[in]       arg: Timeout callback custom argument
 */
static void
conn_timeout_cb(void* arg) {
    uint16_t i;
                                                
    esp.cb.type = ESP_CB_CONN_POLL;             /* Set polling callback type */
    for (i = 0; i < ESP_MAX_CONNS; i++) {       /* Scan all connections */
        if (esp.conns[i].status.f.active) {     /* If connection is active */
            esp.cb.cb.conn_poll.conn = &esp.conns[i];   /* Set connection pointer */
            espi_send_conn_cb(&esp.conns[i]);   /* Send connection callback */
        }
    }
    esp_timeout_add(500, conn_timeout_cb, NULL);/* Schedule timeout again */
}

/**
 * \brief           Send data on already active connection of type UDP to specific remote IP and port
 * \note            In case IP and port values are not set, it will behave as normal send function (suitable for TCP too)
 * \param[in]       conn: Pointer to connection to send data
 * \param[in]       ip: Remote IP address for UDP connection
 * \param[in]       port: Remote port connection
 * \param[in]       data: Pointer to data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent
 * \param[in]       fau: "Free After Use" flag. Set to 1 if stack should free the memory after data sent
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
static espr_t
conn_send(esp_conn_p conn, const void* ip, uint16_t port, const void* data, size_t btw, size_t* bw, uint8_t fau, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("conn != NULL", conn != NULL);   /* Assert input parameters */
    ESP_ASSERT("data != NULL", data != NULL);   /* Assert input parameters */
    ESP_ASSERT("btw > 0", btw > 0);             /* Assert input parameters */
    
    if (bw != NULL) {
        *bw = 0;
    }
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSEND;
    
    ESP_MSG_VAR_REF(msg).msg.conn_send.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_send.data = data;
    ESP_MSG_VAR_REF(msg).msg.conn_send.btw = btw;
    ESP_MSG_VAR_REF(msg).msg.conn_send.bw = bw;
    ESP_MSG_VAR_REF(msg).msg.conn_send.remote_ip = ip;
    ESP_MSG_VAR_REF(msg).msg.conn_send.remote_port = port;
    ESP_MSG_VAR_REF(msg).msg.conn_send.fau = fau;
    ESP_MSG_VAR_REF(msg).msg.conn_send.val_id = conn_get_val_id(conn);
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Flush buffer on connection
 * \param[in]       conn: Connection to flush buffer on
 */
static espr_t
flush_buff(esp_conn_p conn) {
    if (conn != NULL && conn->buff != NULL) {   /* Do we have something ready? */
        if (conn_send(conn, NULL, 0, conn->buff, conn->buff_ptr, NULL, 1, 0) != espOK) {
            esp_mem_free(conn->buff);           /* Free memory manually */
        }
        conn->buff = NULL;
    }
    return espOK;
}

/**
 * \brief           Initialize connection module
 */
void
espi_conn_init(void) {
    esp_timeout_add(500, conn_timeout_cb, NULL);/* Add connection timeout */
}

/**
 * \brief           Starts a new connection of specific type
 * \param[out]      conn: Pointer to pointer to \ref esp_conn_t structure to set new connection reference
 * \param[in]       type: Connection type. This parameter can be a value of \ref esp_conn_type_t enumeration
 * \param[in]       host: Connection host. In case of IP, write it as string, ex. "192.168.1.1"
 * \param[in]       port: Connection port
 * \param[in]       arg: Pointer to user argument passed to connection if successfully connected
 * \param[in]       cb_func: Callback function for this connection. Set to NULL in case of default user callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_start(esp_conn_p* conn, esp_conn_type_t type, const char* host, uint16_t port, void* arg, esp_cb_func_t cb_func, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSTART;
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSTATUS;
    ESP_MSG_VAR_REF(msg).msg.conn_start.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_start.type = type;
    ESP_MSG_VAR_REF(msg).msg.conn_start.host = host;
    ESP_MSG_VAR_REF(msg).msg.conn_start.port = port;
    ESP_MSG_VAR_REF(msg).msg.conn_start.cb_func = cb_func;
    ESP_MSG_VAR_REF(msg).msg.conn_start.arg = arg;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Close specific or all connections
 * \param[in]       conn: Pointer to connection to close. Set to NULL if you want to close all connections.
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_close(esp_conn_p conn, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("conn != NULL", conn != NULL);   /* Assert input parameters */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPCLOSE;
    ESP_MSG_VAR_REF(msg).msg.conn_close.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_close.val_id = conn_get_val_id(conn);
    
    flush_buff(conn);                           /* First flush buffer */
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Send data on already active connection of type UDP to specific remote IP and port
 * \note            In case IP and port values are not set, it will behave as normal send function (suitable for TCP too)
 * \param[in]       conn: Pointer to connection to send data
 * \param[in]       ip: Remote IP address for UDP connection
 * \param[in]       port: Remote port connection
 * \param[in]       data: Pointer to data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_sendto(esp_conn_p conn, const void* ip, uint16_t port, const void* data, size_t btw, size_t* bw, uint32_t blocking) {
    flush_buff(conn);                           /* Flush currently written memory if exists */
    return conn_send(conn, ip, port, data, btw, bw, 0, blocking);
}

/**
 * \brief           Send data on already active connection either as client or server
 * \param[in]       conn: Pointer to connection to send data
 * \param[in]       data: Pointer to data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_send(esp_conn_p conn, const void* data, size_t btw, size_t* bw, uint32_t blocking) {
    flush_buff(conn);                           /* Flush currently written memory if exists */
    return conn_send(conn, NULL, 0, data, btw, bw, 0, blocking);
}

/**
 * \brief           Set argument variable for connection
 * \param[in]       conn: Pointer to connection to set argument
 * \param[in]       arg: Pointer to argument
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 * \sa              esp_conn_get_arg
 */
espr_t
esp_conn_set_arg(esp_conn_p conn, void* arg) {
    ESP_CORE_PROTECT();
    conn->arg = arg;                            /* Set argument for connection */
    ESP_CORE_UNPROTECT();
    return espOK;
}

/**
 * \brief           Get user defined connection argument
 * \param[in]       conn: Pointer to connection to set argument
 * \return          User argument
 * \sa              esp_conn_set_arg
 */
void *
esp_conn_get_arg(esp_conn_p conn) {
    void* arg;
    ESP_CORE_PROTECT();
    arg = conn->arg;                            /* Set argument for connection */
    ESP_CORE_UNPROTECT();
    return arg;
}

/**
 * \brief           Gets connections status
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_get_conns_status(uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSTATUS;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Check if connection type is client
 * \param[in]       conn: Pointer to connection to check for status
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_conn_is_client(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        ESP_CORE_PROTECT();
        res = conn->status.f.active && conn->status.f.client;
        ESP_CORE_UNPROTECT();
    }
    return res;
}

/**
 * \brief           Check if connection type is server
 * \param[in]       conn: Pointer to connection to check for status
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_conn_is_server(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        ESP_CORE_PROTECT();
        res = conn->status.f.active && !conn->status.f.client;
        ESP_CORE_UNPROTECT();
    }
    return res;
}

/**
 * \brief           Check if connection is active
 * \param[in]       conn: Pointer to connection to check for status
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_conn_is_active(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        ESP_CORE_PROTECT();
        res = conn->status.f.active;
        ESP_CORE_UNPROTECT();
    }
    return res;
}

/**
 * \brief           Check if connection is closed
 * \param[in]       conn: Pointer to connection to check for status
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_conn_is_closed(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        ESP_CORE_PROTECT();
        res = !conn->status.f.active;
        ESP_CORE_UNPROTECT();
    }
    return res;
}

/**
 * \brief           Get the number from connection
 * \param[in]       conn: Connection pointer
 * \return          Connection number in case of success or -1 on failure
 */
int8_t
esp_conn_getnum(esp_conn_p conn) {
    int8_t res = -1;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        /* Protection not needed as every connection has always the same number */
        res = conn->num;                        /* Get number */
    }
    return res;
}

/**
 * \brief           Set internal buffer size for SSL connection on ESP device
 * \note            Use this function first before you initialize first SSL connection
 * \param[in]       size: Size of buffer in units of bytes. Valid range is between 2048 and 4096 bytes
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_conn_set_ssl_buffersize(size_t size, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSSLSIZE;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sslsize.size = size;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Get connection from connection based event
 * \param[in]       evt: Event which happened for connection
 * \return          Connection pointer on success or NULL on failure
 */
esp_conn_p
esp_conn_get_from_evt(esp_cb_t* evt) {
    if (evt->type == ESP_CB_CONN_ACTIVE || evt->type == ESP_CB_CONN_CLOSED) {
        return evt->cb.conn_active_closed.conn;
    } else if (evt->type == ESP_CB_CONN_DATA_RECV) {
        return evt->cb.conn_data_recv.conn;
    } else if (evt->type == ESP_CB_CONN_DATA_SEND_ERR) {
        return evt->cb.conn_data_send_err.conn;
    } else if (evt->type == ESP_CB_CONN_DATA_SENT) {
        return evt->cb.conn_data_sent.conn;
    } else if (evt->type == ESP_CB_CONN_POLL) {
        return evt->cb.conn_poll.conn;
    }
    return NULL;
}

/**
 * \brief           Write data to connection buffer and if it is full, send it non-blocking way
 * \note            This function may only be called from core (connection callbacks)
 * \param[in]       conn: Connection to write
 * \param[in]       data: Data to copy to write buffer
 * \param[in]       btw: Number of bytes to write
 * \param[in]       flush: Flush flag. Set to 1 if you want to send data immediatelly after copying
 * \param[out]      mem_available: Available memory size available in current write buffer.
 *                  When the buffer length is reached, current one is sent and a new one is automatically created.
 *                  If function returns espOK and *mem_available = 0, there was a problem
 *                  allocating a new buffer for next operation
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_conn_write(esp_conn_p conn, const void* data, size_t btw, uint8_t flush, size_t* mem_available) {
    size_t len;
    
    const uint8_t* d = data;
    
    ESP_ASSERT("conn != NULL", conn != NULL);   /* Assert input parameters */
    
    /*
     * Steps, performed in write process:
     * 
     * 1. Check if we have buffer already allocated and
     *      write data to the tail of buffer
     *   1.1. In case buffer is full, send it non-blocking,
     *      and enable freeing after it is sent
     * 2. Check how many bytes we can copy as single buffer directly and send
     * 3. Create last buffer and copy remaining data to it even if no remaining data
     *      This is useful when calling function with no parameters (len = 0)
     * 4. Flush (send) current buffer if necessary
     */
    
    /*
     * Step 1
     */
    if (conn->buff != NULL) {
        len = ESP_MIN(conn->buff_len - conn->buff_ptr, btw);
        memcpy(&conn->buff[conn->buff_ptr], d, len);
        
        d += len;
        btw -= len;
        conn->buff_ptr += len;
        
        /*
         * Step 1.1
         */
        if (conn->buff_ptr == conn->buff_len || flush) {
            /* Try to send to processing queue in non-blocking way */
            if (conn_send(conn, NULL, 0, conn->buff, conn->buff_ptr, NULL, 1, 0) != espOK) {
                esp_mem_free(conn->buff);       /* Manually free memory */
            }
            conn->buff = NULL;                  /* Reset pointer */
        }
    }
    
    /*
     * Step 2
     */
    while (btw >= ESP_CONN_MAX_DATA_LEN) {
        uint8_t* buff;
        buff = esp_mem_alloc(ESP_CONN_MAX_DATA_LEN);    /* Allocate memory */
        if (buff != NULL) {
            memcpy(buff, d, ESP_CONN_MAX_DATA_LEN); /* Copy data to buffer */
            if (conn_send(conn, NULL, 0, buff, ESP_CONN_MAX_DATA_LEN, NULL, 1, 0) != espOK) {
                esp_mem_free(buff);             /* Manually free memory */
            }
        } else {
            return espERR;
        }
        
        btw -= ESP_CONN_MAX_DATA_LEN;           /* Decrease remaining length */
        d += ESP_CONN_MAX_DATA_LEN;             /* Advance data pointer */
    }
    
    /*
     * Step 3
     */
    if (conn->buff == NULL) {
        conn->buff = esp_mem_alloc(ESP_CONN_MAX_DATA_LEN);  /* Allocate memory for temp buffer */
        conn->buff_len = ESP_CONN_MAX_DATA_LEN;
        conn->buff_ptr = 0;
    }
    if (btw) {
        if (conn->buff != NULL) {
            memcpy(conn->buff, d, btw);         /* Copy data to memory */
            conn->buff_ptr = btw;
        } else {
            return espERR;
        }
    }
    
    /*
     * Step 4
     */
    if (flush && conn->buff != NULL) {
        flush_buff(conn);
    }
    
    /*
     * Calculate number of available memory after write operation
     */
    if (mem_available != NULL) {
        if (conn->buff != NULL) {
            *mem_available = conn->buff_len - conn->buff_ptr;
        } else {
            *mem_available = 0;
        }
    }
    return espOK;
}
