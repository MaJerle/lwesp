/**	
 * \file            esp_sta.h
 * \brief           Station API
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
#ifndef __ESP_STA_H
#define __ESP_STA_H

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
 * \defgroup        ESP_STA Station API
 * \brief           Station API
 * \{
 */

espr_t      esp_sta_join(const char* name, const char* pass, const uint8_t* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_quit(uint32_t blocking);
espr_t      esp_sta_getip(void* ip, void* gw, void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_sta_setip(const void* ip, const void* gw, const void* nm, uint8_t def, uint32_t blocking);
espr_t      esp_sta_getmac(void* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_setmac(const void* mac, uint8_t def, uint32_t blocking);
espr_t      esp_sta_has_ip(void);
espr_t      esp_sta_copy_ip(void* ip, void* gw, void* nm);
espr_t      esp_sta_list_ap(const char* ssid, esp_ap_t* aps, size_t apsl, size_t* apf, uint32_t blocking);

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

#endif /* __ESP_STA_H */
