/**
 * \file            esp_mqtt_client.c
 * \brief           MQTT client
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
#include "apps/esp_mqtt_client.h"
#include "esp/esp_mem.h"
#include "esp/esp_pbuf.h"

/**
 * \addtogroup      ESP_APP_MQTT_CLIENT MQTT Client
 * \{
 *
 * MQTT client app uses MQTT-3.1.1 protocol versions.
 *
 * List of full specs is available <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">here</a>.
 *
 * \}
 */

#ifndef ESP_DBG_MQTT
#define ESP_DBG_MQTT                        ESP_DBG_OFF
#endif /* ESP_DBG_MQTT */

static espr_t   mqtt_conn_cb(esp_cb_t* cb);
static void     send_data(mqtt_client_t* client);

/**
 * \brief           List of MQTT message types
 */
typedef enum {
    MQTT_MSG_TYPE_CONNECT =     0x01,           /*!< Client requests a connection to a server */
    MQTT_MSG_TYPE_CONNACK =     0x02,           /*!< Acknowledge connection request */
    MQTT_MSG_TYPE_PUBLISH =     0x03,           /*!< Publish message */
    MQTT_MSG_TYPE_PUBACK =      0x04,           /*!< Publish acknowledgement */
    MQTT_MSG_TYPE_PUBREC =      0x05,           /*!< Public received */
    MQTT_MSG_TYPE_PUBREL =      0x06,           /*!< Publish release */
    MQTT_MSG_TYPE_PUBCOMP =     0x07,           /*!< Publish complete */
    MQTT_MSG_TYPE_SUBSCRIBE =   0x08,           /*!< Subscribe to topics */
    MQTT_MSG_TYPE_SUBACK =      0x09,           /*!< Subscribe acknowledgement */
    MQTT_MSG_TYPE_UNSUBSCRIBE = 0x0A,           /*!< Unsubscribe from topics */
    MQTT_MSG_TYPE_UNSUBACK =    0x0B,           /*!< Unsubscribe acknowledgement */
    MQTT_MSG_TYPE_PINGREQ =     0x0C,           /*!< Ping request */
    MQTT_MSG_TYPE_PINGRESP =    0x0D,           /*!< Ping response */
    MQTT_MSG_TYPE_DISCONNECT =  0x0E,           /*!< Disconnect notification */
} mqtt_msg_type_t;

/**
 * \brief           State of MQTT client
 */
typedef enum {
    MQTT_CONN_DISCONNECTED,                     /*!< Connection with server is not established */
    MQTT_CONN_CONNECTING,                       /*!< Client is connecting to server */
    MQTT_CONNECTING,                            /*!< MQTT client is connecting... CONNECT command has been sent to server */
    MQTT_CONNECTED,                             /*!< MQTT is fully connected and ready to send data on topics */
} mqtt_state_t;

/** List of flags for CONNECT message type */
#define MQTT_FLAG_CONNECT_USERNAME      0x80    /*!< Packet contains username */
#define MQTT_FLAG_CONNECT_PASSWORD      0x40    /*!< Packet contains password */
#define MQTT_FLAG_CONNECT_WILL_RETAIN   0x20    /*!< Will retain is enabled */
#define MQTT_FLAG_CONNECT_WILL          0x04    /*!< Packet contains will topic and will message */
#define MQTT_FLAG_CONNECT_CLEAN_SESSION 0x02    /*!< Start with clean session of this client */

/** Parser states */
#define MQTT_PARSER_STATE_INIT          0x00    /*!< MQTT parser in initialized state */
#define MQTT_PARSER_STATE_CALC_REM_LEN  0x01    /*!< MQTT parser in calculating remaining length state */
#define MQTT_PARSER_STATE_READ_REM      0x02    /*!< MQTT parser in reading remaining bytes state */

