/**
 * \file            esp_int.c
 * \brief           Internal functions
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
#include "include/esp.h"
#include "include/esp_int.h"
#include "include/esp_parser.h"
#include "include/esp_unicode.h"
#include "esp_ll.h"

#define IS_CURR_CMD(c)        (esp.msg && esp.msg->cmd == (c))

typedef struct {
    char data[128];
    uint8_t len;
} esp_recv_t;
static esp_recv_t recv;

#define RECV_ADD(ch)        do { recv.data[recv.len++] = ch; recv.data[recv.len] = 0; } while (0)
#define RECV_RESET()        recv.data[(recv.len = 0)] = 0;
#define RECV_LEN()          recv.len
#define RECV_IDX(index)     recv.data[index]

#define ESP_AT_PORT_SEND_STR(str)       esp.ll.send((const uint8_t *)(str), strlen(str))
#define ESP_AT_PORT_SEND_CHR(str)       esp.ll.send((const uint8_t *)(str), 1)
#define ESP_AT_PORT_SEND(d, l)          esp.ll.send((const uint8_t *)(d), l)

static espr_t espi_process_sub_cmd(esp_msg_t* msg, uint8_t is_ok, uint8_t is_error, uint8_t is_ready);

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
 * \brief           Send IP or MAC address to AT port
 * \param[in]       d: Pointer to IP or MAC address
 * \param[in]       is_ip: Set to 1 when sending IP, or 0 when MAC
 * \param[in]       q: Set to 1 to include start and ending quotes
 */
static void
send_ip_mac(const uint8_t* d, uint8_t is_ip, uint8_t q) {
    uint8_t i, ch;
    char str[4];
    
    if (!d) {
        return;
    }
    if (q) {
        ESP_AT_PORT_SEND_STR("\"");             /* Send starting quote character */
    }
    ch = is_ip ? '.' : ':';                     /* Get delimiter character */
    for (i = 0; i < (is_ip ? 4 : 6); i++) {     /* Process byte by byte */
        if (is_ip) {                            /* In case of IP ... */
            number_to_str(d[i], str);           /* ... go to decimal format ... */
        } else {                                /* ... in case of MAC ... */
            byte_to_str(d[i], str);             /* ... go to HEX format */
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
    if (e) {                                    /* Do we have to escape string? */
        while (*str) {                          /* Go through string */
            if (*str == ',' || *str == '"' || *str == '\\') {   /* Check for special character */    
                ESP_AT_PORT_SEND_CHR(&special); /* Send special character */
            }
            ESP_AT_PORT_SEND_CHR(str);          /* Send character */
            str++;
        }
    } else {
        ESP_AT_PORT_SEND_STR(str);              /* Send plain string */
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
 * \brief           Check if received string includes "_CUR" or "_DEF" as current or default setup
 * \param[in]       str: Pointer to string to test
 * \return          1 if current setting, 0 otherwise
 */
static uint8_t
is_received_current_setting(const char* str) {
    return !strstr(str, "_DEF");                /* In case there is no "_DEF", we have current setting active */
}

/**
 * \brief           Process connection callback
 * \note            Before calling function, callback structure must be prepared
 * \param[in]       conn: Pointer to connection to use as callback
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
espi_send_conn_cb(esp_conn_t* conn) {
    if (conn->cb_func) {                        /* Connection custom callback? */
        return conn->cb_func(&esp.cb);          /* Process callback function */
    } else {
        return esp.cb_func(&esp.cb);            /* Process default callback function */
    }
}

/**
 * \brief           Process callback function to user with specific type
 * \param[in]       type: Callback event type
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
espi_send_cb(esp_cb_type_t type) {
    esp.cb.type = type;                         /* Set callback type to process */
    return esp.cb_func(&esp.cb);                /* Call function and return status */
}

/**
 * \brief           Process and send data from device buffer
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
espi_tcpip_process_send_data(void) {
    char ch;
    
    if (!esp_conn_is_active(esp.msg->msg.conn_send.conn)) {
        return espERR;
    }
    ch = esp.msg->msg.conn_send.conn->num + '0';
    ESP_AT_PORT_SEND_STR("AT+CIPSEND=");
    ESP_AT_PORT_SEND_CHR(&ch);
    ESP_AT_PORT_SEND_STR(",");
    esp.msg->msg.conn_send.sent = esp.msg->msg.conn_send.btw > ESP_CONN_MAX_DATA_LEN ? ESP_CONN_MAX_DATA_LEN : esp.msg->msg.conn_send.btw;
    send_number(esp.msg->msg.conn_send.sent, 0);    /* Send length number */
    
    if (esp.msg->msg.conn_send.conn->type == ESP_CONN_TYPE_UDP) {
        const uint8_t* ip = esp.msg->msg.conn_send.remote_ip;   /* Get remote IP */
        uint16_t port = esp.msg->msg.conn_send.remote_port;
        
        if (ip && port) {
            ESP_AT_PORT_SEND_STR(",");
            send_ip_mac(ip, 1, 1);              /* Send IP address including quotes */
            ESP_AT_PORT_SEND_STR(",");
            send_number(port, 0);               /* Send length number */
        }
    }
    ESP_AT_PORT_SEND_STR("\r\n");
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
        esp.msg->msg.conn_send.btw -= esp.msg->msg.conn_send.sent;
        esp.msg->msg.conn_send.data += esp.msg->msg.conn_send.sent;
        if (esp.msg->msg.conn_send.bw) {
            *esp.msg->msg.conn_send.bw += esp.msg->msg.conn_send.sent;
        }
        esp.msg->msg.conn_send.tries = 0;
    } else {
        esp.msg->msg.conn_send.tries++;         /* Increase number of tries */
        if (esp.msg->msg.conn_send.tries == 3) {    /* In case we reached max number of retransmissions */
            return 1;                           /* Return 1 and indicate error */
        }
    }
    if (esp.msg->msg.conn_send.btw) {           /* Do we still have data to send? */
        if (espi_tcpip_process_send_data() != espOK) {  /* Check if we can continue */
            return 1;                           /* Finish as stopped */
        }
        return 0;                               /* We still have data to send */
    }
    return 1;                                   /* Everything was sent, we can stop execution */
}

