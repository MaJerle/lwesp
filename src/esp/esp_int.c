/**
 * \file            esp_at.c
 * \brief           AT commands parsers
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
#define ESP_INTERNAL
#include "esp.h"
#include "esp_int.h"
#include "esp_parser.h"
#include "esp_ll.h"

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
    
uint8_t ipd_buff[1452];

/**
 * \brief           Escapes special characters and sends them directly to AT port
 * \param[in]       str: Input string to escape
 */
static void
escape_and_send(const char* str) {
    char special = '\\';
    
    while (*str) {                              /* Go through string */
        if (*str == ',' || *str == '"' || *str == '\\') {   /* Check for special character */    
            ESP_AT_PORT_SEND_CHR(&special);     /* Send special character */
        }
        ESP_AT_PORT_SEND_CHR(str++);            /* Send character */
    }
}

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
 * \brief           Process and send data from device buffer
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
espi_tcpip_process_send_data(void) {
    char str[6];
    str[0] = esp.msg->msg.conn_send.conn->num + '0';
    ESP_AT_PORT_SEND_STR("AT+CIPSEND=");
    ESP_AT_PORT_SEND_CHR(str);
    ESP_AT_PORT_SEND_STR(",");
    esp.msg->msg.conn_send.sent = esp.msg->msg.conn_send.btw > 2048 ? 2048 : esp.msg->msg.conn_send.btw;
    number_to_str(esp.msg->msg.conn_send.sent, str);
    ESP_AT_PORT_SEND_STR(str);
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
        *esp.msg->msg.conn_send.bw += esp.msg->msg.conn_send.sent;
        esp.msg->msg.conn_send.data += esp.msg->msg.conn_send.sent;
        esp.msg->msg.conn_send.tries = 0;
    } else {
        esp.msg->msg.conn_send.tries++;         /* Increase number of tries */
        if (esp.msg->msg.conn_send.tries == 3) {    /* In case we reached max number of retransmissions */
            return 1;                           /* Return 1 and indicate error */
        }
    }
    if (esp.msg->msg.conn_send.btw) {           /* Do we still have data to send? */
        espi_tcpip_process_send_data();         /* Send next packet */
        return 0;                               /* We still have data to send */
    }
    return 1;                                   /* Everything was sent, we can stop execution */
}

/**
 * \brief           Send connection callback
 * \note            Before calling function, callback structure must be prepared
 * \param[in]       conn: Pointer to connection to use as callback
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
espi_send_conn_cb(esp_conn_t* conn) {
    esp.cb_func(&esp.cb);                       /* Send callback function */
    return espOK;
}

/**
 * \brief           Process received string from ESP 
 * \param[in]       recv: Pointer to \ref esp_rect_t structure with input string
 */