/** Get packet type from incoming byte */
#define MQTT_RCV_GET_PACKET_TYPE(d)     ((mqtt_msg_type_t)((d >> 0x04) & 0x0F))
#define MQTT_RCV_GET_PACKET_QOS(d)      ((d >> 0x01) & 0x03)

/** Requests status */
#define MQTT_REQUEST_FLAG_IN_USE        0x01    /*!< Request object is allocated and in use */
#define MQTT_REQUEST_FLAG_PENDING       0x02    /*!< Request object is pending waiting for response from server */

static const char *
mqtt_msg_type_to_str(mqtt_msg_type_t msg_type) {
    static char str[10];
    sprintf(str, "%02X", (int)msg_type);
    return str;
}

/**
 * \brief           Default event callback function
 * \param[in]       evt: MQTT event
 */
static void
mqtt_evt_fn_default(mqtt_evt_t* evt) {
    ESP_UNUSED(evt);
}

/**
 * \brief           Create new message ID
 * \param[in]       client: MQTT client
 * \return          New packet ID
 */
static uint16_t
create_packet_id(mqtt_client_t* client) {
    client->last_packet_id++;
    if (client->last_packet_id == 0) {
        client->last_packet_id = 1;
    }
    return client->last_packet_id;
}

/**********************************/
/* MQTT requests helper functions */

/**
 * \brief           Create and return new request object
 * \param[in]       client: MQTT client
 * \param[in]       packet_id: Packet ID for QoS 1 or 2
 * \return          Pointer to new request ready to use or NULL if no available memory
 */
static mqtt_request_t *
request_create(mqtt_client_t* client, uint16_t packet_id) {
    mqtt_request_t* request = NULL;
    uint16_t i;
    
    /*
     * Try to find a new request which does not have IN_USE flag set
     */
    for (i = 0; i < MQTT_MAX_REQUESTS; i++) {
        if ((client->requests[i].status & MQTT_REQUEST_FLAG_IN_USE) == 0) {
            request = &client->requests[i];     /* We have empty request */
            break;
        }
    }
    if (request != NULL) {
        request->status = 0;                    /* Reset everything at this point */
    }
    return request;
}

/**
 * \brief           Delete request object and make it free
 * \param[in]       client: MQTT client
 * \param[in]       request: Request object to delete
 */
static void
request_delete(mqtt_client_t* client, mqtt_request_t* request) {
    request->status = 0;                        /* Reset status to make request unused */
}

/**
 * \brief           Set request as pending waiting for server reply
 * \param[in]       client: MQTT client
 * \param[in]       request: Request object to delete
 */
static void
request_set_pending(mqtt_client_t* client, mqtt_request_t* request) {
    request->timeout_start_time = esp_sys_now();/* Set timeout start time */
    request->status |= MQTT_REQUEST_FLAG_PENDING;   /* Set pending flag */
}

/**********************************/
/*  MQTT buffer helper functions  */

/**
 * \brief           Write a fixed header part of MQTT packet to output buffer
 * \param[in]       client: MQTT client
 * \param[in]       type: MQTT Message type
 * \param[in]       dup:
 * \param[in]       qos:
 * \param[in]       retain:
 * \param[in]       rem_len: Remaining packet length, excluding variable length part
 */
static void
write_fixed_header(mqtt_client_t* client, mqtt_msg_type_t type, uint8_t dup, uint8_t qos, uint8_t retain, uint16_t rem_len) {
    uint8_t b;
    
    b = (((uint8_t)type) << 4) | ((dup & 0x01) << 3) | ((qos & 0x03) << 1) | (retain & 0x01);
    esp_buff_write(&client->tx_buff, &b, 1);    /* Write start of packet parameters */
    
    while (rem_len) {                           /* Encode length */
        /*
         * Length if encoded LSB first up to 127 (0x7F) long,
         * where bit 7 indicates we have more data in queue
         */
        b = (rem_len & 0x7F) | (rem_len > 0x7F ? 0x80 : 0);
        esp_buff_write(&client->tx_buff, &b, 1);/* Write single byte */
        rem_len >>= 7;                          /* Go to next 127 bytes */
    }
}

