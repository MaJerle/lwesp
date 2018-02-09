/**
 * \file            esp_int.c
 * \brief           Internal functions
 */

/*
 * Copyright (c) 2018 Tilen Majerle
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
 * This file is part of ESP-AT.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#include "esp/esp_private.h"
#include "esp/esp.h"
#include "esp/esp_int.h"
#include "esp/esp_mem.h"
#include "esp/esp_parser.h"
#include "esp/esp_unicode.h"
#include "system/esp_ll.h"

#define IS_CURR_CMD(c)        (esp.msg != NULL && esp.msg->cmd == (c))

#if !__DOXYGEN__
typedef struct {
    char data[128];
    uint8_t len;
} esp_recv_t;
static esp_recv_t recv_buff;
#endif /* !__DOXYGEN__ */

#define CRLF                "\r\n"

#define RECV_ADD(ch)        do { recv_buff.data[recv_buff.len++] = ch; recv_buff.data[recv_buff.len] = 0; } while (0)
#define RECV_RESET()        do { recv_buff.len = 0; recv_buff.data[0] = 0; } while (0)
#define RECV_LEN()          recv_buff.len
#define RECV_IDX(index)     recv_buff.data[index]

#define ESP_AT_PORT_SEND_STR(str)       esp.ll.send_fn((const uint8_t *)(str), (uint16_t)strlen(str))
#define ESP_AT_PORT_SEND_CHR(str)       esp.ll.send_fn((const uint8_t *)(str), (uint16_t)1)
#define ESP_AT_PORT_SEND(d, l)          esp.ll.send_fn((const uint8_t *)(d), (uint16_t)l)

#define ESP_PORT2NUM(port)              ((uint32_t)(port))

static espr_t espi_process_sub_cmd(esp_msg_t* msg, uint8_t is_ok, uint8_t is_error, uint8_t is_ready);

/**
 * \brief           Free connection send data memory
 * \param[in]       m: Send data message type
 */
#define CONN_SEND_DATA_FREE(m)    do {              \
    if ((m) != NULL && (m)->msg.conn_send.fau) {    \
        (m)->msg.conn_send.fau = 0;                 \
        ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE, "CONN: Free write buffer fau: %p\r\n", (void *)(m)->msg.conn_send.data);   \
        esp_mem_free((void *)(m)->msg.conn_send.data);    \
    }                                               \
} while (0)

/**
 * \brief           Create 2-characters long hex from byte
 * \param[in]       num: Number to convert to string
 * \param[out]      str: Pointer to string to save result to
 */
static void
byte_to_str(uint8_t num, char* str) {
    sprintf(str, "%02X", (unsigned)num);        /* Currently use sprintf only */
}

/**
 * \brief           Create string from number
 * \param[in]       num: Number to convert to string
 * \param[out]      str: Pointer to string to save result to
 */
static void
number_to_str(uint32_t num, char* str) {
    sprintf(str, "%u", (unsigned)num);          /* Currently use sprintf only */
}

/**
 * \brief           Create string from signed number
 * \param[in]       num: Number to convert to string
 * \param[out]      str: Pointer to string to save result to
 */
static void
signed_number_to_str(int32_t num, char* str) {
    sprintf(str, "%d", (signed)num);            /* Currently use sprintf only */
}

/**
 * \brief           Send IP or MAC address to AT port
 * \param[in]       d: Pointer to IP or MAC address
 * \param[in]       is_ip: Set to `1` when sending IP, `0` when MAC
 * \param[in]       q: Set to `1` to include start and ending quotes
 */
static void
send_ip_mac(const void* d, uint8_t is_ip, uint8_t q) {
    uint8_t i, ch;
    char str[4];
    const esp_mac_t* mac = d;
    const esp_ip_t* ip = d;
    
    if (d == NULL) {
        return;
    }
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");             /* Send starting quote character */
    }
    ch = is_ip ? '.' : ':';                     /* Get delimiter character */
    for (i = 0; i < (is_ip ? 4 : 6); i++) {     /* Process byte by byte */
        if (is_ip) {                            /* In case of IP ... */
            number_to_str(ip->ip[i], str);      /* ... go to decimal format ... */
        } else {                                /* ... in case of MAC ... */
            byte_to_str(mac->mac[i], str);      /* ... go to HEX format */
        }
        ESP_AT_PORT_SEND_STR(str);              /* Send str */
        if (i < (is_ip ? 4 : 6) - 1) {          /* Check end if characters */
            ESP_AT_PORT_SEND_CHR(&ch);          /* Send character */
        }
    }
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");             /* Send ending quote character */
    }
}

/**
 * \brief           Send string to AT port, either plain or escaped
 * \param[in]       str: Pointer to input string to string
 * \param[in]       e: Value to indicate string send format, escaped (1) or plain (0)
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (1) or disabled (0)
 */
static void
send_string(const char* str, uint8_t e, uint8_t q) {
    char special = '\\';
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
    if (str != NULL) {
        if (e) {                                /* Do we have to escape string? */
            while (*str) {                      /* Go through string */
                if (*str == ',' || *str == '"' || *str == '\\') {   /* Check for special character */    
                    ESP_AT_PORT_SEND_CHR(&special); /* Send special character */
                }
                ESP_AT_PORT_SEND_CHR(str);      /* Send character */
                str++;
            }
        } else {
            ESP_AT_PORT_SEND_STR(str);          /* Send plain string */
        }
    }
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
}

/**
 * \brief           Send number (decimal) to AT port
 * \param[in]       num: Number to send to AT port
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (1) or disabled (0)
 */
static void
send_number(uint32_t num, uint8_t q) {
    char str[11];

    number_to_str(num, str);                    /* Convert digit to decimal string */
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
    ESP_AT_PORT_SEND_STR(str);                  /* Send string with number */
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
}

/**
 * \brief           Send port number to AT port
 * \param[in]       port: Port number to send
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (1) or disabled (0)
 */
static void
send_port(esp_port_t port, uint8_t q) {
    char str[6];

    number_to_str(ESP_PORT2NUM(port), str);     /* Convert digit to decimal string */
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
    ESP_AT_PORT_SEND_STR(str);                  /* Send string with number */
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
}

/**
 * \brief           Send signed number to AT port
 * \param[in]       num: Number to send to AT port
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (1) or disabled (0)
 */
void
send_signed_number(int32_t num, uint8_t q) {
    char str[11];
    
    signed_number_to_str(num, str);             /* Convert digit to decimal string */
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
    ESP_AT_PORT_SEND_STR(str);                  /* Send string with number */
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");
    }
}

/**
 * \brief           Reset all connections
 * \note            Used to notify upper layer stack to close everything and reset the memory if necessary
 * \param[in]       forced: Flag indicating reset was forced by user
 */
static void
reset_connections(uint8_t forced) {
    size_t i;
    
    esp.cb.type = ESP_CB_CONN_CLOSED;
    esp.cb.cb.conn_active_closed.forced = forced;
    
    for (i = 0; i < ESP_CFG_MAX_CONNS; i++) {   /* Check all connections */
        if (esp.conns[i].status.f.active) {
            esp.conns[i].status.f.active = 0;
            
            esp.cb.cb.conn_active_closed.conn = &esp.conns[i];
            esp.cb.cb.conn_active_closed.client = esp.conns[i].status.f.client;
            espi_send_conn_cb(&esp.conns[i], NULL); /* Send callback function */
        }
    }
}

/**
 * \brief           Reset everything after reset was detected
 * \param[in]       forced: Set to `1` if reset forced by user
 */
static void
reset_everything(uint8_t forced) {
    /**
     * \todo: Put stack to default state:
     *          - Close all the connection in memory
     *          - Clear entire data memory
     *          - Reset esp structure with IP
     *          - Start over init procedure
     */
    
    /*
     * Step 1: Close all connections in memory 
     */
    reset_connections(forced);
    
    esp.status.f.r_got_ip = 0;
    esp.status.f.r_w_conn = 0;

    if (!forced) {
        esp_reset(0);
    }
}

/**
 * \brief           Check if received string includes "_CUR" or "_DEF" as current or default setup
 * \param[in]       str: Pointer to string to test
 * \return          1 if current setting, 0 otherwise
 */
static uint8_t
is_received_current_setting(const char* str) {
    return !strstr(str, "_DEF");                /* In case there is no "_DEF", we have current setting active */
}

/**
 * \brief           Process callback function to user with specific type
 * \param[in]       type: Callback event type
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_send_cb(esp_cb_type_t type) {
    esp_cb_func_t* link;
    esp.cb.type = type;                         /* Set callback type to process */
    
    /*
     * Call callback function for all registered functions
     */
    for (link = esp.cb_func; link != NULL; link = link->next) {
        link->fn(&esp.cb);
    }
    return espOK;
}

/**
 * \brief           Process connection callback
 * \note            Before calling function, callback structure must be prepared
 * \param[in]       conn: Pointer to connection to use as callback
 * \param[in]       cb: Optional function used to call callback. If NULL, connection callback will be used if set
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_send_conn_cb(esp_conn_t* conn, esp_cb_fn cb) {
    if (conn->status.f.in_closing && esp.cb.type != ESP_CB_CONN_CLOSED) {   /* Do not continue if in closing mode */
        return espOK;
    }
    
    if (cb != NULL) {                           /* Try with user connection */
        return cb(&esp.cb);                     /* Call temporary function */
    } else if (conn != NULL && conn->cb_func != NULL) { /* Connection custom callback? */
        return conn->cb_func(&esp.cb);          /* Process callback function */
    } else if (conn == NULL) {
        return espOK;
    }
    
    /*
     * On normal API operation,
     * this part of code should never be entered!
     */
    
    /*
     * If connection doesn't have callback function
     * automatically close the connection?
     *
     * Since function call is non-blocking,
     * it will set active connection to closing mode
     * and further callback events should not be executed anymore
     */
    return esp_conn_close(conn, 0);
}

