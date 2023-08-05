/**
 * \file            lwesp_parser.c
 * \brief           Parse incoming data from AT port
 */

/*
 * Copyright (c) 2023 Tilen MAJERLE
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
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.2-dev
 */
#include "lwesp/lwesp_parser.h"
#include "lwesp/lwesp_private.h"

/* Increase pointer if matches character value */
#define INC_IF_CHAR_EQUAL(p, ch)                                                                                       \
    if (*(p) == (ch)) {                                                                                                \
        ++(p);                                                                                                         \
    }

/**
 * \brief           Parse number from string
 * \note            Input string pointer is changed and number is skipped
 * \param[in,out]   str: Pointer to pointer to string to parse
 * \return          Parsed number
 */
int32_t
lwespi_parse_number(const char** str) {
    int32_t val = 0;
    uint8_t minus = 0;
    const char* p = *str; /*  */

    INC_IF_CHAR_EQUAL(p, '"'); /* Skip leading quotes */
    INC_IF_CHAR_EQUAL(p, ','); /* Skip leading comma */
    INC_IF_CHAR_EQUAL(p, '"'); /* Skip leading quotes */
    if (*p == '-') {           /* Check negative number */
        minus = 1;
        ++p;
    }
    while (LWESP_CHARISNUM(*p)) { /* Parse until character is valid number */
        val = val * 10 + LWESP_CHARTONUM(*p);
        ++p;
    }
    INC_IF_CHAR_EQUAL(p, ','); /* Go to next entry if possible */
    *str = p;                  /* Save new pointer with new offset */

    return minus ? -val : val;
}

/**
 * \brief           Parse port from string
 * \note            Input string pointer is changed and number is skipped
 * \param[in,out]   str: Pointer to pointer to string to parse
 * \return          Parsed port number
 */
lwesp_port_t
lwespi_parse_port(const char** str) {
    lwesp_port_t p;

    p = (lwesp_port_t)lwespi_parse_number(str); /* Parse port */
    return p;
}

/**
 * \brief           Parse number from string as hex
 * \note            Input string pointer is changed and number is skipped
 * \param[in,out]   str: Pointer to pointer to string to parse
 * \return          Parsed number
 */
uint32_t
lwespi_parse_hexnumber(const char** str) {
    int32_t val = 0;
    const char* p = *str; /*  */

    INC_IF_CHAR_EQUAL(p, '"');       /* Skip leading quotes */
    INC_IF_CHAR_EQUAL(p, ',');       /* Skip leading comma */
    INC_IF_CHAR_EQUAL(p, '"');       /* Skip leading quotes */
    while (LWESP_CHARISHEXNUM(*p)) { /* Parse until character is valid number */
        val = val * 16 + LWESP_CHARHEXTONUM(*p);
        ++p;
    }
    INC_IF_CHAR_EQUAL(p, ','); /* Go to next entry if possible */
    *str = p;                  /* Save new pointer with new offset */
    return val;
}

/**
 * \brief           Parse input string as string part of AT command
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer.
 *                      Set to `NULL` in case you want to skip string in source
 * \param[in]       dst_len: Length of destination buffer,
 *                      including memory for `NULL` termination
 * \param[in]       trim: Set to `1` to process entire string,
 *                      even if no memory anymore
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_string(const char** src, char* dst, size_t dst_len, uint8_t trim) {
    const char* p = *src;
    size_t i;

    INC_IF_CHAR_EQUAL(p, ','); /* Skip leading comma */
    INC_IF_CHAR_EQUAL(p, '"'); /* Skip leading quotes */
    i = 0;
    if (dst_len > 0) {
        --dst_len;
    }
    while (*p) {
        if ((*p == '"' && (p[1] == ',' || p[1] == '\r' || p[1] == '\n')) || (*p == '\r' || *p == '\n')) {
            ++p;
            break;
        }
        if (dst != NULL) {
            if (i < dst_len) {
                *dst++ = *p;
                ++i;
            } else if (!trim) {
                break;
            }
        }
        ++p;
    }
    if (dst != NULL) {
        *dst = 0;
    }
    *src = p;
    return 1;
}