static void
write_u8(mqtt_client_t* client, uint8_t num) {
    esp_buff_write(&client->tx_buff, &num, 1);  /* Write single byte */
}

static void
write_u16(mqtt_client_t* client, uint16_t num) {
    write_u8(client, num >> 8);                 /* Write MSB first... */
    write_u8(client, num & 0xFF);               /* ...followed by LSB */
}

static void
write_data(mqtt_client_t* client, const void* data, size_t len) {
    esp_buff_write(&client->tx_buff, data, len);/* Write raw data to buffer */
}

/**
 * \brief           Check if output buffer has enough memory to handle
 *                  all bytes required to encode packet to RAW format
 *
 *                  It calculates additional bytes required to encode
 *                  remaining length itself + 1 byte for packet header
 * \param[in]       client: MQTT client
 * \param[in]       rem_len: Remaining length of packet
 */
static uint8_t
output_check_enough_memory(mqtt_client_t* client, uint16_t rem_len) {
    uint16_t total_len = rem_len + 1;           /* Remaining length + first (packet start) byte */
    
    while (rem_len) {                           /* Calculate bytes for encoding remainig length itself */
        total_len++;
        rem_len >>= 7;                          /* Encoded with 7 bits per byte */
    }
    
    return esp_buff_get_free(&client->tx_buff) >= total_len;
}

/**
 * \brief           Write and send acknowledge/recor
 */
static uint8_t
write_ack_rec_rel_resp(mqtt_client_t* client, mqtt_msg_type_t msg_type, uint16_t pkt_id, uint8_t qos) {
    if (output_check_enough_memory(client, 2)) {/* Check if memory for response available in output queue */
        write_fixed_header(client, msg_type, 0, qos, 0, 2); /* Write fixed header with 2 more bytes for packet id */
        write_u16(client, pkt_id);              /* Write packet ID */
        send_data(client);                      /* Flush data to output */
    } else {
        ESP_DEBUGF(ESP_DBG_MQTT, "MQTT No memory to write ACK/REC/REL entries\r\n");
    }
    return 0;
}

/**
 * \brief           Write string to output buffer
 * \param[in]       client: MQTT client
 * \param[in]       str: String to write to buffer
 */
static void
write_string(mqtt_client_t* client, const char* str, uint16_t len) {
    write_u16(client, len);                     /* Write string length */
    esp_buff_write(&client->tx_buff, (const void *)str, len);   /* Write string to buffer */
}

/**
 * \brief           Send the actual data to the remote
 * \param[in]       client: MQTT client
 */
static void
send_data(mqtt_client_t* client) {
    const void* addr;
    if (client->is_sending) {                   /* We are currently sending data */
        return;
    }
    
    client->last_send_len = esp_buff_get_linear_block_length(&client->tx_buff); /* Get length of linear memory */
    if (client->last_send_len) {                /* Anything to send? */
        addr = esp_buff_get_linear_block_address(&client->tx_buff); /* Get address of linear memory */
        if (esp_conn_send(client->conn, addr, client->last_send_len, &client->last_sent_len, 0) == espOK) {
            client->is_sending = 1;             /* Remember active sending flag */
        }
    }
}

/**
 * \brief           Subscribe/Unsubscribe to/from MQTT topic
 * \param[in]       client: MQTT client
 * \param[in]       topic: MQTT topic to (un)subscribe
 * \param[in]       qos: Quality of service, used only on subscribe part
 * \param[in]       sub: Status set to 1 on subscribe or 0 on unsubscribe
 * \return          1 on success, 0 otherwise
 */
