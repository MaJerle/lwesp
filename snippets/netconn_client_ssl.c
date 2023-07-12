/*
 * Netconn client demonstrates how to connect as a client to server
 * using sequential API from separate thread.
 * 
 * it does not use callbacks to obtain connection status.
 * 
 * Demo connects to NETCONN_HOST at NETCONN_PORT and sends GET request header,
 * then waits for respond and expects server to close the connection accordingly.
 */
#include "lwesp/lwesp.h"
#include "lwesp/lwesp_netconn.h"
#include "netconn_client.h"

/* Certificates, ready to be loaded to the flash */
static uint8_t client_ca[] = {
#include "../certificates/client_ca_generated_atpki.hex"
};
static uint8_t client_cert[] = {
#include "../certificates/client_cert_generated_atpki.hex"
};
static uint8_t client_key[] = {
#include "../certificates/client_key_generated_atpki.hex"
};

/**
 * \brief           Host and port settings
 */
#define NETCONN_HOST "example.com"
#define NETCONN_PORT 443

/**
 * \brief           Request header to send on successful connection
 */
static const char request_header[] = ""
                                     "GET / HTTP/1.1\r\n"
                                     "Host: " NETCONN_HOST "\r\n"
                                     "Connection: close\r\n"
                                     "\r\n";

/**
 * \brief           Netconn client thread implementation
 * \param[in]       arg: User argument
 */
void
netconn_client_ssl_thread(void const* arg) {
    lwespr_t res;
    lwesp_pbuf_p pbuf;
    lwesp_netconn_p client;
    lwesp_sys_sem_t* sem = (void*)arg;

    /* Make sure we are connected to access point first */
    while (!lwesp_sta_has_ip()) {
        lwesp_delay(1000);
    }

    /*
     * First create a new instance of netconn
     * connection and initialize system message boxes
     * to accept received packet buffers
     */
    client = lwesp_netconn_new(LWESP_NETCONN_TYPE_SSL);
    if (client != NULL) {
        struct tm dt;

        /* First erase all client SSL settings */
        lwesp_flash_erase(LWESP_FLASH_PARTITION_CLIENT_CA, 0, 0, NULL, NULL, 1);
        lwesp_flash_erase(LWESP_FLASH_PARTITION_CLIENT_CERT, 0, 0, NULL, NULL, 1);
        lwesp_flash_erase(LWESP_FLASH_PARTITION_CLIENT_KEY, 0, 0, NULL, NULL, 1);

        /* Load new SSL config values to flash partition */
        lwesp_flash_write(LWESP_FLASH_PARTITION_CLIENT_CA, 0, sizeof(client_ca), client_ca, NULL, NULL, 1);
        lwesp_flash_write(LWESP_FLASH_PARTITION_CLIENT_CERT, 0, sizeof(client_cert), client_cert, NULL, NULL, 1);
        lwesp_flash_write(LWESP_FLASH_PARTITION_CLIENT_KEY, 0, sizeof(client_key), client_key, NULL, NULL, 1);

        /* Configure SSL for all connections */
        for (size_t i = 0; i < LWESP_CFG_MAX_CONNS; ++i) {
            lwesp_conn_ssl_set_config(i, 1, 0, 0, NULL, NULL, 1);
        }

        /* Ensure SNTP is enabled, time is required for SSL */
        lwesp_sntp_set_config(1, 2, NULL, NULL, NULL, NULL, NULL, 1);
        do {
            lwesp_sntp_gettime(&dt, NULL, NULL, 1);
            if (dt.tm_year > 100) {
                break;
            }
        } while (1);

        /*
         * Connect to external server as client
         * with custom NETCONN_CONN_HOST and CONN_PORT values
         *
         * Function will block thread until we are successfully connected (or not) to server
         */
        res = lwesp_netconn_connect(client, NETCONN_HOST, NETCONN_PORT);
        if (res == lwespOK) { /* Are we successfully connected? */
            printf("Connected to " NETCONN_HOST "\r\n");
            res = lwesp_netconn_write(client, request_header, sizeof(request_header) - 1); /* Send data to server */
            if (res == lwespOK) {
                res = lwesp_netconn_flush(client); /* Flush data to output */
            }
            if (res == lwespOK) { /* Were data sent? */
                printf("Data were successfully sent to server\r\n");

                /*
                 * Since we sent HTTP request,
                 * we are expecting some data from server
                 * or at least forced connection close from remote side
                 */
                do {
                    /*
                     * Receive single packet of data
                     *
                     * Function will block thread until new packet
                     * is ready to be read from remote side
                     *
                     * After function returns, don't forgot the check value.
                     * Returned status will give you info in case connection
                     * was closed too early from remote side
                     */
                    res = lwesp_netconn_receive(client, &pbuf);
                    if (res
                        == lwespCLOSED) { /* Was the connection closed? This can be checked by return status of receive function */
                        printf("Connection closed by remote side...\r\n");
                        break;
                    } else if (res == lwespTIMEOUT) {
                        printf("Netconn timeout while receiving data. You may try multiple readings before deciding to "
                               "close manually\r\n");
                    }

                    if (res == lwespOK && pbuf != NULL) { /* Make sure we have valid packet buffer */
                        /*
                         * At this point, read and manipulate
                         * with received buffer and check if you expect more data
                         *
                         * After you are done using it, it is important
                         * you free the memory, or memory leaks will appear
                         */
                        printf("Received new data packet of %d bytes\r\n", (int)lwesp_pbuf_length(pbuf, 1));
                        lwesp_pbuf_free_s(&pbuf); /* Free the memory after usage */
                    }
                } while (1);
            } else {
                printf("Error writing data to remote host!\r\n");
            }

            /*
             * Check if connection was closed by remote server
             * and in case it wasn't, close it manually
             */
            if (res != lwespCLOSED) {
                lwesp_netconn_close(client);
            }
        } else {
            printf("Cannot connect to remote host %s:%d!\r\n", NETCONN_HOST, NETCONN_PORT);
        }
        lwesp_netconn_delete(client); /* Delete netconn structure */
    }

    printf("Terminating thread\r\n");
    if (lwesp_sys_sem_isvalid(sem)) {
        lwesp_sys_sem_release(sem);
    }
    lwesp_sys_thread_terminate(NULL); /* Terminate current thread */
}
