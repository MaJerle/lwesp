/**
 * \file            esp_rest_client.h
 * \brief           HTTP REST client implementation based on NETCONN API
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
#ifndef __ESP_HTTP_CLIENT_H
#define __ESP_HTTP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "esp/esp.h"

/**
 * \ingroup         ESP_API
 * \defgroup        ESP_REST_CLIENT HTTP REST client
 * \brief           HTTP REST client
 * \{
 *
 * \warning         Module is under development and not all features are supported and documented
 */

/**
 * \brief           REST server descriptor
 */
typedef struct {
    const char* domain;                         /*!< Domain name for connection, or IP address */
    esp_port_t port;                            /*!< Server REST port */
} esp_rest_desc_t;

/**
 * \brief           REST async callback structure
 */
typedef struct {
    /**
     * \param[in]           hc: HTTP code on response
     */
    uint8_t     (*resp_start_fn)(uint16_t hc, void* arg);
    uint8_t     (*resp_data_fn)(esp_pbuf_p p, size_t offset, void* arg);
    uint8_t     (*resp_end_fn)(void* arg);
} esp_rest_cb_t;

/**
 * \brief           REST response structure
 */
typedef struct {
    uint16_t    http_code;                      /*!< Response HTTP code */
    esp_pbuf_p  p;                              /*!< Pbuf chain of received data */
    size_t      p_offset;                       /*!< Offset in pbuf where data start, ignoring header */
    size_t      content_length;                 /*!< Content-length header value (if exists) */
} esp_rest_resp_t;

/**
 * \brief           URI parameter structure
 */
typedef struct {
    const char* name;                           /*!< Param name */
    const char* value;                          /*!< Param value */
} esp_rest_param_t;

/**
 * \brief           HTTP REST handle
 */
typedef struct {
    const esp_rest_desc_t* desc;                /*!< Descriptor handle */

    const void* tx_data;                        /*!< TX data handle */
    size_t tx_data_len;                         /*!< Length of TX data */

    const esp_rest_param_t* params;             /*!< Pointer to URI parameters */
    size_t params_len;                          /*!< Number of parameters */

    void* arg;                                  /*!< User argument for callbacks */
} esp_rest_t;

typedef struct {
    void* arg;
} esp_rest_rx_callbacks_t;

/**
 * \brief           Pointer type of \ref esp_rest_t
 */
typedef esp_rest_t* esp_rest_p;

espr_t  esp_rest_begin(esp_rest_p* rh, const esp_rest_desc_t* desc);
espr_t  esp_rest_end(esp_rest_p* rh);
espr_t  esp_rest_reset(esp_rest_p* rh);
espr_t  esp_rest_set_params(esp_rest_p* rh, const esp_rest_param_t* params, size_t len);
espr_t  esp_rest_set_arg(esp_rest_p* rh, void* arg);
espr_t  esp_rest_set_tx_data(esp_rest_p* rh, const void* d, size_t len);

espr_t  esp_rest_execute(esp_rest_p* rh, esp_http_method_t m, const char* uri, esp_rest_resp_t* r);
espr_t  esp_rest_execute_with_rx_callback(esp_rest_p* rh, esp_http_method_t m, const char* uri, esp_rest_resp_t* r, esp_rest_rx_callbacks_t* cb);

// Future use cases
espr_t  esp_rest_set_tx_data_cb(esp_rest_p* rh);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_HTTP_CLIENT_H */
