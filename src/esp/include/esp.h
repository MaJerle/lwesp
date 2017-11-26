/**
 * \file            esp.h
 * \brief           ESP AT commands parser
 */
 
/*
 *
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
#ifndef __ESP_H
#define __ESP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Include other library files */
#include "esp_config.h"
#include "esp_typedefs.h"
#include "esp_sys.h"
#include "esp_buff.h"
#include "esp_input.h"

/**
 * \brief           Low level user specific functions
 */
typedef struct {
    uint16_t            (*send)(const uint8_t *, uint16_t); /*!< Callback function to transmit data */
} esp_ll_t;

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
    ESP_CMD_WIFI_CWJAP,                         /*!< Connect to access point */
    ESP_CMD_WIFI_CWQAP,                         /*!< Disconnect from access point */
    ESP_CMD_WIFI_CWLAP,                         /*!< List available access points */
    ESP_CMD_WIFI_CIPSTAMAC_GET,                 /*!< Get MAC address of ESP station */
    ESP_CMD_WIFI_CIPSTAMAC_SET,                 /*!< Set MAC address of ESP station */
    ESP_CMD_WIFI_CIPAPMAC_GET,                  /*!< Get MAC address of ESP access point */
    ESP_CMD_WIFI_CIPAPMAC_SET,                  /*!< Set MAC address of ESP access point */
    ESP_CMD_WIFI_CIPSTA_GET,                    /*!< Get IP address of ESP station */
    ESP_CMD_WIFI_CIPSTA_SET,                    /*!< Set IP address of ESP station */
    ESP_CMD_WIFI_CIPAP_GET,                     /*!< Get IP address of ESP access point */
    ESP_CMD_WIFI_CIPAP_SET,                     /*!< Set IP address of ESP access point */
    ESP_CMD_WIFI_WPS,                           /*!< Set WPS option */
    ESP_CMD_WIFI_MDNS,                          /*!< Configure MDNS function */
    ESP_CMD_WIFI_CWHOSTNAME,                    /*!< Set/Get device hostname */
    
    /**
     * TCP/IP related commands
     */
    ESP_CMD_TCPIP_CIPSTATUS,                    /*!< Get status of connections */
    ESP_CMD_TCPIP_CIPDOMAIN,                    /*!< Get IP address from domain name = DNS function */
    ESP_CMD_TCPIP_CIPSTART,                     /*!< Start client connection */
    ESP_CMD_TCPIP_CIPSSLSIZE,                   /*!< Set SSL buffer size for SSL connection */
    ESP_CMD_TCPIP_CIPSEND,                      /*!< Send network data */
    ESP_CMD_TCPIP_CIPCLOSE,                     /*!< Close active connection */
    ESP_CMD_TCPIP_CIFSR,                        /*!< Get local IP */
    ESP_CMD_TCPIP_CIPMUX,                       /*!< Set single or multiple connections */
    ESP_CMD_TCPIP_CIPSERVER,                    /*!< Enables/Disables server mode */
    ESP_CMD_TCPIP_CIPMODE,                      /*!< Transmission mode, either transparent or normal one */
    ESP_CMD_TCPIP_CIPSTO,                       /*!< Sets connection timeout */
    ESP_CMD_TCPIP_PING,                         /*!< Ping domain */
    ESP_CMD_TCPIP_CIUPDATE,                     /*!< Perform self-update */
    ESP_CMD_TCPIP_CIPSNTPCFG,                   /*!< Configure SNTP servers */
    ESP_CMD_TCPIP_CIPSNTPTIME,                  /*!< Get current time using SNTP */
    ESP_CMD_TCPIP_CIPDNS,                       /*!< Configure user specific DNS servers */
    ESP_CMD_TCPIP_CIPDINFO,                     /*!< Configure what data are received on +IPD statement */
} esp_cmd_t;

/**
 * \brief           List of possible WiFi modes
 */