#if LWESP_CFG_IPV6

/**
 * \brief           Parses IP V6 only, w/o possibility to append IP v4 to it.
 * 
 * \param           ip_str_iterator: Pointer to pointer to string
 * \param           ip: IP structure
 * \return          `1` if IP well parsed, `0` otherwise
 */
uint8_t
lwespi_parse_ipv6(const char** ip_str_iterator, lwesp_ip_t* ip) {
    int8_t index_with_zeros = -1; /* Not found! */
    uint8_t index;

    memset(ip, 0x00, sizeof(*ip));
    for (index = 0; index < 8 && !(**ip_str_iterator == '\0' || **ip_str_iterator == '"'); ++index) {
        const char* ip_str_before = *ip_str_iterator;
        uint16_t seg_value = lwespi_parse_hexnumber(ip_str_iterator);
        const char* ip_str_after = *ip_str_iterator;

        if (**ip_str_iterator == ':') {
            ++(*ip_str_iterator);
        }

        /* If these are equal, a 0 break was detected */
        if (ip_str_before == ip_str_after) {
            /* We cannot have more than 1 separator */
            if (index_with_zeros >= 0) {
                return 0;
            } else {
                /* Save where break was detected */
                index_with_zeros = (int8_t)index;
            }
            if (index == 0) {
                ++(*ip_str_iterator);
            }
        } else {
            ip->addr.ip6.addr[index] = seg_value;
        }
    }

    /*
     * Did we find zeros separator?
     * Now we need to find where it was and compare to
     * number of processed tokens.
     *
     * All is to be shifted, if segment is at the beginning or in the middle
     */
    if (index_with_zeros >= 0) {
        uint8_t segments_to_move = index - (uint8_t)index_with_zeros - 1;
        uint8_t index_start = (uint8_t)index_with_zeros + 1;
        uint8_t index_end = (uint8_t)8 - segments_to_move;

        if (segments_to_move > 0) {
            memmove(&ip->addr.ip6.addr[index_end],                    /* TO */
                    &ip->addr.ip6.addr[index_start],                  /* FROM */
                    (segments_to_move) * sizeof(ip->addr.ip6.addr[0]) /* Count */
            );
            memset(&ip->addr.ip6.addr[index_with_zeros + 1], 0x00,
                   (index_end - index_start) * sizeof(ip->addr.ip6.addr[0]));
        }
    }
    return 1;
}

#endif /* LWESP_CFG_IPV6 */

/**
 * \brief           Parse string as IP address
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[out]      ip: Pointer to IP memory
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_ip(const char** src, lwesp_ip_t* ip) {
    const char* p = *src;
#if LWESP_CFG_IPV6
    char c;
#endif /* LWESP_CFG_IPV6 */

    INC_IF_CHAR_EQUAL(p, '"'); /* Skip leading quotes */

#if LWESP_CFG_IPV6
    /* Find first separator */
    c = 0;
    for (size_t i = 0; i < 6; ++i) {
        if (p[i] == ':' || p[i] == ',') {
            c = p[i];
            break;
        }
    }
#endif /* LWESP_CFG_IPV6 */

    /* Go to original value */
    if (0) {
#if LWESP_CFG_IPV6
    } else if (c == ':') {
        ip->type = LWESP_IPTYPE_V6;

        /* Parse IP address as separate function  */
        memset(&ip->addr, 0x00, sizeof(ip->addr));
        lwespi_parse_ipv6(&p, ip);
#endif /* LWESP_CFG_IPV6 */
    } else {
        ip->type = LWESP_IPTYPE_V4;
        for (size_t i = 0; i < LWESP_ARRAYSIZE(ip->addr.ip4.addr); ++i, ++p) {
            ip->addr.ip4.addr[i] = lwespi_parse_number(&p);
            if (*p != '.') {
                break;
            }
        }
    }
    INC_IF_CHAR_EQUAL(p, '"'); /* Skip trailing quotes */

    *src = p; /* Set new pointer */
    return 1;
}

