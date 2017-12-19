/**
 * \file            esp_app_mqtt.h
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
#ifndef __ESP_APP_MQTT_H
#define __ESP_APP_MQTT_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include "esp.h"

/**
 * \addtogroup      ESP
 * \{
 */
    
/**
 * \defgroup        ESP_APP_MQTT MQTT
 * \brief           MQTT client
 * \{
 */

/**
 * \brief           MQTT client connection
 */
typedef struct {
    const char* user;                           /*!< Username for authentication */
    const char* pass;                           /*!< Password for authentication */
    
    esp_conn_p conn;                            /*!< Active used connection for MQTT */
                                                
} mqtt_client_t;

mqtt_client_t*  mqtt_client_new(void);
espr_t          mqtt_client_connect(mqtt_client_t* client, const char* host, uint16_t port);
espr_t          mqtt_client_disconnect(mqtt_client_t* client);

espr_t          mqtt_client_subscribe(mqtt_client_t* client, const char* topic, uint8_t qos);
espr_t          mqtt_client_unsubscribe(mqtt_client_t* client, const char* topic);

espr_t          mqtt_client_publish(mqtt_client_t* client, const char* topic, const void* payload, uint16_t len, uint8_t qos, uint8_t retain);

    
/**
 * \}
 */
    
/**
 * \}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __ESP_APP_MQTT_H */