/**
 * \brief           Process and send data from device buffer
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
espi_tcpip_process_send_data(void) {
    if (!esp_conn_is_active(esp.msg->msg.conn_send.conn) || /* Is the connection already closed? */
        esp.msg->msg.conn_send.val_id != esp.msg->msg.conn_send.conn->val_id    /* Did validation ID change after we set parameter? */
    ) {
        CONN_SEND_DATA_FREE(esp.msg);           /* Free message data */
        return espERR;
    }
    ESP_AT_PORT_SEND_STR("AT+CIPSEND=");
    send_number(ESP_U32(esp.msg->msg.conn_send.conn->num), 0);
    ESP_AT_PORT_SEND_STR(",");
    esp.msg->msg.conn_send.sent = ESP_MIN(esp.msg->msg.conn_send.btw, ESP_CFG_CONN_MAX_DATA_LEN);
    send_number(ESP_U32(esp.msg->msg.conn_send.sent), 0);   /* Send length number */
    
    /*
     * On UDP connections, IP address and port may be selected
     */
    if (esp.msg->msg.conn_send.conn->type == ESP_CONN_TYPE_UDP) {        
        if (esp.msg->msg.conn_send.remote_ip != NULL && esp.msg->msg.conn_send.remote_port) {
            ESP_AT_PORT_SEND_STR(",");
            send_ip_mac(esp.msg->msg.conn_send.remote_ip, 1, 1);    /* Send IP address including quotes */
            ESP_AT_PORT_SEND_STR(",");
            send_port(esp.msg->msg.conn_send.remote_port, 0);   /* Send length number */
        }
    }
    ESP_AT_PORT_SEND_STR(CRLF);
    return espOK;
}

/**
 * \brief           Process data sent and send remaining
 * \param[in]       sent: Status whether data were sent or not, info received from ESP with "SEND OK" or "SEND FAIL" 
 * \return          1 in case we should stop sending or 0 if we still have data to process
 */
static uint8_t
espi_tcpip_process_data_sent(uint8_t sent) {
    if (sent) {                                 /* Data were successfully sent */
        esp.msg->msg.conn_send.sent_all += esp.msg->msg.conn_send.sent;
        esp.msg->msg.conn_send.btw -= esp.msg->msg.conn_send.sent;
        esp.msg->msg.conn_send.ptr += esp.msg->msg.conn_send.sent;
        if (esp.msg->msg.conn_send.bw) {
            *esp.msg->msg.conn_send.bw += esp.msg->msg.conn_send.sent;
        }
        esp.msg->msg.conn_send.tries = 0;
    } else {                                    /* We were not successful */
        esp.msg->msg.conn_send.tries++;         /* Increase number of tries */
        if (esp.msg->msg.conn_send.tries == ESP_CFG_MAX_SEND_RETRIES) { /* In case we reached max number of retransmissions */
            return 1;                           /* Return 1 and indicate error */
        }
    }
    if (esp.msg->msg.conn_send.btw) {           /* Do we still have data to send? */
        if (espi_tcpip_process_send_data() != espOK) {  /* Check if we can continue */
            return 1;                           /* Finish at this point */
        }
        return 0;                               /* We still have data to send */
    }
    return 1;                                   /* Everything was sent, we can stop execution */
}

/**
 * \brief           Send error event to application layer
 * \param[in]       msg: Message from user with connection start
 * \param[in]       error: Value indicating cause of error
 */
static void
espi_send_conn_error_cb(esp_msg_t* msg, espr_t error) {
    esp_conn_t* conn = &esp.conns[esp.msg->msg.conn_start.num];
    esp.cb.type = ESP_CB_CONN_ERROR;            /* Connection error */
    esp.cb.cb.conn_error.host = esp.msg->msg.conn_start.host;
    esp.cb.cb.conn_error.port = esp.msg->msg.conn_start.port;
    esp.cb.cb.conn_error.type = esp.msg->msg.conn_start.type;
    esp.cb.cb.conn_error.arg = esp.msg->msg.conn_start.arg;
    esp.cb.cb.conn_error.err = error;
    espi_send_conn_cb(conn, esp.msg->msg.conn_start.cb_func);   /* Send event */
}

/**
 * \brief           Process received string from ESP 
 * \param[in]       recv: Pointer to \ref esp_rect_t structure with input string
 */
