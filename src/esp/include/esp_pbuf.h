/**	
 * \file            esp_pbuf.h
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
#ifndef __ESP_PBUF_H
#define __ESP_PBUF_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include "esp.h"

/**
 * \addtogroup      ESP
 * \{
 */
    
/**
 * \defgroup        ESP_PBUF Packet buffer
 * \brief           Packet buffer manager
 * \{
 */
    
esp_pbuf_p      esp_pbuf_new(size_t len);
uint16_t        esp_pbuf_free(esp_pbuf_p pbuf);
const void*     esp_pbuf_data(const esp_pbuf_p pbuf);
size_t          esp_pbuf_length(const esp_pbuf_p pbuf, uint8_t tot);
espr_t          esp_pbuf_take(esp_pbuf_p pbuf, const void* data, size_t len, size_t offset);
size_t          esp_pbuf_copy(esp_pbuf_p pbuf, void* data, size_t len, size_t offset);

espr_t          esp_pbuf_cat(esp_pbuf_p head, const esp_pbuf_p tail);
espr_t          esp_pbuf_chain(esp_pbuf_p head, esp_pbuf_p tail);
espr_t          esp_pbuf_ref(esp_pbuf_p pbuf);

uint8_t         esp_pbuf_get_at(const esp_pbuf_p pbuf, size_t pos, uint8_t* el);
size_t          esp_pbuf_memcmp(const esp_pbuf_p pbuf, size_t offset, const void* data, size_t len);
size_t          esp_pbuf_memfind(const esp_pbuf_p pbuf, const void* data, size_t len, size_t off);

void            esp_pbuf_set_ip(esp_pbuf_p pbuf, const void* ip, uint16_t port);
    
/**
 * \}
 */
    
/**
 * \}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __ESP_PBUF_H */
