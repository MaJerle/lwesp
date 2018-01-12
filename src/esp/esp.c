/**
 * \file            esp.c
 * \brief           Main ESP core file
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
#include "esp/esp.h"
#include "esp/esp_int.h"
#include "esp/esp_mem.h"
#include "esp/esp_threads.h"
#include "system/esp_ll.h"

#if ESP_CFG_OS != 1
#error ESP_CFG_OS must be set to 1!
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
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking, 1000);    /* Send message to producer queue */
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
esp_init(esp_cb_fn cb_func) {
    esp.status.f.initialized = 0;               /* Clear possible init flag */
    esp.cb_func = cb_func ? cb_func : def_callback; /* Set callback function */
    esp.cb_server = esp.cb_func;                /* Set default server callback function */
    
    esp_sys_init();                             /* Init low-level system */
    esp_ll_init(&esp.ll, ESP_CFG_AT_PORT_BAUDRATE); /* Init low-level communication */
    
    esp_sys_sem_create(&esp.sem_sync, 1);       /* Create new semaphore with unlocked state */
    esp_sys_mbox_create(&esp.mbox_producer, ESP_CFG_THREAD_PRODUCER_MBOX_SIZE); /* Producer message queue */
    esp_sys_thread_create(&esp.thread_producer, "producer", esp_thread_producer, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    
    esp_sys_mbox_create(&esp.mbox_process, ESP_CFG_THREAD_PROCESS_MBOX_SIZE);   /* Consumer message queue */
    esp_sys_thread_create(&esp.thread_process,  "process", esp_thread_process, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);

#if !ESP_CFG_INPUT_USE_PROCESS
    esp_buff_init(&esp.buff, ESP_CFG_RCV_BUFF_SIZE);    /* Init buffer for input data */
#endif /* !ESP_CFG_INPUT_USE_PROCESS */
    esp.status.f.initialized = 1;               /* We are initialized now */
    
    /**
     * Call reset command and call default
     * AT commands to prepare basic setup for device
     */
    espi_conn_init();                           /* Init connection module */
    esp_reset(1);
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
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking, 5000);    /* Send message to producer queue */
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
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking, 1000);    /* Send message to producer queue */
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
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking, 2000);    /* Send message to producer queue */
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
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking, 1000);    /* Send message to producer queue */
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
esp_set_server(uint16_t port, uint16_t max_conn, uint16_t timeout, esp_cb_fn cb, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSERVER;
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSERVERMAXCONN;  /* First command is to set maximal number of connections for server */
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.port = port;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.max_conn = max_conn;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.timeout = timeout;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.cb = cb;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking, 1000);    /* Send message to producer queue */
}

/**
 * \brief           Set default callback function for incoming server connections
 * \param[in]       cb_func: Callback function. Set to NULL to use default ESP callback function
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_default_server_callback(esp_cb_fn cb_func) {
    ESP_CORE_PROTECT();                         /* Protect system */
    esp.cb_server = cb_func ? cb_func : esp.cb_func;    /* Set default callback */
    ESP_CORE_UNPROTECT();                       /* Unprotect system */
    return espOK;
}

#if ESP_CFG_DNS || __DOXYGEN__
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
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking, 20000);   /* Send message to producer queue */
}
#endif /* ESP_CFG_DNS || __DOXYGEN__ */

/**
 * \brief           Lock and protect ESP core from multiple access at a time
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_core_lock(void) {
    ESP_CORE_PROTECT();                         /* Lock ESP core */
    return espOK;
}

/**
 * \brief           Unlock and unprotect ESP core from multiple access at a time
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_core_unlock(void) {
    ESP_CORE_UNPROTECT();                       /* Unlock ESP core */
    return espOK;
}

