/**
 * \file            esp_app_mqtt.c
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
#include "include/esp_app_mqtt.h"
#include "../include/esp_mem.h"

/**
 * \brief           Allocate a new MQTT client structure
 * \return          Pointer to new allocated MQTT client structure or NULL on failure
 */
mqtt_client_t*
mqtt_client_new(void) {
    mqtt_client_t* client;
    
    client = esp_mem_alloc(sizeof(*client));    /* Allocate memory for client structure */
    if (client) {
        memset(client, 0x00, sizeof(*client));  /* Reset memory */
    }
    return client;
}

espr_t          mqtt_client_connect(mqtt_client_t* client, const char* host, uint16_t port);
espr_t          mqtt_client_disconnect(mqtt_client_t* client);

espr_t          mqtt_client_subscribe(mqtt_client_t* client, const char* topic, uint8_t qos);
espr_t          mqtt_client_unsubscribe(mqtt_client_t* client, const char* topic);

espr_t          mqtt_client_publish(mqtt_client_t* client, const char* topic, const void* payload, uint16_t len, uint8_t qos, uint8_t retain);

