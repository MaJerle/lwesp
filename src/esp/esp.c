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
#include "esp/esp.h"
#include "esp/esp_int.h"
#include "esp/esp_mem.h"
#include "esp/esp_threads.h"
#include "system/esp_ll.h"

#if ESP_OS != 1
#error ESP_OS must be set to 1!
#endif

esp_t esp;

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
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPDINFO;
    ESP_MSG_VAR_REF(msg).msg.tcpip_dinfo.info = info;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/*
 * Public API functions here
 */

/**
 * \brief           Init and prepare ESP stack
 * \param[in]       cb_func: Event callback function
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_init(esp_cb_func_t cb_func) {
    esp.status.f.initialized = 0;               /* Clear possible init flag */
    esp.cb_func = cb_func ? cb_func : def_callback; /* Set callback function */
    esp.cb_server = esp.cb_func;                /* Set default server callback function */
    
    esp_sys_init();                             /* Init low-level system */
    esp_ll_init(&esp.ll, ESP_AT_PORT_BAUDRATE); /* Init low-level communication */
    
    esp_sys_sem_create(&esp.sem_sync, 1);       /* Create new semaphore with unlocked state */
    esp_sys_mbox_create(&esp.mbox_producer, ESP_THREAD_PRODUCER_MBOX_SIZE); /* Producer message queue */
    esp_sys_thread_create(&esp.thread_producer, "producer", esp_thread_producer, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    
#if !ESP_INPUT_USE_PROCESS
    esp_sys_mbox_create(&esp.mbox_process, ESP_THREAD_PROCESS_MBOX_SIZE);   /* Consumer message queue */
    esp_sys_thread_create(&esp.thread_process,  "process", esp_thread_process, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    
    esp_buff_init(&esp.buff, ESP_RCV_BUFF_SIZE);    /* Init buffer for input data */
#endif /* !ESP_INPUT_USE_PROCESS */
    esp.status.f.initialized = 1;               /* We are initialized now */
    
    /**
     * Call reset command and call default
     * AT commands to prepare basic setup for device
     */
    esp_reset(1);
    espi_conn_init();                           /* Init connection module */
    espi_send_cb(ESP_CB_INIT_FINISH);           /* Call user callback function */
    
    return espOK;
}

/**
 * \brief           Sets WiFi mode to either station only, access point only or both
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_reset(uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_RESET;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
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
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWMODE;
    ESP_MSG_VAR_REF(msg).msg.wifi_mode.mode = mode; /* Set desired mode */
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
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
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_UART;
    ESP_MSG_VAR_REF(msg).msg.uart.baudrate = baud;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
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
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPMUX;
    ESP_MSG_VAR_REF(msg).msg.tcpip_mux.mux = mux;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Enables or disables server mode
 * \param[in]       port: Set port number to enable server on. Use 0 to disable server mode
 * \param[in]       max_conn: Number of maximal connections populated by server
 * \param[in]       timeout: Time used to automatically close the connection in units of seconds. Use 0 to disable timeout feature (not recommended)
 * \param[in]       cb: Connection callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_server(uint16_t port, uint16_t max_conn, uint16_t timeout, esp_cb_func_t cb, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSERVER;
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSERVERMAXCONN;  /* First command is to set maximal number of connections for server */
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.port = port;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.max_conn = max_conn;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.timeout = timeout;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.cb = cb;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Set default callback function for incoming server connections
 * \param[in]       cb_func: Callback function. Set to NULL to use default ESP callback function
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_default_server_callback(esp_cb_func_t cb_func) {
    ESP_CORE_PROTECT();                         /* Protect system */
    esp.cb_server = cb_func ? cb_func : esp.cb_func;    /* Set default callback */
    ESP_CORE_UNPROTECT();                       /* Unprotect system */
    return espOK;
}

#if ESP_DNS || __DOXYGEN__
/**
 * \brief           Get IP address from host name
 * \param[in]       host: Pointer to host name to get IP for
 * \param[out]      ip: Pointer to output variable to save result. At least 4 bytes required
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_dns_getbyhostname(const char* host, void* ip, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("host != NULL", host != NULL);   /* Assert input parameters */
    ESP_ASSERT("ip != NULL", ip != NULL);       /* Assert input parameters */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPDOMAIN;
    ESP_MSG_VAR_REF(msg).msg.dns_getbyhostname.host = host;
    ESP_MSG_VAR_REF(msg).msg.dns_getbyhostname.ip = ip;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}
#endif /* ESP_DNS || __DOXYGEN__ */