/**
 * \brief           Parse string as MAC address
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[out]      mac: Pointer to MAC memory
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_mac(const char** src, lwesp_mac_t* mac) {
    const char* p = *src;

    INC_IF_CHAR_EQUAL(p, '"'); /* Skip leading quotes */
    mac->mac[0] = lwespi_parse_hexnumber(&p);
    ++p;
    mac->mac[1] = lwespi_parse_hexnumber(&p);
    ++p;
    mac->mac[2] = lwespi_parse_hexnumber(&p);
    ++p;
    mac->mac[3] = lwespi_parse_hexnumber(&p);
    ++p;
    mac->mac[4] = lwespi_parse_hexnumber(&p);
    ++p;
    mac->mac[5] = lwespi_parse_hexnumber(&p);
    INC_IF_CHAR_EQUAL(p, '"'); /* Skip trailing quotes */
    INC_IF_CHAR_EQUAL(p, ','); /* Go to next entry if possible */
    *src = p;                  /* Set new pointer */
    return 1;
}

/**
 * \brief           Parse +CIPSTATUS or +CIPSTATE response from ESP device
 * \param[in]       str: Input string to parse
 * \return          Member of \ref lwespr_t enumeration
 */
lwespr_t
lwespi_parse_cipstatus_cipstate(const char* str) {
    uint8_t cn_num = 0;

    cn_num = lwespi_parse_number(&str); /* Parse connection number */
    esp.m.active_conns |= 1 << cn_num;  /* Set flag as active */

    /*
     * If connection looks "alive" in the 
     * cipstatus result, but not alive in internal
     * structure, then force connection close ASAP
     */
    if (!esp.m.conns[cn_num].status.f.active) {
        lwesp_conn_close(&esp.m.conns[cn_num], 0);
    }

    lwespi_parse_string(&str, NULL, 0, 1); /* Parse string and ignore result */
    lwespi_parse_ip(&str, &esp.m.conns[cn_num].remote_ip);
    esp.m.conns[cn_num].remote_port = lwespi_parse_number(&str);
    esp.m.conns[cn_num].local_port = lwespi_parse_number(&str);
    esp.m.conns[cn_num].status.f.client = !lwespi_parse_number(&str);

    return lwespOK;
}

#if LWESP_CFG_CONN_MANUAL_TCP_RECEIVE

/**
 * \brief           Parse +CIPRECVLEN statement
 * \param[in]       str: Input string to parse
 * \return          Member of \ref lwespr_t enumeration
 */
lwespr_t
lwespi_parse_ciprecvlen(const char* str) {
    int32_t len;

    if (*str == '+') {
        str += 12;
    }
    for (size_t i = 0; i < LWESP_CFG_MAX_CONNS; ++i) {
        len = lwespi_parse_number(&str);
        if (len < 0) {
            continue;
        }
        esp.m.conns[i].tcp_available_bytes = len;
    }
    return lwespOK;
}

#endif /* LWESP_CFG_CONN_MANUAL_TCP_RECEIVE */

/**
 * \brief           Parse +IPD statement
 * \param[in]       str: Input string to parse
 * \return          Member of \ref lwespr_t enumeration
 */