/**
 * \brief           Process received string from ESP 
 * \param[in]       recv: Pointer to \ref esp_rect_t structure with input string
 */
static void
espi_parse_received(esp_recv_t* rcv) {
    uint8_t is_ok = 0, is_error = 0, is_ready = 0;
    const char* s;
    
    if (rcv->len == 2 && rcv->data[0] == '\r' && rcv->data[1] == '\n') {
        return;
    }
    //printf("Rcv! s: %s", (char *)rcv->data);
    
    /**
     * Detect most common responses from device
     */
    is_ok = !strcmp(rcv->data, "OK\r\n");       /* Check if received string is OK */
    if (!is_ok) {
        is_error = !strcmp(rcv->data, "ERROR\r\n") || !strcmp(rcv->data, "FAIL\r\n");   /* Check if received string is error */
        if (!is_error) {
            is_ready = !strcmp(rcv->data, "ready\r\n"); /* Check if received string is ready */
        }
    }
    
    /**
     * Read and process statements starting with '+' character
     */
    if (rcv->data[0] == '+') {
        if (!strncmp("+IPD", rcv->data, 4)) {   /* Check received network data */
            espi_parse_ipd(rcv->data + 5);      /* Parse IPD statement and start receiving network data */
        } else if (esp.msg) {
            if ((IS_CURR_CMD(ESP_CMD_WIFI_CIPSTAMAC_GET) && !strncmp(rcv->data, "+CIPSTAMAC", 10)) || 
                (IS_CURR_CMD(ESP_CMD_WIFI_CIPAPMAC_GET) && !strncmp(rcv->data, "+CIPAPMAC", 9))) {
                const char* tmp;
                uint8_t mac[6];
                
                if (rcv->data[9] == '_') {      /* Do we have "_CUR" or "_DEF" included? */
                    tmp = &rcv->data[14];
                } else if (rcv->data[10] == '_') {
                    tmp = &rcv->data[15];
                } else if (rcv->data[9] == ':') {
                    tmp = &rcv->data[10];
                } else if (rcv->data[10] == ':') {
                    tmp = &rcv->data[11];
                }
                
                espi_parse_mac(&tmp, mac);      /* Save as current MAC address */
                if (is_received_current_setting(rcv->data)) {
                    memcpy(esp.msg->cmd == ESP_CMD_WIFI_CIPSTAMAC_GET ? esp.sta.mac : esp.ap.mac, mac, 6);   /* Copy to current setup */
                }
                if (esp.msg->msg.sta_ap_getmac.mac && esp.msg->cmd == esp.msg->cmd_def) {
                    memcpy(esp.msg->msg.sta_ap_getmac.mac, mac, 6); /* Copy to current setup */
                }
            } else if ( (IS_CURR_CMD(ESP_CMD_WIFI_CIPSTA_GET) && !strncmp(rcv->data, "+CIPSTA", 7)) ||
                        (IS_CURR_CMD(ESP_CMD_WIFI_CIPAP_GET) && !strncmp(rcv->data, "+CIPAP", 6))) {
                const char* tmp = NULL;
                uint8_t *a = NULL, *b = NULL;
                uint8_t ip[4], ch;
                esp_ip_mac_t* im;
                            
                im = (esp.msg->cmd == ESP_CMD_WIFI_CIPSTA_GET) ? &esp.sta : &esp.ap;    /* Get IP and MAC structure first */
                
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
                    case 'i': tmp = &rcv->data[10]; a = im->ip; b = esp.msg->msg.sta_ap_getip.ip; break;
                    case 'g': tmp = &rcv->data[15]; a = im->gw; b = esp.msg->msg.sta_ap_getip.gw; break;
                    case 'n': tmp = &rcv->data[15]; a = im->nm; b = esp.msg->msg.sta_ap_getip.nm; break;
                    default: tmp = NULL; a = NULL; b = NULL; break;
                }
                if (tmp) {                      /* Do we have temporary string? */
                    if (rcv->data[6] == '_' || rcv->data[7] == '_') {   /* Do we have "_CUR" or "_DEF" included? */
                        tmp += 4;               /* Skip it */
                    }
                    if (*tmp == ':') {
                        tmp++;
                    }
                    espi_parse_ip(&tmp, ip);    /* Parse IP address */
                    if (is_received_current_setting(rcv->data)) {
                        memcpy(a, ip, 4);       /* Copy to current setup */
                    }
                    if (b && IS_CURR_CMD(esp.msg->cmd_def)) {   /* Is current command the same as default one? */
                        memcpy(b, ip, 4);       /* Copy to user variable */
                    }
                }
            } else if (esp.msg->cmd == ESP_CMD_WIFI_CWLAP && !strncmp(rcv->data, "+CWLAP", 6)) {
                espi_parse_cwlap(rcv->data, esp.msg);   /* Parse CWLAP entry */
#if ESP_DNS || __DOXYGEN__
            } else if (esp.msg->cmd == ESP_CMD_TCPIP_CIPDOMAIN && !strncmp(rcv->data, "+CIPDOMAIN", 10)) {
                espi_parse_cipdomain(rcv->data, esp.msg);   /* Parse CIPDOMAIN entry */
#endif /* ESP_DNS || __DOXYGEN__ */
#if ESP_PING || __DOXYGEN__
            } else if (esp.msg->cmd == ESP_CMD_TCPIP_PING && ESP_CHARISNUM(rcv->data[1])) {
                const char* tmp = &rcv->data[1];
                *esp.msg->msg.tcpip_ping.time = espi_parse_number(&tmp);
#endif /* ESP_PING || __DOXYGEN__ */
            }
        }
    } else if (!strncmp(rcv->data, "WIFI CONNECTED", 14)) {
        esp.status.f.r_w_conn = 1;              /* Wifi is connected */
        espi_send_cb(ESP_CB_WIFI_CONNECTED);    /* Call user callback function */
    } else if (!strncmp(rcv->data, "WIFI DISCONNECT", 15)) {
        esp.status.f.r_w_conn = 0;              /* Wifi is disconnected */
        esp.status.f.r_got_ip = 0;              /* There is no valid IP */
        espi_send_cb(ESP_CB_WIFI_DISCONNECTED); /* Call user callback function */
    } else if (!strncmp(rcv->data, "WIFI GOT IP", 11)) {
        esp.status.f.r_got_ip = 1;              /* Wifi got IP address */
        espi_send_cb(ESP_CB_WIFI_GOT_IP);       /* Call user callback function */
    }
    
    /**
     * Start processing received data
     */
    if (esp.msg) {                              /* Do we have valid message? */
        if (esp.msg->cmd == ESP_CMD_RESET && is_ok) {    /* Check for reset command */
            is_ok = 0;                          /* We must wait for "ready", not only "OK" */
        } else if (esp.msg->cmd == ESP_CMD_TCPIP_CIPSTATUS) {
            if (!strncmp(rcv->data, "+CIPSTATUS", 10)) {
                espi_parse_cipstatus(rcv->data + 11);   /* Parse CIPSTATUS response */
            } else if (is_ok) {
                uint8_t i;
                for (i = 0; i < ESP_MAX_CONNS; i++) {   /* Set current connection statuses */
                    esp.conns[i].status.f.active = !!(esp.active_conns & (1 << i));
                }
            }
        } else if (esp.msg->cmd == ESP_CMD_TCPIP_CIPSEND) {
            if (is_ok) {                        /* Check for OK and clear as we have to check for "> " statement */
                is_ok = 0;                      /* Do not reach on OK */
            }
            if (esp.msg->msg.conn_send.wait_send_ok_err) {
                if (!strncmp("SEND OK", rcv->data, 7)) {    /* Data were sent successfully */
                    esp.msg->msg.conn_send.wait_send_ok_err = 0;
                    is_ok = espi_tcpip_process_data_sent(1);    /* Process as data were sent */
                    if (is_ok) {
                        esp.cb.type = ESP_CB_DATA_SENT; /* Data were fully sent */
                        esp.cb.cb.conn_data_sent.conn = esp.msg->msg.conn_send.conn;
                        espi_send_conn_cb(esp.msg->msg.conn_send.conn); /* Send connection callback */
                    }
                } else if (is_error || !strncmp("SEND FAIL", rcv->data, 9)) {
                    esp.msg->msg.conn_send.wait_send_ok_err = 0;
                    is_error = espi_tcpip_process_data_sent(0); /* Data were not sent due to SEND FAIL or command didn't even start */
                    if (is_error) {
                        esp.cb.type = ESP_CB_DATA_SEND_ERR; /* Error sending data */
                        esp.cb.cb.conn_data_send_err.conn = esp.msg->msg.conn_send.conn;
                        espi_send_conn_cb(esp.ipd.conn);/* Send connection callback */
                    }
                }
            }
        } else if (esp.msg->cmd == ESP_CMD_UART) {  /* In case of UART command */
            if (is_ok) {                        /* We have valid OK result */
                esp_ll_init(&esp.ll, esp.msg->msg.uart.baudrate);   /* Set new baudrate */
            }
        }
    }
    
    /**
     * Check if connection is just active or just closed
     */
    /*
    if (!strncmp(",CONNECT", &rcv->data[1], 8)) {
        const char* tmp = rcv->data; */
    if (rcv->len > 10 && (s = strstr(rcv->data, ",CONNECT\r\n")) != NULL) {
        const char* tmp = s - 1;
        uint8_t num = espi_parse_number(&tmp);
        if (num < ESP_MAX_CONNS) {
            esp_conn_t* conn = &esp.conns[num]; /* Parse received data */
            memset(conn, 0x00, sizeof(*conn));  /* Reset connection parameters */
            conn->num = num;                    /* Set connection number */
            conn->status.f.active = 1;          /* Connection just active */
            if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSTART) && num == esp.msg->msg.conn_start.num) {    /* Did we start connection on our own? */
                conn->status.f.client = 1;      /* Go to client mode */
                conn->cb_func = esp.msg->msg.conn_start.cb_func;    /* Set callback function */
                conn->arg = esp.msg->msg.conn_start.arg;    /* Set argument for function */
                conn->type = esp.msg->msg.conn_start.type;  /* Set connection type */
            } else {                            /* Server connection start */
                conn->status.f.client = 0;      /* We are in server mode this time */
                conn->cb_func = esp.cb_server;  /* Set server default callback */
                conn->arg = NULL;
                conn->type = ESP_CONN_TYPE_TCP; /* Set connection type to TCP. @todo: Wait for ESP team to upgrade AT commands to set other type */
            }
            
            esp.cb.type = ESP_CB_CONN_ACTIVE;   /* Connection just active */
            esp.cb.cb.conn_active_closed.conn = conn;   /* Set connection */
            esp.cb.cb.conn_active_closed.client = conn->status.f.client;    /* Set if it is client or not */
            espi_send_conn_cb(conn);            /* Send event */
        }
    /*
    } else if (!strncmp(",CLOSED", &rcv->data[1], 7)) {
        const char* tmp = rcv->data; */
    } else if ((rcv->len > 9 && (s = strstr(rcv->data, ",CLOSED\r\n")) != NULL) || (rcv->len > 15 && (s = strstr(rcv->data, ",CONNECT FAIL\r\n")) != NULL)) {
        const char* tmp = s - 1;
        uint8_t num = espi_parse_number(&tmp);
        if (num < ESP_MAX_CONNS) {
            esp_conn_t* conn = &esp.conns[num]; /* Parse received data */
            conn->num = num;                    /* Set connection number */
            if (conn->status.f.active) {        /* Is connection actually active? */
                conn->status.f.active = 0;      /* Connection was just closed */
                
                esp.cb.type = ESP_CB_CONN_CLOSED;   /* Connection just active */
                esp.cb.cb.conn_active_closed.conn = conn;   /* Set connection */
                esp.cb.cb.conn_active_closed.client = conn->status.f.client;    /* Set if it is client or not */
                espi_send_conn_cb(conn);        /* Send event */
                
                /**
                 * In case we received x,CLOSED on connection we are currently sending data,
                 * terminate sending of connection with failure
                 */
                if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSEND)) {
                    if (esp.msg->msg.conn_send.conn == conn) {
                        is_error = 1;           /* Set as error to stop processing or waiting for connection */
                    }
                }
            }
        }
    } else if (is_error && IS_CURR_CMD(ESP_CMD_TCPIP_CIPSTART)) {
        esp_conn_t* conn = &esp.conns[esp.msg->msg.conn_start.num];
        /* TODO: Get last connection number we used to start the connection */
        esp.cb.type = ESP_CB_CONN_ERROR;        /* Connection just active */
        esp.cb.cb.conn_error.host = esp.msg->msg.conn_start.host;
        esp.cb.cb.conn_error.port = esp.msg->msg.conn_start.port;
        esp.cb.cb.conn_error.type = esp.msg->msg.conn_start.type;
        espi_send_conn_cb(conn);                /* Send event */
    }
    
    /**
     * In case of any of these events, simply release semaphore
     * and proceed with next command
     */
    if (is_ok || is_error || is_ready) {
        espr_t res = espOK;
        if (esp.msg) {                          /* Do we have active message? */
            res = espi_process_sub_cmd(esp.msg, is_ok, is_error, is_ready);
            if (res != espCONT) {
                if (is_ok || is_ready) {        /* Check ready or ok status */
                    res = esp.msg->res = espOK;
                } else {                        /* Or error status */
                    res = esp.msg->res = espERR;
                }
            } else {
                esp.msg->i++;                   /* Number of continue calls */
            }
        }
        if (res != espCONT) {                   /* Do we have to continue to wait for command? */
            esp_sys_sem_release(&esp.sem_sync); /* Release semaphore */
        }
    }
}