static uint8_t
sub_unsub(mqtt_client_t* client, const char* topic, uint8_t qos, uint8_t sub) {
    uint16_t len_topic, pkt_id;
    uint32_t rem_len;
    mqtt_request_t* request = NULL;
    
    len_topic = strlen(topic);                  /* Get length of topic */
    if (len_topic == 0) {
        return 0;
    }
    
    /*
     * Calculate remaining length of packet
     * 
     * rem_len = 2 (topic_len) + topic_len + 2 (pkt_id) + qos (if sub)
     */
    rem_len = 2 + len_topic + 2;
    if (sub) {
        rem_len++;
    }
    
    if (!output_check_enough_memory(client, rem_len)) { /* Check if enough memory to write packet data */
        return 0;
    }
    
    pkt_id = create_packet_id(client);          /* Create new packet ID */
    request = request_create(client, pkt_id);   /* Create request for packet */
    if (request == NULL) {
        return 0;
    }
    
    write_fixed_header(client, sub ? MQTT_MSG_TYPE_SUBSCRIBE : MQTT_MSG_TYPE_UNSUBSCRIBE, 0, 1, 0, rem_len);
    write_u16(client, pkt_id);                  /* Write packet ID */
    write_string(client, topic, len_topic);     /* Write topic string to packet */
    if (sub) {                                  /* Send quality of service only on subscribe */
        write_u8(client, ESP_MIN(qos, 2));      /* Write quality of service */
    }
    
    request_set_pending(client, request);       /* Set request as pending waiting for server reply */
    send_data(client);                          /* Try to send data */
    
    return 1;
}

/**********************************/
/* Connection callback functions  */

/**
 * \brief           Callback when we are connected to MQTT server
 * \param[in]       client: MQTT client
 */
static void
mqtt_connected_cb(mqtt_client_t* client) {
    uint8_t flags = 0;
    uint16_t rem_len;
    size_t len_id, len_user, len_pass, len_will_topic, len_will_message;

    flags |= MQTT_FLAG_CONNECT_CLEAN_SESSION;   /* Start as clean session */
    
    /*
     * Remaining length consist of fixed header data
     * variable header and possible data
     * 
     * Minimum length consists of 2 + "MQTT" (4) + protocol_level (1) + flags (1) + keep_alive (2)
     */
    rem_len = 10;                               /* Set remaining length of fixed header */
    
    len_id = strlen(client->info->id);          /* Get cliend ID length */
    rem_len += len_id + 2;                      /* Add client id length including length entries */
    
    if (client->info->will_topic != NULL && client->info->will_message != NULL) {
        flags |= MQTT_FLAG_CONNECT_WILL;
        flags |= (ESP_MIN(client->info->will_qos, 2)) << 0x03;  /* Set qos to flags */
        
        len_will_topic = strlen(client->info->will_topic);
        len_will_message = strlen(client->info->will_message);
        
        rem_len += len_will_topic + 2;          /* Add will topic parameter */
        rem_len += len_will_message + 2;        /* Add will message parameter */
    }
    
    if (client->info->user != NULL) {           /* Check for username */
        flags |= MQTT_FLAG_CONNECT_USERNAME;    /* Username is included */
        
        len_user = strlen(client->info->user);  /* Get username length */
        rem_len += len_user + 2;                /* Add username length including length entries */
    }
    
    if (client->info->pass != NULL) {           /* Check for password */
        flags |= MQTT_FLAG_CONNECT_PASSWORD;    /* Password is included */
        
        len_pass = strlen(client->info->pass);  /* Get username length */
        rem_len += len_pass + 2;                /* Add password length including length entries */
    }
    
    if (!output_check_enough_memory(client, rem_len)) { /* Is there enough memory to write everything? */
        return;
    }
    
    /*
     * Write everything to output buffer
     */
    write_fixed_header(client, MQTT_MSG_TYPE_CONNECT, 0, 0, 0, rem_len);
    write_string(client, "MQTT", 4);            /* Protocol name */
    write_u8(client, 4);                        /* Protocol version */
    write_u8(client, flags);                    /* Flags for CONNECT message */
    write_u16(client, client->info->keep_alive);/* Keep alive timeout in units of seconds */
    write_string(client, client->info->id, len_id); /* This is client ID string */
    if (flags & MQTT_FLAG_CONNECT_WILL) {       /* Check for will topic */
        write_string(client, client->info->will_topic, len_will_topic); /* Write topic to packet */
        write_string(client, client->info->will_message, len_will_message); /* Write message to packet */
    }
    if (flags & MQTT_FLAG_CONNECT_USERNAME) {   /* Check for username */
        write_string(client, client->info->user, len_user); /* Write username to packet */
    }
    if (flags & MQTT_FLAG_CONNECT_PASSWORD) {   /* Check for password */
        write_string(client, client->info->user, len_user); /* Write password to packet */
    }
    
    client->parser_state = MQTT_PARSER_STATE_INIT;  /* Reset parser state */

    send_data(client);                          /* Flush and send the actual data */
}

