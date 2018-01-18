/**
 * \file            esp.h
 * \brief           ESP AT commands parser
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
#ifndef __ESP_H
#define __ESP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Include other library files */
#include "esp_config.h"

#include "esp/esp_typedefs.h"
#include "esp/esp_buff.h"
#include "esp/esp_input.h"
#include "system/esp_sys.h"
#include "esp/esp_debug.h"

/**
 * \defgroup        ESP ESP AT lib
 * \brief           ESP stack
 * \{
 */
 
/**
 * \addtogroup      ESP_LL
 * \{
 */
 
/**
 * \brief           Function prototype for AT output data
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
typedef uint16_t (*esp_ll_send_fn)(const void* data, uint16_t len);

/**
 * \brief           Low level user specific functions
 */
typedef struct {
    esp_ll_send_fn send_fn;                     /*!< Callback function to transmit data */
} esp_ll_t;

/**
 * \}
 */

/**
 * \addtogroup      ESP_TYPEDEFS
 * \{
 */

/**
 * \brief           List of possible WiFi modes
 */
typedef enum {
#if ESP_CFG_MODE_STATION || __DOXYGEN__
    ESP_MODE_STA = 1,                           /*!< Set WiFi mode to station only */
#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */
#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
    ESP_MODE_AP = 2,                            /*!< Set WiFi mode to access point only */
#endif /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if (ESP_CFG_MODE_STATION_ACCESS_POINT) || __DOXYGEN__
    ESP_MODE_STA_AP = 3,                        /*!< Set WiFi mode to station and access point */
#endif /* (ESP_CFG_MODE_STATION_ACCESS_POINT) || __DOXYGEN__ */
} esp_mode_t;

/**
 * \brief           List of possible connection types
 */
typedef enum {
    ESP_CONN_TYPE_TCP,                          /*!< Connection type is TCP */
    ESP_CONN_TYPE_UDP,                          /*!< Connection type is UDP */
    ESP_CONN_TYPE_SSL,                          /*!< Connection type is SSL */
} esp_conn_type_t;

/* Forward declarations */
struct esp_cb_t;
struct esp_conn_t;
struct esp_pbuf_t;

/**
 * \brief           Pointer to \ref esp_conn_t structure
 */
typedef struct esp_conn_t* esp_conn_p;

/**
 * \brief           Pointer to \ref esp_pbuf_t structure
 */
typedef struct esp_pbuf_t* esp_pbuf_p;

/**
 * \brief           Data type for callback function
 * \param[in]       cb: Callback event data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
typedef espr_t  (*esp_cb_fn)(struct esp_cb_t* cb);

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
  
    ESP_CB_STA_LIST_AP,                         /*!< Station list AP event */
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
        } conn_data_recv;                       /*!< Network data received. Use with \ref ESP_CB_CONN_DATA_RECV event */
        struct {
            esp_conn_p conn;                    /*!< Connection where data were sent */
            size_t sent;                        /*!< Number of bytes sent on connection */
        } conn_data_sent;                       /*!< Data successfully sent. Use with \ref ESP_CB_CONN_DATA_SENT event */
        struct {
            esp_conn_p conn;                    /*!< Connection where data were sent */
            size_t sent;                        /*!< Number of bytes sent on connection before error occurred */
        } conn_data_send_err;                   /*!< Data were not sent. Use with \ref ESP_CB_CONN_DATA_SEND_ERR event */
        struct {
            const char* host;                   /*!< Host to use for connection */
            uint16_t port;                      /*!< Remote port used for connection */
            esp_conn_type_t type;               /*!< Connection type */
            void* arg;                          /*!< Connection argument used on connection */
            /**
             * \todo: Implement error reason:
             *  - No free connection to start
             *  - Remote host is not responding
             *  - ...
             */
        } conn_error;                           /*!< Client connection start error */
        struct {
            esp_conn_p conn;                    /*!< Pointer to connection */
            uint8_t client;                     /*!< Set to 1 if connection is/was client mode */
            uint8_t forced;                     /*!< Set to 1 if connection action was forced (when active, 1 = CLIENT, 0 = SERVER; when closed, 1 = CMD, 0 = REMOTE) */
        } conn_active_closed;                   /*!< Process active and closed statuses at the same time. Use with \ref ESP_CB_CONN_ACTIVE or \ref ESP_CB_CONN_CLOSED events */
        struct {
            esp_conn_p conn;                    /*!< Set connection pointer */
        } conn_poll;                            /*!< Polling active connection to check for timeouts. Use with \ref ESP_CB_CONN_POLL event */
        struct {
            espr_t status;                      /*!< Status of command */
            esp_ap_t* aps;                      /*!< Pointer to access points */
            size_t len;                         /*!< Number of access points found */
        } sta_list_ap;
    } cb;                                       /*!< Callback event union */
} esp_cb_t;

#define ESP_SIZET_MAX                           ((size_t)(-1))  /*!< Maximal value of size_t variable type */

/**
 * \}
 */

#include "esp/esp_utilities.h"
#include "esp/esp_pbuf.h"
#include "esp/esp_conn.h"
#include "esp/esp_sta.h"
#include "esp/esp_ap.h"
#if ESP_CFG_OS
#include "esp/esp_netconn.h"
#endif /* ESP_CFG_OS */

espr_t      esp_init(esp_cb_fn cb_func);
espr_t      esp_reset(uint32_t blocking);
espr_t      esp_set_at_baudrate(uint32_t baud, uint32_t blocking);
espr_t      esp_set_wifi_mode(esp_mode_t mode, uint32_t blocking);
espr_t      esp_set_mux(uint8_t mux, uint32_t blocking);

espr_t      esp_set_server(uint16_t port, uint16_t max_conn, uint16_t timeout, esp_cb_fn cb, uint32_t blocking);

espr_t      esp_dns_getbyhostname(const char* host, void* ip, uint32_t blocking);

espr_t      esp_core_lock(void);
espr_t      esp_core_unlock(void);

espr_t      esp_cb_register(esp_cb_fn cb_fn);
espr_t      esp_cb_unregister(esp_cb_fn cb_fn);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_H */