typedef enum {
    ESP_MODE_STA = 1,                           /*!< Set WiFi mode to station only */
    ESP_MODE_AP,                                /*!< Set WiFi mode to access point only */
    ESP_MODE_STA_AP                             /*!< Set WiFi mode to station and access point */
} esp_mode_t;

/**
 * \brief           List of possible connection types
 */
typedef enum {
    ESP_CONN_TYPE_TCP,                          /*!< Connection type is TCP */
    ESP_CONN_TYPE_UDP,                          /*!< Connection type is UDP */
    ESP_CONN_TYPE_SSL,                          /*!< Connection type is SSL */
} esp_conn_type_t;

struct esp_cb_t;

/**
 * \brief           Data type for callback function
 */
typedef espr_t  (*esp_cb_func_t)(struct esp_cb_t* cb);

/**
 * \brief           Connection structure
 */
typedef struct {
    uint8_t         num;                        /*!< Connection number */
    uint8_t         remote_ip[4];               /*!< Remote IP address */
    uint16_t        remote_port;                /*!< Remote port number */
    uint16_t        local_port;                 /*!< Local IP address */
    esp_cb_func_t   cb_func;                    /*!< Callback function for connection */
    union {
        struct {
            uint8_t active:1;                   /*!< Status whether connection is active */
            uint8_t client:1;                   /*!< Status whether connection is in client mode */
        } f;
    } status;
} esp_conn_t;

/**
 * \brief           Incoming network data read structure
 */
typedef struct {
    uint8_t             read;                   /*!< Set to 1 when we should process input data as connection data */
    size_t              tot_len;                /*!< Total length of packet */
    size_t              rem_len;                /*!< Remaining bytes to read in current +IPD statement */
    esp_conn_t*         conn;                   /*!< Pointer to connection for network data */
    
    size_t              buff_ptr;               /*!< Buffer pointer to save data to */
    size_t              buff_len;               /*!< Length of entire buffer */
    uint8_t*            buff;                   /*!< Pointer to data buffer used for receiving data */
} esp_ipd_t;

/**
 * \brief           Message queue structure to share between threads
 */
typedef struct esp_msg {
    esp_cmd_t       cmd;                        /*!< Message type received from queue */
    esp_sys_sem_t   sem;                        /*!< Semaphore for the message */
    uint32_t        block_time;                 /*!< Maximal blocking time in units of milliseconds. Use 0 to for non-blocking call */
    espr_t          res;                        /*!< Result of message operation */
    espr_t          (*fn)(struct esp_msg *);    /*!< Processing callback function to process packet */
    uint8_t         def;                        /*!< Since some commands allow default mode saved to flash, value here indicates if command should be saved to flash */
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
        } sta_join;                             /*!< Message for joining to access point */
        struct {
            uint8_t* ip;                        /*!< Pointer to IP variable */
            uint8_t* gw;                        /*!< Pointer to gateway variable */
            uint8_t* nm;                        /*!< Pointer to netmask variable */
            uint8_t def;                        /*!< Value for receiving default or current settings  */
        } sta_ap_getip;                         /*!< Message for reading station or access point IP */
        struct {
            uint8_t* mac;                       /*!< Pointer to MAC variable */
            uint8_t def;                        /*!< Value for receiving default or current settings  */
        } sta_ap_getmac;                        /*!< Message for reading station or access point MAC address */
        struct {
            const uint8_t* ip;                  /*!< Pointer to IP variable */
            const uint8_t* gw;                  /*!< Pointer to gateway variable */
            const uint8_t* nm;                  /*!< Pointer to netmask variable */
            uint8_t def;                        /*!< Value for receiving default or current settings  */
        } sta_ap_setip;                         /*!< Message for setting station or access point IP */
        struct {
            const uint8_t* mac;                 /*!< Pointer to MAC variable */
            uint8_t def;                        /*!< Value for receiving default or current settings  */
        } sta_ap_setmac;                        /*!< Message for setting station or access point MAC address */
        
        /**
         * Connection based commands
         */
        
        struct {
            esp_conn_t** conn;                  /*!< Pointer to pointer to save connection used */
            const char* host;                   /*!< Host to use for connection */
            uint16_t port;                      /*!< Remote port used for connection */
            esp_conn_type_t type;               /*!< Connection type */
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
    } msg;                                      /*!< Group of different possible message contents */
} esp_msg_t;