/**
 * \brief           Process input data received from ESP device
 */
espr_t
espi_process(void) {
    uint8_t ch;
    static uint8_t ch_prev1, ch_prev2;
    static esp_unicode_t unicode;
    
    while (esp_buff_read(&esp.buff, &ch, 1)) {  /* Read entire set of characters from buffer */
        /**
         * First check if we are in IPD mode and process plain data
         * without checking for valid ASCII or unicode format
         */
        if (esp.ipd.read) {                     /* Do we have to read incoming IPD data? */
            size_t len;
            
            if (esp.ipd.buff) {                 /* Do we have active buffer? */
                esp.ipd.buff->payload[esp.ipd.buff_ptr] = ch;   /* Save data character */
            }
            esp.ipd.buff_ptr++;
            esp.ipd.rem_len--;
            
            /**
             * Try to read more data directly from buffer
             */
            len = ESP_MIN(esp.ipd.rem_len, esp.ipd.buff ? (esp.ipd.buff->len - esp.ipd.buff_ptr) : esp.ipd.rem_len);
            ESP_DEBUGF(ESP_DBG_IPD, "IPD: New length: %d bytes\r\n", (int)len);
            if (len) {
                if (esp.ipd.buff) {             /* Is buffer valid? */
                    len = esp_buff_read(&esp.buff, &esp.ipd.buff->payload[esp.ipd.buff_ptr], len);
                    ESP_DEBUGF(ESP_DBG_IPD, "IPD: Bytes read: %d\r\n", (int)len);
                } else {                        /* Simply skip the data in buffer */
                    len = esp_buff_skip(&esp.buff, len);
                    ESP_DEBUGF(ESP_DBG_IPD, "IPD: Bytes skipped: %d\r\n", (int)len);
                }
            }
            if (len) {                          /* Check if we did read anything more */
                esp.ipd.buff_ptr += len;
                esp.ipd.rem_len -= len;
            }
            
            /**
             * Did we reach end of buffer or no more data?
             */
            if (!esp.ipd.rem_len || (esp.ipd.buff && esp.ipd.buff_ptr == esp.ipd.buff->len)) {
                espr_t res = espOK;
                
                /**
                 * Call user callback function with received data
                 */
                if (esp.ipd.buff) {             /* Do we have valid buffer? */
                    esp.cb.type = ESP_CB_DATA_RECV; /* We have received data */
                    esp.cb.cb.conn_data_recv.buff = esp.ipd.buff;
                    esp.cb.cb.conn_data_recv.conn = esp.ipd.conn;
                    res = espi_send_conn_cb(esp.ipd.conn);  /* Send connection callback */
                    
                    ESP_DEBUGF(ESP_DBG_IPD, "IPD: Free packet buffer\r\n");
                    esp_pbuf_free(esp.ipd.buff);    /* Free packet buffer */
                    if (res == espOKIGNOREMORE) {   /* We should ignore more data */
                        ESP_DEBUGF(ESP_DBG_IPD, "IPD: Ignoring more data from this IPD if available\r\n");
                        esp.ipd.buff = NULL;    /* Set to NULL to ignore more data if possibly available */
                    }
                        
                    if (esp.ipd.buff && esp.ipd.rem_len) {  /* Anything more to read? */
                        size_t new_len = ESP_MIN(esp.ipd.rem_len, ESP_IPD_MAX_BUFF_SIZE);   /* Calculate new buffer length */
                        ESP_DEBUGF(ESP_DBG_IPD, "IPD: Allocating new packet buffer of size: %d bytes\r\n", (int)new_len);
                        esp.ipd.buff = esp_pbuf_new(new_len);   /* Allocate new packet buffer */
                        ESP_DEBUGW(ESP_DBG_IPD, esp.ipd.buff == NULL, "IPD: Buffer allocation failed for %d bytes\r\n", (int)new_len);
                        if (esp.ipd.buff) {
                            esp_pbuf_set_ip(esp.ipd.buff, esp.ipd.ip, esp.ipd.port);    /* Set IP and port for received data */
                        }
                    }
                }
                if (!esp.ipd.rem_len) {         /* Check if we read everything */
                    esp.ipd.read = 0;           /* Stop reading data */
                }
                esp.ipd.buff_ptr = 0;           /* Reset input buffer pointer */
            }
            
        /**
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
                            espi_parse_received(&recv); /* Parse received string */
                            RECV_RESET();       /* Reset received string */
                            break;
                        default:
                            RECV_ADD(ch);       /* Any ASCII valid character */
                            break;
                    }
                    
                    /**
                     * If we are waiting for "\n> " sequence when CIPSEND command is active
                     */
                    if (IS_CURR_CMD(ESP_CMD_TCPIP_CIPSEND)) {
                        if (ch_prev2 == '\n' && ch_prev1 == '>' && ch == ' ') {
                            RECV_RESET();       /* Reset received object */
                            
                            /**
                             * Now actually send the data prepared before
                             */
                            ESP_AT_PORT_SEND(esp.msg->msg.conn_send.data, esp.msg->msg.conn_send.sent);
                            esp.msg->msg.conn_send.wait_send_ok_err = 1;    /* Now we are waiting for "SEND OK" or "SEND ERROR" */
                        }
                    }
                    
                    /**
                     * Check if "+IPD" statement is in array and now we received colon,
                     * indicating end of +IPD and start of actual data
                     */
                    if (ch == ':' && RECV_LEN() > 4 && RECV_IDX(0) == '+' && !strncmp(recv.data, "+IPD", 4)) {
                        espi_parse_received(&recv); /* Parse received string */
                        if (esp.ipd.read) {     /* Are we going into read mode? */
                            size_t len;
                            ESP_DEBUGF(ESP_DBG_IPD, "IPD: Data on connection %d with total size %d byte(s)\r\n", (int)esp.ipd.conn->num, esp.ipd.tot_len);
                            
                            len = ESP_MIN(esp.ipd.rem_len, ESP_IPD_MAX_BUFF_SIZE);
                            if (esp.ipd.conn->status.f.active) {    /* If connection is not active, doesn't make sense to read anything */
                                esp.ipd.buff = esp_pbuf_new(len);   /* Allocate new packet buffer */
                                if (esp.ipd.buff) {
                                    esp_pbuf_set_ip(esp.ipd.buff, esp.ipd.ip, esp.ipd.port);    /* Set IP and port for received data */
                                }
                                ESP_DEBUGW(ESP_DBG_IPD, esp.ipd.buff == NULL, "IPD: Buffer allocation failed for %d byte(s)\r\n", (int)len);
                            } else {
                                esp.ipd.buff = NULL;    /* Ignore reading on closed connection */
                                ESP_DEBUGF(ESP_DBG_IPD, "IPD: Connection %d already closed, skipping %d byte(s)\r\n", esp.ipd.conn->num, (int)len);
                            }
                            esp.ipd.conn->status.f.data_received = 1;   /* We have first received data */
                        }
                        esp.ipd.buff_ptr = 0;   /* Reset buffer write pointer */
                        RECV_RESET();           /* Reset received buffer */
                    }
                } else {                        /* We have sequence of unicode characters */
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
    if (msg->cmd_def == ESP_CMD_WIFI_CWJAP) {   /* Is our intention to join to access point? */
        if (msg->cmd == ESP_CMD_WIFI_CWJAP) {   /* Is the current command join? */
            if (is_ok) {                        /* Did we join successfully? */
                msg->cmd = ESP_CMD_WIFI_CIPSTA_GET; /* Go to next command to get IP address */
                espi_initiate_cmd(msg);         /* Start command */
                return espCONT;                 /* Return to continue and not to stop command */
            }
        } else if (msg->cmd == ESP_CMD_WIFI_CIPSTA_GET) {
            if (is_ok) {
                msg->cmd = ESP_CMD_WIFI_CIPSTAMAC_GET;  /* Go to next command to get IP address */
                espi_initiate_cmd(msg);         /* Start command */
                return espCONT;                 /* Return to continue and not to stop command */
            }
        }
    } else if (msg->cmd_def == ESP_CMD_TCPIP_CIPSTART) {    /* Is our intention to join to access point? */
        if (msg->i == 0 && msg->cmd == ESP_CMD_TCPIP_CIPSTATUS) {   /* Was the current command status info? */
            if (is_ok) {
                espr_t res;
                msg->cmd = ESP_CMD_TCPIP_CIPSTART;  /* Now actually start connection */
                res = espi_initiate_cmd(msg);   /* Start connection */
                if (res == espOK) {
                    return espCONT;
                } else {
                    return espERR;
                }
            }
        } else if (msg->i == 1 && msg->cmd == ESP_CMD_TCPIP_CIPSTART) {
            msg->cmd = ESP_CMD_TCPIP_CIPSTATUS; /* Go to status mode */
            if (is_ok) {
                if (espi_initiate_cmd(msg) == espOK) {  /* Get connection status */
                    return espCONT;
                }
            }
        } else if (msg->i == 2 && msg->cmd == ESP_CMD_TCPIP_CIPSTATUS) {
            
        }
    } else if (msg->cmd_def == ESP_CMD_WIFI_CIPSTA_SET) {
        if (msg->i == 0 && msg->cmd == ESP_CMD_WIFI_CIPSTA_SET) {
            if (is_ok) {
                msg->cmd = ESP_CMD_WIFI_CIPSTA_GET;
                if (espi_initiate_cmd(msg) == espOK) {
                    return espCONT;
                }
            }
        }
    } else if (msg->cmd_def == ESP_CMD_RESET) { /* Device is in reset mode */
        esp_cmd_t n_cmd = ESP_CMD_IDLE;
        switch (msg->cmd) {
            case ESP_CMD_RESET: {
                n_cmd = ESP_CMD_WIFI_CWMODE;    /* Set Wifi mode */
                break;
            }
            case ESP_CMD_WIFI_CWMODE: {
                n_cmd = ESP_CMD_TCPIP_CIPMUX;   /* Set multiple connections mode */
                break;
            }
            case ESP_CMD_TCPIP_CIPMUX: {
                n_cmd = ESP_CMD_TCPIP_CIPDINFO; /* Set data info */
                break;
            }
            case ESP_CMD_TCPIP_CIPDINFO: {
                n_cmd = ESP_CMD_TCPIP_CIPSTATUS;    /* Get connection status */
                break;
            }
            default: break;
        }
        if (n_cmd != ESP_CMD_IDLE) {            /* Is there a change of command? */
            msg->cmd = n_cmd;
            if (espi_initiate_cmd(msg) == espOK) {  /* Try to start with new connection */
                return espCONT;
            }
        }
    }
    return is_ok || is_ready ? espOK : espERR;
}

/**
 * \brief           Function to initialize every AT command
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_initiate_cmd(esp_msg_t* msg) {
    switch (msg->cmd) {                         /* Check current message we want to send over AT */
        case ESP_CMD_RESET: {                   /* Reset MCU with AT commands */
            ESP_AT_PORT_SEND_STR("AT+RST\r\n");
            break;
        }
        case ESP_CMD_UART: {                    /* Change UART parameters for AT port */
            char str[8];
            number_to_str(msg->msg.uart.baudrate, str); /* Get string from number */
            ESP_AT_PORT_SEND_STR("AT+UART_CUR=");
            ESP_AT_PORT_SEND_STR(str);
            ESP_AT_PORT_SEND_STR(",8,1,0,0");
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        
        /**
         * WiFi related commands
         */
        
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
            if (msg->msg.sta_join.mac) {
                ESP_AT_PORT_SEND_STR(",");
                send_ip_mac(msg->msg.sta_join.mac, 0, 1);
            }
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_WIFI_CWQAP: {              /* Quit from access point */
            ESP_AT_PORT_SEND_STR("AT+CWQAP\r\n");
            break;
        }
        case ESP_CMD_WIFI_CWLAP: {              /* List access points */
            ESP_AT_PORT_SEND_STR("AT+CWLAP");
            if (msg->msg.ap_list.ssid) {        /* Do we want to filter by SSID? */   
                ESP_AT_PORT_SEND_STR("=");
                send_string(msg->msg.ap_list.ssid, 1, 1);
            }
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_WIFI_CWMODE: {             /* Set WIFI mode */
            esp_mode_t m;
            char c;
            
            if (msg->cmd_def == ESP_CMD_RESET) {/* Is this command part of reset sequence? */
                m = ESP_MODE_STA;               /* Set station mode */
            } else {
                m = msg->msg.wifi_mode.mode;    /* Set user defined mode */
            }
            c = (char)m + '0';                  /* Continue to ASCII mode */
    
            ESP_AT_PORT_SEND_STR("AT+CWMODE=");
            ESP_AT_PORT_SEND_CHR(&c);
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_WIFI_CIPSTA_GET:           /* Get station IP address */
        case ESP_CMD_WIFI_CIPAP_GET: {          /* Get access point IP address */
            ESP_AT_PORT_SEND_STR("AT+CIP");
            if (msg->cmd == ESP_CMD_WIFI_CIPSTA_GET) {
                ESP_AT_PORT_SEND_STR("STA");
            } else {
                ESP_AT_PORT_SEND_STR("AP");
            }
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_getip.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("?\r\n");
            break;
        }
        case ESP_CMD_WIFI_CIPSTAMAC_GET:        /* Get station MAC address */
        case ESP_CMD_WIFI_CIPAPMAC_GET: {       /* Get access point MAC address */
            ESP_AT_PORT_SEND_STR("AT+CIP");
            if (msg->cmd == ESP_CMD_WIFI_CIPSTAMAC_GET) {
                ESP_AT_PORT_SEND_STR("STA");
            } else {
                ESP_AT_PORT_SEND_STR("AP");
            }
            ESP_AT_PORT_SEND_STR("MAC");
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_getmac.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("?\r\n");
            break;
        }
        case ESP_CMD_WIFI_CIPSTA_SET:           /* Set station IP address */
        case ESP_CMD_WIFI_CIPAP_SET: {          /* Set access point IP address */
            ESP_AT_PORT_SEND_STR("AT+CIP");
            if (msg->cmd == ESP_CMD_WIFI_CIPSTA_SET) {
                ESP_AT_PORT_SEND_STR("STA");
            } else {
                ESP_AT_PORT_SEND_STR("AP");
            }
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_setip.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("=");
            send_ip_mac(msg->msg.sta_ap_setip.ip, 1, 1);    /* Send IP address */
            if (msg->msg.sta_ap_setip.gw) {     /* Is gateway set? */
                ESP_AT_PORT_SEND_STR(",");
                send_ip_mac(msg->msg.sta_ap_setip.gw, 1, 1);    /* Send gateway address */
                if (msg->msg.sta_ap_setip.nm) { /* Is netmask set ? */
                    ESP_AT_PORT_SEND_STR(",");
                    send_ip_mac(msg->msg.sta_ap_setip.nm, 1, 1);    /* Send netmask address */
                }
            }
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_WIFI_CIPSTAMAC_SET:        /* Set station MAC address */
        case ESP_CMD_WIFI_CIPAPMAC_SET: {       /* Set access point MAC address */
            ESP_AT_PORT_SEND_STR("AT+CIP");
            if (msg->cmd == ESP_CMD_WIFI_CIPSTAMAC_SET) {
                ESP_AT_PORT_SEND_STR("STA");
            } else {
                ESP_AT_PORT_SEND_STR("AP");
            }
            ESP_AT_PORT_SEND_STR("MAC");
            if (msg->cmd_def == msg->cmd && msg->msg.sta_ap_setmac.def) {
                ESP_AT_PORT_SEND_STR("_DEF");
            } else {
                ESP_AT_PORT_SEND_STR("_CUR");
            }
            ESP_AT_PORT_SEND_STR("=");
            send_ip_mac(msg->msg.sta_ap_setmac.mac, 0, 1);  /* Send MAC address */
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        
        /**
         * TCP/IP related commands
         */
        
        case ESP_CMD_TCPIP_CIPSERVER: {         /* Enable or disable a server */
            char str[6];
    
            ESP_AT_PORT_SEND_STR("AT+CIPSERVER=");
            if (msg->msg.tcpip_server.port) {   /* Do we have valid port? */
                number_to_str(msg->msg.tcpip_server.port, str);
                ESP_AT_PORT_SEND_STR("1,");
                ESP_AT_PORT_SEND_STR(str);
            } else {                            /* Disable server */
                ESP_AT_PORT_SEND_STR("0");
            }
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_TCPIP_CIPSTART: {          /* Start a new connection */
            int8_t i = 0;
            esp_conn_t* c = NULL;
            char str[6];
            msg->msg.conn_start.num = 0;        /* Reset to make sure default value is set */
            for (i = ESP_MAX_CONNS - 1; i >= 0; i--) {  /* Find available connection */
                if (!esp.conns[i].status.f.active || !(esp.active_conns & (1 << i))) {
                    c = &esp.conns[i];
                    c->num = i;
                    msg->msg.conn_start.num = i;/* Set connection number for message structure */
                    break;
                }
            }
            if (!c) {
                return espNOFREECONN;           /* We don't have available connection */
            }
            
            if (msg->msg.conn_start.conn) {     /* Is user interested about connection info? */
                *msg->msg.conn_start.conn = c;  /* Save connection for user */
            }
            
            ESP_AT_PORT_SEND_STR("AT+CIPSTART=");
            str[0] = c->num + '0';
            ESP_AT_PORT_SEND_CHR(str);
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
            number_to_str(msg->msg.conn_start.port, str);
            ESP_AT_PORT_SEND_STR(str);
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_TCPIP_CIPCLOSE: {          /* Close the connection */
            char ch;
            if (!esp_conn_is_active(msg->msg.conn_close.conn)) {
                return espERR;
            }
            ESP_AT_PORT_SEND_STR("AT+CIPCLOSE=");
            ch = msg->msg.conn_close.conn ? msg->msg.conn_close.conn->num + '0' : '5';
            ESP_AT_PORT_SEND_CHR(&ch);
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_TCPIP_CIPSEND: {           /* Send data to connection */
            return espi_tcpip_process_send_data();  /* Process send data */
        }
        case ESP_CMD_TCPIP_CIPSTATUS: {         /* Get status of device and all connections */
            esp.active_conns_last = esp.active_conns;   /* Save as last status */
            esp.active_conns = 0;               /* Reset new status before parsing starts */
            ESP_AT_PORT_SEND_STR("AT+CIPSTATUS\r\n");   /* Send command to AT port */
            break;
        }
        case ESP_CMD_TCPIP_CIPDINFO: {          /* Set info data on +IPD command */
            ESP_AT_PORT_SEND_STR("AT+CIPDINFO=");
            if (msg->cmd_def == ESP_CMD_RESET || msg->msg.tcpip_dinfo.info) {   /* In case of reset mode */
                ESP_AT_PORT_SEND_STR("1");
            } else {
                ESP_AT_PORT_SEND_STR("0");
            }
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        case ESP_CMD_TCPIP_CIPMUX: {            /* Set multiple connections */
            ESP_AT_PORT_SEND_STR("AT+CIPMUX=");
            if (msg->cmd_def == ESP_CMD_RESET || msg->msg.tcpip_mux.mux) {  /* If reset command is active, enable CIPMUX */
                ESP_AT_PORT_SEND_STR("1");
            } else {
                ESP_AT_PORT_SEND_STR("0");
            }
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
#if ESP_DNS || __DOXYGEN__
        case ESP_CMD_TCPIP_CIPDOMAIN: {         /* DNS function */
            ESP_AT_PORT_SEND_STR("AT+CIPDOMAIN=");
            send_string(msg->msg.dns_getbyhostname.host, 1, 1);
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
#endif /* ESP_DNS || __DOXYGEN */
#if ESP_PING || __DOXYGEN__
        case ESP_CMD_TCPIP_PING: {              /* Pinging hostname or IP address */
            ESP_AT_PORT_SEND_STR("AT+PING=");
            send_string(msg->msg.tcpip_ping.host, 1, 1);
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
#endif /* ESP_PING || __DOXYGEN */
        case ESP_CMD_TCPIP_CIPSSLSIZE: {        /* Set SSL size */
            char str[12];
            ESP_AT_PORT_SEND_STR("AT+CIPSSLSIZE=");
            number_to_str(msg->msg.tcpip_sslsize.size, str);
            ESP_AT_PORT_SEND_STR(str);
            ESP_AT_PORT_SEND_STR("\r\n");
            break;
        }
        
        default: 
            return espERR;                      /* Invalid command */
    }
    return espOK;                               /* Valid command */
}

/**
 * \brief           Checks if connection pointer has valid address
 * \param[in]       conn: Address to check if valid connection ptr
 * \return          1 on success, 0 otherwise
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
