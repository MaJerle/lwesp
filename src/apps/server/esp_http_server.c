/**	
 * \file            esp_http_server.c
 * \brief           HTTP server based on callback API
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
#include "include/esp_http_server.h"
#include "esp/include/esp_mem.h"
#include "fs_data.h"
#include "ctype.h"

#ifndef ESP_DBG_SERVER
#define ESP_DBG_SERVER              ESP_DBG_OFF
#endif /* ESP_DBG_SERVER */

#ifndef HTTP_SUPPORT_POST
#define HTTP_SUPPORT_POST           1
#endif /* HTTP_SUPPORT_POST */

#define HTTP_MAX_URI_LEN            256
#define HTTP_MAX_PARAMS             16
#define CRLF                        "\r\n"

char http_uri[HTTP_MAX_URI_LEN + 1];
http_param_t http_params[HTTP_MAX_PARAMS];

/* HTTP init structure with user settings */
static const http_init_t* hi;

/**
 * \brief           List of supported file names for index page
 */
static const char*
http_index_filenames[] = {
    "/index.html",
    "/index.htm"
};

/**
 * \brief           Parse URI from HTTP request and copy it to linear memory location
 * \param[in]       p: Chain of pbufs from request
 * \return          espOK if successfully parsed, member of \ref espr_t otherwise
 */
static espr_t
http_parse_uri(esp_pbuf_p p) {
    size_t pos_s, pos_e, pos_crlf, uri_len;
                                                
    pos_s = esp_pbuf_strfind(p, " ", 0);        /* Find first " " in request header */
    if (pos_s == ESP_SIZET_MAX || (pos_s != 3 && pos_s != 4)) {
        return espERR;
    }
    pos_crlf = esp_pbuf_strfind(p, "\r\n", 0);  /* Find CRLF position */
    if (pos_crlf == ESP_SIZET_MAX) {
        return espERR;
    }
    pos_e = esp_pbuf_strfind(p, " ", pos_s + 1);/* Find second " " in request header */
    if (pos_e == ESP_SIZET_MAX) {               /* If there is no second " " */
        /*
         * HTTP 0.9 request is "GET /\r\n" without
         * space between request URI and CRLF
         */
        pos_e = pos_crlf;                       /* Use the one from CRLF */
    }
    
    uri_len = pos_e - pos_s - 1;                /* Get length of uri */
    if (uri_len > HTTP_MAX_URI_LEN) {
        return espERR;
    }
    esp_pbuf_copy(p, http_uri, uri_len, pos_s + 1); /* Copy data from pbuf to linear memory */
    http_uri[uri_len] = 0;                      /* Set terminating 0 */
    
    return espOK;
}

/**
 * \brief           Extract parameters from user request URI
 * \param[in]       params: RAM variable with parameters
 * \return          Number of parameters extracted
 */
static size_t
http_get_params(char* params) {
    size_t cnt = 0, i;
    char *amp, *eq;
    
    if (params) {
        for (i = 0; params && i < HTTP_MAX_PARAMS; i++, cnt++) {
            http_params[i].name = params;
            
            eq = params;
            amp = strchr(params, '&');          /* Find next & in a sequence */
            if (amp) {                          /* In case we have it */
                *amp = 0;                       /* Replace it with 0 to end current param */
                params = ++amp;                 /* Go to next one */
            } else {
                params = NULL;
            }
            
            eq = strchr(eq, '=');               /* Find delimiter */
            if (eq) {
                *eq = 0;
                http_params[i].value = eq + 1;
            } else {
                http_params[i].value = NULL;
            }
        }
    }
    return cnt;
}

/**
 * \brief           Get file from uri in format /folder/file?param1=value1&...
 * \param[in]       hs: HTTP state
 * \param[in]       uri: Input uri to get file for
 * \return          1 on success, 0 otherwise
 */
