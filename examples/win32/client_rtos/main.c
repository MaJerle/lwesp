/**
 * \file            main.c
 * \brief           Main file
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
 * Before you start using WIN32 implementation with USB and VCP,
 * check esp_ll_win32.c implementation and choose your COM port!
 */
#include "stdafx.h"
#include "esp/esp.h"
#include "station_manager.h"
#include "netconn_client.h"

static espr_t esp_callback_func(esp_cb_t* cb);
static espr_t esp_conn_callback_func(esp_cb_t* cb);

#define CONN_HOST           "example.com"
#define CONN_PORT           80

/**
 * \brief           Minimal connection request
 */
static const uint8_t
request_data[] = ""
"GET / HTTP/1.1\r\n"
"Host: " CONN_HOST "\r\n"
"Connection: close\r\n"
"\r\n";

/**
 * \brief           Program entry point
 */
int
main(void) {
    printf("Starting ESP application!\r\n");

    /*
     * Initialize ESP with default callback function
     */
    esp_init(esp_callback_func);

    /*
     * Connect to access point.
     *
     * Try unlimited time until access point accepts up.
     * Check for station_manager.c to define preferred access points ESP should connect to
     */
    connect_to_preferred_access_point(1);

    /*
     * Start new connections to example.com
     *
     * Use non-blocking method and process further data in callback function
     */
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, CONN_PORT, NULL, esp_conn_callback_func, 0);
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, CONN_PORT, NULL, esp_conn_callback_func, 0);
    
    /*
     * An example of connection which should fail in connecting.
     * In this case, \ref ESP_CB_CONN_ERROR event should be triggered
     * in callback function processing
     */
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, 10, NULL, esp_conn_callback_func, 0);

    /*
     * Do not stop program here as we still need to wait
     * for commands to be processed
     */
    while (1) {
        esp_delay(1000);
    }

    return 0;
}

/**
 * \brief           Callback function for connection events
 * \param[in]       cb: Event information with data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_conn_callback_func(esp_cb_t* cb) {
    esp_conn_p conn;

    conn = esp_conn_get_from_evt(cb);           /* Get connection handle from event */
    switch (cb->type) {
        case ESP_CB_CONN_ACTIVE: {              /* Connection just active */
            printf("Connection %d active!\r\n", (int)esp_conn_getnum(conn));
            printf("Sending data on connection %d to remote server\r\n", (int)esp_conn_getnum(conn));
            esp_conn_send(conn, request_data, sizeof(request_data) - 1, NULL, 0);
            break;
        }
        case ESP_CB_CONN_DATA_SENT: {           /* Data successfully sent */
            size_t len = cb->cb.conn_data_sent.sent;
            printf("Successfully sent %d bytes on connection %d\r\n", (int)len, (int)esp_conn_getnum(conn));
            break;
        }
        case ESP_CB_CONN_DATA_SEND_ERR: {       /* Error trying to send data on connection */
            size_t len = cb->cb.conn_data_send_err.sent;
            printf("Error trying to send data on connection %d\r\n", (int)esp_conn_getnum(conn));
            break;
        }
        case ESP_CB_CONN_DATA_RECV: {           /* Connection data received */
            esp_pbuf_p p;
            p = cb->cb.conn_data_recv.buff;     /* Get received buffer */
            if (p != NULL) {
                printf("Connection %d data received with %d bytes\r\n",
                    (int)esp_conn_getnum(conn), (int)esp_pbuf_length(p, 1));
            }
            break;
        }
        case ESP_CB_CONN_CLOSED: {              /* Connection closed */
            printf("Connection %d closed!\r\n", (int)esp_conn_getnum(conn));
            break;
        }
        case ESP_CB_CONN_ERROR: {               /* Error connecting to server */
            const char* host = cb->cb.conn_error.host;
            esp_port_t port = cb->cb.conn_error.port;
            printf("Error connecting to %s:%d\r\n", host, (int)port);
            break;
        }
    }
    return espOK;
}

/**
* \brief           Event callback function for ESP stack
* \param[in]       cb: Event information with data
* \return          espOK on success, member of \ref espr_t otherwise
*/
static espr_t
esp_callback_func(esp_cb_t* cb) {
    switch (cb->type) {
    case ESP_CB_INIT_FINISH: {
        printf("Device initialized!\r\n");
        break;
    }
    case ESP_CB_RESET: {
        printf("Device reset!\r\n");
        break;
    }
    default: break;
    }
    return espOK;
}