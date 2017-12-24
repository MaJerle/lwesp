/** 
 * \file            esp_http_server_utils.h
 * \brief           HTTP server utility functions
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
#ifndef __ESP_HTTP_SERVER_UTILS_H
#define __ESP_HTTP_SERVER_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "esp.h"
#include "fs_data.h"

struct http_state;

/**
 * \brief           HTTP parameters on http URI
 */
typedef struct {
    const char* name;                           /*!< Name of parameter */
    const char* value;                          /*!< Parameter value */
} http_param_t;

/**
 * \brief           CGI callback function
 */
typedef char *  (*http_cgi_fn)(http_param_t *, size_t);

/**
 * \brief           CGI structure
 */
typedef struct {
    const char* uri;                            /*!< URI path for CGI handler */
    http_cgi_fn fn;                             /*!< Callback function to call when we have a CGI match */
} http_cgi_t;

typedef espr_t  (*http_post_start_fn)(struct http_state *, const char *, uint32_t);
typedef espr_t  (*http_post_data_fn)(struct http_state *, esp_pbuf_p);
typedef espr_t  (*http_post_end_fn)(struct http_state *);

/**
 * \brief           HTTP server initialization structure
 */
typedef struct {
    http_post_start_fn post_start_fn;           /*!< Callback function for post start */
    http_post_data_fn post_data_fn;             /*!< Callback functon for post data */
    http_post_end_fn post_end_fn;               /*!< Callback functon for post end */
    
    const http_cgi_t* cgi;                      /*!< Pointer to array of CGI entries. Set to NULL if not used */
    size_t cgi_count;                           /*!< Length of CGI array. Set to 0 if not used */
} http_init_t;

/**
 * \brief           Request method type
 */
typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
} http_req_method_t;

/**
 * \brief           HTTP state structure
 */
typedef struct http_state {
    esp_conn_p conn;                            /*!< Connection handle */
    esp_pbuf_p p;                               /*!< Header received pbuf starts here */
    
    http_req_method_t req_method;               /*!< Used request method */
    uint8_t headers_received;                   /*!< Did we fully received a headers? */
    uint32_t content_length;                    /*!< Total expected content length for request (on POST) (without headers) */
    uint32_t content_received;                  /*!< Content length received so far (without headers) */
    uint8_t process_resp;                       /*!< Process with response flag */
    
    fs_file_t resp_file;                        /*!< Response file structure */
    uint8_t resp_file_opened;                   /*!< Status if response file is opened and ready */
    
    void * arg;                                 /*!< User optional argument */
} http_state_t;

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __ESP_HTTP_SERVER_UTILS_H */
