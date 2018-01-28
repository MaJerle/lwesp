/*
 * Netconn server example is based on single "user" thread
 * which listens for new connections and accepts them.
 *
 * When a new client is accepted by server,
 * separate thread for client is created where 
 * data is read, processed and send back to user
 */
#include "netconn_server.h"
#include "esp/esp.h"

static void netconn_server_processing_thread(void* const arg);

/**
 * \brief           Main page response file
 */
static const uint8_t
resp_data_mainpage[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"\r\n"
"<html><head><link rel=\"stylesheet\" href=\"style.css\" type=\"text/css\" /></head><body>Netconn driven website!</body></html>";;

/**
 * \brief           Style file response
 */
static const uint8_t
resp_data_style[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/css\r\n"
"\r\n"
"body { color: red; font-family: Tahoma, Arial; };";

/**
 * \brief           Netconn server thread implementation
 * \param[in]       arg: User argument
 */
void
netconn_server_thread(void const* arg) {
    espr_t res;
    esp_netconn_p server, client;
                                                
    /*
     * First create a new instance of netconn 
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    server = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    if (server != NULL) {
        printf("Server netconn created\r\n");
        /*
         * Bind network connection to port 80
         */
        res = esp_netconn_bind(server, 80);
        if (res == espOK) {
            printf("Server netconn listens on port 80\r\n");
            /*
             * Start listening for incoming connections
             * on previously binded port
             */
            res = esp_netconn_listen(server);
            
            while (1) {
                /*
                 * Wait and accept new client connection
                 * 
                 * Function will block thread until 
                 * new client is connected to server
                 */
                res = esp_netconn_accept(server, &client);
                if (res == espOK) {
                    printf("Netconn new client connected. Starting new thread...\r\n");
                    /*
                     * Start new thread for this request.
                     *
                     * Read and write back data to user in separated thread
                     * to allow processing of multiple requests at the same time
                     */
                    if (esp_sys_thread_create(NULL, "client", (esp_sys_thread_fn)netconn_server_processing_thread, client, 512, ESP_SYS_THREAD_PRIO)) {
                        printf("Netconn client thread created\r\n");
                    } else {
                        printf("Netconn client thread creation failed!\r\n");
                    }
                }
            }
        } else {
            printf("Netconn server cannot bind to port\r\n");
        }
    } else {
        printf("Cannot create server netconn\r\n");
    }
    
    /*
     * Terminate current thread
     */
    esp_sys_thread_terminate(NULL);
}

/**
 * \brief           Thread to process single active connection
 * \param[in]       arg: Thread argument
 */
static void
netconn_server_processing_thread(void* const arg) {
    esp_netconn_p client;
    esp_pbuf_p pbuf, p = NULL;
    espr_t res;

    client = arg;                               /* Client handle is passed to argument */
                                                
    printf("A new connection accepted!\r\n");   /* Print simple message */
                    
    do {
        /*
         * Client was accepted, we are now
         * expecting client will send to us some data
         *
         * Wait for data and block thread for that time
         */
        res = esp_netconn_receive(client, &pbuf);

        if (res == espOK) {
            printf("Netconn data received, %d bytes\r\n", (int)esp_pbuf_length(pbuf, 1));
            /*
            * Check if all headers were received
            */
            if (p == NULL) {
                p = pbuf;                       /* Set as first buffer */
            } else {
                esp_pbuf_cat(p, pbuf);          /* Concatenate buffers together */
            }
            if (esp_pbuf_strfind(pbuf, "\r\n\r\n", 0) != ESP_SIZET_MAX) {
                if (esp_pbuf_strfind(pbuf, "GET / ", 0) != ESP_SIZET_MAX) {
                    printf("Main page request\r\n");
                    esp_netconn_write(client, resp_data_mainpage, sizeof(resp_data_mainpage) - 1);
                } else if (esp_pbuf_strfind(pbuf, "GET /style.css ", 0) != ESP_SIZET_MAX) {
                    printf("Style page request\r\n");
                    esp_netconn_write(client, resp_data_style, sizeof(resp_data_style) - 1);
                } 
                esp_netconn_close(client);      /* Close netconn connection */
                esp_pbuf_free(p);               /* Do not forget to free memory after usage! */
                p = NULL;
                break;
            }
        }
    } while (res == espOK);

    if (p != NULL) {                            /* Free received data */
        esp_pbuf_free(p);
        p = NULL;
    }
    esp_netconn_delete(client);                 /* Destroy client memory */
    esp_sys_thread_terminate(NULL);             /* Terminate this thread */
}
