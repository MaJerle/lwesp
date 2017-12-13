/**
 * \file            esp_private.h
 * \brief           Private structures and enumerations
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
#ifndef __ESP_PRIV_H
#define __ESP_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdint.h"
#include "stdlib.h"
#include "string.h"

#define ESP_INTERNAL
#ifdef ESP_INTERNAL

#include "esp.h"

/**
 * \brief           List of possible messages
 */
typedef enum {
    ESP_CMD_IDLE = 0,                           /*!< IDLE mode */
    ESP_CMD_DATA_RECV,                          /*!< Data were received from device */
    
    /**
     * Basic AT commands
     */
    ESP_CMD_RESET,                              /*!< Reset device */
    ESP_CMD_GMR,
    ESP_CMD_GSLP,
    ESP_CMD_ECHO,
    ESP_CMD_RESTORE,
    ESP_CMD_UART,
    ESP_CMD_SLEEP,
    ESP_CMD_WAKEUPGPIO,
    ESP_CMD_RFPOWER,
    ESP_CMD_RFVDD,
    ESP_CMD_RFAUTOTRACE,
    ESP_CMD_SYSRAM,
    ESP_CMD_SYSADC,
    ESP_CMD_SYSIOSETCFG,
    ESP_CMD_SYSIOGETCFG,
    ESP_CMD_SYSGPIODIR,
    ESP_CMD_SYSGPIOWRITE,
    ESP_CMD_SYSGPIOREAD,
    
    /**
     * WiFi based commands
     */
    ESP_CMD_WIFI_CWMODE,                        /*!< Set/Get wifi mode */
#if ESP_MODE_STATION || __DOXYGEN__
    ESP_CMD_WIFI_CWJAP,                         /*!< Connect to access point */
    ESP_CMD_WIFI_CWQAP,                         /*!< Disconnect from access point */
    ESP_CMD_WIFI_CWLAP,                         /*!< List available access points */
    ESP_CMD_WIFI_CIPSTAMAC_GET,                 /*!< Get MAC address of ESP station */
    ESP_CMD_WIFI_CIPSTAMAC_SET,                 /*!< Set MAC address of ESP station */
    ESP_CMD_WIFI_CIPSTA_GET,                    /*!< Get IP address of ESP station */
    ESP_CMD_WIFI_CIPSTA_SET,                    /*!< Set IP address of ESP station */
#endif /* ESP_MODE_STATION || __DOXYGEN__ */
#if ESP_MODE_ACCESS_POINT || __DOXYGEN__
    ESP_CMD_WIFI_CWSAP_GET,                     /*!< Get software access point configuration */
    ESP_CMD_WIFI_CWSAP_SET,                     /*!< Set software access point configuration */
    ESP_CMD_WIFI_CIPAPMAC_GET,                  /*!< Get MAC address of ESP access point */
    ESP_CMD_WIFI_CIPAPMAC_SET,                  /*!< Set MAC address of ESP access point */
    ESP_CMD_WIFI_CIPAP_GET,                     /*!< Get IP address of ESP access point */
    ESP_CMD_WIFI_CIPAP_SET,                     /*!< Set IP address of ESP access point */
#endif /* ESP_MODE_STATION || __DOXYGEN__ */
    ESP_CMD_WIFI_WPS,                           /*!< Set WPS option */
    ESP_CMD_WIFI_MDNS,                          /*!< Configure MDNS function */
    ESP_CMD_WIFI_CWHOSTNAME,                    /*!< Set/Get device hostname */
    
    /**
     * TCP/IP related commands
     */
    ESP_CMD_TCPIP_CIPSTATUS,                    /*!< Get status of connections */
#if ESP_DNS || __DOXYGEN__
    ESP_CMD_TCPIP_CIPDOMAIN,                    /*!< Get IP address from domain name = DNS function */
#endif /* ESP_DNS || __DOXYGEN__ */
    ESP_CMD_TCPIP_CIPSTART,                     /*!< Start client connection */
    ESP_CMD_TCPIP_CIPSSLSIZE,                   /*!< Set SSL buffer size for SSL connection */
    ESP_CMD_TCPIP_CIPSEND,                      /*!< Send network data */
    ESP_CMD_TCPIP_CIPCLOSE,                     /*!< Close active connection */
    ESP_CMD_TCPIP_CIFSR,                        /*!< Get local IP */
    ESP_CMD_TCPIP_CIPMUX,                       /*!< Set single or multiple connections */
    ESP_CMD_TCPIP_CIPSERVER,                    /*!< Enables/Disables server mode */
    ESP_CMD_TCPIP_CIPMODE,                      /*!< Transmission mode, either transparent or normal one */
    ESP_CMD_TCPIP_CIPSTO,                       /*!< Sets connection timeout */
#if ESP_PING || __DOXYGEN__
    ESP_CMD_TCPIP_PING,                         /*!< Ping domain */
#endif /* ESP_PING || __DOXYGEN__ */
    ESP_CMD_TCPIP_CIUPDATE,                     /*!< Perform self-update */
#if ESP_SNTP || __DOXYGEN__
    ESP_CMD_TCPIP_CIPSNTPCFG,                   /*!< Configure SNTP servers */
    ESP_CMD_TCPIP_CIPSNTPTIME,                  /*!< Get current time using SNTP */
#endif /* ESP_SNT || __DOXYGEN__ */
    ESP_CMD_TCPIP_CIPDNS,                       /*!< Configure user specific DNS servers */
    ESP_CMD_TCPIP_CIPDINFO,                     /*!< Configure what data are received on +IPD statement */
} esp_cmd_t;

