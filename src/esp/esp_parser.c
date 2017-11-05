/**
 * \file            esp_parser.c
 * \brief           Parse incoming data from AT port
 */

/*
 * Contains list of functions to parse different input strings
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
#include "esp_parser.h"

/**
 * \brief           Parse number from string
 * \note            Input string pointer is changed and number is skipped
 * \param[in]       Pointer to pointer to string to parse
 * \return          Parsed number
 */
int32_t
espi_parse_number(const char** str) {
    int32_t val = 0;
    uint8_t minus = 0;
    const char* p = *str;                       /*  */
    
    if (*p == '"') {                            /* Skip leading quotes */
        p++;
    }
    if (*p == ',') {                            /* Skip leading comma */
        p++;
    }
    if (*p == '"') {                            /* Skip leading quotes */
        p++;
    }
    if (*p == '-') {                            /* Check negative number */
        minus = 1;
        p++;
    }
    while (ESP_CHARISNUM(*p)) {                 /* Parse until character is valid number */
        val = val * 10 + ESP_CHARTONUM(*p);
        p++;
    }
    if (*p == ',') {                            /* Go to next entry if possible */
        p++;
    }
    *str = p;                                   /* Save new pointer with new offset */
    
    return minus ? -val : val;
}

/**
 * \brief           Parse input string as string part of AT command
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer. Use NULL in case you want only skip string in source
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_string(const char** src, char* dst) {
    const char* p = *src;
    
    if (*p == ',') {
        p++;
    }
    if (*p == '"') {
        p++;
    }
    while (*p) {
        if (*p == '"' && (p[1] == ',' || p[1] == '\r' || p[1] == '\n')) {
            p++;
            break;
        }
        if (dst) {
            *dst++ = *p;
        }
        p++;
    }
    if (dst) {
        *dst = 0;
    }
    *src = p;
    return 1;
}

/**
 * \brief           Parse string as IP address
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer. Use NULL in case you want only skip string in source
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_ip(const char** src, uint8_t* ip) {
    const char* p = *src;
    
    ip[0] = espi_parse_number(&p); p++;
    ip[1] = espi_parse_number(&p); p++;
    ip[2] = espi_parse_number(&p); p++;
    ip[3] = espi_parse_number(&p);
    *src = p;                                   /* Set new pointer */
    return 1;
}

/**
 * \brief           Parse +CIPSTATUS response from ESP device
 * \param[in]       str: Input string to parse
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_parse_cipstatus(const char* str) {
    uint8_t cn_num = 0, i;
    
    cn_num = espi_parse_number(&str);           /* Parse connection number */
    esp.active_conns |= 1 << cn_num;            /* Set flag as active */
    
    espi_parse_string(&str, NULL);              /* Parse string and ignore result */
   
    for (i = 0; i < 4; i++) {                   /* Process connection IP */
        esp.conns[cn_num].remote_ip[i] = espi_parse_number(&str);
        str++;
    }
    esp.conns[cn_num].remote_port = espi_parse_number(&str);
    esp.conns[cn_num].local_port = espi_parse_number(&str);
    esp.conns[cn_num].status.f.client = !espi_parse_number(&str);
    
    return espOK;
}

/**
 * \brief           Parse +IPD statement
 * \param[in]       str: Input string to parse
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_parse_ipd(const char* str) {
    uint8_t conn;
    size_t len;
    
    conn = espi_parse_number(&str);             /* Parse number for connection number */
    len = espi_parse_number(&str);              /* Parse number for number of bytes to read */
    espi_parse_ip(&str, esp.conns[conn].remote_ip);
    esp.conns[conn].local_port = espi_parse_number(&str);
    
    esp.ipd.read = 1;                           /* Start reading network data */
    esp.ipd.tot_len = len;                      /* Total number of bytes in this received packet */
    esp.ipd.rem_len = len;                      /* Number of remaining bytes to read */
    esp.ipd.conn = &esp.conns[conn];            /* Pointer to connection where data are received */
    
    return espOK;
}