/**
 * \brief           Process incoming fully received message
 * \param[in]       client: MQTT client
 * \return          1 on success, 0 otherwise
 */
static uint8_t
mqtt_process_incoming_message(mqtt_client_t* client) {
    mqtt_msg_type_t msg_type;
    uint16_t pkt_id;
    uint8_t qos;
    msg_type = MQTT_RCV_GET_PACKET_TYPE(client->msg_hdr_byte);  /* Get packet type from message header byte */
    
    switch (msg_type) {
        case MQTT_MSG_TYPE_CONNACK: {
            uint8_t err;
            err = client->rx_buff[0];           /* Get result of connection */
            if (err == 0) {
                ESP_DEBUGF(ESP_DBG_MQTT, "MQTT Successfully connected and ready to subscribe to topics\r\n");
                client->evt.type = MQTT_EVT_CONNECTED;
                client->evt_fn(&client->evt);   /* Call user function */
            }
            break;
        }
        case MQTT_MSG_TYPE_SUBACK:
        case MQTT_MSG_TYPE_UNSUBACK: {
            pkt_id = client->rx_buff[0] << 8 | client->rx_buff[1];
            ESP_DEBUGF(ESP_DBG_MQTT, "MQTT (un)subscribe received: %d\r\n", (int)pkt_id);
            ESP_DEBUGF(ESP_DBG_MQTT, "MQTT (un)subscribe result: %d\r\n", (int)client->rx_buff[2]);
            
            client->evt.type = MQTT_EVT_SUBSCRIBED;
            client->evt_fn(&client->evt);   /* Call user function */
            break;
        }
        case MQTT_MSG_TYPE_PUBACK: {
            break;
        }
        case MQTT_MSG_TYPE_PUBLISH: {
            uint16_t topic_len, data_len;
            uint8_t* topic;
            uint8_t* data;
            
            qos = MQTT_RCV_GET_PACKET_QOS(client->msg_hdr_byte);    /* Get QoS from received packet */
            topic_len = client->rx_buff[0] << 8 | client->rx_buff[1];
            topic = &client->rx_buff[2];        /* Start of topic */
            
            data = topic + topic_len;           /* Get data pointer */
            
            /*
             * Packet ID is only available 
             * if quality of service is not 0
             */
            if (qos > 0) {
                pkt_id = client->rx_buff[2 + topic_len] << 8 | client->rx_buff[2 + topic_len + 1];  /* Get packet ID */
                data += 2;                      /* Increase pointer for 2 bytes */
            } else {
                pkt_id = 0;                     /* No packet ID */
            }
            data_len = client->msg_rem_len - (data - client->rx_buff);  /* Calculate length of remaining data */
            
            ESP_DEBUGF(ESP_DBG_MQTT,
                "MQTT publish packet received on topic %.*s; QoS: %d; pkt_id: %d; data_len: %d\r\n",
                topic_len, (const char *)topic, (int)qos, (int)pkt_id, (int)data_len);
            
            if (qos > 0) {                      /* We have to reply on QoS > 0 */
                mqtt_msg_type_t resp_msg_type = qos == 1 ? MQTT_MSG_TYPE_PUBACK : MQTT_MSG_TYPE_PUBREC;
                ESP_DEBUGF(ESP_DBG_MQTT, "MQTT sending publish resp: %s on pkt_id: %d\r\n",
                            mqtt_msg_type_to_str(resp_msg_type), (int)pkt_id);
                
                write_ack_rec_rel_resp(client, resp_msg_type, pkt_id, qos);
            }
            
            break;
        }
        default: 
            return 0;
    }
    return 1;
}

