/**
 * \file            esp_parser.h
 * \brief           Parser of AT responses
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
#ifndef __ESP_PARSER_H
#define __ESP_PARSER_H

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

#include "esp.h"

espr_t      espi_parse_cipstatus(const char* str);
espr_t      espi_parse_ipd(const char* str);
    
int32_t     espi_parse_number(const char** str);
uint8_t     espi_parse_string(const char** src, char* dst, size_t dst_len, uint8_t trim);
uint8_t     espi_parse_ip(const char** src, uint8_t* ip);
uint8_t     espi_parse_mac(const char** src, uint8_t* mac);
    
uint8_t     espi_parse_cwlap(const char* src, esp_msg_t* msg);
espr_t      espi_parse_cwlif(const char* str, esp_msg_t* msg);
uint8_t     espi_parse_cipdomain(const char* src, esp_msg_t* msg);
uint8_t     espi_parse_cipsntptime(const char* str, esp_msg_t* msg);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* __ESP_PARSER_H */