static void
espi_parse_received(esp_recv_t* rcv) {
    uint8_t is_ok = 0, is_error = 0, is_ready = 0;
    const char* s;

    /* Try to remove non-parsable strings */
    if ((rcv->len == 2 && rcv->data[0] == '\r' && rcv->data[1] == '\n')
        /*
         * Condition below can only be used if AT echo is disabled
         * otherwise it may happen that some message is inserted in between AT command echo, such as:
         *
         * AT+CIPCLOSE=0+LINK_CONN:0,2,"TCP",1,"192.168.0.14",57551,80\r\n\r\n
         *
         * Instead of:
         * AT+CIPCLOSE=0\r\n
         * +LINK_CONN:0,2,"TCP",1,"192.168.0.14",57551,80\r\n
         */
        /* || (rcv->len > 3 && rcv->data[0] == 'A' && rcv->data[1] == 'T' && rcv->data[2] == '+') */) {
        return;
    }
    
    /* Detect most common responses from device */
    is_ok = !strcmp(rcv->data, "OK" CRLF);      /* Check if received string is OK */
    if (!is_ok) {
        is_error = !strcmp(rcv->data, "ERROR" CRLF) || !strcmp(rcv->data, "FAIL" CRLF); /* Check if received string is error */
        if (!is_error) {
            is_ready = !strcmp(rcv->data, "ready" CRLF);    /* Check if received string is ready */
        }
    }
    
    /*
     * In case ready is received, there was a reset on device,
     * either forced by us or problem on device itself
     */
    if (is_ready) {
        if (IS_CURR_CMD(ESP_CMD_RESET)) {       /* Did we force reset? */
            esp.cb.cb.reset.forced = 1;
        } else {                                /* Reset due unknown error */
            esp.cb.cb.reset.forced = 0;
        }
        reset_everything(esp.cb.cb.reset.forced);   /* Put everything to default state */
        espi_send_cb(ESP_CB_RESET);             /* Call user callback function */
    }
    
    /*
     * Read and process statements starting with '+' character
     */
    if (rcv->data[0] == '+') {
        if (!strncmp("+IPD", rcv->data, 4)) {   /* Check received network data */
            espi_parse_ipd(rcv->data + 5);      /* Parse IPD statement and start receiving network data */
#if ESP_CFG_MODE_ACCESS_POINT
        } else if (!strncmp(rcv->data, "+STA_CONNECTED", 14)) {
            espi_parse_ap_conn_disconn_sta(&rcv->data[15], 1);  /* Parse string and send to user layer */
        } else if (!strncmp(rcv->data, "+STA_DISCONNECTED", 17)) {
            espi_parse_ap_conn_disconn_sta(&rcv->data[18], 1);  /* Parse string and send to user layer */
        } else if (!strncmp(rcv->data, "+DIST_STA_IP", 12)) {
            espi_parse_ap_ip_sta(&rcv->data[13]);   /* Parse string and send to user layer */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        } else if (esp.msg != NULL) {
            if (
#if ESP_CFG_MODE_STATION
                (IS_CURR_CMD(ESP_CMD_WIFI_CIPSTAMAC_GET) && !strncmp(rcv->data, "+CIPSTAMAC", 10))
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_STATION_ACCESS_POINT
                    || 
#endif /* ESP_CFG_MODE_STATION_ACCESS_POINT */
#if ESP_CFG_MODE_ACCESS_POINT
                (IS_CURR_CMD(ESP_CMD_WIFI_CIPAPMAC_GET) && !strncmp(rcv->data, "+CIPAPMAC", 9))
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            ) {
                const char* tmp;
                esp_mac_t mac;
                
                if (rcv->data[9] == '_') {      /* Do we have "_CUR" or "_DEF" included? */
                    tmp = &rcv->data[14];
                } else if (rcv->data[10] == '_') {
                    tmp = &rcv->data[15];
                } else if (rcv->data[9] == ':') {
                    tmp = &rcv->data[10];
                } else if (rcv->data[10] == ':') {
                    tmp = &rcv->data[11];
                }
                
                espi_parse_mac(&tmp, &mac);     /* Save as current MAC address */
                if (is_received_current_setting(rcv->data)) {
#if ESP_CFG_MODE_STATION_ACCESS_POINT
                    memcpy(esp.msg->cmd == ESP_CMD_WIFI_CIPSTAMAC_GET ? &esp.sta.mac : &esp.ap.mac, &mac, 6);   /* Copy to current setup */
#elif ESP_CFG_MODE_STATION
                    memcpy(&esp.sta.mac, &mac, 6);  /* Copy to current setup */
#else
                    memcpy(&esp.ap.mac, &mac, 6);   /* Copy to current setup */
#endif /* ESP_CFG_MODE_STATION_ACCESS_POINT */
                }
                if (esp.msg->msg.sta_ap_getmac.mac != NULL && esp.msg->cmd == esp.msg->cmd_def) {
                    memcpy(esp.msg->msg.sta_ap_getmac.mac, &mac, sizeof(mac));  /* Copy to current setup */
                }
            } else if (
#if ESP_CFG_MODE_STATION
                (IS_CURR_CMD(ESP_CMD_WIFI_CIPSTA_GET) && !strncmp(rcv->data, "+CIPSTA", 7))
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_STATION_ACCESS_POINT
                    || 
#endif /* ESP_CFG_MODE_STATION_ACCESS_POINT */
#if ESP_CFG_MODE_ACCESS_POINT
                (IS_CURR_CMD(ESP_CMD_WIFI_CIPAP_GET) && !strncmp(rcv->data, "+CIPAP", 6))
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            ) {
                const char* tmp = NULL;
                esp_ip_t ip, *a = NULL, *b = NULL;
                esp_ip_mac_t* im;
                uint8_t ch = 0;
                         
#if ESP_CFG_MODE_STATION_ACCESS_POINT              
                im = (esp.msg->cmd == ESP_CMD_WIFI_CIPSTA_GET) ? &esp.sta : &esp.ap;    /* Get IP and MAC structure first */
#elif ESP_CFG_MODE_STATION
                im = &esp.sta;                  /* Get IP and MAC structure first */
#else
                im = &esp.ap;                   /* Get IP and MAC structure first */
#endif /* ESP_CFG_MODE_STATION_ACCESS_POINT */
                
                /* We expect "+CIPSTA_CUR:" or "+CIPSTA_DEF:" or "+CIPAP_CUR:" or "+CIPAP_DEF:" or "+CIPSTA:" or "+CIPAP:" ... */
                if (rcv->data[6] == '_') {
                    ch = rcv->data[11];
                } else if (rcv->data[7] == '_') {
                    ch = rcv->data[12];
                } else if (rcv->data[6] == ':') {
                    ch = rcv->data[7];
                } else if (rcv->data[7] == ':') {
                    ch = rcv->data[8];
                }
                switch (ch) {
                    case 'i': tmp = &rcv->data[10]; a = &im->ip; b = esp.msg->msg.sta_ap_getip.ip; break;
                    case 'g': tmp = &rcv->data[15]; a = &im->gw; b = esp.msg->msg.sta_ap_getip.gw; break;
                    case 'n': tmp = &rcv->data[15]; a = &im->nm; b = esp.msg->msg.sta_ap_getip.nm; break;
                    default: tmp = NULL; a = NULL; b = NULL; break;
                }
                if (tmp != NULL) {              /* Do we have temporary string? */
                    if (rcv->data[6] == '_' || rcv->data[7] == '_') {   /* Do we have "_CUR" or "_DEF" included? */
                        tmp += 4;               /* Skip it */
                    }
                    if (*tmp == ':') {
                        tmp++;
                    }
                    espi_parse_ip(&tmp, &ip);   /* Parse IP address */
                    if (is_received_current_setting(rcv->data)) {
                        memcpy(a, &ip, sizeof(ip)); /* Copy to current setup */
                    }
                    if (b != NULL && IS_CURR_CMD(esp.msg->cmd_def)) {   /* Is current command the same as default one? */
                        memcpy(b, &ip, sizeof(ip)); /* Copy to user variable */
                    }
                }
#if ESP_CFG_MODE_STATION
            } else if (IS_CURR_CMD(ESP_CMD_WIFI_CWLAP) && !strncmp(rcv->data, "+CWLAP", 6)) {
                espi_parse_cwlap(rcv->data, esp.msg);   /* Parse CWLAP entry */
            } else if (IS_CURR_CMD(ESP_CMD_WIFI_CWJAP) && !strncmp(rcv->data, "+CWJAP", 6)) {
                const char* tmp = &rcv->data[7];/* Go to the number position */
                esp.msg->msg.sta_join.error_num = (uint8_t)espi_parse_number(&tmp);
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_DNS
            } else if (esp.msg->cmd == ESP_CMD_TCPIP_CIPDOMAIN && !strncmp(rcv->data, "+CIPDOMAIN", 10)) {
                espi_parse_cipdomain(rcv->data, esp.msg);   /* Parse CIPDOMAIN entry */
#endif /* ESP_CFG_DNS */
#if ESP_CFG_PING
            } else if (esp.msg->cmd == ESP_CMD_TCPIP_PING && ESP_CHARISNUM(rcv->data[1])) {
                const char* tmp = &rcv->data[1];
                *esp.msg->msg.tcpip_ping.time = espi_parse_number(&tmp);
#endif /* ESP_CFG_PING */
#if ESP_CFG_SNTP
            } else if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSNTPTIME) && !strncmp(rcv->data, "+CIPSNTPTIME", 12)) {
                espi_parse_cipsntptime(rcv->data, esp.msg); /* Parse CIPSNTPTIME entry */
#endif /* ESP_CFG_SNTP */
#if ESP_CFG_HOSTNAME
            } else if (IS_CURR_CMD(ESP_CMD_WIFI_CWHOSTNAME_GET) && !strncmp(rcv->data, "+CWHOSTNAME", 11)) {
                espi_parse_hostname(rcv->data, esp.msg);    /* Parse HOSTNAME entry */
#endif /* ESP_CFG_HOSTNAME */
            }
        }
