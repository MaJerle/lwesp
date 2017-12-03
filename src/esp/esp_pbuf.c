/**	
 * \file            esp_pbuf.c
 * \brief           Packet buffer manager
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
#define ESP_INTERNAL
#include "include/esp_private.h"
#include "include/esp_pbuf.h"
#include "include/esp_mem.h"

/**
 * \brief           Allocate packet buffer for network data of specific size
 * \param[in]       len: Length of payload memory to allocate
 * \return          Pointer to allocated memory or NULL in case of failure
 */
esp_pbuf_p
esp_pbuf_alloc(size_t len) {
    esp_pbuf_p p;
    
    p = esp_mem_calloc(1, ESP_MEM_ALIGN(sizeof(*p)) + len); /* Allocate memory for packet buffer */
    ESP_DEBUGW(ESP_DBG_PBUF, p == NULL, "PBUF: Failed to allocate %d bytes\r\n", (int)len);
    ESP_DEBUGW(ESP_DBG_PBUF, p != NULL, "PBUF: Allocated %d bytes\r\n", (int)len);
    if (p) {
        p->len = len;                           /* Set payload length */
        p->payload = (uint8_t *)(((char *)p) + ESP_MEM_ALIGN(sizeof(*p)));  /* Set pointer to payload data */
    }
    return p;
}

/**
 * \brief           Free previously allocated packet buffer
 * \param[in]       pbuf: Packet buffer to free
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_pbuf_free(esp_pbuf_p pbuf) {
    if (pbuf) {
        ESP_DEBUGF(ESP_DBG_PBUF, "PBUF: Free pbuf len %d bytes\r\n", (int)pbuf->len);
        esp_mem_free(pbuf);
        return espOK;
    }
    return espERR;
}

/**
 * \brief           Get data pointer from packet buffer
 * \param[in]       pbuf: Packet buffer
 * \return          Pointer to data buffer or NULL if invalid buffer
 */
const void *
esp_pbuf_data(esp_pbuf_p pbuf) {
    return pbuf ? pbuf->payload : NULL;
}

/**
 * \brief           Get length of packet buffer
 * \param[in]       pbuf: Packet buffer
 * \return          Length of data in units of bytes
 */
size_t
esp_pbuf_length(esp_pbuf_p pbuf) {
    return pbuf ? pbuf->len : 0;
}

/**
 * \brief           Set IP address and port number for received data
 * \param[in]       pbuf: Packet buffer
 * \param[in]       ip: IP to assing to packet buffer
 * \param[in]       port: Port number to assign to packet buffer
 */
void
esp_pbuf_set_ip(esp_pbuf_p pbuf, void* ip, uint16_t port) {
    if (pbuf && ip) {
        memcpy(pbuf->ip, ip, 4);
        pbuf->port = port;
    }
}