/**
 * \brief           Parse incoming raw data and try to construct clean packet from it
 * \param[in]       client: MQTT client
 * \param[in]       pbuf: Received packet buffer with data
 * \return          1 on success, 0 otherwise
 */
static uint8_t
mqtt_parse_incoming(mqtt_client_t* client, esp_pbuf_p pbuf) {
    size_t idx, tot_len, buff_len, buff_offset;
    const uint8_t* d;
    uint8_t ch;
    
    tot_len = esp_pbuf_length(pbuf, 1);         /* Get total length of packet buffer */
    ESP_DEBUGF(ESP_DBG_MQTT, "MQTT Data received len: %d\r\n", (int)tot_len);
    
    buff_offset = 0;
    buff_len = 0;
    do {
        buff_offset += buff_len;                /* Calculate new offset of buffer */
        d = esp_pbuf_get_linear_addr(pbuf, buff_offset, &buff_len); /* Get address pointer */
        
        idx = 0;
        while (d != NULL && idx < buff_len) {   /* Process entire linear buffer */
            ch = d[idx++];                      /* Get element */
            switch (client->parser_state) {     /* Check parser state */
                case MQTT_PARSER_STATE_INIT: {  /* We are waiting for start byte and packet type */                    
                    /* Save other info about message */
                    client->msg_hdr_byte = ch;  /* Save first entry */
                    
                    client->msg_rem_len = 0;    /* Reset remaining length */
                    client->msg_curr_pos = 0;   /* Reset current buffer write pointer */
                    
                    client->parser_state = MQTT_PARSER_STATE_CALC_REM_LEN;
                    break;
                }
                case MQTT_PARSER_STATE_CALC_REM_LEN: {  /* Calculate remaining length of packet */
                    client->msg_rem_len <<= 7;  /* Shift remaining length by 7 bits */
                    client->msg_rem_len |= (ch & 0x7F);
                    if ((ch & 0x80) == 0) {     /* Is this last entry? */
                        client->parser_state = MQTT_PARSER_STATE_READ_REM;
                    }
                    break;
                }
                case MQTT_PARSER_STATE_READ_REM: {  /* Read remaining bytes and write to RX buffer */
                    client->rx_buff[client->msg_curr_pos++] = ch;   /* Write received character */
                    
                    if (client->msg_curr_pos == client->msg_rem_len) {
                        mqtt_process_incoming_message(client);  /* Process incoming packet */
                        
                        client->parser_state = MQTT_PARSER_STATE_INIT;  /* Go to initial state and listen for next received packet */
                    }
                    break;
                }
                default:
                    client->parser_state = MQTT_PARSER_STATE_INIT;
            }
        }
    } while (buff_len);
    return 0;
}

/**
 * \brief           Received data callback function
 * \param[in]       client: MQTT client
 * \param[in]       pbuf: Received packet buffer with data
 * \return          1 on success, 0 otherwise
 */
static uint8_t
mqtt_data_recv_cb(mqtt_client_t* client, esp_pbuf_p pbuf) {
    
    mqtt_parse_incoming(client, pbuf);
    return 1;
}