/**
 * \brief           List of possible callback types received to user
 */
typedef enum {
    ESP_CB_INIT_FINISH,                         /*!< Initialization has been finished at this point */
    
    ESP_CB_DATA_RECV,                           /*!< Connection data received */
    ESP_CB_DATA_SENT,                           /*!< Data were successfully sent */
    ESP_CB_DATA_SEND_ERR,                       /*!< Error trying to send data */
    ESP_CB_CONN_ACTIVE,                         /*!< Connection just became active */
    ESP_CB_CONN_ERROR,                          /*!< Client connection start was not successful */
    ESP_CB_CONN_CLOSED,                         /*!< Connection was just closed */
} esp_cb_type_t;

/**
 * \brief           Global callback structure to pass as parameter to callback function
 */
typedef struct esp_cb_t {
    esp_cb_type_t       type;                   /*!< Callback type */
    union {
        struct {
            esp_conn_t* conn;                   /*!< Connection where data were received */
            const uint8_t* buff;                /*!< Pointer to received data */
            size_t      len;                    /*!< Number of data bytes received. Use when \ref ESP_CB_DATA_RECV event is received */
        } conn_data_recv;                       /*!< Network data received */
        struct {
            esp_conn_t* conn;                   /*!< Connection where data were sent */
        } conn_data_sent;                       /*!< Data successfully sent */
        struct {
            esp_conn_t* conn;                   /*!< Connection where data were sent */
        } conn_data_send_err;                   /*!< Data were not sent */
        struct {
            const char* host;                   /*!< Host to use for connection */
            uint16_t port;                      /*!< Remote port used for connection */
            esp_conn_type_t type;               /*!< Connection type */
        } conn_error;                           /*!< Client connection start error */
        struct {
            esp_conn_t* conn;                   /*!< Pointer to connection */
        } conn_active_closed;                   /*!< Process active and closed statuses at the same time. Use with \ref ESP_CB_CONN_ACTIVE or \ref ESP_CB_CONN_CLOSED events */
    } cb;
} esp_cb_t;

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
    esp_sys_mbox_t      mbox_consumer;          /*!< Consumer message queue handle */
    esp_sys_thread_t    thread_producer;        /*!< Producer thread handle */
    esp_sys_thread_t    thread_consumer;        /*!< Consumer thread handle */
    esp_ll_t            ll;                     /*!< Low level functions */
    esp_buff_t          buff;                   /*!< Input processing buffer */
    esp_cmd_t           cmd;                    /*!< Current active command */
    esp_msg_t*          msg;                    /*!< Pointer to current user message being executed */
    
    uint8_t             active_conns;           /*!< Bit field of currently active connections */
    uint8_t             active_conns_last;      /*!< The same as previous but status before last check */
    
    esp_conn_t          conns[ESP_MAX_CONNS];   /*!< Array of all connection structures */
    
    esp_ipd_t           ipd;                    /*!< Incoming data structure */
    esp_cb_t            cb;                     /*!< Callback processing structure */
    
    esp_cb_func_t       cb_func;                /*!< Default callback function */
    esp_cb_func_t       cb_server;              /*!< Default callback function for server connections */
    
    esp_ip_mac_t        sta;                    /*!< Station IP and MAC addressed */
    esp_ip_mac_t        ap;                     /*!< Access point IP and MAC addressed */
    
    union {
        struct {
            uint8_t     r_ok:1;                 /*!< Set to 1 when OK response is received */
            uint8_t     r_err:1;                /*!< Set to 1 when error response is received */
            uint8_t     r_rdy:1;                /*!< Set to 1 when ready response is received */
        } f;                                    /*!< Flags structure */
    } status;                                   /*!< Status structure */
} esp_t;