static void
espi_parse_received(esp_recv_t* rcv) {
    uint8_t is_ok = 0, is_error = 0, is_ready = 0;
    
    //printf("Rcv! AC: %d, s: %s\r\n", (int)esp.cmd, (char *)rcv->data);
    
    /**
     * Detect most common responses from device
     */
    is_ok = !strcmp(rcv->data, "OK\r\n");       /* Check if received string is OK */
    if (!is_ok) {
        is_error = !strcmp(rcv->data, "ERROR\r\n"); /* Check if received string is error */
        if (!is_error) {
            is_ready = !strcmp(rcv->data, "ready\r\n"); /* Check if received string is ready */
            if (is_ready) {
                esp.status.f.r_rdy = 1;
            }
        } else {
            esp.status.f.r_err = 1;
        }
    } else {
        esp.status.f.r_ok = 1;
    }
    
    /**
     * Read and process statements starting with '+' character
     */
    if (rcv->data[0] == '+') {
        if (!strncmp("+IPD", rcv->data, 4)) {   /* Check received network data */
            espi_parse_ipd(rcv->data + 5);      /* Parse IPD statement and start receiving network data */
        }
    }
    
    /**
     * Start processing received data
     */
    if (esp.cmd == ESP_CMD_RESET && is_ok) {    /* Check for reset command */
        is_ok = 0;                              /* We must wait for "ready", not only "OK" */
    } else if (esp.cmd == ESP_CMD_TCPIP_CIPSTATUS) {
        if (!strncmp(rcv->data, "+CIPSTATUS", 10)) {
            espi_parse_cipstatus(rcv->data + 11);   /* Parse CIPSTATUS response */
        } else if (is_ok) {
            uint8_t i;
            for (i = 0; i < ESP_MAX_CONNS; i++) {   /* Set current connection statuses */
                esp.conns[i].status.f.active = !!(esp.active_conns & (1 << i));
            }
        }
    } else if (esp.cmd == ESP_CMD_TCPIP_CIPSEND) {
        if (is_ok) {                            /* Check for OK and clear as we have to check for "> " statement */
            is_ok = 0;                          /* Do not reach on OK */
        }
        if (!strncmp("SEND OK", rcv->data, 7)) {    /* Data were sent successfully */
            is_ok = espi_tcpip_process_data_sent(1);    /* Process as data were sent */
            if (is_ok) {
                esp.cb.type = ESP_CB_DATA_SENT; /* Data were fully sent */
                esp.cb.cb.conn_data_sent.conn = esp.msg->msg.conn_send.conn;
                espi_send_conn_cb(esp.ipd.conn);/* Send connection callback */
            }
        } else if (!strncmp("SEND FAIL", rcv->data, 9)) {
            is_error = espi_tcpip_process_data_sent(0); /* Data were not sent */
            if (is_error) {
                esp.cb.type = ESP_CB_DATA_SENT_ERR; /* Error sending data */
                esp.cb.cb.conn_data_sent_err.conn = esp.msg->msg.conn_send.conn;
                espi_send_conn_cb(esp.ipd.conn);/* Send connection callback */
            }
        }
    } else if (esp.cmd == ESP_CMD_UART) {       /* In case of UART command */
        if (is_ok) {                            /* We have valid OK result */
            //esp_ll_init(&esp.ll, esp.msg->msg.uart.baudrate);   /* Set new baudrate */
        }
    }
    
    /**
     * Check if connection is just active or just closed
     */
    if (!strncmp(",CONNECT", &rcv->data[1], 8)) {
        const char* tmp = rcv->data;
        uint8_t num = espi_parse_number(&tmp);
        if (num < ESP_MAX_CONNS) {
            esp_conn_t* conn = &esp.conns[num]; /* Parse received data */
            conn->num = num;                    /* Set connection number */
            conn->status.f.active = 1;          /* Connection just active */
            
            esp.cb.type = ESP_CB_CONN_ACTIVE;   /* Connection just active */
            esp.cb.cb.conn_active_closed.conn = conn;   /* Set connection */
            espi_send_conn_cb(conn);            /* Send event */
        }
    } else if (!strncmp(",CLOSED", &rcv->data[1], 7)) {
        const char* tmp = rcv->data;
        uint8_t num = espi_parse_number(&tmp);
        if (num < ESP_MAX_CONNS) {
            esp_conn_t* conn = &esp.conns[num]; /* Parse received data */
            conn->num = num;                    /* Set connection number */
            conn->status.f.active = 0;          /* Connection was just closed */
            
            esp.cb.type = ESP_CB_CONN_CLOSED;   /* Connection just active */
            esp.cb.cb.conn_active_closed.conn = conn;   /* Set connection */
            espi_send_conn_cb(conn);            /* Send event */
        }
    }
    
    /**
     * In case of any of these events, simply release semaphore
     * and proceed with next command
     */
    if (is_ok || is_error || is_ready) {
        if (esp.msg) {                          /* Do we have active message? */
            if (is_ok || is_ready) {            /* Check ready or ok status */
                esp.msg->res = espOK;
            } else {                            /* Or error status */
                esp.msg->res = espERR;
            }
        }
        esp_sys_sem_release(&esp.sem_sync);     /* Release semaphore */
    }
}

/**
 * \brief           Process input data received from ESP device
 */
