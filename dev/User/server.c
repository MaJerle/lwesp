#include "main.h"
#include "server.h"
#include "esp.h"
#include "fs_data.h"
#include "ctype.h"

#define ESP_DBG_SERVER              ESP_DBG_OFF

/**
 * \brief           Server a client connection
 * \param[in]       client: New connected client
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
server_serve(esp_netconn_p client) {
    esp_pbuf_p pbuf = NULL, pbuf_tmp = NULL;
    espr_t res;
    size_t pos, data_pos;
    uint8_t is_get = 0, ch;
    size_t cont_len, pbuf_tot_len;
    
    do {
        /**
         * Receive HTTP data from client
         * packet by packet until we have \r\n\r\n
         * indicating end of headers.
         */
        res = esp_netconn_receive(client, &pbuf_tmp);
        
        if (res == espOK) {
            if (!pbuf) {                        /* Is this first packet? */
                pbuf = pbuf_tmp;                /* Set it as first */
            } else {                            /* If not first packet */
                esp_pbuf_cat(pbuf, pbuf_tmp);   /* Connect buffers together */
            }
            /*
             * Try to find \r\n\r\n sequence
             * which indicates end of headers
             */
            if ((pos = esp_pbuf_memfind(pbuf, "\r\n\r\n", 4, 0)) != ESP_SIZET_MAX) {
                /**
                 * At this point, all headers are received
                 * We can start process them into something useful
                 */
                data_pos = pos + 4;             /* Ignore 4 bytes of CR LF sequence */
                
                /**
                 * Check method type we are dealing with
                 * either GET or POST are supported
                 */
                if (!esp_pbuf_memcmp(pbuf, 0, "GET", 3)) {
                    is_get = 1;                 /* We are operating in GET method */
                    ESP_DEBUGF(ESP_DBG_SERVER, "We have GET method and we are not expecting more data to be received!\r\n");
                    break;
                } else if (!esp_pbuf_memcmp(pbuf, 0, "POST", 4)) {
                    ESP_DEBUGF(ESP_DBG_SERVER, "We have POST method!\r\n");
                    
                    /**
                     * Try to read content length parameter
                     * and parse length to know how much data we should expect on POST
                     * command to be sure everything is received before processed
                     */
                    if (((pos = esp_pbuf_memfind(pbuf, "Content-Length:", 15, 0)) != ESP_SIZET_MAX) ||
                        (pos = esp_pbuf_memfind(pbuf, "content-length:", 15, 0)) != ESP_SIZET_MAX
                    ) {
                        ESP_DEBUGF(ESP_DBG_SERVER, "POST: Found Content length entry\r\n");
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
                        ESP_DEBUGF(ESP_DBG_SERVER, "Content length: %d\r\n", (int)cont_len);
                        pbuf_tot_len = esp_pbuf_length(pbuf, 1);    /* Get total length of pbuf */
                        
                        /**
                         * Do we still have some data to be received?
                         * In this case wait blocked until all data are received
                         */
                        while (cont_len > (pbuf_tot_len - data_pos)) {
                            ESP_DEBUGF(ESP_DBG_SERVER, "Waiting for more POST data\r\n");
                            res = esp_netconn_receive(client, &pbuf_tmp);   /* Wait for next packet */
                            if (res == espOK) { /* Do we have more data? */
                                esp_pbuf_cat(pbuf, pbuf_tmp);   /* Connect them together */
                                pbuf_tot_len = esp_pbuf_length(pbuf, 1);    /* Get new length */
                            } else {
                                break;          /* Something went wrong, maybe connection closed */
                            }
                        }
                        ESP_DEBUGF(ESP_DBG_SERVER, "We received all data on POST\r\n");
                    } else {
                        ESP_DEBUGF(ESP_DBG_SERVER, "POST: No content length entry found in header! We are not expecting more data\r\n");
                    }
                    break;
                }
            }
        } else {
            break;
        }
    } while (1);
    
    /**
     * Take care of output to be sent back to user
     * Output depends on request header in pbuf chain
     */
    if (res == espOK) {                         /* Everything ready to start? */
        fs_file_t* file = fs_data_open_file(pbuf, is_get);  /* Open file */
        if (file) {                             /* Do we have a file to write? */
            esp_netconn_write(client, file->data, file->len);   /* Write file data to user */
        }
    }
    esp_pbuf_free(pbuf);                        /* Free received memory after usage */
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
server_thread(void const* arg) {
    esp_netconn_p server, client;
    espr_t res;
    
    ESP_DEBUGF(ESP_DBG_SERVER, "API server thread started\r\n");
    
    /**
     * Create a new netconn base connection
     * acting as mother of all clients
     */
    server = esp_netconn_new(ESP_NETCONN_TYPE_TCP); /* Prepare a new connection */
    if (server) {
        ESP_DEBUGF(ESP_DBG_SERVER, "API connection created\r\n");
        
        /**
         * Bind base connection to port 80
         */
        res = esp_netconn_bind(server, 80);     /* Bind a connection on port 80 */
        if (res == espOK) {
            ESP_DEBUGF(ESP_DBG_SERVER, "API connection binded\r\n");
            
            /**
             * Start listening on a server
             * for incoming client connections
             */
            res = esp_netconn_listen(server);   /* Start listening on a connection */
            
            /**
             * Process unlimited time
             */
            while (1) {
                ESP_DEBUGF(ESP_DBG_SERVER, "API waiting connection\r\n");
                
                /**
                 * Wait for client to connect to server.
                 * Function will block thread until we reach a new client
                 */
                res = esp_netconn_accept(server, &client);  /* Accept for a new connection */
                if (res == espOK) {             /* Do we have a new connection? */
                    ESP_DEBUGF(ESP_DBG_SERVER, "API new connection accepted: %d\r\n", (int)esp_netconn_getconnnum(client));
                    
                    /**
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