/**
 * \brief           Data sent callback
 * \param[in]       client: MQTT client
 * \param[in]       successful: Send status. Set to 1 on success or 0 if send error occurred
 * \return          1 on success, 0 otherwise
 */
static uint8_t
mqtt_data_sent_cb(mqtt_client_t* client, uint8_t successful) {
    client->is_sending = 0;                     /* We are not sending anymore */

    if (successful) {                           /* Were data successfully sent? */
        esp_buff_skip(&client->tx_buff, client->last_sent_len); /* Skip buffer for actual skipped data */
    }
    send_data(client);                          /* Try to send more */
    return 1;
}

/**
 * \brief           Poll for client connection
 *                  Called every 500ms when MQTT client TCP connection is established
 * \param[in]       client: MQTT client
 * \return          1 on success, 0 otherwise
 */
static uint8_t
mqtt_poll(mqtt_client_t* client) {
    client->poll_time++;
    return 1;
}

/**
 * \brief           Connection callback
 * \param[in]       cb: Callback parameters
 * \result          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
mqtt_conn_cb(esp_cb_t* cb) {
    esp_conn_p conn;
    mqtt_client_t* client;
    
    conn = esp_conn_get_from_evt(cb);           /* Get connection from event */
    if (conn != NULL) {
        client = esp_conn_get_arg(conn);        /* Get client structure from connection */
        if (client == NULL) {
            esp_conn_close(conn, 0);            /* Force connection close immediatelly */
            return espERR;
        }
    } else {
        return espERR;
    }
    
    /*
     * Check and process events
     */
    switch (cb->type) {
        /*
         * Connection active to MQTT server
         */
        case ESP_CB_CONN_ACTIVE: {
            ESP_DEBUGF(ESP_DBG_MQTT, "MQTT: Connected to server\r\n");
            mqtt_connected_cb(client);          /* Call function to process status */
            break;
        }
        
        /*
         * A new packet of data received
         * on MQTT client connection
         */
        case ESP_CB_CONN_DATA_RECV: {
            ESP_DEBUGF(ESP_DBG_MQTT, "MQTT Data received\r\n");
            mqtt_data_recv_cb(client, cb->cb.conn_data_recv.buff);  /* Call user to process received data */
            break;
        }
        
        /*
         * Data were sent on MQTT client connection
         */
        case ESP_CB_CONN_DATA_SENT: {
            mqtt_data_sent_cb(client, 1);       /* Data sent callback */
            break;
        }
        
        /*
         * There was an error sending data to remote
         */
        case ESP_CB_CONN_DATA_SEND_ERR: {
            mqtt_data_sent_cb(client, 0);       /* Data sent error */
            break;
        }
        
        /*
         * Periodic poll for connection
         */
        case ESP_CB_CONN_POLL: {
            mqtt_poll(client);                  /* Poll client */
            break;
        }
        
        /*
         * Connection closed for some reason
         */
        case ESP_CB_CONN_CLOSED: {
            if (cb->cb.conn_active_closed.forced) {
                ESP_DEBUGF(ESP_DBG_MQTT, "MQTT connection closed by user\r\n");
            } else {
                ESP_DEBUGF(ESP_DBG_MQTT, "MQTT connection closed by remote server\r\n");
            }
            break;
        }
        default:
            break;
    }
    return espOK;
}

/**
 * \brief           Allocate a new MQTT client structure
 * \param[in]       buff_len: Length of raw data output buffer
 * \return          Pointer to new allocated MQTT client structure or NULL on failure
 */
mqtt_client_t *
mqtt_client_new(size_t tx_buff_len, size_t rx_buff_len) {
    mqtt_client_t* client;
    
    client = esp_mem_alloc(sizeof(*client));    /* Allocate memory for client structure */
    if (client) {
        memset(client, 0x00, sizeof(*client));  /* Reset memory */
        if (!esp_buff_init(&client->tx_buff, tx_buff_len)) {
            esp_mem_free(client);
            client = NULL;
        }
        if (client != NULL) {
            client->rx_buff_len = rx_buff_len;
            client->rx_buff = esp_mem_alloc(rx_buff_len);
            if (client->rx_buff == NULL) {
                esp_buff_free(&client->tx_buff);
                esp_mem_free(client);
                client = NULL;
            }
        }
    }
    return client;
}

