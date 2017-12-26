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

#include "esp/esp.h"
#include "fs_data.h"

/**
 * \addtogroup      ESP_APP_HTTP_SERVER
 * \{
 */

/**
 * \defgroup        ESP_CONFIG_APP_HTTP HTTP server configuration
 * \brief           Configuration of HTTP server app
 * \{
 */

/**
 * \brief           Server debug default setting
 */
#ifndef ESP_DBG_SERVER
#define ESP_DBG_SERVER                  ESP_DBG_OFF
#endif

#ifndef HTTP_SSI_TAG_START
#define HTTP_SSI_TAG_START              "<!--#"     /*!< SSI tag start string */
#define HTTP_SSI_TAG_START_LEN          5           /*!< SSI tag start length */
#endif

#ifndef HTTP_SSI_TAG_END
#define HTTP_SSI_TAG_END                "-->"       /*!< SSI tag end string */
#define HTTP_SSI_TAG_END_LEN            3           /*!< SSI tag end length */
#endif

/**
 * \brief           Maximal length of tag name excluding start and end parts of tag
 */
#ifndef HTTP_SSI_TAG_MAX_LEN
#define HTTP_SSI_TAG_MAX_LEN            10
#endif

/**
 * \brief           Enables (1) or disables (0) support for POST request
 */
#ifndef HTTP_SUPPORT_POST
#define HTTP_SUPPORT_POST               1
#endif

/**
 * \brief           Maximal length of allowed uri length including parameters in format /uri/sub/path?param=value
 */
#ifndef HTTP_MAX_URI_LEN
#define HTTP_MAX_URI_LEN                256
#endif

/**
 * \brief           Maximal number of parameters in URI
 */
#ifndef HTTP_MAX_PARAMS
#define HTTP_MAX_PARAMS                 16
#endif

/**
 * \}
 */

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

/**
 * \brief           Post request started prototype with non-zero content length
 * \param[in]       hs: HTTP state
 * \param[in]       uri: POST request URI
 * \param[in]       content_length: Total content length (Content-Length HTTP parameter) in units of bytes
 * \return          espOK on success, member of \ref espr_t otherwise
 */
typedef espr_t  (*http_post_start_fn)(struct http_state* hs, const char* uri, uint32_t content_length);

/**
 * \brief           Post data received on POST request prototype
 * \note            This function may be called multiple time until content_length from \ref http_post_start_fn callback is not reached
 * \param[in]       hs: HTTP state
 * \param[in]       pbuf: Packet buffer wit reciveed data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
typedef espr_t  (*http_post_data_fn)(struct http_state* hs, esp_pbuf_p pbuf);

/**
 * \brief           End of POST data request prototype
 * \param[in]       hs: HTTP state
 * \return          espOK on success, member of \ref espr_t otherwise
 */
typedef espr_t  (*http_post_end_fn)(struct http_state* hs);

/**
 * \brief           SSI (Server Side Includes) callback function prototype
 * \note            User can use server write functions to directly write to connection output
 * \param[in]       hs: HTTP state
 * \param[in]       tag_name: Name of TAG to replace with user content
 * \param[in]       tag_len: Length of TAG
 * \return          Number of bytes written to output
 */
typedef size_t  (*http_ssi_fn)(struct http_state* hs, const char* tag_name, size_t tag_len);

/**
 * \brief           HTTP server initialization structure
 */
typedef struct {
    http_post_start_fn post_start_fn;           /*!< Callback function for post start */
    http_post_data_fn post_data_fn;             /*!< Callback functon for post data */
    http_post_end_fn post_end_fn;               /*!< Callback functon for post end */
    
    const http_cgi_t* cgi;                      /*!< Pointer to array of CGI entries. Set to NULL if not used */
    size_t cgi_count;                           /*!< Length of CGI array. Set to 0 if not used */
    
    http_ssi_fn ssi_fn;                         /*!< SSI callback function */
} http_init_t;

/**
 * \brief           Request method type
 */
typedef enum {
    HTTP_METHOD_GET,                            /*!< HTTP request method GET */
    HTTP_METHOD_POST,                           /*!< HTTP request method POST */
} http_req_method_t;

/**
 * \brief           List of SSI TAG parsing states
 */
typedef enum {
    HTTP_SSI_STATE_WAIT_BEGIN = 0x00,           /*!< Waiting beginning of tag */
    HTTP_SSI_STATE_BEGIN = 0x01,                /*!< Beginning detected, parsing it */
    HTTP_SSI_STATE_TAG = 0x02,                  /*!< Parsing TAG value */
    HTTP_SSI_STATE_END = 0x03,                  /*!< Parsing end of TAG */
} http_ssi_state_t;

/**
 * \brief           HTTP state structure
 */
typedef struct http_state {
    esp_conn_p conn;                            /*!< Connection handle */
    esp_pbuf_p p;                               /*!< Header received pbuf starts here */
    
    uint32_t conn_mem_available;                /*!< Available memory in connection send queue */
    uint32_t written_total;                     /*!< Total number of bytes written into send buffer */
    uint32_t sent_total;                        /*!< Number of bytes we already sent */
    
    http_req_method_t req_method;               /*!< Used request method */
    uint8_t headers_received;                   /*!< Did we fully received a headers? */
    uint32_t content_length;                    /*!< Total expected content length for request (on POST) (without headers) */
    uint32_t content_received;                  /*!< Content length received so far (without headers) */
    uint8_t process_resp;                       /*!< Process with response flag */
    
    fs_file_t resp_file;                        /*!< Response file structure */
    uint8_t resp_file_opened;                   /*!< Status if response file is opened and ready */
    const uint8_t* buff;                        /*!< Buffer pointer with data */
    uint32_t buff_len;                          /*!< Total length of buffer */
    uint32_t buff_ptr;                          /*!< Current buffer pointer */
    
    void * arg;                                 /*!< User optional argument */
    
    /* SSI tag parsing */
    uint8_t is_ssi;                             /*!< Flag if current request is SSI enabled */
    http_ssi_state_t ssi_state;                 /*!< Current SSI state when parsing SSI tags */
    char ssi_tag_buff[HTTP_SSI_TAG_START_LEN + HTTP_SSI_TAG_END_LEN + HTTP_SSI_TAG_MAX_LEN + 1];    /*!< Temporary buffer for SSI tag storing */
    size_t ssi_tag_buff_ptr;                    /*!< Current write pointer */
    size_t ssi_tag_buff_written;                /*!< Number of bytes written so far to output buffer in case tag is not valid */
    size_t ssi_tag_len;                         /*!< Length of SSI tag */
} http_state_t;

/**
 * \}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __ESP_HTTP_SERVER_UTILS_H */