#if ESP_CFG_MODE_STATION
    } else if (!strncmp(rcv->data, "WIFI", 4)) {
        if (!strncmp(&rcv->data[5], "CONNECTED", 9)) {
            esp.status.f.r_w_conn = 1;          /* Wifi is connected */
            espi_send_cb(ESP_CB_WIFI_CONNECTED);/* Call user callback function */
        } else if (!strncmp(&rcv->data[5], "DISCONNECT", 10)) {
            esp.status.f.r_w_conn = 0;          /* Wifi is disconnected */
            esp.status.f.r_got_ip = 0;          /* There is no valid IP */
            espi_send_cb(ESP_CB_WIFI_DISCONNECTED); /* Call user callback function */
        } else if (!strncmp(&rcv->data[5], "GOT IP", 6)) {
            esp.status.f.r_got_ip = 1;          /* Wifi got IP address */
            espi_send_cb(ESP_CB_WIFI_GOT_IP);   /* Call user callback function */
            if (!IS_CURR_CMD(ESP_CMD_WIFI_CWJAP)) { /* In case of auto connection */
                esp_sta_getip(NULL, NULL, NULL, 0, 0);  /* Get new IP address */
            }
        }
    } else if (IS_CURR_CMD(ESP_CMD_GMR)) {
        if (!strncmp(rcv->data, "AT version", 10)) {
            espi_parse_at_sdk_version(&rcv->data[11], &esp.version_at);
        } else if (!strncmp(rcv->data, "SDK version", 11)) {
            espi_parse_at_sdk_version(&rcv->data[12], &esp.version_sdk);
        }
#endif /* ESP_CFG_MODE_STATION */
    }
    
    /*
     * Start processing received data
     */
    if (esp.msg != NULL) {                      /* Do we have valid message? */
        if (IS_CURR_CMD(ESP_CMD_RESET) && is_ok) {  /* Check for reset command */
            is_ok = 0;                          /* We must wait for "ready", not only "OK" */
        } else if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSTATUS)) {
            if (!strncmp(rcv->data, "+CIPSTATUS", 10)) {
                espi_parse_cipstatus(rcv->data + 11);   /* Parse CIPSTATUS response */
            } else if (is_ok) {
                uint8_t i;
                for (i = 0; i < ESP_CFG_MAX_CONNS; i++) {   /* Set current connection statuses */
                    esp.conns[i].status.f.active = !!(esp.active_conns & (1 << i));
                }
            }
        } else if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSTART)) {
            /*
             * \todo: Request Espressif for numerical response instead of random messages
             */
            /*
            if (!strncmp(rcv->data, "DNS Fail", 8)) {
                
            } else if (!strncmp(rcv->data, "ID ERROR", 8)) {
                
            } else if (!strncmp(rcv->data, "Link type ERROR", 15)) {
                
            }
            */
        } else if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSEND)) {
            if (is_ok) {                        /* Check for OK and clear as we have to check for "> " statement after OK */
                is_ok = 0;                      /* Do not reach on OK */
            }
            if (esp.msg->msg.conn_send.wait_send_ok_err) {
                if (!strncmp("SEND OK", rcv->data, 7)) {    /* Data were sent successfully */
                    esp.msg->msg.conn_send.wait_send_ok_err = 0;
                    is_ok = espi_tcpip_process_data_sent(1);    /* Process as data were sent */
                    if (is_ok && esp.msg->msg.conn_send.conn->status.f.active) {
                        CONN_SEND_DATA_FREE(esp.msg);   /* Free message data */
                        esp.cb.type = ESP_CB_CONN_DATA_SENT;    /* Data were fully sent */
                        esp.cb.cb.conn_data_sent.conn = esp.msg->msg.conn_send.conn;
                        esp.cb.cb.conn_data_sent.sent = esp.msg->msg.conn_send.sent_all;
                        espi_send_conn_cb(esp.msg->msg.conn_send.conn, NULL);   /* Send connection callback */
                    }
                } else if (is_error || !strncmp("SEND FAIL", rcv->data, 9)) {
                    esp.msg->msg.conn_send.wait_send_ok_err = 0;
                    is_error = espi_tcpip_process_data_sent(0); /* Data were not sent due to SEND FAIL or command didn't even start */
                    if (is_error && esp.msg->msg.conn_send.conn->status.f.active) {
                        CONN_SEND_DATA_FREE(esp.msg);   /* Free message data */
                        esp.cb.type = ESP_CB_CONN_DATA_SEND_ERR;/* Error sending data */
                        esp.cb.cb.conn_data_send_err.conn = esp.msg->msg.conn_send.conn;
                        esp.cb.cb.conn_data_send_err.sent = esp.msg->msg.conn_send.sent_all;
                        espi_send_conn_cb(esp.ipd.conn, NULL);  /* Send connection callback */
                    }
                }
            } else if (is_error) {
                CONN_SEND_DATA_FREE(esp.msg);   /* Free message data */
            }
        } else if (IS_CURR_CMD(ESP_CMD_UART)) { /* In case of UART command */
            if (is_ok) {                        /* We have valid OK result */
                esp_ll_init(&esp.ll, esp.msg->msg.uart.baudrate);   /* Set new baudrate */
            }
#if ESP_CFG_MODE_ACCESS_POINT
        } else if (IS_CURR_CMD(ESP_CMD_WIFI_CWLIF) && ESP_CHARISNUM(rcv->data[0])) {
            espi_parse_cwlif(rcv->data, esp.msg);   /* Parse CWLIF entry */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        }
    }
    
    /*
     * Check if connection is just active (or closed):
     *
     * Since new ESP AT release, it is possible to get
     * connection status by using +LINK_CONN message.
     *
     * Check LINK_CONN messages
     */
    if (rcv->len > 20 && (s = strstr(rcv->data, "+LINK_CONN:")) != NULL) {
        if (espi_parse_link_conn(s) && esp.link_conn.num < ESP_CFG_MAX_CONNS) {
            uint8_t id;
            esp_conn_t* conn = &esp.conns[esp.link_conn.num];   /* Get connection pointer */
            if (esp.link_conn.failed && conn->status.f.active) {/* Connection failed and now closed? */
                conn->status.f.active = 0;      /* Connection was just closed */
                
                esp.cb.type = ESP_CB_CONN_CLOSED;   /* Connection just active */
                esp.cb.cb.conn_active_closed.conn = conn;   /* Set connection */
                esp.cb.cb.conn_active_closed.client = conn->status.f.client;    /* Set if it is client or not */
                /** @todo: Check if we really tried to close connection which was just closed */
                esp.cb.cb.conn_active_closed.forced = IS_CURR_CMD(ESP_CMD_TCPIP_CIPCLOSE);  /* Set if action was forced = current action = close connection */
                espi_send_conn_cb(conn, NULL);  /* Send event */
            
                /* Check if write buffer is set */
                if (conn->buff != NULL) {
                    ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE, "CONN: Free write buffer: %p\r\n", conn->buff);
                    esp_mem_free(conn->buff);   /* Free the memory */
                    conn->buff = NULL;
                }
            } else if (!esp.link_conn.failed && !conn->status.f.active) {
                id = conn->val_id;
                memset(conn, 0x00, sizeof(*conn));  /* Reset connection parameters */
                conn->num = esp.link_conn.num;  /* Set connection number */
                conn->status.f.active = !esp.link_conn.failed;  /* Check if connection active */
                conn->val_id = ++id;            /* Set new validation ID */
                
                conn->type = esp.link_conn.type;/* Set connection type */
                memcpy(&conn->remote_ip, &esp.link_conn.remote_ip, sizeof(conn->remote_ip));
                conn->remote_port = esp.link_conn.remote_port;
                conn->local_port = esp.link_conn.local_port;
                conn->status.f.client = !esp.link_conn.is_server;
                
                if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSTART)
                    && esp.link_conn.num == esp.msg->msg.conn_start.num
                    && conn->status.f.client) { /* Did we start connection on our own and connection is client? */
                    conn->status.f.client = 1;  /* Go to client mode */
                    conn->cb_func = esp.msg->msg.conn_start.cb_func;    /* Set callback function */
                    conn->arg = esp.msg->msg.conn_start.arg;    /* Set argument for function */
                } else {                        /* Server connection start */
                    conn->cb_func = esp.cb_server;  /* Set server default callback */
                    conn->arg = NULL;
                    conn->type = ESP_CONN_TYPE_TCP; /* Set connection type to TCP. @todo: Wait for ESP team to upgrade AT commands to set other type */
                }
                
                esp.cb.type = ESP_CB_CONN_ACTIVE;   /* Connection just active */
                esp.cb.cb.conn_active_closed.conn = conn;   /* Set connection */
                esp.cb.cb.conn_active_closed.client = conn->status.f.client;    /* Set if it is client or not */
                esp.cb.cb.conn_active_closed.forced = conn->status.f.client;    /* Set if action was forced = if client mode */
                espi_send_conn_cb(conn, NULL);  /* Send event */
            }
        }
    /*
    } else if (!strncmp(",CLOSED", &rcv->data[1], 7)) {
        const char* tmp = rcv->data; */
    } else if ( (rcv->len > 9  && (s = strstr(rcv->data, ",CLOSED" CRLF)) != NULL) || 
                (rcv->len > 15 && (s = strstr(rcv->data, ",CONNECT FAIL" CRLF)) != NULL)) {
        const char* tmp = s;
        uint32_t num = 0;
        while (tmp >= rcv->data && ESP_CHARISNUM(tmp[-1])) {
            tmp--;
        }
        num = espi_parse_number(&tmp);          /* Parse connection number */
        if (num < ESP_CFG_MAX_CONNS) {
            esp_conn_t* conn = &esp.conns[num]; /* Parse received data */
            conn->num = num;                    /* Set connection number */
            if (conn->status.f.active) {        /* Is connection actually active? */
                conn->status.f.active = 0;      /* Connection was just closed */
                
                esp.cb.type = ESP_CB_CONN_CLOSED;   /* Connection just active */
                esp.cb.cb.conn_active_closed.conn = conn;   /* Set connection */
                esp.cb.cb.conn_active_closed.client = conn->status.f.client;    /* Set if it is client or not */
                /** @todo: Check if we really tried to close connection which was just closed */
                esp.cb.cb.conn_active_closed.forced = IS_CURR_CMD(ESP_CMD_TCPIP_CIPCLOSE);  /* Set if action was forced = current action = close connection */
                espi_send_conn_cb(conn, NULL);  /* Send event */
                
                /**
                 * In case we received x,CLOSED on connection we are currently sending data,
                 * terminate sending of connection with failure
                 */
                if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSEND)) {
                    if (esp.msg->msg.conn_send.conn == conn) {
                        /** \todo: Find better idea to handle what to do in this case */
                        //is_error = 1;           /* Set as error to stop processing or waiting for connection */
                    }
                }
            }
            
            /* Check if write buffer is set */
            if (conn->buff != NULL) {
                ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE, "CONN: Free write buffer: %p\r\n", conn->buff);
                esp_mem_free(conn->buff);       /* Free the memory */
                conn->buff = NULL;
            }
        }
    } else if (is_error && IS_CURR_CMD(ESP_CMD_TCPIP_CIPSTART)) {
        /*
         * Notify user about failed connection,
         * but only if connection callback is known.
         *
         * This will prevent notifying wrong connection
         * in case connection is already active for some reason
         * but new callback is not set by user
         */
        if (esp.msg->msg.conn_start.cb_func != NULL) {  /* Connection must be closed */
            espi_send_conn_error_cb(esp.msg, espERRCONNFAIL);
        }
    }
    
    /*
     * In case of any of these events, simply release semaphore
     * and proceed with next command
     */
    if (is_ok || is_error || is_ready) {
        espr_t res = espOK;
        if (esp.msg != NULL) {                  /* Do we have active message? */
            res = espi_process_sub_cmd(esp.msg, is_ok, is_error, is_ready);
            if (res != espCONT) {               /* Shall we continue with next subcommand under this one? */
                if (is_ok || is_ready) {        /* Check ready or ok status */
                    res = esp.msg->res = espOK;
                } else {                        /* Or error status */
                    res = esp.msg->res = res;   /* Set the error status */
                }
            } else {
                esp.msg->i++;                   /* Number of continue calls */
            }
        }
        /*
         * When the command is finished,
         * release synchronization semaphore
         * from user thread and start with next command
         */
        if (res != espCONT) {                   /* Do we have to continue to wait for command? */
            esp_sys_sem_release(&esp.sem_sync); /* Release semaphore */
        }
    }
}

#if !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__
/**
 * \brief           Process data from input buffer
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_process_buffer(void) {
    void* data;
    size_t len;
    
    do {
        /*
         * Get length of linear memory in buffer
         * we can process directly as memory
         */
        len = esp_buff_get_linear_block_length(&esp.buff);
        if (len) {
            /*
             * Get memory address of first element
             * in linear block to process
             */
            data = esp_buff_get_linear_block_address(&esp.buff);
            
            /*
             * Process actual received data
             */
            espi_process(data, len);
            
            /*
             * Once they are processed, simply skip
             * the buffer memory and start over
             */
            esp_buff_skip(&esp.buff, len);
        }
    } while (len);
    return espOK;
}
#endif /* !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__ */