lwesp_conn_p
lwespi_parse_ipd(const char* str) {
    lwesp_conn_p c;
    uint8_t conn, is_data_ipd;
    size_t len;

    if (*str == '+') {
        str += 5;
    }

    conn = lwespi_parse_number(&str); /* Parse number for connection number */
    len = lwespi_parse_number(&str);  /* Parse number for number of available_bytes/bytes_to_read */
    c = conn < LWESP_CFG_MAX_CONNS ? &esp.m.conns[conn] : NULL;
    if (c == NULL) {
        return NULL;
    }

    /*
     * First check if this string is "notification only" or actual "data packet".
     *
     * Take decision based on ':' character before data. We can expect 3 types of format:
     *
     * +IPD,conn_num,available_bytes<CR><LF>                    : Notification only, for TCP connection
     * +IPD,conn_num,bytes_in_packet:data                       : Data packet w/o remote ip/port,
     *                                                              as response on manual TCP read or if AT+CIPDINFO=0
     * +IPD,conn_num,bytes_in_packet,remote_ip,remote_port:data : Data packet w/ remote ip/port,
     *                                                              as response on automatic read of all connection types
     */
    is_data_ipd = strchr(str, ':') != NULL; /* Check if we have ':' in string */

    if (0) {
#if LWESP_CFG_CONN_MANUAL_TCP_RECEIVE
        /*
     * Check if +IPD is only notification and not actual data packet
     * +IPD should always be only
     * notification message and never include data.
     * 
     * Actual data read shall be done with different command
     */
    } else if (!is_data_ipd) {        /* If not data packet */
        c->tcp_available_bytes = len; /* Set new value for number of bytes available to read from device */
#endif                                /* LWESP_CFG_CONN_MANUAL_TCP_RECEIVE */

        /*
         * If additional information are enabled (IP and PORT),
         * parse them and save.
         *
         * Even if information is enabled, in case of manual TCP
         * receive, these information are not present.
         *
         * Check for ':' character if it is end of string and determine how to proceed
         */
    } else if (*str != ':') {
        lwespi_parse_ip(&str, &esp.m.ipd.ip);     /* Parse incoming packet IP */
        esp.m.ipd.port = lwespi_parse_port(&str); /* Get port on IPD data */

        LWESP_MEMCPY(&esp.m.conns[conn].remote_ip, &esp.m.ipd.ip, sizeof(esp.m.ipd.ip));
        LWESP_MEMCPY(&esp.m.conns[conn].remote_port, &esp.m.ipd.port, sizeof(esp.m.ipd.port));
    }

    if (is_data_ipd) {           /* Shall we start IPD read procedure? */
        esp.m.ipd.tot_len = len; /* Total number of bytes in this received packet or notification message */
        esp.m.ipd.conn = c;      /* Pointer to connection we have data for or notification message */
        esp.m.ipd.read = 1;      /* Start reading network data */
        esp.m.ipd.rem_len = len; /* Number of remaining bytes to read */
    }
    return c;
}

/**
 * \brief           Parse AT and SDK versions from AT+GMR response
 * \param[in]       str: String starting with version numbers
 * \param[out]      version_out: Output variable to save version
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_at_sdk_version(const char* str, lwesp_sw_version_t* version_out) {
    uint32_t version = 0;

    version |= ((uint8_t)lwespi_parse_number(&str)) << 24;
    ++str;
    version |= ((uint8_t)lwespi_parse_number(&str)) << 16;
    ++str;
    version |= ((uint8_t)lwespi_parse_number(&str)) << 8;
    version_out->version = version;
    return 1;
}

/**
 * \brief           Parse +LINK_CONN received string for new connection active
 * \param[in]       str: Pointer to input string starting with +LINK_CONN
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_link_conn(const char* str) {
    if (str == NULL) {
        return 0;
    }
    if (*str == '+') {
        str += 11;
    }
    esp.m.link_conn.failed = lwespi_parse_number(&str);
    esp.m.link_conn.num = lwespi_parse_number(&str);
    if (!strncmp(str, "\"TCP\"", 5)) {
        esp.m.link_conn.type = LWESP_CONN_TYPE_TCP;
        str += 6;
    } else if (!strncmp(str, "\"UDP\"", 5)) {
        esp.m.link_conn.type = LWESP_CONN_TYPE_UDP;
        str += 6;
    } else if (!strncmp(str, "\"SSL\"", 5)) {
        esp.m.link_conn.type = LWESP_CONN_TYPE_SSL;
        str += 6;
#if LWESP_CFG_IPV6
    } else if (!strncmp(str, "\"TCPv6\"", 7)) {
        esp.m.link_conn.type = LWESP_CONN_TYPE_TCPV6;
        str += 8;
    } else if (!strncmp(str, "\"UDPv6\"", 7)) {
        esp.m.link_conn.type = LWESP_CONN_TYPE_UDPV6;
        str += 8;
    } else if (!strncmp(str, "\"SSLv6\"", 7)) {
        esp.m.link_conn.type = LWESP_CONN_TYPE_SSLV6;
        str += 8;
#endif /* LWESP_CFG_IPV6 */
    } else {
        return 0;
    }
    esp.m.link_conn.is_server = lwespi_parse_number(&str);
    lwespi_parse_ip(&str, &esp.m.link_conn.remote_ip);
    esp.m.link_conn.remote_port = lwespi_parse_port(&str);
    esp.m.link_conn.local_port = lwespi_parse_port(&str);
    return 1;
}