/**
 * \brief           Connection structure
 */
typedef struct esp_conn_t {
    esp_conn_type_t type;                       /*!< Connection type */
    uint8_t         num;                        /*!< Connection number */
    uint8_t         remote_ip[4];               /*!< Remote IP address */
    uint16_t        remote_port;                /*!< Remote port number */
    uint16_t        local_port;                 /*!< Local IP address */
    esp_cb_func_t   cb_func;                    /*!< Callback function for connection */
    void*           arg;                        /*!< User custom argument */
    union {
        struct {
            uint8_t active:1;                   /*!< Status whether connection is active */
            uint8_t client:1;                   /*!< Status whether connection is in client mode */
            uint8_t data_received:1;            /*!< Status whether first data were received on connection */
        } f;
    } status;
} esp_conn_t;

/**
 * \brief           Packet buffer structure
 */
typedef struct esp_pbuf_t {
    struct esp_pbuf_t* next;                    /*!< Next pbuf in chain list */
    size_t tot_len;                             /*!< Total length of pbuf chain */
    size_t len;                                 /*!< Length of payload */
    uint16_t ref;                               /*!< Number of references to this structure */
    uint8_t* payload;                           /*!< Pointer to payload memory */
    uint8_t ip[4];                              /*!< Remote address for received IPD data */
    uint16_t port;                              /*!< Remote port for received IPD data */
} esp_pbuf_t;

/**
 * \brief           Incoming network data read structure
 */
typedef struct {
    uint8_t             read;                   /*!< Set to 1 when we should process input data as connection data */
    size_t              tot_len;                /*!< Total length of packet */
    size_t              rem_len;                /*!< Remaining bytes to read in current +IPD statement */
    esp_conn_p          conn;                   /*!< Pointer to connection for network data */
    uint8_t             ip[4];                  /*!< Remote IP address on from IPD data */
    uint16_t            port;                   /*!< Remote port on IPD data */
    
    size_t              buff_ptr;               /*!< Buffer pointer to save data to */
    esp_pbuf_p          buff;                   /*!< Pointer to data buffer used for receiving data */
} esp_ipd_t;

/**
 * \brief           Message queue structure to share between threads
 */
