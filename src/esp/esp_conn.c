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
#include "include/esp_private.h"
#include "include/esp_conn.h"
#include "include/esp_mem.h"
#include "include/esp_timeout.h"

static
void conn_timeout_cb(void* arg) {
    uint16_t i;
                                                
    for (i = 0; i < ESP_MAX_CONNS; i++) {       /* Scan all connections */
        if (esp.conns[i].status.f.active) {     /* If connection is active */
            esp.cb.type = ESP_CB_CONN_POLL;     /* Set polling callback type */
            esp.cb.cb.conn_poll.conn = &esp.conns[i];   /* Set connection pointer */
            espi_send_conn_cb(&esp.conns[i]);   /* Send connection callback */
        }
    }
    esp_timeout_add(500, conn_timeout_cb, NULL);/* Schedule timeout again */
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
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("conn != NULL", conn != NULL);   /* Assert input parameters */
    ESP_ASSERT("data != NULL", data != NULL);   /* Assert input parameters */
    ESP_ASSERT("conn > 0", btw > 0);            /* Assert input parameters */
    
    if (bw) {
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
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
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
    return esp_conn_sendto(conn, NULL, 0, data, btw, bw, blocking);
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
    if (conn && espi_is_valid_conn_ptr(conn)) {
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
    if (conn && espi_is_valid_conn_ptr(conn)) {
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
    if (conn && espi_is_valid_conn_ptr(conn)) {
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
    if (conn && espi_is_valid_conn_ptr(conn)) {
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
    if (conn && espi_is_valid_conn_ptr(conn)) {
        /* Protection not needed as every connection has always the same number */
        res = conn->num;                        /* Get number */
    }
    return res;
}

/**
 * \brief           Set internal buffer size for SSL connection on ESP device
 * \note            Use this function first before you initialize first SSL connection
 * \param[in]       size: Size of buffer in units of bytes. Valid range is between 2048 and 4096 bytes
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