/**
 * \brief           Process input data received from ESP device
 * \param[in]       data: Pointer to data to process
 * \param[in]       data_len: Length of data to process in units of bytes
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_process(const void* data, size_t data_len) {
    uint8_t ch;
    size_t d_len = data_len;
    const uint8_t* d;
    static uint8_t ch_prev1, ch_prev2;
    static esp_unicode_t unicode;
    
    d = data;                                   /* Go to byte format */
    d_len = data_len;
    while (d_len) {                             /* Read entire set of characters from buffer */
        ch = *d++;                              /* Get next character */
        d_len--;                                /* Decrease remaining length */
        
        /*
         * First check if we are in IPD mode and process plain data
         * without checking for valid ASCII or unicode format
         */
        if (esp.ipd.read) {                     /* Do we have to read incoming IPD data? */
            size_t len;
            
            if (esp.ipd.buff != NULL) {         /* Do we have active buffer? */
                esp.ipd.buff->payload[esp.ipd.buff_ptr] = ch;   /* Save data character */
            }
            esp.ipd.buff_ptr++;
            esp.ipd.rem_len--;
            
            /*
             * Try to read more data directly from buffer
             */
            if (d_len) {
                len = ESP_MIN(esp.ipd.rem_len, esp.ipd.buff != NULL ? (esp.ipd.buff->len - esp.ipd.buff_ptr) : esp.ipd.rem_len);
                len = ESP_MIN(len, d_len);      /* Get number of bytes we can read/skip */
            } else {
                len = 0;                        /* No data to process more */
            }
            ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE, "IPD: New length to read: %d bytes\r\n", (int)len);
            if (len) {
                if (esp.ipd.buff != NULL) {     /* Is buffer valid? */
                    memcpy(&esp.ipd.buff->payload[esp.ipd.buff_ptr], d, len);
                    ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE, "IPD: Bytes read: %d\r\n", (int)len);
                } else {                        /* Simply skip the data in buffer */
                    ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE, "IPD: Bytes skipped: %d\r\n", (int)len);
                }
                d_len -= len;                   /* Decrease effective length */
                d += len;                       /* Skip remaining length */
                esp.ipd.buff_ptr += len;        /* Forward buffer pointer */
                esp.ipd.rem_len -= len;         /* Decrease remaining length */
            }
            
            /*
             * Did we reach end of buffer or no more data?
             */
            if (!esp.ipd.rem_len || (esp.ipd.buff != NULL && esp.ipd.buff_ptr == esp.ipd.buff->len)) {
                espr_t res = espOK;
                
                /*
                 * Call user callback function with received data
                 */
                if (esp.ipd.buff != NULL) {     /* Do we have valid buffer? */                  
                    /*
                     * Send data buffer to upper layer
                     *
                     * From this moment, user is responsible for packet
                     * buffer and must free it manually
                     */
                    esp.cb.type = ESP_CB_CONN_DATA_RECV;/* We have received data */
                    esp.cb.cb.conn_data_recv.buff = esp.ipd.buff;
                    esp.cb.cb.conn_data_recv.conn = esp.ipd.conn;
                    res = espi_send_conn_cb(esp.ipd.conn, NULL);    /* Send connection callback */
                    
                    esp_pbuf_free(esp.ipd.buff);    /* Free packet buffer at this point */
                    ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE, "IPD: Free packet buffer\r\n");
                    if (res == espOKIGNOREMORE) {   /* We should ignore more data */
                        ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE, "IPD: Ignoring more data from this IPD if available\r\n");
                        esp.ipd.buff = NULL;    /* Set to NULL to ignore more data if possibly available */
                    }
                        
                    /*
                     * Create new data packet if case if:
                     * 
                     *  - Previous one was successful and more data to read and
                     *  - Connection is not in closing state
                     */
                    if (esp.ipd.buff != NULL && esp.ipd.rem_len && !esp.ipd.conn->status.f.in_closing) {
                        size_t new_len = ESP_MIN(esp.ipd.rem_len, ESP_CFG_IPD_MAX_BUFF_SIZE);   /* Calculate new buffer length */
                        
                        ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE, "IPD: Allocating new packet buffer of size: %d bytes\r\n", (int)new_len);
                        esp.ipd.buff = esp_pbuf_new(new_len);   /* Allocate new packet buffer */

                        ESP_DEBUGW(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING,
                            esp.ipd.buff == NULL, "IPD: Buffer allocation failed for %d bytes\r\n", (int)new_len);
                        
                        if (esp.ipd.buff != NULL) {
                            esp_pbuf_set_ip(esp.ipd.buff, &esp.ipd.ip, esp.ipd.port);   /* Set IP and port for received data */
                        }
                    } else {
                        esp.ipd.buff = NULL;    /* Reset it */
                    }
                }
                if (!esp.ipd.rem_len) {         /* Check if we read everything */
                    esp.ipd.buff = NULL;        /* Reset buffer pointer */
                    esp.ipd.read = 0;           /* Stop reading data */
                }
                esp.ipd.buff_ptr = 0;           /* Reset input buffer pointer */
            }
            
        /*
         * We are in command mode where we have to process byte by byte
         * Simply check for ASCII and unicode format and process data accordingly
         */
        } else {
            espr_t res = espERR;
            if (ESP_ISVALIDASCII(ch)) {         /* Manually check if valid ASCII character */
                res = espOK;
                unicode.t = 1;                  /* Manually set total to 1 */
                unicode.r = 0;                  /* Reset remaining bytes */
            } else if (ch >= 0x80) {            /* Process only if more than ASCII can hold */
                res = espi_unicode_decode(&unicode, ch);    /* Try to decode unicode format */
            }
            
            if (res == espERR) {                /* In case of an ERROR */
                unicode.r = 0;
            }
            if (res == espOK) {                 /* Can we process the character(s) */
                if (unicode.t == 1) {           /* Totally 1 character? */
                    switch (ch) {
                        case '\n':
                            RECV_ADD(ch);       /* Add character to input buffer */
                            espi_parse_received(&recv_buff);	/* Parse received string */
                            RECV_RESET();       /* Reset received string */
                            break;
                        default:
                            RECV_ADD(ch);       /* Any ASCII valid character */
                            break;
                    }
                    
                    /*
                     * If we are waiting for "\n> " sequence when CIPSEND command is active
                     */
                    if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSEND)) {
                        if (ch_prev2 == '\n' && ch_prev1 == '>' && ch == ' ') {
                            RECV_RESET();       /* Reset received object */
                            
                            /*
                             * Now actually send the data prepared before
                             */
                            ESP_AT_PORT_SEND(&esp.msg->msg.conn_send.data[esp.msg->msg.conn_send.ptr], esp.msg->msg.conn_send.sent);
                            esp.msg->msg.conn_send.wait_send_ok_err = 1;    /* Now we are waiting for "SEND OK" or "SEND ERROR" */
                        }
                    }
                    
                    /*
                     * Check if "+IPD" statement is in array and now we received colon,
                     * indicating end of +IPD and start of actual data
                     */
                    if (ch == ':' && RECV_LEN() > 4 && RECV_IDX(0) == '+' && !strncmp(recv_buff.data, "+IPD", 4)) {
                        espi_parse_received(&recv_buff);	/* Parse received string */
                        if (esp.ipd.read) {     /* Are we going into read mode? */
                            size_t len;
                            ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                                "IPD: Data on connection %d with total size %d byte(s)\r\n", (int)esp.ipd.conn->num, esp.ipd.tot_len);
                            
                            len = ESP_MIN(esp.ipd.rem_len, ESP_CFG_IPD_MAX_BUFF_SIZE);
                            
                            /*
                             * Read received data in case of:
                             * 
                             *  - Connection is active and
                             *  - Connection is not in closing mode
                             */
                            if (esp.ipd.conn->status.f.active && !esp.ipd.conn->status.f.in_closing) {
                                esp.ipd.buff = esp_pbuf_new(len);   /* Allocate new packet buffer */
                                if (esp.ipd.buff != NULL) {
                                    esp_pbuf_set_ip(esp.ipd.buff, &esp.ipd.ip, esp.ipd.port);   /* Set IP and port for received data */
                                }
                                ESP_DEBUGW(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING, esp.ipd.buff == NULL,
                                    "IPD: Buffer allocation failed for %d byte(s)\r\n", (int)len);
                            } else {
                                esp.ipd.buff = NULL;    /* Ignore reading on closed connection */
                                ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE, "IPD: Connection %d closed or in closing, skipping %d byte(s)\r\n", esp.ipd.conn->num, (int)len);
                            }
                            esp.ipd.conn->status.f.data_received = 1;   /* We have first received data */
                        }
                        esp.ipd.buff_ptr = 0;   /* Reset buffer write pointer */
                        RECV_RESET();           /* Reset received buffer */
                    }
                } else {                        /* We have sequence of unicode characters */
                    /*
                     * Unicode sequence characters are not "meta" characters
                     * so it is safe to just add them to receive array without checking
                     * what are the actual values
                     */
                    uint8_t i;
                    for (i = 0; i < unicode.t; i++) {
                        RECV_ADD(unicode.ch[i]);    /* Add character to receive array */
                    }
                }
            } else if (res != espINPROG) {      /* Not in progress? */
                RECV_RESET();                   /* Invalid character in sequence */
            }
        }
        
        ch_prev2 = ch_prev1;                    /* Save previous character to previous previous */
        ch_prev1 = ch;                          /* Char current to previous */
    }
    return espOK;
}

/**
 * \brief           Process current command with known execution status and start another if necessary
 * \param[in]       msg: Pointer to current message
 * \param[in]       is_ok: Status whether last command result was OK
 * \param[in]       is_error: Status whether last command result was ERROR
 * \param[in]       is_ready: Status whether last command result was ready
 * \return          espCONT if you sent more data and we need to process more data, or espOK on success, or espERR on error
 */