#if LWESP_CFG_MODE_STATION || __DOXYGEN__
/**
 * \brief           Parse received message for list access points
 * \param[in]       str: Pointer to input string starting with +CWLAP
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cwlap(const char* str, lwesp_msg_t* msg) {
    if (!CMD_IS_DEF(LWESP_CMD_WIFI_CWLAP) || /* Do we have valid message here and enough memory to save everything? */
        msg->msg.ap_list.aps == NULL || msg->msg.ap_list.apsi >= msg->msg.ap_list.apsl) {
        return 0;
    }
    if (*str == '+') { /* Does string contain '+' as first character */
        str += 7;      /* Skip this part */
    }
    if (*str != '(') { /* We must start with opening bracket */
        return 0;
    }
    ++str;

    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ecn = (lwesp_ecn_t)lwespi_parse_number(&str);
    lwespi_parse_string(&str, msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ssid,
                        sizeof(msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ssid), 1);
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].rssi = (int16_t)lwespi_parse_number(&str);
    lwespi_parse_mac(&str, &msg->msg.ap_list.aps[msg->msg.ap_list.apsi].mac);
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].ch = (uint8_t)lwespi_parse_number(&str);
#if LWESP_CFG_ACCESS_POINT_STRUCT_FULL_FIELDS
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].scan_type = (uint8_t)lwespi_parse_number(&str); /* Scan type */
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].scan_time_min =
        (uint16_t)lwespi_parse_number(&str); /* Scan time minimum */
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].scan_time_max =
        (uint16_t)lwespi_parse_number(&str); /* Scan time maximum */
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].freq_offset = (int16_t)lwespi_parse_number(&str); /* Freq offset */
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].freq_cal = (int16_t)lwespi_parse_number(&str);    /* Freqcal value */
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].pairwise_cipher =
        (lwesp_ap_cipher_t)lwespi_parse_number(&str); /* Pairwise cipher */
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].group_cipher =
        (lwesp_ap_cipher_t)lwespi_parse_number(&str); /* Group cipher */
#else
    /* Read and ignore values */
    lwespi_parse_number(&str);
    lwespi_parse_number(&str);
    lwespi_parse_number(&str);
    lwespi_parse_number(&str);
    lwespi_parse_number(&str);
    lwespi_parse_number(&str);
    lwespi_parse_number(&str);
#endif /* !LWESP_CFG_ACCESS_POINT_STRUCT_FULL_FIELDS */
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].bgn = lwespi_parse_number(&str);
    msg->msg.ap_list.aps[msg->msg.ap_list.apsi].wps = lwespi_parse_number(&str);

    /* Go to next entry */
    ++msg->msg.ap_list.apsi;            /* Increase number of found elements */
    if (msg->msg.ap_list.apf != NULL) { /* Set pointer if necessary */
        *msg->msg.ap_list.apf = msg->msg.ap_list.apsi;
    }
    return 1;
}

