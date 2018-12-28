/**
 * \file            esp_rest_client.c
 * \brief           HTTP REST client based on NETCONN API
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
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#include "esp/esp_rest_client.h"
#include "esp/esp_netconn.h"
#include "esp/esp_mem.h"

#if ESP_CFG_REST_CLIENT || __DOXYGEN__
#if !ESP_CFG_NETCONN
#error To use REST_CLIENT API, ESP_CFG_NETCONN feature must be enabled!
#endif /* !ESP_CFG_NETCONN */

/**
 * \brief           Begin rest operation, allocate memory and prepare server descriptor
 * \param[in]       rh: Pointer to \ref esp_rest_p structure
 * \param[in]       desc: Server rest descriptor, used for request generation
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_rest_begin(esp_rest_p* rh, const esp_rest_desc_t* desc) {
    ESP_ASSERT("rh != NULL", rh != NULL);       /* Check input parameters */
    ESP_ASSERT("desc != NULL", desc != NULL);   /* Check input parameters */

    *rh = esp_mem_alloc(sizeof(**rh));          /* Allocate memory for handle */
    if (*rh != NULL) {
        ESP_MEMSET(*rh, 0x00, sizeof(**rh));    /* Reset memory */
        (*rh)->desc = desc;                     /* Set descriptor */
        return espOK;
    }
    return espERRMEM;
}

/**
 * \brief           End rest operation, free all allocated memory
 * \param[in]       rh: Pointer to \ref esp_rest_p structure
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_rest_end(esp_rest_p* rh) {
    ESP_ASSERT("rh != NULL && *rh != NULL", rh != NULL && *rh != NULL); /* Check input parameters */

    esp_mem_free(*rh);                          /* Release memory */
    *rh = NULL;                                 /* Reset user variable to NULL */
    return espOK;
}

/**
 * \brief           Reset memory after rest operation to be ready for next one with the same handle
 * \param[in]       rh: Pointer to \ref esp_rest_p structure
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_rest_reset(esp_rest_p* rh) {
    const esp_rest_desc_t* desc;
    ESP_ASSERT("rh != NULL && *rh != NULL", rh != NULL && *rh != NULL); /* Check input parameters */

    desc = (*rh)->desc;
    ESP_MEMSET(*rh, 0x00, sizeof(**rh));
    (*rh)->desc = desc;
    return espOK;
}

/**
 * \brief           Set URI params for request
 * \param[in]       rh: Pointer to \ref esp_rest_p structure
 * \param[in]       params: URI params used on request method
 * \param[in]       len: Number of params in params argument
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_rest_set_params(esp_rest_p* rh, const esp_rest_param_t* params, size_t len) {
    ESP_ASSERT("rh != NULL && *rh != NULL", rh != NULL && *rh != NULL); /* Check input parameters */
    if (len > 0) {
        ESP_ASSERT("params != NULL", params != NULL); /* Check input parameters */
    }

    (*rh)->params = params;
    (*rh)->params_len = len;
    return espOK;
}

/**
 * \brief           Set user custom argument
 * \param[in]       rh: Pointer to \ref esp_rest_p structure
 * \param[in]       arg: User argument
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_rest_set_arg(esp_rest_p* rh, void* arg) {
    ESP_ASSERT("rh != NULL && *rh != NULL", rh != NULL && *rh != NULL); /* Check input parameters */

    (*rh)->arg = arg;
    return espOK;
}

/**
 * \brief           Set user TX data to send on request
 * \param[in]       rh: Pointer to \ref esp_rest_p structure
 * \param[in]       d: Pointer to user data
 * \param[in]       len: Length of data in units of bytes
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_rest_set_tx_data(esp_rest_p* rh, const void* d, size_t len) {
    ESP_ASSERT("rh != NULL && *rh != NULL", rh != NULL && *rh != NULL); /* Check input parameters */

    (*rh)->tx_data = d;                         /* Set TX data */
    (*rh)->tx_data_len = len;                   /* Set TX data length in units of bytes */
    return espOK;
}

/**
 * \brief           Execute REST call and pass everything in single shot.
 *
 *                  Function will block until all data received.
 *                  User must ensure there is enough memory to handle entire response.
 *                  Take a look at \ref esp_rest_execute_with_cb to use callback for every received
 *                  packet and to ensure there is always available memory for future packets
 * \param[in]       rh: Pointer to \ref esp_rest_p structure
 * \param[in]       m: HTTP method used in request header
 * \param[in]       uri: URI to open, example: "/test/data", excluding URI params
 * \param[out]      r: Pointer to response information structure for later usage
 * \return          \ref espr_t on success, member of \ref espr_t otherwise
 */