uint8_t
http_get_file_from_uri(http_state_t* hs, char* uri) {
    size_t uri_len;
    
    
    memset(&hs->resp_file, 0x00, sizeof(hs->resp_file));
    uri_len = strlen(uri);                      /* Get URI total length */
    if ((uri_len == 1 && uri[0] == '/') ||      /* Index file only requested */
        (uri_len > 1 && uri[0] == '/' && uri[1] == '?')) {  /* Index file + parameters */
        size_t i;
        /*
         * Scan index files and check if there is one from user
         * available to return as main file
         */
        for (i = 0; i < sizeof(http_index_filenames) / sizeof(http_index_filenames[0]); i++) {
            hs->resp_file_opened = fs_data_open_file(&hs->resp_file, http_index_filenames[i], 0); /* Give me a file with desired path */
            if (hs->resp_file_opened) {         /* Do we have a file? */
                break;
            }
        }
    }
    
    /*
     * We still don't have a file,
     * maybe there was a request for specific file and possible parameters
     */
    if (!hs->resp_file_opened) {
        char* req_params;
        size_t params_len;
        req_params = strchr(uri, '?');          /* Search for params delimiter */
        if (req_params) {                       /* We found parameters? */
            req_params[0] = 0;                  /* Reset everything at this point */
            req_params++;                       /* Skip NULL part and go to next one */
        }
        
        params_len = http_get_params(req_params);   /* Get request params from request */
        if (hi != NULL && hi->cgi != NULL) {    /* Check if any user specific controls to process */
            size_t i;
            for (i = 0; i < hi->cgi_count; i++) {
                if (!strcmp(hi->cgi[i].uri, uri)) {
                    uri = hi->cgi[i].fn(http_params, params_len);
                    break;
                }
            }
        }
        hs->resp_file_opened = fs_data_open_file(&hs->resp_file, uri, 0);   /* Give me a new file now */
    }
    
    /*
     * We still don't have a file!
     * Try with 404 error page if available by user
     */
    if (!hs->resp_file_opened) {
        hs->resp_file_opened = fs_data_open_file(&hs->resp_file, NULL, 1);  /* Get 404 error page */
    }
    return hs->resp_file_opened;
}

/**
 * \brief           Send the received pbuf to user space
 * \param[in]       hs: HTTP state context
 * \param[in]       pbuf: Pbuf with received data
 * \param[in]       offset: Offset in pbuf where to start reading the buffer
 */
static void
http_post_send_to_user(http_state_t* hs, esp_pbuf_p pbuf, size_t offset) {
    esp_pbuf_p new_pbuf;

    if (hi == NULL || hi->post_data_fn == NULL) {
        return;
    }
    
    new_pbuf = esp_pbuf_skip(pbuf, offset, &offset);    /* Skip pbufs and create this one */
    if (new_pbuf != NULL) {
        esp_pbuf_advance(new_pbuf, offset);     /* Advance pbuf for remaining bytes */
    
        hi->post_data_fn(hs, new_pbuf);         /* Notify user with data */
    }
}

/**
 * \brief           Send response back to connection
 * \param[in]       hs: HTTP state
 * \param[in]       ft: Flag indicating function was called first time to send the response
 */
static void
send_response(http_state_t* hs, uint8_t ft) {
    uint8_t close = 0;
    /*
     * Do we have a file ready to be send?
     */
    if (hs->resp_file_opened) {
        if (!ft) {
            hs->resp_file.sent_total += hs->resp_file.sent;
        }
        
        /*
         * Do we still have some data to send?
         */
        if (hs->resp_file.sent_total < hs->resp_file.len) {
            /*
             * Static files (const array) have data set already
             * so we can send entire data immediatelly
             */
            if (hs->resp_file.is_static) {
                size_t available;
                //esp_conn_send(hs->conn, hs->resp_file.data, hs->resp_file.len &hs->resp_file.sent, 0);
                esp_conn_write(hs->conn, &hs->resp_file.data[0], hs->resp_file.len, 1, NULL);
                hs->resp_file.sent_total += hs->resp_file.len;
            } else {
                /* 
                 * Todo: Implement read to temporary buffer and send
                 */
            }
        } else {
            fs_data_close_file(&hs->resp_file); /* Close file */
            close = 1;
        }
    } else {
        close = 1;
    }
    
    if (close) {
        esp_conn_close(hs->conn, 0);            /* Close the connection as no file opened in this case */
    }
}

