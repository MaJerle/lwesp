/**
 * \file            esp.c
 * \brief           Main ESP core file
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
#include "esp.h"
#include "esp_int.h"
#include "esp_mem.h"
#include "esp_ll.h"
#include "esp_threads.h"

esp_t esp;

/**
 * \brief           Sends message from API function to producer queue for further processing
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
static espr_t
send_msg_to_producer_queue(esp_msg_t* msg, espr_t (*process_fn)(esp_msg_t *), uint32_t block_time) {
    espr_t res = msg->res = espOK;
    if (block_time) {                           /* In case message is blocking */
        if (!esp_sys_sem_create(&msg->sem, 0)) {/* Create semaphore and lock it immediatelly */
            ESP_MSG_VAR_FREE(msg);              /* Release memory and return */
            return espERR;
        }
    }
    msg->block_time = block_time;               /* Set blocking status if necessary */
    msg->fn = process_fn;                       /* Save processing function to be called as callback */
    if (block_time) {
        esp_sys_mbox_put(&esp.mbox_producer, msg);  /* Write message to producer queue and wait forever */
    } else {
        if (!esp_sys_mbox_putnow(&esp.mbox_producer, msg)) {    /* Write message to producer queue immediatelly */
            res = espERR;
        }
    }
    if (block_time && res == espOK) {           /* In case we have blocking request */
        uint32_t time;
        time = esp_sys_sem_wait(&msg->sem, 0);  /* Wait forever for access to semaphore */
        if (ESP_SYS_TIMEOUT == time) {          /* If semaphore was not accessed in given time */
            res = espERR;                       /* Semaphore not released in time */
        } else {
            res = msg->res;                     /* Set response status from message response */
        }
        if (esp_sys_sem_isvalid(&msg->sem)) {   /* In case we have valid semaphore */
            esp_sys_sem_delete(&msg->sem);      /* Delete semaphore object */
        }
        ESP_MSG_VAR_FREE(msg);                  /* Release message */
    }
    return res;
}

/**
 * \brief           Default callback function for events
 * \param[in]       cb: Pointer to callback data structure
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
def_callback(esp_cb_t* cb) {
    return espOK;
}

/**
 * Internal API functions here
 */

/**
 * \brief           Enable more data on +IPD command
 * \param[in]       info: Set to 1 or 0 if you want enable or disable more data on +IPD statement
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_set_dinfo(uint8_t info, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPDINFO;
    ESP_MSG_VAR_REF(msg).msg.tcpip_dinfo.info = info;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_tcpip_misc, blocking);    /* Send message to producer queue */
}

/**
 * Public API functions here
 */

/**
 * \brief           Init and prepare ESP stack
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_init(esp_cb_func_t cb_func) {
    esp_sys_init();                             /* Init low-level system */
    esp_ll_init(&esp.ll, 115200);               /* Init low-level communication */
    
    esp.cb_func = cb_func ? cb_func : def_callback; /* Set callback function */
    
    esp_sys_sem_create(&esp.sem_sync, 1);       /* Create new semaphore with unlocked state */
    esp_sys_mbox_create(&esp.mbox_consumer, 20);/* Consumer message queue */
    esp_sys_mbox_create(&esp.mbox_producer, 20);/* Producer message queue */
    esp_sys_thread_create(&esp.thread_producer, "producer", esp_thread_producer, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    esp_sys_thread_create(&esp.thread_consumer, "consumer", esp_thread_consumer, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    
    esp_buff_init(&esp.buff, 0x400);            /* Init buffer for input data */
    
    esp_reset(0);                               /* Reset ESP device */
    esp_set_wifi_mode(ESP_MODE_STA, 0);         /* Set WiFi mode after reset */
    esp_set_mux(1, 0);                          /* Go to multiple connections mode */
    espi_set_dinfo(1, 0);                       /* Enable additional data on +IPD */
    esp_get_conns_status(0);                    /* Get connection status */
    
    esp.cb.type = ESP_CB_INIT_FINISH;           /* Init was successful */
    esp.cb_func(&esp.cb);                       /* Call callback function */
    
    return espOK;
}

/**
 * \brief           Sets WiFi mode to either station only, access point only or both
 * \param[in]       mode: Mode of operation
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_reset(uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_RESET;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_reset, blocking); /* Send message to producer queue */
}

/**
 * \brief           Sets WiFi mode to either station only, access point only or both
 * \param[in]       mode: Mode of operation. This parameter can be a value of \ref esp_mode_t enumeration
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_wifi_mode(esp_mode_t mode, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_WIFI_CWMODE;
    ESP_MSG_VAR_REF(msg).msg.wifi_mode.mode = mode; /* Set desired mode */
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_set_wifi_mode, blocking); /* Send message to producer queue */
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
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSTATUS;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_get_conns_status, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Quit (disconnect) from access point
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_quit(uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_WIFI_CWQAP;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_sta_join_quit, blocking); /* Send message to producer queue */
}

