/**	
 * \file            esp_netconn_server.c
 * \brief           HTTP server based on netconn API
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

#define ESP_DBG_SERVER              ESP_DBG_OFF

#define HTTP_MAX_URI_LEN            256
#define HTTP_MAX_PARAMS             16
#define CRLF                        "\r\n"

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
} http_req_method_t;

/**
 * \brief           HTTP state structure
 */
typedef struct {
    esp_pbuf_p p;                               /*!< Header received pbuf starts here */
    
    http_req_method_t req_method;               /*!< Used request method */
    uint8_t headers_received;                   /*!< Did we fully received a headers? */
    uint32_t content_length;                    /*!< Total expected content length for request (on POST) (without headers) */
    uint32_t content_received;                  /*!< Content length received so far (without headers) */
    
    const fs_file_t* resp_file;                 /*!< Response file name pointer */
} http_state_t;

char http_uri[HTTP_MAX_URI_LEN + 1];
http_param_t http_params[HTTP_MAX_PARAMS];

/* CGI parameters */
static const http_cgi_t* cgi_list;
static size_t cgi_size;

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
 * \param[in]       uri: Input uri to get file for
 * \return          Pointer to file on success or NULL on failure
 */
const fs_file_t*
http_get_file_from_uri(char* uri) {
    size_t uri_len;
    const fs_file_t* file = NULL;
    
    uri_len = strlen(uri);                      /* Get URI total length */
    if ((uri_len == 1 && uri[0] == '/') ||      /* Index file only requested */
        (uri_len > 1 && uri[0] == '/' && uri[1] == '?')) {  /* Index file + parameters */
        size_t i;
        /*
         * Scan index files and check if there is one from user
         * available to return as main file
         */
        for (i = 0; i < sizeof(http_index_filenames) / sizeof(http_index_filenames[0]); i++) {
            file = fs_data_open_file(http_index_filenames[i], 0);   /* Give me a file with desired path */
            if (file) {                         /* Do we have a file? */
                break;
            }
        }
    }
    
    /*
     * We still don't have a file,
     * maybe there was a request for specific file and possible parameters
     */
    if (file == NULL) {
        char* req_params;
        size_t params_len;
        req_params = strchr(uri, '?');          /* Search for params delimiter */
        if (req_params) {                       /* We found parameters? */
            req_params[0] = 0;                  /* Reset everything at this point */
            req_params++;                       /* Skip NULL part and go to next one */
        }
        
        params_len = http_get_params(req_params);   /* Get request params from request */
        if (cgi_list) {                         /* Check if any user specific controls to process */
            size_t i;
            for (i = 0; i < cgi_size; i++) {
                if (!strcmp(cgi_list[i].uri, uri)) {
                    uri = cgi_list[i].fn(http_params, params_len);
                    break;
                }
            }
        }
        file = fs_data_open_file(uri, 0);       /* Give me a new file now */
    }
    
    /*
     * We still don't have a file!
     * Try with 404 error page if available by user
     */
    if (file == NULL) {
        file = fs_data_open_file(NULL, 1);      /* Get 404 error page */
    }
    return file;
}

/**
 * \brief           Send the received pbuf to user space
 * \param[in]       pbuf: Pbuf with received data
 * \param[in]       offset: Offset in pbuf where to start reading the buffer
 */
