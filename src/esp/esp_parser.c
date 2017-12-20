/**
 * \file            esp_parser.c
 * \brief           Parse incoming data from AT port
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
#include "include/esp_private.h"
#include "include/esp_parser.h"
#include "include/esp_mem.h"

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
 * \brief           Parse number from string as hex
 * \note            Input string pointer is changed and number is skipped
 * \param[in]       Pointer to pointer to string to parse
 * \return          Parsed number
 */
uint32_t
espi_parse_hexnumber(const char** str) {
    int32_t val = 0;
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
    while (ESP_CHARISHEXNUM(*p)) {              /* Parse until character is valid number */
        val = val * 16 + ESP_CHARHEXTONUM(*p);
        p++;
    }
    if (*p == ',') {                            /* Go to next entry if possible */
        p++;
    }
    *str = p;                                   /* Save new pointer with new offset */
    return val;
}

/**
 * \brief           Parse input string as string part of AT command
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer. Use NULL in case you want only skip string in source
 * \param[in]       dst_len: Length of distance buffer, including memory for NULL termination
 * \param[in]       trim: Set to 1 to process entire string, even if no memory anymore
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_string(const char** src, char* dst, size_t dst_len, uint8_t trim) {
    const char* p = *src;
    size_t i;
    
    if (*p == ',') {
        p++;
    }
    if (*p == '"') {
        p++;
    }
    i = 0;
    if (dst_len) {
        dst_len--;
    }
    while (*p) {
        if (*p == '"' && (p[1] == ',' || p[1] == '\r' || p[1] == '\n')) {
            p++;
            break;
        }
        if (dst) {
            if (i < dst_len) {
                *dst++ = *p;
                i++;
            } else if (!trim) {
                break;
            }
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
    
    if (*p == '"') {
        p++;
    }
    ip[0] = espi_parse_number(&p); p++;
    ip[1] = espi_parse_number(&p); p++;
    ip[2] = espi_parse_number(&p); p++;
    ip[3] = espi_parse_number(&p);
    if (*p == '"') {
        p++;
    }
    
    *src = p;                                   /* Set new pointer */
    return 1;
}

/**
 * \brief           Parse string as MAC address
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer. Use NULL in case you want only skip string in source
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_mac(const char** src, uint8_t* mac) {
    const char* p = *src;
    
    if (*p == '"') {                            /* Go to next entry if possible */
        p++;
    }
    mac[0] = espi_parse_hexnumber(&p); p++;
    mac[1] = espi_parse_hexnumber(&p); p++;
    mac[2] = espi_parse_hexnumber(&p); p++;
    mac[3] = espi_parse_hexnumber(&p); p++;
    mac[4] = espi_parse_hexnumber(&p); p++;
    mac[5] = espi_parse_hexnumber(&p);
    if (*p == '"') {                            /* Skip quotes if possible */
        p++;
    }
    if (*p == ',') {                            /* Go to next entry if possible */
        p++;
    }
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
    
    espi_parse_string(&str, NULL, 0, 1);        /* Parse string and ignore result */
    
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
    espi_parse_ip(&str, esp.ipd.ip);            /* Parse incoming packet IP */
    esp.ipd.port = espi_parse_number(&str);     /* Get port on IPD data */
    
    memcpy(esp.conns[conn].remote_ip, esp.ipd.ip, sizeof(esp.ipd.ip));
    memcpy(&esp.conns[conn].remote_port, &esp.ipd.port, sizeof(esp.ipd.port));
    
    esp.ipd.read = 1;                           /* Start reading network data */
    esp.ipd.tot_len = len;                      /* Total number of bytes in this received packet */
    esp.ipd.rem_len = len;                      /* Number of remaining bytes to read */
    esp.ipd.conn = &esp.conns[conn];            /* Pointer to connection we have data for */
    
    return espOK;
}

#if ESP_MODE_STATION || __DOXYGEN__
/**
 * \brief           Parse received message for list access points
 * \param[in]       str: Pointer to input string starting with +CWLAP
 * \param[in]       msg: Pointer to message
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_cwlap(const char* str, esp_msg_t* msg) {
    if (!msg || msg->cmd != ESP_CMD_WIFI_CWLAP ||   /* Do we have valid message here and enough memory to save everything? */
        !msg->msg.ap_list.aps || msg->msg.ap_list.apsi >= msg->msg.ap_list.apsl ||
        msg->cmd_def != msg->cmd) {   
        return 0;
    }
    if (*str == '+') {                          /* Does string contain '+' as first character */
        str += 7;                               /* Skip this part */
    }
    if (*str++ != '(') {                        /* We must start with opening bracket */
        return 0;
    }
    
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ecn = (esp_ecn_t)espi_parse_number(&str);
    espi_parse_string(&str, msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ssid, sizeof(msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ssid), 1);
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].rssi = espi_parse_number(&str);
    espi_parse_mac(&str, msg->msg.ap_list.aps[msg->msg.ap_list.apsi].mac);
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ch = espi_parse_number(&str);
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].offset = espi_parse_number(&str);
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].cal = espi_parse_number(&str);
    
    /*
     * New version of AT (not yet public) has some more data on AT+CWLAP 
     * and these info are not known yet as there is no docs updated.
     *
     * For now just disable the check
     */
    if (*str++ != ')') {                        /* We must end with closing bracket */
        //return 0;
    }
    msg->msg.ap_list.apsi++;                    /* Increase number of found elements */
    if (msg->msg.ap_list.apf) {                 /* Set pointer if necessary */
        *msg->msg.ap_list.apf = msg->msg.ap_list.apsi;
    }
    return 1;
}
#endif /* ESP_MODE_STATION || __DOXYGEN__ */