/**
 * \brief           Parse received message for current AP information
 * \param[in]       str: Pointer to input string starting with +CWJAP
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cwjap(const char* str, lwesp_msg_t* msg) {
    if (!CMD_IS_DEF(
            LWESP_CMD_WIFI_CWJAP_GET)) { /* Do we have valid message here and enough memory to save everything? */
        return 0;
    }
    if (*str == '+') { /* Does string contain '+' as first character */
        str += 7;      /* Skip this part */
    }
    if (*str != '"') { /* We must start with quotation mark */
        return 0;
    }
    lwespi_parse_string(&str, esp.msg->msg.sta_info_ap.info->ssid, LWESP_CFG_MAX_SSID_LENGTH, 1);
    lwespi_parse_mac(&str, &esp.msg->msg.sta_info_ap.info->mac);
    esp.msg->msg.sta_info_ap.info->ch = lwespi_parse_number(&str);
    esp.msg->msg.sta_info_ap.info->rssi = lwespi_parse_number(&str);

    LWESP_UNUSED(msg);

    return 1;
}

#endif /* LWESP_CFG_MODE_STATION || __DOXYGEN__ */

#if LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
/**
 * \brief           Parse received message for list stations
 * \param[in]       str: Pointer to input string starting with +CWLAP
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cwlif(const char* str, lwesp_msg_t* msg) {
    if (!CMD_IS_DEF(LWESP_CMD_WIFI_CWLIF) /* Do we have valid message here and enough memory to save everything? */
        || msg->msg.sta_list.stas == NULL || msg->msg.sta_list.stai >= msg->msg.sta_list.stal) {
        return 0;
    }

    if (*str == '+') {
        str += 7;
    }

    lwespi_parse_ip(&str, &msg->msg.sta_list.stas[msg->msg.sta_list.stai].ip);
    lwespi_parse_mac(&str, &msg->msg.sta_list.stas[msg->msg.sta_list.stai].mac);

    ++msg->msg.sta_list.stai;             /* Increase number of found elements */
    if (msg->msg.sta_list.staf != NULL) { /* Set pointer if necessary */
        *msg->msg.sta_list.staf = msg->msg.sta_list.stai;
    }
    return 1;
}

/**
 * \brief           Parse MAC address and send to user layer
 * \param[in]       str: Input string excluding `+DIST_STA_IP:` part
 * \param[in]       is_conn: Set to `1` if station connected or `0` if station disconnected
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_ap_conn_disconn_sta(const char* str, uint8_t is_conn) {
    lwesp_mac_t mac;

    lwespi_parse_mac(&str, &mac); /* Parse MAC address */

    esp.evt.evt.ap_conn_disconn_sta.mac = &mac;
    lwespi_send_cb(is_conn ? LWESP_EVT_AP_CONNECTED_STA : LWESP_EVT_AP_DISCONNECTED_STA); /* Send event function */
    return 1;
}

/**
 * \brief           Parse received string "+DIST_STA_IP" and send notification to user layer
 * \param[in]       str: Input string excluding "+DIST_STA_IP:" part
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_ap_ip_sta(const char* str) {
    lwesp_mac_t mac;
    lwesp_ip_t ip;

    lwespi_parse_mac(&str, &mac); /* Parse MAC address */
    lwespi_parse_ip(&str, &ip);   /* Parse IP address */

    esp.evt.evt.ap_ip_sta.mac = &mac;
    esp.evt.evt.ap_ip_sta.ip = &ip;
    lwespi_send_cb(LWESP_EVT_AP_IP_STA); /* Send event function */
    return 1;
}

/**
 * \brief           Parse received message for current AP information
 * \param[in]       str: Pointer to input string starting with +CWSAP
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cwsap(const char* str, lwesp_msg_t* msg) {
    if (!CMD_IS_DEF(
            LWESP_CMD_WIFI_CWSAP_GET)) { /* Do we have valid message here and enough memory to save everything? */
        return 0;
    }
    if (*str == '+') { /* Does string contain '+' as first character */
        str += 7;      /* Skip this part */
    }
    if (*str != '"') { /* We must start with quotation mark */
        return 0;
    }
    lwespi_parse_string(&str, esp.msg->msg.ap_conf_get.ap_conf->ssid, LWESP_CFG_MAX_SSID_LENGTH, 1);
    lwespi_parse_string(&str, esp.msg->msg.ap_conf_get.ap_conf->pwd, LWESP_CFG_MAX_PWD_LENGTH, 1);
    esp.msg->msg.ap_conf_get.ap_conf->ch = lwespi_parse_number(&str);
    esp.msg->msg.ap_conf_get.ap_conf->ecn = lwespi_parse_number(&str);
    esp.msg->msg.ap_conf_get.ap_conf->max_cons = lwespi_parse_number(&str);
    esp.msg->msg.ap_conf_get.ap_conf->hidden = lwespi_parse_number(&str);

    LWESP_UNUSED(msg);

    return 1;
}
#endif /* LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */

#if LWESP_CFG_PING || __DOXYGEN__

/**
 * \brief           Parse received time for ping
 * \param[in]       str: Pointer to input string starting with +time
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_ping_time(const char* str, lwesp_msg_t* msg) {
    if (!CMD_IS_DEF(LWESP_CMD_TCPIP_PING)) {
        return 0;
    }
    if (*str == '+') {
        str += 6;
    }
    msg->msg.tcpip_ping.time = lwespi_parse_number(&str);
    if (msg->msg.tcpip_ping.time_out != NULL) {
        *msg->msg.tcpip_ping.time_out = msg->msg.tcpip_ping.time;
    }
    return 1;
}

#endif /* LWESP_CFG_PING || __DOXYGEN__ */

#if LWESP_CFG_DNS || __DOXYGEN__

/**
 * \brief           Parse received message domain DNS name
 * \param[in]       str: Pointer to input string starting with +CWLAP
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cipdomain(const char* str, lwesp_msg_t* msg) {
    if (!CMD_IS_DEF(LWESP_CMD_TCPIP_CIPDOMAIN)) {
        return 0;
    }
    if (*str == '+') {
        str += 11;
    }
    lwespi_parse_ip(&str, msg->msg.dns_getbyhostname.ip); /* Parse IP address */
    return 1;
}

#endif /* LWESP_CFG_DNS || __DOXYGEN__ */

#if LWESP_CFG_SNTP || __DOXYGEN__

/**
 * \brief           Parse received message for SNTP configuration
 * \param[in]       str: Pointer to input string starting with +CIPSNTPCFG
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_sntp_cfg(const char* str, lwesp_msg_t* msg) {
    int32_t num;
    if (!CMD_IS_DEF(LWESP_CMD_TCPIP_CIPSNTPCFG_GET)) {
        return 0;
    }
    if (*str == '+') { /* Check input string */
        str += 12;
    }
    num = lwespi_parse_number(&str);
    if (msg->msg.tcpip_sntp_cfg_get.en != NULL) {
        *msg->msg.tcpip_sntp_cfg_get.en = num;
    }
    num = lwespi_parse_number(&str);
    if (msg->msg.tcpip_sntp_cfg_get.tz != NULL) {
        *msg->msg.tcpip_sntp_cfg_get.tz = (int16_t)num;
    }
    /* TODO: Parse hostnames... */
    return 1;
}

/**
 * \brief           Parse received message for SNTP sync interval
 * \param[in]       str: Pointer to input string starting with +CIPSNTPINTV
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cipsntpintv(const char* str, lwesp_msg_t* msg) {
    int32_t num;
    if (!CMD_IS_DEF(LWESP_CMD_TCPIP_CIPSNTPINTV_GET)) {
        return 0;
    }
    if (*str == '+') { /* Check input string */
        str += 13;
    }
    num = lwespi_parse_number(&str);
    if (msg->msg.tcpip_sntp_intv_get.interval != NULL) {
        *msg->msg.tcpip_sntp_intv_get.interval = (uint32_t)num;
    }
    return 1;
}