void
http_post_send_to_user(esp_pbuf_p pbuf, size_t offset) {
    const uint8_t* post_data;
    size_t post_data_len = 0;

    /*
     * Process all pbufs with linear addresses
     * and call user function for each part of pbuf
     */
    do {
        offset += post_data_len;                /* Get next offset */
        post_data = esp_pbuf_get_linear_addr(pbuf, offset, &post_data_len);
        
        /*
         * Call user function with data here
         */
    } while (post_data_len);

    (void)post_data;                            /* Prevent compiler warnings */
    (void)post_data_len;                        /* Prevent compiler warnings */
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
                        /*
                         * At this point, all headers are received
                         * We can start process them into something useful
                         */
                        data_pos = pos + 4;     /* Ignore 4 bytes of CRLF sequence */
                        
                        /*
                         * Check for request method used on this connection
                         */
                        if (!esp_pbuf_strcmp(hs->p, "POST", 0)) {
                            hs->req_method = HTTP_METHOD_POST;  /* Save a new value as POST method */
                            
                            /*
                             * Try to find content length on this request
                             * search for 2 possible values "Content-Length" or "content-length" parameters
                             */
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
                                pbuf_total_len = esp_pbuf_length(hs->p, 1); /* Get total length of current received pbuf */
                                
                                /*
                                 * Call user POST start method here
                                 * to notify him to prepare himself to receive POST data
                                 */
                                
                                /*
                                 * Check if there is anything to send already
                                 * to user from data part of request
                                 */
                                if (pbuf_total_len > hs->content_length) {
                                    http_post_send_to_user(hs->p, data_pos);
                                    hs->content_received = pbuf_total_len - hs->content_length; /* Set already received content length */
                                    
                                    /*
                                     * Did we receive everything in single shot?
                                     */
                                    if (hs->content_received >= hs->content_length) {
                                        /*
                                         * Start the response part here!
                                         */
                                    }
                                }
                            }
                        } else if (!esp_pbuf_strcmp(hs->p, "GET", 0)) {
                            hs->req_method = HTTP_METHOD_GET;
                            
                            /*
                             * Start the response part here
                             */
                            if (http_parse_uri(hs->p) == espOK) {   /* Try to parse URI from request */
                                hs->resp_file = http_get_file_from_uri(http_uri);   /* Get file from URI */
                                if (hs->resp_file) {
                                    esp_conn_send(conn, hs->resp_file->data, hs->resp_file->len, NULL, 0);  /* Use non-blocking method */
                                }
                            }
                            esp_pbuf_free(hs->p);   /* Free the headers memory at this point */
                        }
                    }
                } else {
                    /*
                     * We are receiving request data now
                     * as headers are already received
                     */
                    if (hs->req_method == HTTP_METHOD_POST) {
                        size_t tot_len;
                        
                        tot_len = esp_pbuf_length(p, 1);    /* Get length of pbuf */
                        hs->content_received += tot_len;
                        
                        http_post_send_to_user(p, 0);   /* Send data directly to user */
                        esp_pbuf_free(p);       /* Free the memory */
                        
                        /*
                         * Did we receive all the data on POST?
                         */
                        if (hs->content_received >= hs->content_length) {
                            /*
                             * Start the response part here!
                             */
                        }
                    } else {
                        /* On anything else (GET) we should not receive more data! Violation of protocol */
                        esp_pbuf_free(p);       /* Just free the memory and ignore */
                    }
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
                fs_data_close_file(hs->resp_file);  /* Close response file */
                esp_conn_close(conn, 0);         /* Close a connection at this point */
            } else {
                close = 1;
            }
            break;
        }
        
        /*
         * Connection was just closed, either forced by user or by remote side
         */
        case ESP_CB_CONN_CLOSED: {
            if (hs != NULL) {
                if (hs->p != NULL) {
                    esp_pbuf_free(hs->p);       /* Free packet buffer */
                }
                esp_mem_free(hs);
            }
            break;
        }
        default:
            break;
    }
    
    if (close) {                                /* Do we have to close a connection? */
        if (hs != NULL) {
            if (hs->p != NULL) {
                esp_pbuf_free(hs->p);           /* Free packet buffer */
            }
            esp_mem_free(hs);                   /* Free http state */
        }
        esp_conn_set_arg(conn, NULL);           /* Reset connection argument if any exists */
        esp_conn_close(conn, 0);                /* Close a connection */
    }
    
    return espOK;
}

/**
 * \brief           Initialize HTTP server at specific port
 * \param[in]       port: Port for HTTP server, usually 80
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_http_server_init(uint16_t port) {
    if (esp_set_server(port, 1) == espOK) {     /* Set server at user port */
        esp_set_default_server_callback(http_evt_cb);
    }
    return espOK;
}