typedef struct esp_msg {
    esp_cmd_t       cmd_def;                    /*!< Default message type received from queue */
    esp_cmd_t       cmd;                        /*!< Since some commands can have different subcommands, sub command is used here */
    uint8_t         i;
    esp_sys_sem_t   sem;                        /*!< Semaphore for the message */
    uint32_t        block_time;                 /*!< Maximal blocking time in units of milliseconds. Use 0 to for non-blocking call */
    espr_t          res;                        /*!< Result of message operation */
    espr_t          (*fn)(struct esp_msg *);    /*!< Processing callback function to process packet */
    union {
        struct {
            uint32_t baudrate;                  /*!< Baudrate for AT port */
        } uart;
        struct {
            esp_mode_t mode;                    /*!< Mode of operation */                    
        } wifi_mode;                            /*!< When message type \ref ESP_CMD_WIFI_MODE is used */
        struct {
            const char* name;                   /*!< AP name */
            const char* pass;                   /*!< AP password */
            const uint8_t* mac;                 /*!< Specific MAC address to use when connecting to AP */
            uint8_t def;                        /*!< Value indicates to connect as current only or as default */
        } sta_join;                             /*!< Message for joining to access point */
        struct {
            uint8_t* ip;                        /*!< Pointer to IP variable */
            uint8_t* gw;                        /*!< Pointer to gateway variable */
            uint8_t* nm;                        /*!< Pointer to netmask variable */
            uint8_t def;                        /*!< Value for receiving default or current settings */
        } sta_ap_getip;                         /*!< Message for reading station or access point IP */
        struct {
            uint8_t* mac;                       /*!< Pointer to MAC variable */
            uint8_t def;                        /*!< Value for receiving default or current settings */
        } sta_ap_getmac;                        /*!< Message for reading station or access point MAC address */
        struct {
            const uint8_t* ip;                  /*!< Pointer to IP variable */
            const uint8_t* gw;                  /*!< Pointer to gateway variable */
            const uint8_t* nm;                  /*!< Pointer to netmask variable */
            uint8_t def;                        /*!< Value for receiving default or current settings */
        } sta_ap_setip;                         /*!< Message for setting station or access point IP */
        struct {
            const uint8_t* mac;                 /*!< Pointer to MAC variable */
            uint8_t def;                        /*!< Value for receiving default or current settings */
        } sta_ap_setmac;                        /*!< Message for setting station or access point MAC address */
        
        struct {
            const char* ssid;                   /*!< Pointer to optional filter SSID name to search */
            esp_ap_t* aps;                      /*!< Pointer to array to save access points */
            size_t apsl;                        /*!< Length of input array of access points */
            size_t apsi;                        /*!< Current access point array */
            size_t* apf;                        /*!< Pointer to output variable holding number of access points found */
        } ap_list;                              /*!< List for access points */
        
        struct {
            const char* ssid;                   /*!< Name of access point */
            const char* pwd;                    /*!< Password of access point */
            esp_ecn_t ecn;                      /*!< Ecryption used */
            uint8_t ch;                         /*!< RF Channel used */
            uint8_t max_sta;                    /*!< Max allowed connected stations */
            uint8_t hid;                        /*!< Configuration if network is hidden or visible */
            uint8_t def;                        /*!< Save as default configuration */
        } ap_conf;                              /*!< Parameters to configura access point */
        
        /**
         * Connection based commands
         */
        struct {
            esp_conn_t** conn;                  /*!< Pointer to pointer to save connection used */
            const char* host;                   /*!< Host to use for connection */
            uint16_t port;                      /*!< Remote port used for connection */
            esp_conn_type_t type;               /*!< Connection type */
            void* arg;                          /*!< Connection custom argument */
            esp_cb_func_t cb_func;              /*!< Callback function to use on connection */
            uint8_t num;                        /*!< Connection number used for start */
        } conn_start;                           /*!< Structure for starting new connection */
        struct {
            esp_conn_t* conn;                   /*!< Pointer to connection to close */
        } conn_close;
        struct {
            esp_conn_t* conn;                   /*!< Pointer to connection to send data */
            size_t btw;                         /*!< Number of bytes to write */
            size_t* bw;                         /*!< Number of bytes written so far */
            const uint8_t* data;                /*!< Data to send */
            size_t sent;                        /*!< Number of bytes sent in last packet */
            uint8_t tries;                      /*!< Number of tries used for last packet */
            uint8_t wait_send_ok_err;           /*!< Set to 1 when we wait for SEND OK or SEND ERROR */
            const uint8_t* remote_ip;           /*!< Remote IP address for UDP connection */
            uint16_t remote_port;               /*!< Remote port address for UDP connection */
        } conn_send;                            /*!< Structure to send data on connection */
        
        /*
         * TCP/IP based commands
         */
        struct {
            uint8_t mux;                        /*!< Mux status, either enabled or disabled */
        } tcpip_mux;                            /*!< Used for setting up multiple connections */
        struct {
            uint16_t port;                      /*!< Server port number */
        } tcpip_server;
        struct {
            uint8_t info;                       /*!< New info status */
        } tcpip_dinfo;                          /*!< Structure to enable more info on +IPD command */
        struct {
            const char* host;                   /*!< Hostname to ping */
            uint32_t* time;                     /*!< Pointer to time variable */
        } tcpip_ping;                           /*!< Pinging structure */
        struct {
            size_t size;                        /*!< Size for SSL in uints of bytes */
        } tcpip_sslsize;                        /*!< TCP SSL size for SSL connections */
        struct {
            const char* host;                   /*!< Hostname to resolve IP address for */
            uint8_t* ip;                        /*!< Pointer to IP address to save result */
        } dns_getbyhostname;                    /*!< DNS function */
    } msg;                                      /*!< Group of different possible message contents */
} esp_msg_t;

/**
 * \brief           Sequential API structure
 */
typedef struct esp_netconn_t {
    esp_netconn_type_t type;                    /*!< Netconn type */
    uint16_t listen_port;                       /*!< Port on which we are listening */
    
    size_t rcv_packets;                         /*!< Number of received packets so far on this connection */
    esp_conn_t* conn;                           /*!< Pointer to actual connection */
    
    esp_sys_sem_t mbox_accept;                  /*!< List of active connections waiting to be processed */
    esp_sys_sem_t mbox_receive;                 /*!< Message queue for receive mbox */
} esp_netconn_t;

/**
 * \brief           IP and MAC structure with netmask and gateway addresses
 */
