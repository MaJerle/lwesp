/**
 * \file            esp_netconn.h
 * \brief           API functions for sequential calls
 */
 
/*
 *
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
#ifndef __ESP_NETCONN_H
#define __ESP_NETCONN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "esp.h"

/**
 * \addtogroup      ESP
 * \{
 */

/**
 * \defgroup        ESP_NETCONN Network connection
 * \brief           Network connection
 * \{
 *
 * Netconn provides sequential API to work with connection 
 * in either server or client mode.
 *
 * It was designed to allow programming in blocking way (instead of events),
 * but still without data copy between calls
 *
 * To use netconn API, RTOS support is mandatory.
 * Processing is done in core thread of ESP library,
 * but user application thread communicates with main one using message queues,
 * which must be part of OS system implementation.
 *
 * All the NETCONN API functions are blocking and will wait for response before returning.
 */
 
/**
 * \brief           Netconn connection type
 */
typedef enum {
    ESP_NETCONN_TYPE_TCP = ESP_CONN_TYPE_TCP,   /*!< TCP connection */
    ESP_NETCONN_TYPE_SSL = ESP_CONN_TYPE_SSL,   /*!< SSL connection */
    ESP_NETCONN_TYPE_UDP = ESP_CONN_TYPE_UDP,   /*!< UDP connection */
} esp_netconn_type_t;

struct esp_netconn_t;
typedef struct esp_netconn_t* esp_netconn_p;

esp_netconn_p   esp_netconn_new(esp_netconn_type_t type);
espr_t          esp_netconn_delete(esp_netconn_p nc);
espr_t          esp_netconn_bind(esp_netconn_p nc, uint16_t port);
espr_t          esp_netconn_connect(esp_netconn_p nc, const char* host, uint16_t port);
espr_t          esp_netconn_receive(esp_netconn_p nc, esp_pbuf_p* pbuf);
espr_t          esp_netconn_close(esp_netconn_p nc);
int8_t          esp_netconn_getconnnum(esp_netconn_p nc);

/* TCP only */
espr_t          esp_netconn_listen(esp_netconn_p nc);
espr_t          esp_netconn_accept(esp_netconn_p nc, esp_netconn_p* new_api);
espr_t          esp_netconn_write(esp_netconn_p nc, const void* data, size_t btw);

/* UDP only */
espr_t          esp_netconn_send(esp_netconn_p nc, const void* data, size_t btw);
espr_t          esp_netconn_sendto(esp_netconn_p nc, const void* ip, uint16_t port, const void* data, size_t btw);

/**
 * \}
 */
 
/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_NETCONN_H */