/**
 * \brief           Parse received message for SNTP time
 * \param[in]       str: Pointer to input string starting with +CIPSNTPTIME
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cipsntptime(const char* str, lwesp_msg_t* msg) {
    const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    if (!CMD_IS_DEF(LWESP_CMD_TCPIP_CIPSNTPTIME)) {
        return 0;
    }
    if (*str == '+') { /* Check input string */
        str += 13;
    }

    /* Scan for day in a week */
    for (size_t i = 0; i < LWESP_ARRAYSIZE(days); ++i) {
        if (!strncmp(str, days[i], 3)) {
            msg->msg.tcpip_sntp_time.dt->tm_mday = (int)i;
            break;
        }
    }
    str += 4;

    /* Scan for month in a year */
    for (size_t i = 0; i < LWESP_ARRAYSIZE(months); ++i) {
        if (!strncmp(str, months[i], 3)) {
            msg->msg.tcpip_sntp_time.dt->tm_mon = (int)i;
            break;
        }
    }
    str += 4;
    if (*str == ' ') { /* Numbers < 10 could have one more space */
        ++str;
    }
    msg->msg.tcpip_sntp_time.dt->tm_mday = lwespi_parse_number(&str);
    ++str;
    msg->msg.tcpip_sntp_time.dt->tm_hour = lwespi_parse_number(&str);
    ++str;
    msg->msg.tcpip_sntp_time.dt->tm_min = lwespi_parse_number(&str);
    ++str;
    msg->msg.tcpip_sntp_time.dt->tm_sec = lwespi_parse_number(&str);
    ++str;
    msg->msg.tcpip_sntp_time.dt->tm_year = lwespi_parse_number(&str) - 1900;
    return 1;
}

#endif /* LWESP_CFG_SNTP || __DOXYGEN__ */

#if LWESP_CFG_HOSTNAME || __DOXYGEN__

/**
 * \brief           Parse received message for HOSTNAME
 * \param[in]       str: Pointer to input string starting with +CWHOSTNAME
 * \param[in]       msg: Pointer to message
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_hostname(const char* str, lwesp_msg_t* msg) {
    size_t i;
    if (!CMD_IS_DEF(LWESP_CMD_WIFI_CWHOSTNAME_GET)) {
        return 0;
    }
    if (*str == '+') { /* Check input string */
        str += 12;
    }
    msg->msg.wifi_hostname.hostname_get[0] = 0;
    if (*str != '\r') {
        i = 0;
        for (; i < (msg->msg.wifi_hostname.length - 1) && *str && *str != '\r'; ++i, ++str) {
            msg->msg.wifi_hostname.hostname_get[i] = *str;
        }
        msg->msg.wifi_hostname.hostname_get[i] = 0;
    }
    return 1;
}

#endif /* LWESP_CFG_HOSTNAME || __DOXYGEN__ */

/**
 * \brief           Parse received message for DHCP
 * \param[in]       str: Pointer to input string starting with +CWDHCP
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwespi_parse_cwdhcp(const char* str) {
    uint8_t val;

    if (!CMD_IS_CUR(LWESP_CMD_WIFI_CWDHCP_GET)) {
        return 0;
    }
    if (*str == '+') {
        str += 8;
    }

    val = lwespi_parse_number(&str);

#if LWESP_CFG_MODE_ACCESS_POINT
    esp.m.ap.dhcp = (val & 0x01) == 0x01;
#endif /* LWESP_CFG_MODE_ACCESS_POINT */
#if LWESP_CFG_MODE_STATION
    esp.m.sta.dhcp = (val & 0x02) == 0x02;
#endif /* LWESP_CFG_MODE_STATION */

    return 1;
}

#if LWESP_CFG_WEBSERVER || __DOXYGEN__

/**
 * \brief           Parse +WEBSERVER response from ESP device
 * \param[in]       str: Input string to parse
 * \return          Member of \ref lwespr_t enumeration
 */
uint8_t
lwespi_parse_webserver(const char* str) {

    esp.evt.evt.ws_status.code = lwespi_parse_number(&str);
    lwespi_send_cb(LWESP_EVT_WEBSERVER); /* Send event function */
    return 1;
}

#endif /* LWESP_CFG_WEBSERVER || __DOXYGEN__ */