typedef struct {
    uint8_t             ip[4];                  /*!< IP address */
    uint8_t             gw[4];                  /*!< Gateway address */
    uint8_t             nm[4];                  /*!< Netmask address */
    uint8_t             mac[6];                 /*!< MAC address */
} esp_ip_mac_t;

/**
 * \brief           ESP global structure
 */
typedef struct {    
    esp_sys_sem_t       sem_sync;               /*!< Synchronization semaphore between threads */
    esp_sys_mbox_t      mbox_producer;          /*!< Producer message queue handle */
    esp_sys_mbox_t      mbox_process;           /*!< Consumer message queue handle */
    esp_sys_thread_t    thread_producer;        /*!< Producer thread handle */
    esp_sys_thread_t    thread_process;         /*!< Processing thread handle */
    esp_ll_t            ll;                     /*!< Low level functions */
    esp_buff_t          buff;                   /*!< Input processing buffer */
    
    esp_msg_t*          msg;                    /*!< Pointer to current user message being executed */
    
    uint8_t             active_conns;           /*!< Bit field of currently active connections */
    uint8_t             active_conns_last;      /*!< The same as previous but status before last check */
    
    esp_conn_t          conns[ESP_MAX_CONNS];   /*!< Array of all connection structures */
    
    esp_ipd_t           ipd;                    /*!< Incoming data structure */
    esp_cb_t            cb;                     /*!< Callback processing structure */
    
    esp_cb_func_t       cb_func;                /*!< Default callback function */
    esp_cb_func_t       cb_server;              /*!< Default callback function for server connections */
    
#if ESP_MODE_STATION || __DOXYGEN__
    esp_ip_mac_t        sta;                    /*!< Station IP and MAC addressed */
#endif /* ESP_MODE_STATION || __DOXYGEN__ */
#if ESP_MODE_ACCESS_POINT || __DOXYGEN__
    esp_ip_mac_t        ap;                     /*!< Access point IP and MAC addressed */
#endif /* ESP_MODE_ACCESS_POINT || __DOXYGEN__ */
    
    union {
        struct {            
            uint8_t     r_got_ip:1;             /*!< Flag indicating ESP has IP */
            uint8_t     r_w_conn:1;             /*!< Flag indicating ESP is connected to wifi */
        } f;                                    /*!< Flags structure */
    } status;                                   /*!< Status structure */
} esp_t;

/**
 * \brief           Unicode support structure
 */
typedef struct esp_unicode_t {
    uint8_t ch[4];                              /*!< UTF-8 max characters */
    uint8_t t;                                  /*!< Total expected length in UTF-8 sequence */
    uint8_t r;                                  /*!< Remaining bytes in UTF-8 sequence */
    espr_t res;                                 /*!< Current result of processing */
} esp_unicode_t;

extern esp_t esp;

#define ESP_CHARISNUM(x)                    ((x) >= '0' && (x) <= '9')
#define ESP_CHARISHEXNUM(x)                 (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))
#define ESP_CHARTONUM(x)                    ((x) - '0')
#define ESP_CHARHEXTONUM(x)                 (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'f') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'F') ? ((x) - 'A' + 10) : 0)))
#define ESP_MIN(x, y)                       ((x) < (y) ? (x) : (y))
#define ESP_MAX(x, y)                       ((x) > (y) ? (x) : (y))
#define ESP_ISVALIDASCII(x)                 (((x) >= 32 && (x) <= 126) || (x) == '\r' || (x) == '\n')
#define ESP_UNUSED(x)                       ((void)(x))
    
#ifndef ESP_DBG_ASSERT
#define ESP_DBG_ASSERT                      ESP_DBG_OFF
#endif /* ESP_DBG_ASSERT */

#define ESP_ASSERT(msg, c)   do {   \
    if (!(c)) {                     \
        ESP_DEBUGF(ESP_DBG_ASSERT, "Wrong parameters on file %s and line %d: %s\r\n", __FILE__, __LINE__, msg); \
        return espPARERR;           \
    }                               \
} while (0)

/**
 * System protection macros
 */
#define ESP_CORE_UNPROTECT()                esp_sys_unprotect()
#define ESP_CORE_PROTECT()                  esp_sys_protect()

const char * espi_dbg_msg_to_string(esp_cmd_t cmd);

espr_t      espi_process(void);
espr_t      espi_initiate_cmd(esp_msg_t* msg);
uint8_t     espi_is_valid_conn_ptr(esp_conn_p conn);
espr_t      espi_send_cb(esp_cb_type_t type);

espr_t      espi_send_msg_to_producer_mbox(esp_msg_t* msg, espr_t (*process_fn)(esp_msg_t *), uint32_t block_time);


#endif /* ESP_INTERNAL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_PRIV_H */