static espr_t
http_evt_cb(esp_cb_t* cb) {
    uint8_t close = 0;
    esp_conn_p conn;
    http_state_t* hs = NULL;
    
    conn = esp_conn_get_from_evt(cb);           /* Get connection from event */
    if (conn != NULL) {
        hs = esp_conn_get_arg(conn);            /* Get connection argument */
    }
    switch (cb->type) {
        /*
         * A new connection just became active
         */
        case ESP_CB_CONN_ACTIVE: {
            hs = esp_mem_calloc(1, sizeof(*hs));
            if (hs != NULL) {
                hs->conn = conn;                /* Save connection handle */
                esp_conn_set_arg(conn, hs);     /* Set argument for connection */
            } else {
                ESP_DEBUGF(ESP_DBG_SERVER, "Cannot allocate memory for http state\r\n");
                close = 1;                      /* No memory, close the connection */
            }
            break;
        }
        
        /*
         * Data received on connection
         */
        case ESP_CB_CONN_DATA_RECV: {
            esp_pbuf_p p = cb->cb.conn_data_recv.buff;
            size_t data_pos, pos, pbuf_total_len;
            uint8_t ch;
            
            if (hs != NULL) {                   /* Do we have a valid http state? */
                /*
                 * Check if we have to receive headers data first
                 * before we can proceed with everything else
                 */
                if (!hs->headers_received) {    /* Are we still waiting for headers data? */
                    if (hs->p == NULL) {
                        hs->p = p;              /* This is a first received packet */
                    } else {
                        esp_pbuf_cat(hs->p, p); /* Add new packet to the end of linked list of recieved data */
                    }
                    esp_pbuf_ref(p);            /* Reference a new received pbuf */
                
                    /*
                     * Check if headers are fully received.
                     * To know this, search for "\r\n\r\n" sequence in received data
                     */
                    if ((pos = esp_pbuf_strfind(hs->p, CRLF CRLF, 0)) != ESP_SIZET_MAX) {
                        ESP_DEBUGF(ESP_DBG_SERVER, "HTTP headers received!\r\n");
                        hs->headers_received = 1;   /* Flag received headers */
                        
                        /*
                         * Parse the URI, process request and open response file
                         */
                        if (http_parse_uri(hs->p) == espOK) {
                            http_get_file_from_uri(hs, http_uri);   /* Open file */
                        }
                        
                        /*
                         * At this point, all headers are received
                         * We can start process them into something useful
                         */
                        data_pos = pos + 4;     /* Ignore 4 bytes of CRLF sequence */
                        
                        /*
                         * Check for request method used on this connection
                         */
                        if (!esp_pbuf_strcmp(hs->p, "POST ", 0)) {
                            hs->req_method = HTTP_METHOD_POST;  /* Save a new value as POST method */
                            
                            /*
                             * Try to find content length on this request
                             * search for 2 possible values "Content-Length" or "content-length" parameters
                             */
                            hs->content_length = 0;
                            if (((pos = esp_pbuf_strfind(hs->p, "Content-Length:", 0)) != ESP_SIZET_MAX) ||
                                (pos = esp_pbuf_strfind(hs->p, "content-length:", 0)) != ESP_SIZET_MAX) {
                                
                                pos += 15;      /* Skip this part */
                                if (esp_pbuf_get_at(hs->p, pos, &ch) && ch == ' ') {
                                    pos++;
                                }
                                esp_pbuf_get_at(hs->p, pos, &ch);
                                while (ch >= '0' && ch <= '9') {
                                    hs->content_length = 10 * hs->content_length + (ch - '0');
                                    pos++;
                                    if (!esp_pbuf_get_at(hs->p, pos, &ch)) {
                                        break;
                                    }
                                }
                            }
                            
                            /*
                             * Check if we are expecting any data on POST request
                             */
                            if (hs->content_length) {
                                /*
                                 * Call user POST start method here
                                 * to notify him to prepare himself to receive POST data
                                 */
                                if (hi != NULL && hi->post_start_fn != NULL) {
                                    hi->post_start_fn(hs, http_uri, hs->content_length);
                                }
                                
                                /*
                                 * Check if there is anything to send already
                                 * to user from data part of request
                                 */
                                pbuf_total_len = esp_pbuf_length(hs->p, 1); /* Get total length of current received pbuf */
                                if ((pbuf_total_len - data_pos) > 0) {
                                    hs->content_received = pbuf_total_len - data_pos;
                                    
                                    /*
                                     * Send data to user
                                     */
                                    http_post_send_to_user(hs, hs->p, data_pos);
                                    
                                    /*
                                     * Did we receive everything in single packet?
                                     * Close POST loop at this point and notify user
                                     */
                                    if (hs->content_received >= hs->content_length) {
                                        hs->process_resp = 1;   /* Process with response to user */
                                        if (hi != NULL && hi->post_end_fn != NULL) {
                                            hi->post_end_fn(hs);
                                        }
                                    }
                                }
                            }
                        } else if (!esp_pbuf_strcmp(hs->p, "GET ", 0)) {
                            hs->req_method = HTTP_METHOD_GET;
                            hs->process_resp = 1;   /* Process with response to user */
                        }
                    }
                } else {
                    /*
                     * We are receiving request data now
                     * as headers are already received
                     */
                    if (hs->req_method == HTTP_METHOD_POST) {
                        /*
                         * Did we receive all the data on POST?
                         */
                        if (hs->content_received < hs->content_length) {
                            size_t tot_len;
                            
                            tot_len = esp_pbuf_length(p, 1);    /* Get length of pbuf */
                            hs->content_received += tot_len;
                            
                            http_post_send_to_user(hs, p, 0);   /* Send data directly to user */
                            
                            /**
                             * Check if everything received
                             */
                            if (hs->content_received >= hs->content_length) {
                                hs->process_resp = 1;   /* Process with response to user */
                                
                                /*
                                 * Stop the response part here!
                                 */
                                if (hi != NULL && hi->post_end_fn) {
                                    hi->post_end_fn(hs);
                                }
                            }
                        }
                    } else {
                        /* On anything else (GET) we should not receive more data! Violation of protocol */
                    }
                }
                
                /* Do the processing on response */
                if (hs->process_resp) {
                    send_response(hs, 1);       /* Send the response data */
                }
            } else {
                close = 1;
            }
            break;
        }
        
        /*
         * Data were successfully sent on a connection
         */
        case ESP_CB_CONN_DATA_SENT: {
            if (hs != NULL) {
                send_response(hs, 0);           /* Send more data if required */
            }
            break;
        }
        
        /*
         * Connection was just closed, either forced by user or by remote side
         */
        case ESP_CB_CONN_CLOSED: {
            if (hs != NULL) {
#if HTTP_SUPPORT_POST
                if (hs->req_method == HTTP_METHOD_POST) {
                    if (hs->content_received < hs->content_length) {
                        if (hi != NULL && hi->post_end_fn) {
                            hi->post_end_fn(hs);
                        }
                    }
                }
#endif /* HTTP_SUPPORT_POST */
                if (hs->p != NULL) {
                    esp_pbuf_free(hs->p);       /* Free packet buffer */
                    hs->p = NULL;
                }
                esp_mem_free(hs);
                hs = NULL;
            }
            break;
        }
        
        /*
         * Poll the connection
         */
        case ESP_CB_CONN_POLL: {
            break;
        }
        default:
            break;
    }
    
    if (close) {                                /* Do we have to close a connection? */
        esp_conn_close(conn, 0);                /* Close a connection */
    }
    
    return espOK;
}

/**
 * \brief           Initialize HTTP server at specific port
 * \param[in]       init: Initialization structure for server
 * \param[in]       port: Port for HTTP server, usually 80
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_http_server_init(const http_init_t* init, uint16_t port) {
    espr_t res;
    if ((res = esp_set_server(port, ESP_MAX_CONNS / 2, 80, http_evt_cb, 1)) == espOK) {
        hi = init;
    }
    return res;
}
