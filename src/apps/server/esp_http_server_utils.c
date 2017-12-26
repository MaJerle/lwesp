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
#include "esp/esp_http_server_utils.h"
#include "fs_data.h"
#include "ctype.h"

#define ESP_DBG_SERVER              ESP_DBG_OFF

#define HTTP_MAX_URI_LEN            256
#define HTTP_MAX_PARAMS             16

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
 * \return          Pointer to parsed uri or NULL on failure
 */
const char *
http_parse_uri(esp_pbuf_p p) {
    size_t pos_s, pos_e, pos_crlf, uri_len;
                                                
    pos_s = esp_pbuf_strfind(p, " ", 0);        /* Find first " " in request header */
    if (pos_s == ESP_SIZET_MAX || (pos_s != 3 && pos_s != 4)) {
        return NULL;
    }
    pos_crlf = esp_pbuf_strfind(p, "\r\n", 0);  /* Find CRLF position */
    if (pos_crlf == ESP_SIZET_MAX) {
        return NULL;
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
        return NULL;
    }
    esp_pbuf_copy(p, http_uri, uri_len, pos_s + 1); /* Copy data from pbuf to linear memory */
    http_uri[uri_len] = 0;                      /* Set terminating 0 */
    
    return NULL;
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
uint8_t
http_get_file_from_uri(hs, char* uri) {
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
 * \brief           Server a client connection
 * \param[in]       client: New connected client
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
server_serve(esp_netconn_p client) {
    esp_pbuf_p pbuf = NULL, pbuf_tmp = NULL;
    espr_t res;
    size_t pos, data_pos, cont_len, pbuf_tot_len;
    uint8_t ch;
    
    do {
        /*
         * Receive HTTP data from client,
         * packet by packet until we have \r\n\r\n,
         * indicating end of headers.
         */
        res = esp_netconn_receive(client, &pbuf_tmp);
        
        if (res == espOK) {
            if (!pbuf) {                        /* Is this first packet? */
                pbuf = pbuf_tmp;                /* Set it as first */
            } else {                            /* If not first packet */
                esp_pbuf_cat(pbuf, pbuf_tmp);   /* Connect buffers together */
                pbuf_tmp = NULL;                /* We do not reference to previous buffer anymore */
            }
            
            /*
             * Try to find first \r\n\r\n sequence
             * which indicates end of headers in HTTP request
             */
            if ((pos = esp_pbuf_strfind(pbuf, "\r\n\r\n", 0)) != ESP_SIZET_MAX) {
                /*
                 * At this point, all headers are received
                 * We can start process them into something useful
                 */
                data_pos = pos + 4;             /* Ignore 4 bytes of CR LF sequence */
                
                /*
                 * Check method type we are dealing with
                 * either GET or POST are supported currently
                 */
                if (!esp_pbuf_strcmp(pbuf, "GET", 0)) {
                    ESP_DEBUGF(ESP_DBG_SERVER, "We have GET method and we are not expecting more data to be received!\r\n");
                    break;
                } else if (!esp_pbuf_strcmp(pbuf, "POST", 0)) {
                    ESP_DEBUGF(ESP_DBG_SERVER, "We have POST method!\r\n");
                    
                    /*
                     * Try to read content length parameter
                     * and parse length to know how much data we should expect on POST
                     * command to be sure everything is received before processed
                     */
                    if (((pos = esp_pbuf_strfind(pbuf, "Content-Length:", 0)) != ESP_SIZET_MAX) ||
                        (pos = esp_pbuf_strfind(pbuf, "content-length:", 0)) != ESP_SIZET_MAX) {
                        /*
                         * We have found content-length header
                         * Now we need to calculate actual length of data
                         */
                        pos += 15;
                        if (esp_pbuf_get_at(pbuf, pos, &ch) && ch == ' ') {
                            pos++;
                        }
                        esp_pbuf_get_at(pbuf, pos, &ch);
                        cont_len = 0;
                        while (isdigit(ch)) {
                            cont_len = 10 * cont_len + (ch - '0');
                            pos++;
                            if (!esp_pbuf_get_at(pbuf, pos, &ch)) {
                                break;
                            }
                        }
                        ESP_DEBUGF(ESP_DBG_SERVER, "POST: Found content length: %d bytes\r\n", (int)cont_len);
                        pbuf_tot_len = esp_pbuf_length(pbuf, 1);    /* Get total length of pbuf */
                        
                        /*
                         * Check if we have some data already in current pbufs
                         * and call user function in case we have some data
                         */
                        if (pbuf_tot_len > data_pos) {
                            size_t post_data_len;
                            const uint8_t* post_data;
                            
                            /*
                             * Get linear memory address and length from post data
                             * to send to user space application
                             */
                            post_data = esp_pbuf_get_linear_addr(pbuf, data_pos, &post_data_len);
                            (void)(post_data);
                            
                            /*
                             * @todo: Call user function with POST data here
                             */
                            ESP_DEBUGF(ESP_DBG_SERVER, "POST DATA: %.*s\r\n", post_data_len, post_data);
                            
                            /*
                             * Check if we have content length set and in case we have, 
                             * set new content length variable
                             */
                            if (cont_len > post_data_len) {
                                cont_len -= post_data_len;
                            } else {
                                cont_len = 0;   /* Clear length to zero immediatelly */
                            }
                        }
                        
                        /*
                         * Do we still have some data to be received?
                         * In this case wait blocked until all data are received
                         */
                        while (cont_len) {
                            ESP_DEBUGF(ESP_DBG_SERVER, "Waiting for more POST data\r\n");
                            
                            res = esp_netconn_receive(client, &pbuf_tmp);   /* Wait for next packet */
                            
                            if (res == espOK) { /* Do we have more data? */
                                size_t len, tot_len, offset;
                                const uint8_t* data;
                                
                                /*
                                 * Get memory address and new length.
                                 * Since there is no offset, 
                                 * entire memory should be returned and length is the entire pbuf length
                                 */
                                tot_len = esp_pbuf_length(pbuf_tmp, 1); /* Get total length of pbuf chain (if exists) */
                                offset = 0;
                                len = 0;
                                do {
                                    offset += len;  /* Set new offset */
                                    data = esp_pbuf_get_linear_addr(pbuf_tmp, offset, &len);
                                    if (!data) {    /* End of pbuf reached? */
                                        break;
                                    }
                                    
                                    /**
                                     * @todo: Call user function with POST data here
                                     */
                                    ESP_DEBUGF(ESP_DBG_SERVER, "POST DATA: %.*s\r\n", len, data);
                                } while ((len + offset) < tot_len);
                                
                                if (cont_len > tot_len) {
                                    cont_len -= tot_len;
                                } else {
                                    cont_len = 0;
                                }
                                esp_pbuf_free(pbuf_tmp);    /* Free unused memory */
                            } else {
                                break;          /* Something went wrong, maybe connection closed */
                            }
                        }
                        ESP_DEBUGF(ESP_DBG_SERVER, "We received all data on POST\r\n");
                    } else {
                        ESP_DEBUGF(ESP_DBG_SERVER, "POST: No content length entry found in header! We are not expecting more data\r\n");
                    }
                    break;
                } else {
                    res = espERR;               /* Invalid method received */
                    break;
                }
            }
        } else {
            break;
        }
    } while (1);
    
    /*
     * Take care of output to be sent back to user
     * Output depends on request header in pbuf chain
     */
    if (res == espOK) {                         /* Everything ready to start? */
        if (http_parse_uri(pbuf) == espOK) {    /* Try to parse URI from request */
            const fs_file_t* file = http_get_file_from_uri(http_uri);   /* Get file from URI */
            if (file) {
                esp_netconn_write(client, file->data, file->len);
                esp_netconn_flush(client);      /* Flush data and send them */
                fs_data_close_file(file);       /* Close file name */
            }
        }
    }
    if (pbuf) {
        esp_pbuf_free(pbuf);                    /* Free received memory after usage */
    }
    if (res != espCLOSED) {                     /* Should we close a connection? */
        esp_netconn_close(client);              /* Manually close the connection */
    }
    esp_netconn_delete(client);                 /* Delete memory for netconn */
    
    return res;
}

/**
 * \brief           Thread for processing connections acting as server
 * \param[in]       arg: Thread argument
 */
void
server_thread(void* arg) {
    esp_netconn_p server, client;
    espr_t res;
    
    ESP_DEBUGF(ESP_DBG_SERVER, "API server thread started\r\n");
    
    /*
     * Create a new netconn base connection
     * acting as mother of all clients
     */
    server = esp_netconn_new(ESP_NETCONN_TYPE_TCP); /* Prepare a new connection */
    if (server) {
        ESP_DEBUGF(ESP_DBG_SERVER, "API connection created\r\n");
        
        /*
         * Bind base connection to port 80
         */
        res = esp_netconn_bind(server, 80);     /* Bind a connection on port 80 */
        if (res == espOK) {
            ESP_DEBUGF(ESP_DBG_SERVER, "API connection binded\r\n");
            
            /*
             * Start listening on a server
             * for incoming client connections
             */
            res = esp_netconn_listen(server);   /* Start listening on a connection */
            
            /*
             * Process unlimited time
             */
            while (1) {
                ESP_DEBUGF(ESP_DBG_SERVER, "API waiting connection\r\n");
                
                /*
                 * Wait for client to connect to server.
                 * Function will block thread until we reach a new client
                 */
                res = esp_netconn_accept(server, &client);  /* Accept for a new connection */
                if (res == espOK) {             /* Do we have a new connection? */
                    ESP_DEBUGF(ESP_DBG_SERVER, "API new connection accepted: %d\r\n", (int)esp_netconn_getconnnum(client));
                    
                    /*
                     * Process client and serve according to request
                     * It will make sure to close connection and to delete
                     * connection data from memory
                     */
                    server_serve(client);       /* Server connection to a client */
                }
            }
        }
    }
}

/**
 * \brief           Initialize server using NETCONN API (RTOS required)
 * \param[in]       cgi: Pointer to list of CGI handlers. Set to NULL if not used
 * \param[in]       Length of CGI handlers. Set to 0 if not used
 */
espr_t
esp_netconn_server_init(const http_cgi_t* cgi, size_t cgi_len) {
    esp_sys_thread_t thread;
    
    cgi_list = cgi;
    cgi_size = cgi_len;
    esp_sys_thread_create(&thread, "netconn_server", server_thread, NULL, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO);
    
    return espOK;
}