#if ESP_MODE_ACCESS_POINT || __DOXYGEN__
/**
 * \brief           Parse received message for list stations
 * \param[in]       str: Pointer to input string starting with +CWLAP
 * \param[in]       msg: Pointer to message
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_cwlif(const char* str, esp_msg_t* msg) {
    if (!msg || msg->cmd != ESP_CMD_WIFI_CWLIF ||   /* Do we have valid message here and enough memory to save everything? */
        !msg->msg.sta_list.stas || msg->msg.sta_list.stai >= msg->msg.sta_list.stal ||
        msg->cmd_def != msg->cmd) {   
        return 0;
    }
    
    espi_parse_ip(&str, msg->msg.sta_list.stas[msg->msg.sta_list.stai].ip);
    espi_parse_mac(&str, msg->msg.sta_list.stas[msg->msg.sta_list.stai].mac);

    msg->msg.sta_list.stai++;                   /* Increase number of found elements */
    if (msg->msg.sta_list.staf) {               /* Set pointer if necessary */
        *msg->msg.sta_list.staf = msg->msg.sta_list.stai;
    }
    return 1;
}
#endif /* ESP_MODE_ACCESS_POINT || __DOXYGEN__ */

#if ESP_DNS || __DOXYGEN__
/**
 * \brief           Parse received message domain DNS name
 * \param[in]       str: Pointer to input string starting with +CWLAP
 * \param[in]       msg: Pointer to message
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_cipdomain(const char* str, esp_msg_t* msg) {
    if (!msg || msg->cmd != ESP_CMD_TCPIP_CIPDOMAIN ||  /* Do we have valid message here and enough memory to save everything? */
        msg->cmd_def != msg->cmd) {   
        return 0;
    }
    if (*str == '+') {
        str += 11;
    }
    espi_parse_ip(&str, msg->msg.dns_getbyhostname.ip); /* Parse IP address */
    return 1;
}
#endif /* ESP_DNS || __DOXYGEN__ */

#if ESP_SNTP || __DOXYGEN__

/**
 * \brief           Parse received message for SNTP time
 * \param[in]       str: Pointer to input string starting with +CWLAP
 * \param[in]       msg: Pointer to message
 * \return          1 on success, 0 otherwise
 */
uint8_t
espi_parse_cipsntptime(const char* str, esp_msg_t* msg) {
    if (!msg || msg->cmd_def != ESP_CMD_TCPIP_CIPSNTPTIME) {
        return 0;
    }
    if (*str == '+') {                              /* Check input string */
        str += 13;
    }
    /**
     * Scan for day in a week
     */
    if (!strncmp(str, "Mon", 3)) {
        msg->msg.tcpip_sntp_time.dt->day = 1;
    } else if (!strncmp(str, "Tue", 3)) {
        msg->msg.tcpip_sntp_time.dt->day = 2;
    } else if (!strncmp(str, "Wed", 3)) {
        msg->msg.tcpip_sntp_time.dt->day = 3;
    } else if (!strncmp(str, "Thu", 3)) {
        msg->msg.tcpip_sntp_time.dt->day = 4;
    } else if (!strncmp(str, "Fri", 3)) {
        msg->msg.tcpip_sntp_time.dt->day = 5;
    } else if (!strncmp(str, "Sat", 3)) {
        msg->msg.tcpip_sntp_time.dt->day = 6;
    } else if (!strncmp(str, "Sun", 3)) {
        msg->msg.tcpip_sntp_time.dt->day = 7;
    }
    str += 4;
    
    /**
     * Scan for month in a year
     */
    if (!strncmp(str, "Jan", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 1;
    } else if (!strncmp(str, "Feb", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 2;
    } else if (!strncmp(str, "Mar", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 3;
    } else if (!strncmp(str, "Apr", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 4;
    } else if (!strncmp(str, "May", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 5;
    } else if (!strncmp(str, "Jun", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 6;
    } else if (!strncmp(str, "Jul", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 7;
    } else if (!strncmp(str, "Aug", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 8;
    } else if (!strncmp(str, "Sep", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 9;
    } else if (!strncmp(str, "Oct", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 10;
    } else if (!strncmp(str, "Nov", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 11;
    } else if (!strncmp(str, "Dec", 3)) {
        msg->msg.tcpip_sntp_time.dt->month = 12;
    }
    str += 4;
    
    msg->msg.tcpip_sntp_time.dt->date = espi_parse_number(&str);
    str++;
    msg->msg.tcpip_sntp_time.dt->hours = espi_parse_number(&str);
    str++;
    msg->msg.tcpip_sntp_time.dt->minutes = espi_parse_number(&str);
    str++;
    msg->msg.tcpip_sntp_time.dt->seconds = espi_parse_number(&str);
    str++;
    msg->msg.tcpip_sntp_time.dt->year = espi_parse_number(&str);
    return 1;
}

#endif /* ESP_SNTP || __DOXYGEN__ */
