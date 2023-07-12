/*
 * Netconn server example is based on single thread
 * and it listens for single client only on port 23.
 * 
 * When new client connects, application processes client in the same thread.
 * When multiple clients get connected at the same time, 
 * each of them waits all previous to be processed first, hence it may
 * introduce latency, in some cases even clearly visible in (for example) user browser 
 */
#include "netconn_server_1thread.h"
#include "lwesp/lwesp_netconn.h"
#include "lwesp/lwesp.h"

/**
 * \brief           Basic thread for netconn server to test connections
 * \param[in]       arg: User argument
 */
void
netconn_server_1thread_thread(void* arg) {
    lwespr_t res;
    lwesp_netconn_p server, client;
    lwesp_pbuf_p p;

    LWESP_UNUSED(arg);

    /* Create netconn for server */
    server = lwesp_netconn_new(LWESP_NETCONN_TYPE_TCP);
    if (server == NULL) {
        printf("Cannot create server netconn!\r\n");
    }

    /* Bind it to port 23 */
    res = lwesp_netconn_bind(server, 23);
    if (res != lwespOK) {
        printf("Cannot bind server\r\n");
        goto out;
    }

    /* Start listening for incoming connections with maximal 1 client */
    res = lwesp_netconn_listen_with_max_conn(server, 1);
    if (res != lwespOK) {
        goto out;
    }

    /* Unlimited loop */
    while (1) {
        /* Accept new client */
        res = lwesp_netconn_accept(server, &client);
        if (res != lwespOK) {
            break;
        }
        printf("New client accepted!\r\n");
        while (1) {
            /* Receive data */
            res = lwesp_netconn_receive(client, &p);
            if (res == lwespOK) {
                printf("Data received!\r\n");
                lwesp_pbuf_free_s(&p);
            } else {
                printf("Netconn receive returned: %d\r\n", (int)res);
                if (res == lwespCLOSED) {
                    printf("Connection closed by client\r\n");
                    break;
                }
            }
        }
        /* Delete client */
        if (client != NULL) {
            lwesp_netconn_delete(client);
            client = NULL;
        }
    }
    /* Delete client */
    if (client != NULL) {
        lwesp_netconn_delete(client);
        client = NULL;
    }

out:
    printf("Terminating netconn thread!\r\n");
    if (server != NULL) {
        lwesp_netconn_delete(server);
    }
    lwesp_sys_thread_terminate(NULL);
}