#include "esp_init.h"

extern esp_t esp;

#define ESP_CHARISNUM(x)                    ((x) >= '0' && (x) <= '9')
#define ESP_CHARISHEXNUM(x)                 (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))
#define ESP_CHARTONUM(x)                    ((x) - '0')
#define ESP_CHARHEXTONUM(x)                 (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'f') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'F') ? ((x) - 'A' + 10) : 0)))
#define ESP_ISVALIDASCII(x)                 (((x) >= 32 && (x) <= 126) || (x) == '\r' || (x) == '\n')
#define ESP_MIN(x, y)                       ((x) < (y) ? (x) : (y))
#define ESP_MAX(x, y)                       ((x) > (y) ? (x) : (y))

#define ESP_ASSERT(msg, c)   do {   \
    if (!(c)) {                     \
        ESP_DEBUGF(ESP_DBG_ASSERT, "Wrong parameters on file %s and line %d\r\n", __FILE__, __LINE__); \
        return espPARERR;           \
    }                               \
} while (0)

/**
 * \brief           Align x value to specific number of bits, provided from \ref GUI_MEM_ALIGNMENT configuration
 * \param[in]       x: Input value to align
 * \retval          Input value aligned to specific number of bytes
 */
#define ESP_MEM_ALIGN(x)            ((x + (ESP_MEM_ALIGNMENT - 1)) & ~(ESP_MEM_ALIGNMENT - 1))

espr_t      esp_reset(uint32_t blocking);
espr_t      esp_set_at_baudrate(uint32_t baud, uint32_t blocking);
espr_t      esp_set_wifi_mode(esp_mode_t mode, uint32_t blocking);
espr_t      esp_get_conns_status(uint32_t blocking);
espr_t      esp_set_mux(uint8_t mux, uint32_t blocking);

espr_t      esp_set_server(uint16_t port, uint32_t blocking);
espr_t      esp_set_default_server_callback(esp_cb_func_t cb_func);

espr_t      esp_sta_join(const char* name, const char* pass, const uint8_t* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_quit(uint32_t blocking);


espr_t      esp_sta_getip(void* ip, void* gw, void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_sta_setip(const void* ip, const void* gw, const void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_sta_getmac(void* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_setmac(const void* mac, uint8_t def, uint32_t blocking);

espr_t      esp_ap_getip(void* ip, void* gw, void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_ap_setip(const void* ip, const void* gw, const void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_ap_getmac(void* mac, uint8_t def, uint32_t blocking);
espr_t      esp_ap_setmac(const void* mac, uint8_t def, uint32_t blocking);

/**
 * \defgroup        ESP_API_CONN Connection API
 * \brief           Connection API functions
 * \{
 */
 
espr_t      esp_conn_start(esp_conn_t** conn, esp_conn_type_t type, const char* host, uint16_t port, esp_cb_func_t cb_func, uint32_t blocking);
espr_t      esp_conn_close(esp_conn_t* conn, uint32_t blocking);
espr_t      esp_conn_send(esp_conn_t* conn, const void* data, size_t btw, size_t* bw, uint32_t blocking);
espr_t      esp_conn_set_ssl_buffer(size_t size, uint32_t blocking);
uint8_t     esp_conn_is_client(esp_conn_t* conn);
uint8_t     esp_conn_is_server(esp_conn_t* conn);
uint8_t     esp_conn_is_active(esp_conn_t* conn);
uint8_t     esp_conn_is_closed(esp_conn_t* conn);
 
/**
 * \}
 */
 
const char * espi_dbg_msg_to_string(esp_cmd_t cmd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_H */