espr_t
espi_process(void) {
    uint8_t ch;
    size_t len;
    static uint8_t ch_prev1, ch_prev2;
    
    while (esp_buff_read(&esp.buff, &ch, 1)) {  /* Read entire set of characters from buffer */
        if (esp.ipd.read) {                     /* Do we have to read incoming IPD data? */
            esp.ipd.buff[esp.ipd.buff_ptr] = ch;/* Save data character */
            esp.ipd.buff_ptr++;
            esp.ipd.rem_len--;
            
            /**
             * Try to read more data directly from buffer
             */
            len = esp_buff_read(&esp.buff, &esp.ipd.buff[esp.ipd.buff_ptr], ESP_MIN(esp.ipd.rem_len, esp.ipd.buff_len - esp.ipd.buff_ptr));
            if (len) {                          /* Check if we did read anything more */
                esp.ipd.buff_ptr += len;
                esp.ipd.rem_len -= len;
            }
            
            /**
             * Did we reach end of buffer or no more data*
             */
            if (!esp.ipd.rem_len || esp.ipd.buff_ptr == esp.ipd.buff_len) {
                /**
                 * Call user callback function with received data
                 */
                esp.cb.type = ESP_CB_DATA_RECV; /* We have received data */
                esp.cb.cb.conn_data_recv.buff = esp.ipd.buff;
                esp.cb.cb.conn_data_recv.len = esp.ipd.buff_ptr;
                esp.cb.cb.conn_data_recv.conn = esp.ipd.conn;
                espi_send_conn_cb(esp.ipd.conn);/* Send connection callback */
                
                if (!esp.ipd.rem_len) {         /* Check if we read everything */
                    esp.ipd.read = 0;           /* Stop reading data */
                }
                esp.ipd.buff_ptr = 0;           /* Reset input buffer pointer */
            }
        } else if (ESP_ISVALIDASCII(ch)) {      /* Check for ASCII characters only */
            switch (ch) {
                case '\n':
                    RECV_ADD(ch);               /* Add character to input buffer */
                    espi_parse_received(&recv); /* Parse received string */
                    RECV_RESET();               /* Reset received string */
                    break;
                default:
                    RECV_ADD(ch);               /* Any ASCII valid character */
                    break;
            }
            
            /**
             * If we are waiting for "\n> " sequence when CIPSEND command is active
             */
            if (esp.msg && esp.msg->cmd == ESP_CMD_TCPIP_CIPSEND) {
                if (ch_prev2 == '\n' && ch_prev1 == '>' && ch == ' ') {
                    RECV_RESET();               /* Reset received object */
                    
                    /**
                     * Now actually send the data prepared before
                     */
                    ESP_AT_PORT_SEND(esp.msg->msg.conn_send.data, esp.msg->msg.conn_send.sent);
                }
            }
            
            /**
             * Check if "+IPD" statement is in array and now we received colon,
             * indicating end of +IPD and start of actual data
             */
            if (ch == ':' && RECV_LEN() > 4 && RECV_IDX(0) == '+' && !strncmp(recv.data, "+IPD", 4)) {
                espi_parse_received(&recv);     /* Parse received string */
                esp.ipd.buff = ipd_buff;
                esp.ipd.buff_len = sizeof(ipd_buff);
                esp.ipd.buff_ptr = 0;
                RECV_RESET();                   /* Reset received buffer */
            }
        } else {
            RECV_RESET();                       /* Invalidate received data */
        }
        
        ch_prev2 = ch_prev1;                    /* Save previous character to previous previous */
        ch_prev1 = ch;                          /* Char current to previous */
    }
    return espOK;
}

/**
 * \brief           Send AT command to reset device
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_reset(esp_msg_t* msg) {
    ESP_AT_PORT_SEND_STR("AT+RST\r\n");         /* Send command to AT port */
    return espOK;
}

/**
 * \brief           Send AT command to set AT port baudrate
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_basic_uart(esp_msg_t* msg) {
    char str[8];
    number_to_str(msg->msg.uart.baudrate, str); /* Get string from number */
    ESP_AT_PORT_SEND_STR("AT+UART_DEF=");
    ESP_AT_PORT_SEND_STR(str);
    ESP_AT_PORT_SEND_STR(",8,1,0,0");
    ESP_AT_PORT_SEND_STR("\r\n");
    
    return espOK;
}

/**
 * \brief           Miscellaneous AT commands in TCP/IP part of commands
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_tcpip_misc(esp_msg_t* msg) {
    if (msg->cmd == ESP_CMD_TCPIP_CIPDINFO) {
        ESP_AT_PORT_SEND_STR("AT+CIPDINFO=");
        if (msg->msg.tcpip_dinfo.info) {
            ESP_AT_PORT_SEND_STR("1");
        } else {
            ESP_AT_PORT_SEND_STR("0");
        }
        ESP_AT_PORT_SEND_STR("\r\n");
    } else if (msg->cmd == ESP_CMD_TCPIP_CIPMUX) {
        ESP_AT_PORT_SEND_STR("AT+CIPMUX=");
        if (msg->msg.tcpip_mux.mux) {
            ESP_AT_PORT_SEND_STR("1");
        } else {
            ESP_AT_PORT_SEND_STR("0");
        }
        ESP_AT_PORT_SEND_STR("\r\n");
    }
    return espOK;
}

/**
 * \brief           Send AT command to set WiFi mode
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_set_wifi_mode(esp_msg_t* msg) {
    char c = (char)msg->msg.wifi_mode.mode + '0';   /* Go to ASCII */
    
    ESP_AT_PORT_SEND_STR("AT+CWMODE=");
    ESP_AT_PORT_SEND_CHR(&c);
    ESP_AT_PORT_SEND_STR("\r\n");
    return espOK;
}

/**
 * \brief           Send AT command to set server
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_tcpip_server(esp_msg_t* msg) {
    char str[6];
    
    ESP_AT_PORT_SEND_STR("AT+CIPSERVER=");
    if (msg->msg.tcpip_server.port) {           /* Do we have valid port? */
        number_to_str(msg->msg.tcpip_server.port, str);
        ESP_AT_PORT_SEND_STR("1,");
        ESP_AT_PORT_SEND_STR(str);
    } else {                                    /* Disable server */
        ESP_AT_PORT_SEND_STR("0");
    }
    ESP_AT_PORT_SEND_STR("\r\n");
    return espOK;
}