static espr_t
espi_process_sub_cmd(esp_msg_t* msg, uint8_t is_ok, uint8_t is_error, uint8_t is_ready) {
#if ESP_CFG_MODE_STATION
    if (msg->cmd_def == ESP_CMD_WIFI_CWJAP) {   /* Is our intention to join to access point? */
        if (msg->cmd == ESP_CMD_WIFI_CWJAP) {   /* Is the current command join? */
            if (is_ok) {                        /* Did we join successfully? */
                msg->cmd = ESP_CMD_WIFI_CIPSTA_GET; /* Go to next command to get IP address */
                if (msg->fn(msg) == espOK) {
                    return espCONT;             /* Return to continue and not to stop command */
                }
            } else {
                /*
                 * Parse received error message,
                 * if final result was error and decide what type
                 * of error should be returned for user
                 */
                switch (msg->msg.sta_join.error_num) {
                    case 1: return espERRCONNTIMEOUT;
                    case 2: return espERRPASS;
                    case 3: return espERRNOAP;
                    case 4: return espERRCONNFAIL;
                    default: return espOK;
                }
            }
        } else if (msg->cmd == ESP_CMD_WIFI_CIPSTA_GET) {
            if (is_ok) {
                msg->cmd = ESP_CMD_WIFI_CIPSTAMAC_GET;  /* Go to next command to get MAC address */
                if (msg->fn(msg) == espOK) {
                    return espCONT;             /* Return to continue and not to stop command */
                }
            }
        }
    }
    if (msg->cmd_def == ESP_CMD_WIFI_CIPSTA_SET) {
        if (msg->i == 0 && msg->cmd == ESP_CMD_WIFI_CIPSTA_SET) {
            if (is_ok) {
                msg->cmd = ESP_CMD_WIFI_CIPSTA_GET;
                if (msg->fn(msg) == espOK) {
                    return espCONT;
                }
            }
        }
    }
    if (msg->cmd_def == ESP_CMD_WIFI_CWLAP) {
        esp.cb.cb.sta_list_ap.aps = msg->msg.ap_list.aps;
        esp.cb.cb.sta_list_ap.len = msg->msg.ap_list.apsi;
        esp.cb.cb.sta_list_ap.status = is_ok ? espOK : espERR;
        espi_send_cb(ESP_CB_STA_LIST_AP);
    }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
    if (msg->cmd_def == ESP_CMD_WIFI_CWMODE &&
        (msg->msg.wifi_mode.mode == ESP_MODE_AP
#if ESP_CFG_MODE_STATION
    || msg->msg.wifi_mode.mode == ESP_MODE_STA_AP
#endif /* ESP_CFG_MODE_STATION */
        )) {
        if (msg->cmd == ESP_CMD_WIFI_CWMODE) {
            if (is_ok) {
                msg->cmd = ESP_CMD_WIFI_CIPAP_GET;  /* Go to next command to get IP address */
                if (msg->fn(msg) == espOK) {
                    return espCONT;             /* Return to continue and not to stop command */
                }
            }
        } else if (msg->cmd == ESP_CMD_WIFI_CIPAP_GET) {
            if (is_ok) {
                msg->cmd = ESP_CMD_WIFI_CIPAPMAC_GET;   /* Go to next command to get IP address */
                if (msg->fn(msg) == espOK) {
                    return espCONT;             /* Return to continue and not to stop command */
                }
            }
        }
    }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
    if (msg->cmd_def == ESP_CMD_TCPIP_CIPSTART) {   /* Is our intention to join to access point? */
        if (msg->i == 0 && msg->cmd == ESP_CMD_TCPIP_CIPSTATUS) {   /* Was the current command status info? */
            if (is_ok) {
                espr_t res;
                msg->cmd = ESP_CMD_TCPIP_CIPSTART;  /* Now actually start connection */
                res = msg->fn(msg);   /* Start connection */
                if (res == espOK) {
                    return espCONT;
                } else {
                    return espERR;
                }
            }
        } else if (msg->i == 1 && msg->cmd == ESP_CMD_TCPIP_CIPSTART) {
            msg->cmd = ESP_CMD_TCPIP_CIPSTATUS; /* Go to status mode */
            if (is_ok) {
                if (msg->fn(msg) == espOK) {  /* Get connection status */
                    return espCONT;
                }
            }
        } else if (msg->i == 2 && msg->cmd == ESP_CMD_TCPIP_CIPSTATUS) {
            
        }
    }
    if (msg->cmd_def == ESP_CMD_RESET) {        /* Device is in reset mode */
        esp_cmd_t n_cmd = ESP_CMD_IDLE;
        switch (msg->cmd) {
            case ESP_CMD_RESET: {
                n_cmd = ESP_CFG_AT_ECHO ? ESP_CMD_ATE1 : ESP_CMD_ATE0;  /* Set ECHO mode */
                break;
            }
            case ESP_CMD_ATE0:
            case ESP_CMD_ATE1: {
                n_cmd = ESP_CMD_GMR;            /* Get AT software version */
                break;
            }
            case ESP_CMD_GMR: {
                n_cmd = ESP_CMD_WIFI_CWMODE;    /* Set Wifi mode */
                break;
            }
            case ESP_CMD_WIFI_CWMODE: {
                n_cmd = ESP_CMD_SYSMSG;         /* Set Wifi mode */
                break;
            }
            case ESP_CMD_SYSMSG: {
                n_cmd = ESP_CMD_TCPIP_CIPMUX;   /* Set multiple connections mode */
                break;
            }
            case ESP_CMD_TCPIP_CIPMUX: {
#if ESP_CFG_MODE_STATION
                n_cmd = ESP_CMD_WIFI_CWLAPOPT;  /* Set visible data for CWLAP command */
                break;
            }
            case ESP_CMD_WIFI_CWLAPOPT: {
                n_cmd = ESP_CMD_TCPIP_CIPSTATUS;/* Get connection status */
                break;
            }
            case ESP_CMD_TCPIP_CIPSTATUS: {
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
                n_cmd = ESP_CMD_WIFI_CIPAP_GET; /* Get access point IP */
                break;
            }
            case ESP_CMD_WIFI_CIPAP_GET: {
                n_cmd = ESP_CMD_WIFI_CIPAPMAC_GET;  /* Get access point MAC */
                break;
            }
            case ESP_CMD_WIFI_CIPAPMAC_GET: {
#endif /* ESP_CFG_MODE_STATION */
                n_cmd = ESP_CMD_TCPIP_CIPDINFO; /* Set visible data on +IPD */
                break;
            }
            default: break;
        }
        if (n_cmd != ESP_CMD_IDLE) {            /* Is there a change of command? */
            msg->cmd = n_cmd;
            if (msg->fn(msg) == espOK) {        /* Try to start with new connection */
                return espCONT;
            }
        }
    }
    
    /*
     * Are we enabling server mode for some reason?
     */
    if (msg->cmd_def == ESP_CMD_TCPIP_CIPSERVER && msg->msg.tcpip_server.port) {
        if (msg->cmd == ESP_CMD_TCPIP_CIPSERVERMAXCONN) {
            /* Since not all AT versions support CIPSERVERMAXCONN command, ignore result */
            msg->cmd = ESP_CMD_TCPIP_CIPSERVER;
            if (msg->fn(msg) == espOK) {        /* Try to start with new connection */
                return espCONT;
            }
        } else if (msg->cmd == ESP_CMD_TCPIP_CIPSERVER) {
            if (is_ok) {
                esp.cb_server = msg->msg.tcpip_server.cb;   /* Set server callback function */
//                msg->cmd = ESP_CMD_TCPIP_CIPSTO;
//                if (msg->fn(msg) == espOK) {  /* Try to start with new connection */
//                    return espCONT;
//                }
            }
        }
    }
    return is_ok || is_ready ? espOK : espERR;
}