espr_t
esp_rest_execute(esp_rest_p* rh, esp_http_method_t m, const char* uri, esp_rest_resp_t* r) {
    esp_netconn_p nc;
    espr_t res;
    esp_pbuf_p pbuf;
    esp_rest_p rhh;

    ESP_ASSERT("rh != NULL && *rh != NULL", rh != NULL && *rh != NULL); /* Check input parameters */
    ESP_ASSERT("uri != NULL", uri != NULL);     /* Check input parameters */
    ESP_ASSERT("r != NULL", r != NULL);         /* Check input parameters */

    rhh = *rh;                                  /* Get REST handle */
    r->p = NULL;                                /* Reset pbuf pointer */

    /*Start netconn and connect to server */
    nc = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    if (nc != NULL) {
        res = esp_netconn_connect(nc, rhh->desc->domain, rhh->desc->port);
        if (res == espOK) {
            uint8_t check_http_code = 1, check_headers_end = 1;

            /* Request method + uri + HTTP version */
            switch (m) {
                case ESP_HTTP_METHOD_POST:      esp_netconn_write(nc, "POST", 4);       break;
                case ESP_HTTP_METHOD_PUT:       esp_netconn_write(nc, "PUT", 3);        break;
                case ESP_HTTP_METHOD_CONNECT:   esp_netconn_write(nc, "CONNECT", 7);    break;
                case ESP_HTTP_METHOD_DELETE:    esp_netconn_write(nc, "DELETE", 6);     break;
                case ESP_HTTP_METHOD_HEAD:      esp_netconn_write(nc, "HEAD", 4);       break;
                case ESP_HTTP_METHOD_OPTIONS:   esp_netconn_write(nc, "OPTIONS", 7);    break;
                case ESP_HTTP_METHOD_PATCH:     esp_netconn_write(nc, "PATCH", 5);      break;
                case ESP_HTTP_METHOD_TRACE:     esp_netconn_write(nc, "TRACE", 5);      break;
                case ESP_HTTP_METHOD_GET:
                default:                        esp_netconn_write(nc, "GET", 3);        break;
            }
            esp_netconn_write(nc, " ", 1);
            esp_netconn_write(nc, uri, strlen(uri));
            if (rhh->params_len && rhh->params != NULL) {
                esp_netconn_write(nc, "?", 1);
                for (size_t p_i = 0; p_i < rhh->params_len; p_i++) {
                    if (rhh->params[p_i].name != NULL && rhh->params[p_i].value != NULL) {
                        esp_netconn_write(nc, rhh->params[p_i].name, strlen(rhh->params[p_i].name));
                        esp_netconn_write(nc, "=", 1);
                        esp_netconn_write(nc, rhh->params[p_i].value, strlen(rhh->params[p_i].value));

                        if (p_i < (rhh->params_len - 1)) {
                            esp_netconn_write(nc, "&", 1);
                        }
                    }
                }
            }
            esp_netconn_write(nc, " HTTP/1.1\r\n", 11);

            /* Host */
            esp_netconn_write(nc, "Host: ", 6);
            esp_netconn_write(nc, rhh->desc->domain, strlen(rhh->desc->domain));
            esp_netconn_write(nc, "\r\n", 2);

            esp_netconn_write(nc, "Connection: close\r\n", 19); /* Connection close */

            if (rhh->tx_data_len && rhh->tx_data != NULL) { /* Content length */
                char tx_len_str[11];
                sprintf(tx_len_str, "%d", (int)rhh->tx_data_len);
                esp_netconn_write(nc, "Content-Length: ", 16);
                esp_netconn_write(nc, tx_len_str, strlen(tx_len_str));
                esp_netconn_write(nc, "\r\n", 2);
            }

            esp_netconn_write(nc, "\r\n", 2);   /* End of headers */

            /* Send user data if exists */
            if (rhh->tx_data_len && rhh->tx_data != NULL) {
                esp_netconn_write(nc, rhh->tx_data, rhh->tx_data_len);
            }

            /* Flush and force send everything */
            esp_netconn_flush(nc);              /* Flush and force send */

            /* Handle received data */
            while (1) {
                res = esp_netconn_receive(nc, &pbuf);   /* Receive new packet of data */

                if (res == espOK) {             /* We have new data */
                    if (r->p == NULL) {         /* Check if we already have first buffer */
                        r->p = pbuf;            /* Set as first buffer */
                    } else {
                        esp_pbuf_cat(r->p, pbuf);   /* Concat buffers together */
                    }
                } else {
                    if (res == espCLOSED) {     /* Connection closed at this point */
                        res = espOK;
                    }
                    break;
                }

                /*
                 * Check if we can detect HTTP response code
                 *
                 * Response is: "HTTP/1.1 code", minimum `12` characters
                 */
                if (check_http_code && r->p != NULL &&
                    esp_pbuf_length(r->p, 1) >= 12 && !esp_pbuf_memcmp(r->p, "HTTP/", 5, 0)) {
                    size_t pos = 9;
                    uint8_t el;

                    r->http_code = 0;
                    while (esp_pbuf_get_at(r->p, pos++, &el) &&
                        (el >= '0' && el <= '9')) {
                        r->http_code = 10 * (r->http_code) + (el - '0');
                    }
                    check_http_code = 0;        /* No need to check for HTTP code anymore */
                }

                /* Calculate offset in pbuf where actual data start */
                if (check_headers_end && r->p != NULL) {
                    r->p_offset = esp_pbuf_memfind(r->p, "\r\n\r\n", 4, 0);
                    if (r->p_offset != ESP_SIZET_MAX) { /* Did we receive all headers now? */
                        size_t p;
                        r->p_offset += 4;

                        /* Parse Content-Length header if exists */
                        r->content_length = 0;
                        if ((p = esp_pbuf_memfind(r->p, "Content-Length:", 15, 0)) != ESP_SIZET_MAX ||
                            (p = esp_pbuf_memfind(r->p, "content-length:", 15, 0)) != ESP_SIZET_MAX) {
                            uint8_t el;
                            p += 15;            /* Skip content part */
                            if (esp_pbuf_get_at(r->p, p, &el) && el == ' ') {   /* Skip space entry */
                                p++;
                            }
                            while (esp_pbuf_get_at(r->p, p++, &el) &&
                                (el >= '0' && el <= '9')) {
                                r->content_length = 10 * (r->content_length) + (el - '0');
                            }
                        }

                        check_headers_end = 0;  /* No need to check for headers anymore */
                    } else {
                        r->p_offset = 0;
                    }
                }
            }
        }
        esp_netconn_delete(nc);                 /* Delete netconn connection */
    } else {
        res = espERRMEM;
    }
    return res;
}

#endif /* ESP_CFG_REST_CLIENT || __DOXYGEN__ */
