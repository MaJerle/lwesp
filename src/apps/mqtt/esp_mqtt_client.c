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

#ifndef ESP_DBG_MQTT
#define ESP_DBG_MQTT                        ESP_DBG_OFF
#endif /* ESP_DBG_MQTT */

static espr_t mqtt_conn_cb(esp_cb_t* cb);

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

/** List of flags for CONNECT message type */
#define MQTT_FLAG_CONNECT_USERNAME      0x80
#define MQTT_FLAG_CONNECT_PASSWORD      0x40
#define MQTT_FLAG_CONNECT_WILL_RETAIN   0x20
#define MQTT_FLAG_CONNECT_WILL          0x04
#define MQTT_FLAG_CONNECT_CLEAN_SESSION 0x02

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
    esp_buff_write(&client->buff, &b, 1);       /* Write start of packet parameters */
    
    while (rem_len) {                           /* Encode length */
        /*
         * Length if encoded LSB first up to 127 (0x7F) long,
         * where bit 7 indicates we have more data in queue
         */
        b = (rem_len & 0x7F) | (rem_len > 0x7F ? 0x80 : 0);
        esp_buff_write(&client->buff, &b, 1);   /* Write single byte */
        rem_len >>= 7;                          /* Go to next 127 bytes */
    }
}

static void
write_u8(mqtt_client_t* client, uint8_t num) {
    esp_buff_write(&client->buff, &num, 1);     /* Write single byte */
}

static void
write_u16(mqtt_client_t* client, uint16_t num) {
    write_u8(client, num >> 8);                 /* Write MSB first... */
    write_u8(client, num & 0xFF);               /* ...followed by LSB */
}

static void
write_data(mqtt_client_t* client, const void* data, size_t len) {
    esp_buff_write(&client->buff, data, len);   /* Write raw data to buffer */
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
    
    client->last_send_len = esp_buff_get_linear_block_length(&client->buff);/* Get length of linear memory */
    if (client->last_send_len) {                /* Anything to send? */
        addr = esp_buff_get_linear_block_address(&client->buff);    /* Get address of linear memory */
        if (esp_conn_send(client->conn, addr, client->last_send_len, &client->last_sent_len, 0) == espOK) {
            client->is_sending = 1;             /* Remember active sending flag */
        }
    }
}

/**
 * \brief           Write string to output buffer
 * \param[in]       client: MQTT client
 * \param[in]       str: String to write to buffer
 */
static void
write_string(mqtt_client_t* client, const char* str, uint16_t len) {
    write_u16(client, len);                     /* Write string length */
    esp_buff_write(&client->buff, (const void *)str, len);  /* Write string to buffer */
}

/**
 * \brief           Allocate a new MQTT client structure
 * \param[in]       buff_len: Length of raw data output buffer
 * \return          Pointer to new allocated MQTT client structure or NULL on failure
 */
mqtt_client_t *
mqtt_client_new(size_t buff_len) {
    mqtt_client_t* client;
    
    client = esp_mem_alloc(sizeof(*client));    /* Allocate memory for client structure */
    if (client) {
        memset(client, 0x00, sizeof(*client));  /* Reset memory */
        if (!esp_buff_init(&client->buff, buff_len)) {
            esp_mem_free(client);
            client = NULL;
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
mqtt_client_connect(mqtt_client_t* client, const char* host, uint16_t port) {
    ESP_ASSERT("client != NULL", client != NULL);   /* Assert input parameters */
    ESP_ASSERT("host != NULL", host != NULL);   /* Assert input parameters */
    ESP_ASSERT("port > 0", port > 0);           /* Assert input parameters */
    
    return esp_conn_start(&client->conn, ESP_CONN_TYPE_TCP, host, port, client, mqtt_conn_cb, 0);
}

espr_t
mqtt_client_disconnect(mqtt_client_t* client);

espr_t
mqtt_client_subscribe(mqtt_client_t* client, const char* topic, uint8_t qos);

espr_t
mqtt_client_unsubscribe(mqtt_client_t* client, const char* topic);

espr_t
mqtt_client_publish(mqtt_client_t* client, const char* topic, const void* payload, uint16_t len, uint8_t qos, uint8_t retain);

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
            uint8_t flags;
            uint16_t rem_len = 10 + 2 + 9;
            
            ESP_DEBUGF(ESP_DBG_MQTT, "Connected to MQTT server\r\n");
            
            flags |= MQTT_FLAG_CONNECT_CLEAN_SESSION;
            
            write_fixed_header(client, MQTT_MSG_TYPE_CONNECT, 0, 0, 0, rem_len);
            write_string(client, "MQTT", 4);    /* Protocol name */
            write_u8(client, 4);                /* Protocol version */
            write_u8(client, flags);            /* Flags for CONNECT message */
            write_u16(client, 10);              /* Keep alive timeout in units of seconds */
            write_string(client, "client_id", 9);
            
            send_data(client);                  /* Flush and send the actual data */
            break;
        }
        
        case ESP_CB_CONN_DATA_SENT: {
            client->is_sending = 0;             /* We are not sending anymore */
            esp_buff_skip(&client->buff, client->last_sent_len);    /* Skip buffer for actual skipped data */
            
            send_data(client);                  /* Try to send more */
            break;
        }
        
        case ESP_CB_CONN_DATA_SEND_ERR: {
            client->is_sending = 0;             /* We are not sending anymore */
            
            send_data(client);                  /* Try to send again */
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