/**
 * \brief           Connect to MQTT server
 * \param[in,out]   client: MQTT client structure
 * \param[in]       host: Host address for server
 * \param[in]       port: Host port number
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
mqtt_client_connect(mqtt_client_t* client, const char* host, uint16_t port, mqtt_evt_fn evt_fn, const mqtt_client_info_t* info) {
    ESP_ASSERT("client != NULL", client != NULL);   /* Assert input parameters */
    ESP_ASSERT("host != NULL", host != NULL);   /* Assert input parameters */
    ESP_ASSERT("port > 0", port > 0);           /* Assert input parameters */
    ESP_ASSERT("info != NULL", info != NULL);   /* Assert input parameters */
    
    client->info = info;                        /* Save client info parameters */
    client->evt_fn = evt_fn != NULL ? evt_fn : mqtt_evt_fn_default;
    return esp_conn_start(&client->conn, ESP_CONN_TYPE_TCP, host, port, client, mqtt_conn_cb, 0);
}

/**
 * \brief           Disconnect from MQTT server
 * \todo            Implement disconnect sequence
 * \param[in]       client: MQTT client
 * \return          espOK if request sent to queue or member of \ref espr_t otherwise
 */
espr_t
mqtt_client_disconnect(mqtt_client_t* client) {
    return esp_conn_close(client->conn, 0);     /* Close the connection in non-blocking mode */
}

espr_t
mqtt_client_subscribe(mqtt_client_t* client, const char* topic, uint8_t qos) {
    return sub_unsub(client, topic, qos, 1) == 1 ? espOK : espERR;  /* Subscribe to topic */
}

espr_t
mqtt_client_unsubscribe(mqtt_client_t* client, const char* topic) {
    return sub_unsub(client, topic, 0, 0) == 1 ? espOK : espERR;    /* Unsubscribe from topic */
}

espr_t
mqtt_client_publish(mqtt_client_t* client, const char* topic, const void* payload, uint16_t payload_len, uint8_t qos, uint8_t retain) {
    uint16_t len_topic, pkt_id;
    uint32_t rem_len;
    mqtt_request_t* request = NULL;
    
    len_topic = strlen(topic);                  /* Get length of topic */
    if (len_topic == 0) {
        return 0;
    }
    
    /*
     * Calculate remaining length of packet
     * 
     * rem_len = 2 (topic_len) + topic_len + 2 (pkt_idm only if qos > 0) + payload_len
     */
    rem_len = 2 + len_topic + (payload != NULL ? payload_len : 0);
    if (qos > 0) {
        rem_len += 2;
    }
    
    if (!output_check_enough_memory(client, rem_len)) {
        return espERRMEM;
    }
    
    pkt_id = create_packet_id(client);          /* Create new packet ID */
    request = request_create(client, pkt_id);   /* Create request for packet */
    if (request == NULL) {
        return espERRMEM;
    }
    
    write_fixed_header(client, MQTT_MSG_TYPE_PUBLISH, 0, ESP_MIN(qos, 2), 1, rem_len);
    write_string(client, topic, len_topic);     /* Write topic string to packet */
    if (qos > 0) {
        write_u16(client, pkt_id);              /* Write packet ID */
    }
    if (payload != NULL && payload_len > 0) {
        write_data(client, payload, payload_len);   /* Write RAW topic payload */
    }    
    
    request_set_pending(client, request);       /* Set request as pending waiting for server reply */
    send_data(client);                          /* Try to send data */
    
    return espOK;
}
