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
    uint16_t            (*fn_send)(const void *, uint16_t); /*!< Callback function to transmit data */
} esp_ll_t;

/**
 * \brief           List of possible WiFi modes
 */
typedef enum {
#if ESP_MODE_STATION || __DOXYGEN__
    ESP_MODE_STA = 1,                           /*!< Set WiFi mode to station only */
#endif /* ESP_MODE_STATION || __DOXYGEN__ */
#if ESP_MODE_ACCESS_POINT || __DOXYGEN__
    ESP_MODE_AP = 2,                            /*!< Set WiFi mode to access point only */
#endif /* ESP_MODE_ACCESS_POINT || __DOXYGEN__ */
#if (ESP_MODE_STATION_ACCESS_POINT) || __DOXYGEN__
    ESP_MODE_STA_AP = 3,                        /*!< Set WiFi mode to station and access point */
#endif /* (ESP_MODE_STATION_ACCESS_POINT) || __DOXYGEN__ */
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
    ESP_CB_RESET,                               /*!< Device reset detected */
    
    ESP_CB_INIT_FINISH,                         /*!< Initialization has been finished at this point */
    
    ESP_CB_CONN_DATA_RECV,                      /*!< Connection data received */
    ESP_CB_CONN_DATA_SENT,                      /*!< Data were successfully sent */
    ESP_CB_CONN_DATA_SEND_ERR,                  /*!< Error trying to send data */
    ESP_CB_CONN_ACTIVE,                         /*!< Connection just became active */
    ESP_CB_CONN_ERROR,                          /*!< Client connection start was not successful */
    ESP_CB_CONN_CLOSED,                         /*!< Connection was just closed */
    ESP_CB_CONN_POLL,                           /*!< Poll for connection if there are any changes */
    
    ESP_CB_WIFI_CONNECTED,                      /*!< Station just connected to AP */
    ESP_CB_WIFI_GOT_IP,                         /*!< Station has valid IP */
    ESP_CB_WIFI_DISCONNECTED,                   /*!< Station just disconnected from AP */
} esp_cb_type_t;

/**
 * \brief           Global callback structure to pass as parameter to callback function
 */
typedef struct esp_cb_t {
    esp_cb_type_t type;                         /*!< Callback type */
    union {
        struct {
            esp_conn_p conn;                    /*!< Connection where data were received */
            esp_pbuf_p buff;                    /*!< Pointer to received data */
        } conn_data_recv;                       /*!< Network data received */
        struct {
            esp_conn_p conn;                    /*!< Connection where data were sent */
            size_t sent;                        /*!< Number of bytes sent on connection */
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
            esp_conn_p conn;                    /*!< Pointer to connection */
            uint8_t client;                     /*!< Set to 1 if connection is/was client mode */
            uint8_t forced;                     /*!< Set to 1 if connection action was forced (when active, 1 = CLIENT, 0 = SERVER; when closed, 1 = CMD, 0 = REMOTE) */
        } conn_active_closed;                   /*!< Process active and closed statuses at the same time. Use with \ref ESP_CB_CONN_ACTIVE or \ref ESP_CB_CONN_CLOSED events */
        struct {
            esp_conn_p conn;                    /*!< Set connection pointer */
        } conn_poll;
    } cb;
} esp_cb_t;

#define ESP_SIZET_MAX                           ((size_t)(-1))

#include "esp_init.h"
#include "esp_pbuf.h"
#include "esp_conn.h"
#include "esp_sta.h"
#include "esp_ap.h"
#include "esp_netconn.h"

#define ESP_ASSERT(msg, c)   do {   \
    if (!(c)) {                     \
        ESP_DEBUGF(ESP_DBG_ASSERT, "Wrong parameters on file %s and line %d: %s\r\n", __FILE__, __LINE__, msg); \
        return espPARERR;           \
    }                               \
} while (0)

/**
 * \brief           Align x value to specific number of bytes, provided from \ref ESP_MEM_ALIGNMENT configuration
 * \param[in]       x: Input value to align
 * \retval          Input value aligned to specific number of bytes
 */
#define ESP_MEM_ALIGN(x)            ((x + (ESP_MEM_ALIGNMENT - 1)) & ~(ESP_MEM_ALIGNMENT - 1))

espr_t      esp_reset(uint32_t blocking);
espr_t      esp_set_at_baudrate(uint32_t baud, uint32_t blocking);
espr_t      esp_set_wifi_mode(esp_mode_t mode, uint32_t blocking);
espr_t      esp_set_mux(uint8_t mux, uint32_t blocking);

espr_t      esp_set_server(uint16_t port, uint16_t max_conn, uint16_t timeout, esp_cb_func_t cb, uint32_t blocking);

espr_t      esp_dns_getbyhostname(const char* host, void* ip, uint32_t blocking);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_H */