/**
 * \brief           Joins as station to access point
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_join(const char* name, const char* pass, const uint8_t* mac, uint8_t def, uint32_t blocking) {
    espr_t resp;
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_WIFI_CWJAP;
    
    ESP_MSG_VAR_REF(msg).def = def;
    ESP_MSG_VAR_REF(msg).msg.sta_join.name = name;
    ESP_MSG_VAR_REF(msg).msg.sta_join.pass = pass;
    ESP_MSG_VAR_REF(msg).msg.sta_join.mac = mac;
    
    esp_sta_quit(0);                            /* Disconnect first */
    
    resp = send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_sta_join_quit, blocking); /* Send message to producer queue */
    
    /* Send commands to aquire IP of station and MAC as well */
    esp_sta_getip(0);
//    esp_sta_getmac(0);
//    esp_ap_getip(0);
//    esp_ap_getmac(0);
    
    return resp;
}

/**
 * \brief           Get IP of station mode
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_getip(uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_WIFI_CIPSTA_GET;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_cip_sta_ap_cmd, blocking); /* Send message to producer queue */
}

/**
 * \brief           Sets baudrate of AT port (usually UART)
 * \param[in]       baud: Baudrate in units of bits per second
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_at_baudrate(uint32_t baud, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_UART;
    ESP_MSG_VAR_REF(msg).msg.uart.baudrate = baud;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_basic_uart, blocking);    /* Send message to producer queue */
}

/**
 * \brief           Set multiple connections mux
 * \param[in]       mux: Set to 1 or 0 if you want enable or disable multiple connections
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_mux(uint8_t mux, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPMUX;
    ESP_MSG_VAR_REF(msg).msg.tcpip_mux.mux = mux;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_tcpip_misc, blocking);    /* Send message to producer queue */
}

/**
 * \brief           Enables or disables server mode
 * \param[in]       port: Set port number to enable server on. Use 0 to disable server mode
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_server(uint16_t port, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSERVER;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.port = port;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_tcpip_server, blocking); /* Send message to producer queue */
}

/**
 * \brief           Starts a new connection of specific type
 * \param[out]      conn: Pointer to pointer to \ref esp_conn_t structure to set new connection reference
 * \param[in]       type: Connection type. This parameter can be a value of \ref esp_conn_type_t enumeration
 * \param[in]       host: Connection host
 * \param[in]       host: Connection port
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_start(esp_conn_t** conn, esp_conn_type_t type, const char* host, uint16_t port, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSTART;
    
    ESP_MSG_VAR_REF(msg).msg.conn_start.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_start.type = type;
    ESP_MSG_VAR_REF(msg).msg.conn_start.host = host;
    ESP_MSG_VAR_REF(msg).msg.conn_start.port = port;
    
    esp_get_conns_status(0);                    /* Get connection statuses first */
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_tcpip_conn, blocking);    /* Send message to producer queue */
}

/**
 * \brief           Close specific or all connections
 * \param[in]       conn: Pointer to connection to close. Set to NULL if you want to close all connections.
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_close(esp_conn_t* conn, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPCLOSE;
    
    ESP_MSG_VAR_REF(msg).msg.conn_close.conn = conn;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_tcpip_conn, blocking);    /* Send message to producer queue */
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
esp_conn_send(esp_conn_t* conn, const void* data, size_t btw, size_t* bw, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    *bw = 0;
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSEND;
    
    ESP_MSG_VAR_REF(msg).msg.conn_send.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_send.data = data;
    ESP_MSG_VAR_REF(msg).msg.conn_send.btw = btw;
    ESP_MSG_VAR_REF(msg).msg.conn_send.bw = bw;
    
    return send_msg_to_producer_queue(&ESP_MSG_VAR_REF(msg), espi_tcpip_conn, blocking);    /* Send message to producer queue */
}

espr_t
esp_conn_is_client(esp_conn_t* conn) {
    ESP_ASSERT(conn != NULL);                   /* Assert input parameters */
    return conn->status.f.client ? espOK : espERR;  /* Return client status */
}

espr_t
esp_conn_is_server(esp_conn_t* conn) {
    ESP_ASSERT(conn != NULL);                   /* Assert input parameters */
    return !conn->status.f.client ? espOK : espERR; /* Return server status */
}

espr_t
esp_conn_is_active(esp_conn_t* conn) {
    ESP_ASSERT(conn != NULL);                   /* Assert input parameters */
    return conn->status.f.active ? espOK : espERR;  /* Return active status */
}

espr_t
esp_conn_is_closed(esp_conn_t* conn) {
    ESP_ASSERT(conn != NULL);                   /* Assert input parameters */
    return !conn->status.f.active ? espOK : espERR; /* Return closed status */
}

espr_t
esp_conn_set_ssl_buffer(size_t size, uint32_t blocking);

