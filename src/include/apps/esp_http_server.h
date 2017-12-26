/** 
 * \file            esp_http_server.h
 * \brief           HTTP server with callback API
 */
 
/*
 * Copyright (c) 2017 Tilen Majerle
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#ifndef __ESP_HTTP_SERVER_H
#define __ESP_HTTP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \addtogroup      ESP_APPS
 * \{
 */

/**
 * \defgroup        ESP_APP_HTTP_SERVER HTTP server
 * \brief           HTTP server based on callback API
 * \{
 */

#include "apps/esp_apps.h"
#include "apps/esp_http_server_utils.h"

/**
 * \brief           Write string to HTTP server output
 * \note            May only be called from SSI callback function
 * \param[in]       hs: HTTP handle
 * \param[in]       str: String to write
 * \return          Number of bytes written to output
 * \sa              esp_http_server_write
 */
#define     esp_http_server_write_string(hs, str)   esp_http_server_write(hs, str, strlen(str))

espr_t      esp_http_server_init(const http_init_t* init, uint16_t port);
size_t      esp_http_server_write(http_state_t* hs, const void* data, size_t len);

/**
 * \}
 */

/**
 * \}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __ESP_HTTP_SERVER_H */
