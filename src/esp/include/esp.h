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
struct esp_conn_t;
typedef struct esp_conn_t* esp_conn_p;

struct esp_pbuf_t;
typedef struct esp_pbuf_t* esp_pbuf_p;

/**
 * \brief           Data type for callback function
 */
typedef espr_t  (*esp_cb_func_t)(struct esp_cb_t* cb);

/**
 * \brief           List of possible callback types received to user
 */
typedef enum esp_cb_type_t {
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
            struct esp_conn_t* conn;            /*!< Connection where data were received */
            esp_pbuf_p buff;                    /*!< Pointer to received data */
        } conn_data_recv;                       /*!< Network data received */
        struct {
            struct esp_conn_t* conn;            /*!< Connection where data were sent */
        } conn_data_sent;                       /*!< Data successfully sent */
        struct {
            struct esp_conn_t* conn;            /*!< Connection where data were sent */
        } conn_data_send_err;                   /*!< Data were not sent */
        struct {
            const char* host;                   /*!< Host to use for connection */
            uint16_t port;                      /*!< Remote port used for connection */
            esp_conn_type_t type;               /*!< Connection type */
        } conn_error;                           /*!< Client connection start error */
        struct {
            struct esp_conn_t* conn;            /*!< Pointer to connection */
            uint8_t client;                     /*!< Set to 1 if connection is (was) client or 0 if server */
        } conn_active_closed;                   /*!< Process active and closed statuses at the same time. Use with \ref ESP_CB_CONN_ACTIVE or \ref ESP_CB_CONN_CLOSED events */
    } cb;
} esp_cb_t;

#define ESP_SIZET_MAX                           ((size_t)(-1))

#include "esp_init.h"
#include "esp_pbuf.h"
#include "esp_netconn.h"

/**
 * \brief           Align x value to specific number of bits, provided from \ref GUI_MEM_ALIGNMENT configuration
 * \param[in]       x: Input value to align
 * \retval          Input value aligned to specific number of bytes
 */
#define ESP_MEM_ALIGN(x)            ((x + (ESP_MEM_ALIGNMENT - 1)) & ~(ESP_MEM_ALIGNMENT - 1))

espr_t      esp_reset(uint32_t blocking);
espr_t      esp_set_at_baudrate(uint32_t baud, uint32_t blocking);
espr_t      esp_set_wifi_mode(esp_mode_t mode, uint32_t blocking);
espr_t      esp_set_mux(uint8_t mux, uint32_t blocking);

espr_t      esp_set_server(uint16_t port, uint32_t blocking);
espr_t      esp_set_default_server_callback(esp_cb_func_t cb_func);

espr_t      esp_sta_join(const char* name, const char* pass, const uint8_t* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_quit(uint32_t blocking);


espr_t      esp_sta_getip(void* ip, void* gw, void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_sta_setip(const void* ip, const void* gw, const void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_sta_getmac(void* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_setmac(const void* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_has_ip(void);
espr_t      esp_sta_copy_ip(void* ip, void* gw, void* nm);

espr_t      esp_ap_getip(void* ip, void* gw, void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_ap_setip(const void* ip, const void* gw, const void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_ap_getmac(void* mac, uint8_t def, uint32_t blocking);
espr_t      esp_ap_setmac(const void* mac, uint8_t def, uint32_t blocking);

espr_t      esp_ap_list(const char* ssid, esp_ap_t* aps, size_t apsl, size_t* apf, uint32_t blocking);

espr_t      esp_dns_getbyhostname(const char* host, void* ip, uint32_t blocking);

espr_t      esp_ping(const char* host, uint32_t* time, uint32_t blocking);


/**
 * \defgroup        esp_netconn_CONN Connection API
 * \brief           Connection API functions
 * \{
 */
 
espr_t      esp_conn_start(esp_conn_p* conn, esp_conn_type_t type, const char* host, uint16_t port, void* arg, esp_cb_func_t cb_func, uint32_t blocking);
espr_t      esp_conn_close(esp_conn_p conn, uint32_t blocking);
espr_t      esp_conn_send(esp_conn_p conn, const void* data, size_t btw, size_t* bw, uint32_t blocking);
espr_t      esp_conn_sendto(esp_conn_p conn, const void* ip, uint16_t port, const void* data, size_t btw, size_t* bw, uint32_t blocking);
espr_t      esp_conn_set_arg(esp_conn_p conn, void* arg);
uint8_t     esp_conn_is_client(esp_conn_p conn);
uint8_t     esp_conn_is_server(esp_conn_p conn);
uint8_t     esp_conn_is_active(esp_conn_p conn);
uint8_t     esp_conn_is_closed(esp_conn_p conn);
int8_t      esp_conn_getnum(esp_conn_p conn);
espr_t      esp_conn_set_ssl_buffersize(size_t size, uint32_t blocking);
espr_t      esp_get_conns_status(uint32_t blocking);
 
/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_H */
