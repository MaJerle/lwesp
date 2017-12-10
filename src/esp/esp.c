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
#include "include/esp.h"
#include "include/esp_int.h"
#include "include/esp_mem.h"
#include "include/esp_ll.h"
#include "include/esp_threads.h"

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
    esp_ll_init(&esp.ll, ESP_AT_PORT_BAUDRATE); /* Init low-level communication */
    
    esp.cb_func = cb_func ? cb_func : def_callback; /* Set callback function */
    esp.cb_server = esp.cb_func;                /* Set default server callback function */
    
    esp_sys_sem_create(&esp.sem_sync, 1);       /* Create new semaphore with unlocked state */
    esp_sys_mbox_create(&esp.mbox_producer, ESP_THREAD_PRODUCER_MBOX_SIZE); /* Producer message queue */
    esp_sys_mbox_create(&esp.mbox_process, ESP_THREAD_PROCESS_MBOX_SIZE);   /* Consumer message queue */
    esp_sys_thread_create(&esp.thread_producer, "producer", esp_thread_producer, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    esp_sys_thread_create(&esp.thread_process,  "process", esp_thread_consumer, &esp, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    
    esp_buff_init(&esp.buff, ESP_RCV_BUFF_SIZE);    /* Init buffer for input data */
    
    /**
     * Call reset command and call default
     * AT commands to prepare basic setup for device
     */
    esp_reset(1);
    
    espi_send_cb(ESP_CB_INIT_FINISH);           /* Call user callback function */
    
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

/******************************************************************************/
/******************************************************************************/
/***                        Station related functions                        **/
/******************************************************************************/
/******************************************************************************/

/**
 * \brief           Quit (disconnect) from access point
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_quit(uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWQAP;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Joins as station to access point
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_join(const char* name, const char* pass, const uint8_t* mac, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("name != NULL", name != NULL);   /* Assert input parameters */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWJAP;
    ESP_MSG_VAR_REF(msg).msg.sta_join.def = def;
    ESP_MSG_VAR_REF(msg).msg.sta_join.name = name;
    ESP_MSG_VAR_REF(msg).msg.sta_join.pass = pass;
    ESP_MSG_VAR_REF(msg).msg.sta_join.mac = mac;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Get station IP address
 * \param[out]      ip: Pointer to variable to save IP address. Memory of at least 4 bytes is required
 * \param[out]      gw: Pointer to output variable to save gateway address. Memory of at least 4 bytes is required
 * \param[out]      nm: Pointer to output variable to save netmask address. Memory of at least 4 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) IP to read
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_getip(void* ip, void* gw, void* nm, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPSTA_GET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.ip = ip;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.gw = gw;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.nm = nm;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Set station IP address
 * \param[in]       ip: Pointer to IP address. Memory of at least 4 bytes is required
 * \param[in]       gw: Pointer to gateway address. Set to NULL to use default gateway. Memory of at least 4 bytes is required
 * \param[in]       nm: Pointer to netmask address. Set to NULL to use default netmask. Memory of at least 4 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) IP to set
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_setip(const void* ip, const void* gw, const void* nm, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("ip != NULL", ip != NULL);       /* Assert input parameters */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPSTA_SET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.ip = ip;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.gw = gw;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.nm = nm;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Get station MAC address
 * \param[out]      mac: Pointer to output variable to save MAC address. Memory of at least 6 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) IP to read
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_getmac(void* mac, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPSTAMAC_GET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getmac.mac = mac;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getmac.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Set station MAC address
 * \param[in]       mac: Pointer to variable with MAC address. Memory of at least 6 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) MAC to write
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_setmac(const void* mac, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("mac != NULL", mac != NULL);     /* Assert input parameters */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPSTAMAC_SET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setmac.mac = mac;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setmac.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Check if ESP got IP from access point
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_sta_has_ip(void) {
    uint8_t res;
    ESP_CORE_PROTECT();
    res = esp.status.f.r_got_ip;
    ESP_CORE_UNPROTECT();
    return res ? espOK : espERR;
}

/**
 * \brief           Copies IP address from internal value for user
 * \note            In case you want to refresh actual value from ESP device, use \ref esp_sta_getip function
 * \param[out]      ip: Pointer to output IP variable. Set to NULL if not interested in IP address
 * \param[out]      gw: Pointer to output gateway variable. Set to NULL if not interested in gateway address
 * \param[out]      nm: Pointer to output netmask variable. Set to NULL if not interested in netmask address
 */
espr_t
esp_sta_copy_ip(void* ip, void* gw, void* nm) {
    espr_t res = espERR;
    if ((ip || gw || nm) && esp_sta_has_ip() == espOK) {    /* Do we have a valid IP address? */
        ESP_CORE_PROTECT();                     /* Protect ESP core */
        if (ip) {
            memcpy(ip, esp.sta.ip, 4);          /* Copy IP address */
        }
        if (gw) {
            memcpy(gw, esp.sta.gw, 4);          /* Copy gateway address */
        }
        if (nm) {
            memcpy(nm, esp.sta.nm, 4);          /* Copy netmask address */
        }
        res = espOK;
        ESP_CORE_UNPROTECT();                   /* Unprotect ESP core */
    }
    return res;
}

/******************************************************************************/
/******************************************************************************/
/***                      Access point related functions                     **/
/******************************************************************************/
/******************************************************************************/

/**
 * \brief           Get access point IP address
 * \param[out]      ip: Pointer to variable to save IP address. Memory of at least 4 bytes is required
 * \param[out]      gw: Pointer to output variable to save gateway address. Memory of at least 4 bytes is required
 * \param[out]      nm: Pointer to output variable to save netmask address. Memory of at least 4 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) IP to read
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ap_getip(void* ip, void* gw, void* nm, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAP_GET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.ip = ip;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.gw = gw;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.nm = nm;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Set IP of station mode
 * \param[in]       ip: Pointer to IP address. Memory of at least 4 bytes is required
 * \param[in]       gw: Pointer to gateway address. Set to NULL to use default gateway. Memory of at least 4 bytes is required
 * \param[in]       nm: Pointer to netmask address. Set to NULL to use default netmask. Memory of at least 4 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) IP to set
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ap_setip(const void* ip, const void* gw, const void* nm, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("ip != NULL", ip != NULL);       /* Assert input parameters */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAP_SET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.ip = ip;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.gw = gw;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.nm = nm;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Get MAC of station mode
 * \param[out]      mac: Pointer to output variable to save MAC address. Memory of at least 6 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) IP to read
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ap_getmac(void* mac, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAPMAC_GET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getmac.mac = mac;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getmac.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Set MAC of station mode
 * \param[in]       mac: Pointer to variable with MAC address. Memory of at least 6 bytes is required
 * \param[in]       def: Status whether default (1) or current (1) MAC to write
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ap_setmac(const void* mac, uint8_t def, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("mac != NULL", mac != NULL);     /* Assert input parameters */
    ESP_ASSERT("Bit 0 of byte 0 in AP MAC must be 0!", !(((uint8_t *)mac)[0] & 0x01));
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAPMAC_SET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setmac.mac = mac;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_setmac.def = def;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           List for available access points ESP can connect to
 * \param[in]       ssid: Optional SSID name to search for. Set to NULL to disable filter
 * \param[in]       aps: Pointer to array of available access point parameters
 * \param[in]       apsl: Length of aps array
 * \param[out]      apf: Pointer to output variable to save number of access points found
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ap_list(const char* ssid, esp_ap_t* aps, size_t apsl, size_t* apf, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    if (apf) {
        *apf = 0;
    }
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWLAP;
    ESP_MSG_VAR_REF(msg).msg.ap_list.ssid = ssid;
    ESP_MSG_VAR_REF(msg).msg.ap_list.aps = aps;
    ESP_MSG_VAR_REF(msg).msg.ap_list.apsl = apsl;
    ESP_MSG_VAR_REF(msg).msg.ap_list.apf = apf;
    
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
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_server(uint16_t port, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSERVER;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.port = port;
    
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

/******************************************************************************/
/******************************************************************************/
/***                       Connection related functions                      **/
/******************************************************************************/
/******************************************************************************/

/**
 * \brief           Starts a new connection of specific type
 * \param[out]      conn: Pointer to pointer to \ref esp_conn_t structure to set new connection reference
 * \param[in]       type: Connection type. This parameter can be a value of \ref esp_conn_type_t enumeration
 * \param[in]       host: Connection host. In case of IP, write it as string, ex. "192.168.1.1"
 * \param[in]       host: Connection port
 * \param[in]       arg: Pointer to user argument passed to connection if successfully connected
 * \param[in]       cb_func: Callback function for this connection. Set to NULL in case of default user callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_start(esp_conn_t** conn, esp_conn_type_t type, const char* host, uint16_t port, void* arg, esp_cb_func_t cb_func, uint32_t blocking) {
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
 * \param[in]       ip: Remote port connection
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
 */
espr_t
esp_conn_set_arg(esp_conn_p conn, void* arg) {
    ESP_CORE_PROTECT();
    conn->arg = arg;                            /* Set argument for connection */
    ESP_CORE_UNPROTECT();
    return espOK;
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

#if ESP_PING || __DOXYGEN__
/**
 * \brief           Ping external host
 * \param[in]       host: Host name to ping
 * \param[out]      time: Pointer to output variable to save ping time
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ping(const char* host, uint32_t* time, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_ASSERT("host != NULL", host != NULL);   /* Assert input parameters */
    ESP_ASSERT("time != NULL", time != NULL);   /* Assert input parameters */
		
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_PING;
    ESP_MSG_VAR_REF(msg).msg.tcpip_ping.host = host;
    ESP_MSG_VAR_REF(msg).msg.tcpip_ping.time = time;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}
#endif /* ESP_PING || __DOXYGEN__ */