/**
 * \brief           Function to initialize every AT command
 * \note            Never call this function directly. Set as initialization function for command and use `msg->fn(msg)`
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_initiate_cmd(esp_msg_t* msg) {
    switch (msg->cmd) {                         /* Check current message we want to send over AT */
        case ESP_CMD_RESET: {                   /* Reset MCU with AT commands */
            ESP_AT_PORT_SEND_STR("AT+RST" CRLF);
            break;
        }
        case ESP_CMD_ATE0: {                    /* Disable AT echo mode */
            ESP_AT_PORT_SEND_STR("ATE0" CRLF);
            break;
        }
        case ESP_CMD_ATE1: {                    /* Enable AT echo mode */
            ESP_AT_PORT_SEND_STR("ATE1" CRLF);
            break;
        }
        case ESP_CMD_GMR: {                     /* Get AT version */
            ESP_AT_PORT_SEND_STR("AT+GMR" CRLF);
            break;
        }
        case ESP_CMD_SYSMSG: {
            ESP_AT_PORT_SEND_STR("AT+SYSMSG_CUR=3" CRLF);
            break;
        }
        case ESP_CMD_UART: {                    /* Change UART parameters for AT port */
            ESP_AT_PORT_SEND_STR("AT+UART_CUR=");
            send_number(ESP_U32(msg->msg.uart.baudrate), 0);
            ESP_AT_PORT_SEND_STR(",8,1,0,0");
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_WIFI_CWLAPOPT: {
            ESP_AT_PORT_SEND_STR("AT+CWLAPOPT=1,2048" CRLF);
            break;
        }
        
        /*
         * WiFi related commands
         */
 
#if ESP_CFG_MODE_STATION       
        case ESP_CMD_WIFI_CWJAP: {              /* Try to join to access point */
            ESP_AT_PORT_SEND_STR("AT+CWJAP_");
            if (msg->msg.sta_join.def) {
                ESP_AT_PORT_SEND_STR("DEF=\"");
            } else {
                ESP_AT_PORT_SEND_STR("CUR=\"");
            }
            send_string(msg->msg.sta_join.name, 1, 0);
            ESP_AT_PORT_SEND_STR("\",\"");
            send_string(msg->msg.sta_join.pass, 1, 0);
            ESP_AT_PORT_SEND_STR("\"");
            if (msg->msg.sta_join.mac != NULL) {
                ESP_AT_PORT_SEND_STR(",");
                send_ip_mac(msg->msg.sta_join.mac, 0, 1);
            }
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_WIFI_CWQAP: {              /* Quit from access point */
            ESP_AT_PORT_SEND_STR("AT+CWQAP" CRLF);
            break;
        }
        case ESP_CMD_WIFI_CWLAP: {              /* List access points */
            ESP_AT_PORT_SEND_STR("AT+CWLAP");
            if (msg->msg.ap_list.ssid != NULL) {/* Do we want to filter by SSID? */   
                ESP_AT_PORT_SEND_STR("=");
                send_string(msg->msg.ap_list.ssid, 1, 1);
            }
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_WIFI_CWAUTOCONN: {         /* Set autoconnect feature */
            ESP_AT_PORT_SEND_STR("AT+CWAUTOCONN=");
            send_number(ESP_U32(!!msg->msg.sta_autojoin.en), 0);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#endif /* ESP_CFG_MODE_STATION */
        case ESP_CMD_WIFI_CWMODE: {             /* Set WIFI mode */
            esp_mode_t m;
            char c;
            
            if (msg->cmd_def == ESP_CMD_RESET) {/* Is this command part of reset sequence? */
#if ESP_CFG_MODE_STATION_ACCESS_POINT
                m = ESP_MODE_STA_AP;            /* Set station and access point mode */
#elif ESP_CFG_MODE_STATION
                m = ESP_MODE_STA;               /* Set station mode */
#else
                m = ESP_MODE_AP;                /* Set access point mode */
#endif /* ESP_CFG_MODE_STATION_ACCESS_POINT */
            } else {
                m = msg->msg.wifi_mode.mode;    /* Set user defined mode */
            }
            c = (char)m + '0';                  /* Continue to ASCII mode */
    
            ESP_AT_PORT_SEND_STR("AT+CWMODE=");
            ESP_AT_PORT_SEND_CHR(&c);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTA_GET:           /* Get station IP address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAP_GET:            /* Get access point IP address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            ESP_AT_PORT_SEND_STR("AT+CIP");
#if ESP_CFG_MODE_STATION
            if (msg->cmd == ESP_CMD_WIFI_CIPSTA_GET) {
                ESP_AT_PORT_SEND_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (msg->cmd == ESP_CMD_WIFI_CIPAP_GET) {
                ESP_AT_PORT_SEND_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_getip.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("?" CRLF);
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTAMAC_GET:        /* Get station MAC address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAPMAC_GET:         /* Get access point MAC address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            ESP_AT_PORT_SEND_STR("AT+CIP");
#if ESP_CFG_MODE_STATION
            if (msg->cmd == ESP_CMD_WIFI_CIPSTAMAC_GET) {
                ESP_AT_PORT_SEND_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (msg->cmd == ESP_CMD_WIFI_CIPAPMAC_GET) {
                ESP_AT_PORT_SEND_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            ESP_AT_PORT_SEND_STR("MAC");
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_getmac.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("?" CRLF);
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTA_SET:           /* Set station IP address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAP_SET:            /* Set access point IP address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            ESP_AT_PORT_SEND_STR("AT+CIP");
#if ESP_CFG_MODE_STATION
            if (msg->cmd == ESP_CMD_WIFI_CIPSTA_SET) {
                ESP_AT_PORT_SEND_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (msg->cmd == ESP_CMD_WIFI_CIPAP_SET) {
                ESP_AT_PORT_SEND_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_setip.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("=");
            send_ip_mac(msg->msg.sta_ap_setip.ip, 1, 1);    /* Send IP address */
            if (msg->msg.sta_ap_setip.gw != NULL) { /* Is gateway set? */
                ESP_AT_PORT_SEND_STR(",");
                send_ip_mac(msg->msg.sta_ap_setip.gw, 1, 1);    /* Send gateway address */
                if (msg->msg.sta_ap_setip.nm != NULL) { /* Is netmask set ? */
                    ESP_AT_PORT_SEND_STR(",");
                    send_ip_mac(msg->msg.sta_ap_setip.nm, 1, 1);    /* Send netmask address */
                }
            }
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTAMAC_SET:        /* Set station MAC address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAPMAC_SET:         /* Set access point MAC address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            ESP_AT_PORT_SEND_STR("AT+CIP");
#if ESP_CFG_MODE_STATION
            if (msg->cmd == ESP_CMD_WIFI_CIPSTAMAC_SET) {
                ESP_AT_PORT_SEND_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (msg->cmd == ESP_CMD_WIFI_CIPAPMAC_SET) {
                ESP_AT_PORT_SEND_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            ESP_AT_PORT_SEND_STR("MAC");
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_setmac.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("=");
            send_ip_mac(msg->msg.sta_ap_setmac.mac, 0, 1);  /* Send MAC address */
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CWSAP_SET: {          /* Set access point parameters */
            ESP_AT_PORT_SEND_STR("AT+CWSAP");
            if (msg->msg.ap_conf.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("=");
            send_string(msg->msg.ap_conf.ssid, 1, 1);   /* Send escaped string */
            ESP_AT_PORT_SEND_STR(",");
            send_string(msg->msg.ap_conf.pwd, 1, 1);    /* Send escaped string */
            ESP_AT_PORT_SEND_STR(",");
            send_number(ESP_U32(msg->msg.ap_conf.ch), 0);
            ESP_AT_PORT_SEND_STR(",");
            send_number(ESP_U32(msg->msg.ap_conf.ecn), 0);
            ESP_AT_PORT_SEND_STR(",");
            send_number(ESP_U32(msg->msg.ap_conf.max_sta), 0);
            ESP_AT_PORT_SEND_STR(",");
            send_number(ESP_U32(!!msg->msg.ap_conf.hid), 0);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_WIFI_CWLIF: {              /* List stations connected on access point */
            ESP_AT_PORT_SEND_STR("AT+CWLIF" CRLF);
            break;
        }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
#if ESP_CFG_HOSTNAME
        case ESP_CMD_WIFI_CWHOSTNAME_SET: {     /* List stations connected on access point */
            ESP_AT_PORT_SEND_STR("AT+CWHOSTNAME=");
            send_string(msg->msg.wifi_hostname.hostname, 1, 1);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_WIFI_CWHOSTNAME_GET: {     /* List stations connected on access point */
            ESP_AT_PORT_SEND_STR("AT+CWHOSTNAME?" CRLF);
            break;
        }
#endif /* ESP_CFG_HOSTNAME */
#if ESP_CFG_WPS
        case ESP_CMD_WIFI_WPS: {
            ESP_AT_PORT_SEND_STR("AT+WPS=");
            send_number(ESP_U32(!!msg->msg.wps_cfg.en), 0);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#endif /* ESP_CFG_WPS */
#if ESP_CFG_MDNS
        case ESP_CMD_WIFI_MDNS: {
            ESP_AT_PORT_SEND_STR("AT+MDNS=");
            send_number(ESP_U32(!!msg->msg.mdns.en), 0);
            if (msg->msg.mdns.en) {             /* Send the rest only in case mDNS should be enabled */
                ESP_AT_PORT_SEND_STR(",");
                send_string(msg->msg.mdns.host, 0, 1);
                ESP_AT_PORT_SEND_STR(",");
                send_string(msg->msg.mdns.server, 0, 1);
                ESP_AT_PORT_SEND_STR(",");
                send_port(msg->msg.mdns.port, 0);
            }
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#endif /* ESP_CFG_MDNS */
        
        /*
         * TCP/IP related commands
         */
        
        case ESP_CMD_TCPIP_CIPSERVER: {         /* Enable or disable a server */    
            ESP_AT_PORT_SEND_STR("AT+CIPSERVER=");
            if (msg->msg.tcpip_server.port) {   /* Do we have valid port? */
                ESP_AT_PORT_SEND_STR("1,");
                send_port(msg->msg.tcpip_server.port, 0);
            } else {                            /* Disable server */
                ESP_AT_PORT_SEND_STR("0");
            }
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_TCPIP_CIPSERVERMAXCONN: {  /* Maximal number of connections */
            uint16_t max_conn;
            if (msg->cmd_def == ESP_CMD_TCPIP_CIPSERVER) {
                max_conn = ESP_MIN(msg->msg.tcpip_server.max_conn, ESP_CFG_MAX_CONNS);
            } else {
                max_conn = ESP_CFG_MAX_CONNS;
            }
            ESP_AT_PORT_SEND_STR("AT+CIPSERVERMAXCONN=");
            send_number(ESP_U32(max_conn), 0);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_TCPIP_CIPSTO: {            /* Set server connection timeout */
            uint16_t timeout;
            if (msg->cmd_def == ESP_CMD_TCPIP_CIPSERVER) {
                timeout = msg->msg.tcpip_server.timeout;
            } else {
                timeout = 100;
            }
            ESP_AT_PORT_SEND_STR("AT+CIPSTO=");
            send_number(ESP_U32(timeout), 0);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#if ESP_CFG_MODE_STATION        
        case ESP_CMD_TCPIP_CIPSTART: {          /* Start a new connection */
            int8_t i = 0;
            esp_conn_t* c = NULL;
            char str[6];
            
            /* Do we have wifi connection? */
            if (esp_sta_has_ip() != espOK) {
                espi_send_conn_error_cb(msg, espERRNOIP);
                return espERRNOIP;
            }
            
            msg->msg.conn_start.num = 0;        /* Reset to make sure default value is set */
            for (i = ESP_CFG_MAX_CONNS - 1; i >= 0; i--) {  /* Find available connection */
                if (!esp.conns[i].status.f.active || !(esp.active_conns & (1 << i))) {
                    c = &esp.conns[i];
                    c->num = i;
                    msg->msg.conn_start.num = i;/* Set connection number for message structure */
                    break;
                }
            }
            if (c == NULL) {
                espi_send_conn_error_cb(msg, espERRNOFREECONN);
                return espERRNOFREECONN;        /* We don't have available connection */
            }
            
            if (msg->msg.conn_start.conn != NULL) { /* Is user interested about connection info? */
                *msg->msg.conn_start.conn = c;  /* Save connection for user */
            }
            
            ESP_AT_PORT_SEND_STR("AT+CIPSTART=");
            send_number(ESP_U32(i), 0);
            ESP_AT_PORT_SEND_STR(",\"");
            if (msg->msg.conn_start.type == ESP_CONN_TYPE_SSL) {
                ESP_AT_PORT_SEND_STR("SSL");
            } else if (msg->msg.conn_start.type == ESP_CONN_TYPE_TCP) {
                ESP_AT_PORT_SEND_STR("TCP");
            } else if (msg->msg.conn_start.type == ESP_CONN_TYPE_UDP) {
                ESP_AT_PORT_SEND_STR("UPD");
            }
            ESP_AT_PORT_SEND_STR("\",\"");
            ESP_AT_PORT_SEND_STR(msg->msg.conn_start.host);
            ESP_AT_PORT_SEND_STR("\",");
            send_port(msg->msg.conn_start.port, 0);
            ESP_AT_PORT_SEND_STR(str);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#endif /* ESP_CFG_MODE_STATION */

        case ESP_CMD_TCPIP_CIPCLOSE: {          /* Close the connection */
            if (msg->msg.conn_close.conn != NULL &&
                /* Is connection already closed or command for this connection is not valid anymore? */
                (!esp_conn_is_active(msg->msg.conn_close.conn) || msg->msg.conn_close.conn->val_id != msg->msg.conn_close.val_id)) {
                return espERR;
            }
            ESP_AT_PORT_SEND_STR("AT+CIPCLOSE=");
            send_number(ESP_U32(msg->msg.conn_close.conn ? msg->msg.conn_close.conn->num : ESP_CFG_MAX_CONNS), 0);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_TCPIP_CIPSEND: {           /* Send data to connection */
            return espi_tcpip_process_send_data();  /* Process send data */
        }
        case ESP_CMD_TCPIP_CIPSTATUS: {         /* Get status of device and all connections */
            esp.active_conns_last = esp.active_conns;   /* Save as last status */
            esp.active_conns = 0;               /* Reset new status before parsing starts */
            ESP_AT_PORT_SEND_STR("AT+CIPSTATUS" CRLF);   /* Send command to AT port */
            break;
        }
        case ESP_CMD_TCPIP_CIPDINFO: {          /* Set info data on +IPD command */
            ESP_AT_PORT_SEND_STR("AT+CIPDINFO=1" CRLF);
            break;
        }
        case ESP_CMD_TCPIP_CIPMUX: {            /* Set multiple connections */
            ESP_AT_PORT_SEND_STR("AT+CIPMUX=");
            if (msg->cmd_def == ESP_CMD_RESET || msg->msg.tcpip_mux.mux) {  /* If reset command is active, enable CIPMUX */
                ESP_AT_PORT_SEND_STR("1");
            } else {
                ESP_AT_PORT_SEND_STR("0");
            }
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_TCPIP_CIPSSLSIZE: {        /* Set SSL size */
            ESP_AT_PORT_SEND_STR("AT+CIPSSLSIZE=");
            send_number(ESP_U32(msg->msg.tcpip_sslsize.size), 0);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#if ESP_CFG_DNS
        case ESP_CMD_TCPIP_CIPDOMAIN: {         /* DNS function */
            ESP_AT_PORT_SEND_STR("AT+CIPDOMAIN=");
            send_string(msg->msg.dns_getbyhostname.host, 1, 1);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#endif /* ESP_CFG_DNS */
#if ESP_CFG_PING
        case ESP_CMD_TCPIP_PING: {              /* Pinging hostname or IP address */
            ESP_AT_PORT_SEND_STR("AT+PING=");
            send_string(msg->msg.tcpip_ping.host, 1, 1);
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
#endif /* ESP_CFG_PING */
#if ESP_CFG_SNTP
        case ESP_CMD_TCPIP_CIPSNTPCFG: {        /* Configure SNTP */
            ESP_AT_PORT_SEND_STR("AT+CIPSNTPCFG=");
            send_number(ESP_U32(!!msg->msg.tcpip_sntp_cfg.en), 0);
            ESP_AT_PORT_SEND_STR(",");
            send_signed_number(ESP_U32(msg->msg.tcpip_sntp_cfg.tz), 0);
            if (msg->msg.tcpip_sntp_cfg.h1 != NULL && strlen(msg->msg.tcpip_sntp_cfg.h1)) {
                ESP_AT_PORT_SEND_STR(",");
                send_string(msg->msg.tcpip_sntp_cfg.h1, 0, 1);
            }
            if (msg->msg.tcpip_sntp_cfg.h2 != NULL && strlen(msg->msg.tcpip_sntp_cfg.h2)) {
                ESP_AT_PORT_SEND_STR(",");
                send_string(msg->msg.tcpip_sntp_cfg.h2, 0, 1);
            }
            if (msg->msg.tcpip_sntp_cfg.h3 != NULL && strlen(msg->msg.tcpip_sntp_cfg.h3)) {
                ESP_AT_PORT_SEND_STR(",");
                send_string(msg->msg.tcpip_sntp_cfg.h3, 0, 1);
            }
            ESP_AT_PORT_SEND_STR(CRLF);
            break;
        }
        case ESP_CMD_TCPIP_CIPSNTPTIME: {       /* Get time over SNTP */
            ESP_AT_PORT_SEND_STR("AT+CIPSNTPTIME?" CRLF);
            break;
        }
#endif /* ESP_CFG_SNTP */
        
        default: 
            return espERR;                      /* Invalid command */
    }
    return espOK;                               /* Valid command */
}

/**
 * \brief           Checks if connection pointer has valid address
 * \param[in]       conn: Address to check if valid connection ptr
 * \return          `1` on success, `0` otherwise
 */
uint8_t
espi_is_valid_conn_ptr(esp_conn_p conn) {
    uint8_t i = 0;
    for (i = 0; i < sizeof(esp.conns) / sizeof(esp.conns[0]); i++) {
        if (conn == &esp.conns[i]) {
            return 1;
        }
    }
    return 0;
}

/**
 * \brief           Send message from API function to producer queue for further processing
 * \param[in]       msg: New message to process
 * \param[in]       process_fn: callback function used to process message
 * \param[in]       block_time: Time used to block function. Use 0 for non-blocking call
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_send_msg_to_producer_mbox(esp_msg_t* msg, espr_t (*process_fn)(esp_msg_t *), uint32_t block, uint32_t max_block_time) {
    espr_t res = msg->res = espOK;
    if (block) {                                /* In case message is blocking */
        if (!esp_sys_sem_create(&msg->sem, 0)) {/* Create semaphore and lock it immediatelly */
            ESP_MSG_VAR_FREE(msg);              /* Release memory and return */
            return espERRMEM;
        }
    }
    if (!msg->cmd) {                            /* Set start command if not set by user */
        msg->cmd = msg->cmd_def;                /* Set it as default */
    }
    msg->is_blocking = block;                   /* Set status if message is blocking */
    msg->block_time = max_block_time;           /* Set blocking status if necessary */
    msg->fn = process_fn;                       /* Save processing function to be called as callback */
    if (block) {
        esp_sys_mbox_put(&esp.mbox_producer, msg);  /* Write message to producer queue and wait forever */
    } else {
        if (!esp_sys_mbox_putnow(&esp.mbox_producer, msg)) {    /* Write message to producer queue immediatelly */
            ESP_MSG_VAR_FREE(msg);              /* Release message */
            res = espERR;
        }
    }
    if (block && res == espOK) {                /* In case we have blocking request */
        uint32_t time;
        time = esp_sys_sem_wait(&msg->sem, max_block_time); /* Wait forever for semaphore access for max block time */
        if (ESP_SYS_TIMEOUT == time) {          /* If semaphore was not accessed in given time */
            res = espERR;                       /* Semaphore not released in time */
        } else {
            res = msg->res;                     /* Set response status from message response */
        }
        if (esp_sys_sem_isvalid(&msg->sem)) {   /* In case we have valid semaphore */
            esp_sys_sem_delete(&msg->sem);      /* Delete semaphore object */
            esp_sys_sem_invalid(&msg->sem);     /* Invalidate semaphore object */
        }
        ESP_MSG_VAR_FREE(msg);                  /* Release message */
    }
    return res;
}