/**
 * \brief           Send AT command to get connections status
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_get_conns_status(esp_msg_t* msg) {
    esp.active_conns_last = esp.active_conns;   /* Save as last status */
    esp.active_conns = 0;                       /* Reset new status before parsing starts */
    ESP_AT_PORT_SEND_STR("AT+CIPSTATUS\r\n");   /* Send command to AT port */
    return espOK;
}

/**
 * \brief           Connects to or disconnects from access point
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_sta_join_quit(esp_msg_t* msg) {
    if (msg->cmd == ESP_CMD_WIFI_CWJAP) {       /* In case we want to join */
        ESP_AT_PORT_SEND_STR("AT+CWJAP_");      /* Send command to AT port */
        if (msg->def) {
            ESP_AT_PORT_SEND_STR("DEF=\"");
        } else {
            ESP_AT_PORT_SEND_STR("CUR=\"");
        }
        escape_and_send(msg->msg.sta_join.name);
        ESP_AT_PORT_SEND_STR("\",\"");
        escape_and_send(msg->msg.sta_join.pass);
        ESP_AT_PORT_SEND_STR("\"");
        if (msg->msg.sta_join.mac) {
            uint8_t ch = ':', i;
            char s[3];
            ESP_AT_PORT_SEND_STR(",\"");
            for (i = 0; i < 6; i++) {
                byte_to_str(msg->msg.sta_join.mac[i], s);
                ESP_AT_PORT_SEND_STR(s);
                if (i != 5) {
                    ESP_AT_PORT_SEND_CHR(&ch);
                }
            }
            ESP_AT_PORT_SEND_STR("\"");
        }
        ESP_AT_PORT_SEND_STR("\r\n");
    } else {
        ESP_AT_PORT_SEND_STR("AT+CWQAP\r\n");   /* Quit from access point */ 
    }

    return espOK;
}

/**
 * \brief           Deal with CIPSTA and CIPAP commands for IP and MAC addresses
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_cip_sta_ap_cmd(esp_msg_t* msg) {
    if (msg->cmd == ESP_CMD_WIFI_CIPSTA_GET) {  /* Get IP address of station */
        ESP_AT_PORT_SEND_STR("AT+CIPSTA_CUR?\r\n");
    } else if (msg->cmd == ESP_CMD_WIFI_CIPAP_GET) {/* Get IP address of access point */
        ESP_AT_PORT_SEND_STR("AT+CIPAP_CUR?\r\n");
    } else if (msg->cmd == ESP_CMD_WIFI_CIPSTAMAC_GET) {/* Get MAC address of station */
        ESP_AT_PORT_SEND_STR("AT+CIPSTAMAC_CUR?\r\n");
    } else if (msg->cmd == ESP_CMD_WIFI_CIPAPMAC_GET) { /* Get MAC address of access point */
        ESP_AT_PORT_SEND_STR("AT+CIPSTAMAC_CUR?\r\n");
    }
    return espOK;
}

/**
 * \brief           Process everything related with connection
 * \note            Start, send data and close operations are performed here
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_tcpip_conn(esp_msg_t* msg) {
    int8_t i = 0;
    esp_conn_t* c = NULL;
    char str[6];
    
    if (msg->cmd == ESP_CMD_TCPIP_CIPSTART) {   /* In case user wants to start a new connection */
        for (i = ESP_MAX_CONNS - 1; i >= 0; i--) {  /* Find available connection */
            if (!esp.conns[i].status.f.active || !(esp.active_conns & (1 << i))) {
                c = &esp.conns[i];
                c->num = i;
                break;
            }
        }
        if (!c) {
            return espERR;
        }
        c->status.f.client = 1;                 /* Set as client mode */
        *msg->msg.conn_start.conn = c;          /* Save connection for user */
        
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
        
        esp_get_conns_status(0);                /* Get connection statuses after */
    } else if (msg->cmd == ESP_CMD_TCPIP_CIPCLOSE) {
        ESP_AT_PORT_SEND_STR("AT+CIPCLOSE=");
        str[0] = msg->msg.conn_close.conn ? msg->msg.conn_close.conn->num + '0' : '5';
        ESP_AT_PORT_SEND_CHR(str);
        ESP_AT_PORT_SEND_STR("\r\n");
    } else if (msg->cmd == ESP_CMD_TCPIP_CIPSEND) {
        espi_tcpip_process_send_data();         /* Process send data */
    }
    return espOK;
}
