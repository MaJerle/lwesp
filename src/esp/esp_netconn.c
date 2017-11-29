/**
 * \file            esp_api.c
 * \brief           API functions for sequential calls
 */

/*
 * Copyright (c) 2017, Tilen MAJERLE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *  * Neither the name of the author nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \author          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "esp_netconn.h"
#include "esp_mem.h"

espi_netconn_t* listen_api;                     /* Main connection in listening mode */

/**
 * \brief           Callback function for every server connection
 * \param[in]       cb: Pointer to callback structure
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
esp_cb(esp_cb_t* cb) {
    esp_conn_t* conn;
    espi_netconn_t* api;
    uint8_t close = 0;
    
    switch (cb->type) {
        case ESP_CB_CONN_ACTIVE: {              /* A new connection active as server */
            conn = cb->cb.conn_active_closed.conn;
            if (!esp_conn_is_server(conn)) {    /* Connection must be in server mode */
                return espERR;
            }
            if (listen_api) {                   /* Do we have known listening API? */
                api = espi_netconn_new();       /* Create new API */
                if (api) {
                    api->conn = conn;           /* Set connection callback */
                    esp_conn_set_arg(conn, api);/* Set argument for connection */
                    if (esp_sys_mbox_isvalid(&listen_api->mbox_accept)) {
                        if (!esp_sys_mbox_putnow(&listen_api->mbox_accept, api)) {
                            close = 1;
                        }
                    } else {
                        close = 1;
                    }
                } else {
                    close = 1;
                }
            } else {
                close = 1;                      /* Close the connection at this point */
            }
            if (close) {
                esp_conn_close(conn, 0);        /* Close the connection */
                if (api) {
                    espi_netconn_delete(api);   /* Free memory for API */
                }
            }
            break;
        }
        
        /**
         * We have a new data received 
         * on a conection which should be of type server
         * and should have netconn structure as argument
         */
        case ESP_CB_DATA_RECV: {
            conn = cb->cb.conn_data_recv.conn;  /* Get connection */
            api = conn->arg;                    /* Get API from connection */
            if (!api || !esp_sys_mbox_isvalid(&api->mbox_receive) || 
                !esp_sys_mbox_putnow(&api->mbox_receive, (void *)cb->cb.conn_data_recv.buff)) {
                return espOKIGNOREMORE;         /* Return OK to free the memory */
            }
            return espOKMEM;                    /* Return ok but do not release memory */
        }
        case ESP_CB_CONN_CLOSED: {
            conn = cb->cb.conn_active_closed.conn;  /* Get connection */
            api = conn->arg;                    /* Get API from connection */
            if (api) {
                espi_netconn_delete(api);       /* Delete API */
            }
            break;
        }
        default:
            return espERR;
    }
    return espOK;
}

espi_netconn_t*
espi_netconn_new(void) {
    espi_netconn_t* a;
    a = esp_mem_calloc(1, sizeof(*a));          /* Allocate memory for core object */
    if (a) {
        if (!esp_sys_mbox_create(&a->mbox_accept, 5)) {
            ESP_DEBUGF(ESP_DBG_API, "API: cannot create accept MBOX\r\n");
            goto free_ret;
        }
        if (!esp_sys_mbox_create(&a->mbox_receive, 10)) {
            ESP_DEBUGF(ESP_DBG_API, "API: cannot create receive MBOX\r\n");
            goto free_ret;
        }
    }
    return a;
free_ret:
    if (esp_sys_mbox_isvalid(&a->mbox_accept)) {
        esp_sys_mbox_delete(&a->mbox_accept);
    }
    if (esp_sys_mbox_isvalid(&a->mbox_receive)) {
        esp_sys_mbox_delete(&a->mbox_receive);
    }
    if (a) {
        esp_mem_free(a);
    }
    return NULL;
}

espr_t
espi_netconn_delete(espi_netconn_t* api) {
    if (esp_sys_mbox_isvalid(&api->mbox_accept)) {
        esp_sys_mbox_delete(&api->mbox_accept);
    }
    if (esp_sys_mbox_isvalid(&api->mbox_receive)) {
        esp_sys_mbox_delete(&api->mbox_receive);
    }
    if (api) {
        esp_mem_free(api);
    }
    return espOK;
}

espr_t
espi_netconn_bind(espi_netconn_t* api, uint16_t port) {
    if (esp_set_server(port, 1) != espOK) {     /* Enable server on selected port */
        return espERR;
    }
    esp_set_default_server_callback(esp_cb);    /* Set callback function for server connections */
    
    return espOK;
}

espr_t
espi_netconn_listen(espi_netconn_t* api) {
    listen_api = api;                           /* Set current main API in listening state */
    return espOK;
}

espr_t
espi_netconn_accept(espi_netconn_t* api, espi_netconn_t** new_api) {
    espi_netconn_t* tmp;
    uint32_t time;
    if (api != listen_api) {                    /* Currently only one API is allowed in listening state */
        return espERR;
    }
    
    *new_api = NULL;
    time = esp_sys_mbox_get(&api->mbox_accept, (void **)&tmp, 0);
    if (time == ESP_SYS_TIMEOUT) {
        return espERR;
    }
    *new_api = tmp;                             /* Set new pointer */
    return espOK;                               /* We have a new connection */
}

espr_t
espi_netconn_write(espi_netconn_t* api, const void* data, size_t btw) {
    return esp_conn_send(api->conn, data, btw, NULL, 1);
}

espr_t
espi_netconn_receive(espi_netconn_t* api, esp_pbuf_t** pbuf) {
    return esp_sys_mbox_get(&api->mbox_receive, (void **)pbuf, 0) ? espOK : espERR;
}

espr_t
espi_netconn_close(espi_netconn_t* api) {
    esp_pbuf_t* pbuf;
    if (esp_conn_close(api->conn, 1) == espOK) {    /* Close the connection */
        while (1) {                             /* Connection was closed, flush now entire mbox */
            if (!esp_sys_mbox_getnow(&api->mbox_receive, (void *)&pbuf)) {
                break;
            }
            esp_pbuf_free(pbuf);                /* Free the packet buffer memory */
        }
    }               
    return espOK;
}

