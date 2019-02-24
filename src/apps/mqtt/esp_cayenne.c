/**
 * \file            esp_cayenne.c
 * \brief           MQTT client for Cayenne
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
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#include "esp/apps/esp_cayenne.h"
#include "esp/esp_mem.h"
#include "esp/esp_pbuf.h"

#error "This driver is not ready-to-use yet and shall not be used in final product"

#if !ESP_CFG_NETCONN || !ESP_CFG_MODE_STATION
#error "Netconn and station mode must be enabled!"
#endif /* !ESP_CFG_NETCONN || !ESP_CFG_MODE_STATION */

typedef struct {
    esp_cayenne_topic_t topic;
    const char* str;
} topic_cmd_str_pair_t;

/**
 * \brief           List of key-value pairs for topic type and string
 */
const static topic_cmd_str_pair_t
topic_cmd_str_pairs[] = {
    { ESP_CAYENNE_TOPIC_DATA, "data" },
    { ESP_CAYENNE_TOPIC_COMMAND, "cmd" },
    { ESP_CAYENNE_TOPIC_CONFIG, "conf" },
    { ESP_CAYENNE_TOPIC_RESPONSE, "response" },
    { ESP_CAYENNE_TOPIC_SYS_MODEL, "sys/model" },
    { ESP_CAYENNE_TOPIC_SYS_VERSION, "sys/version" },
    { ESP_CAYENNE_TOPIC_SYS_CPU_MODEL, "sys/cpu/model" },
    { ESP_CAYENNE_TOPIC_SYS_CPU_SPEED, "sys/cpu/speed" },
    { ESP_CAYENNE_TOPIC_DIGITAL, "digital" },
    { ESP_CAYENNE_TOPIC_DIGITAL_COMMAND, "digital-cmd" },
    { ESP_CAYENNE_TOPIC_DIGITAL_CONFIG, "digital-conf" },
    { ESP_CAYENNE_TOPIC_ANALOG, "analog" },
    { ESP_CAYENNE_TOPIC_ANALOG_COMMAND, "analog-cmd" },
    { ESP_CAYENNE_TOPIC_ANALOG_CONFIG, "analog-conf" }
};

/**
 * \brief           Protection mutex
 */
static esp_sys_mutex_t
prot_mutex;

/**
 * \brief           Topic name for publish/subscribe
 */
static char
topic_name[256];

/**
 * \brief           Payload data
 */
static char
payload_data[128];

/**
 * \brief           Build topic string based on input parameters
 */
static espr_t
build_topic(char* topic_str, size_t topic_str_len, const char* username,
            const char* client_id, esp_cayenne_topic_t topic, uint16_t channel) {
    size_t rem_len, token_len;
    char ch_token[6];
    ESP_ASSERT("topic_str != NULL", topic_str != NULL);
    ESP_ASSERT("username != NULL", username != NULL);
    ESP_ASSERT("client_id != NULL", client_id != NULL);
    ESP_ASSERT("topic < ESP_CAYENNE_TOPIC_END", topic < ESP_CAYENNE_TOPIC_END);

    /* Assert for basic part without topic */
    ESP_ASSERT("topic_str_len > string_length", topic_str_len > (strlen(ESP_CAYENNE_API_VERSION) + strlen(username) + strlen(client_id) + 11));

    topic_str[0] = 0;

    /* Base part */
    strcat(topic_str, ESP_CAYENNE_API_VERSION);
    strcat(topic_str, "/");
    strcat(topic_str, username);
    strcat(topic_str, "/things/");
    strcat(topic_str, client_id);
    strcat(topic_str, "/");
    rem_len = topic_str_len - strlen(topic_str) - 1;

    /* Topic string */
    for (size_t i = 0; i < ESP_ARRAYSIZE(topic_cmd_str_pairs); i++) {
        if (topic == topic_cmd_str_pairs[i].topic) {
            ESP_ASSERT("strlen(topic_cmd_str_pairs[i].str) <= rem_len", strlen(topic_cmd_str_pairs[i].str) <= rem_len);
            strcat(topic_str, topic_cmd_str_pairs[i].str);
            break;
        }
    }
    rem_len = topic_str_len - strlen(topic_str) - 1;

    /* Channel */
    if (channel != ESP_CAYENNE_NO_CHANNEL) {
        if (channel == ESP_CAYENNE_ALL_CHANNELS) {
            ESP_ASSERT("rem_len >= 2", rem_len >= 2);
            strcat(topic_str, "/+");
        } else {
            sprintf(ch_token, "%d", (int)channel);
            ESP_ASSERT("strlen(ch_token) <= rem_len", strlen(ch_token) <= rem_len);
            strcat(topic_str, ch_token);
        }
    }

    printf("TOPIC: %s\r\n", topic_str);

    return espOK;
}

/**
 * \brief           Cayenne thread
 * \param[in]       arg: Thread argument. Pointer to \ref esp_mqtt_client_cayenne_t structure
 */
static void
mqtt_thread(void * const arg) {
    esp_cayenne_t* c = arg;
    esp_mqtt_conn_status_t status;
    esp_mqtt_client_api_buf_p buf;
    espr_t res;

    /* Create mutex */
    esp_core_lock();
    if (!esp_sys_mutex_isvalid(&prot_mutex)) {
        esp_sys_mutex_create(&prot_mutex);
    }
    esp_core_unlock();

    /* Release calling thread now */
    if (esp_sys_sem_isvalid(&c->sem)) {
        esp_sys_sem_release(&c->sem);
    }

    while (1) {
        /* Device must be connected to access point */
        while (!esp_sta_has_ip()) {
            esp_delay(1000);
        }

        /* Connect to API server */
        status = esp_mqtt_client_api_connect(c->api_c, ESP_CAYENNE_HOST, ESP_CAYENNE_PORT, c->info_c);
        if (status != ESP_MQTT_CONN_STATUS_ACCEPTED) {
            /* Find out reason not to be accepted and decide accordingly */
        } else {
            /* We are connected and ready to subscribe/publish/receive packets */
            esp_cayenne_subscribe(c, ESP_CAYENNE_TOPIC_COMMAND, ESP_CAYENNE_ALL_CHANNELS);

            /* Unlimited loop */
            while (1) {
                /* Wait for new received packet or connection closed */
                res = esp_mqtt_client_api_receive(c->api_c, &buf, 0);

                if (res == espOK) {
                    if (buf != NULL) {
                        printf("Packet received!\r\n");

                        esp_mqtt_client_api_buf_free(buf);
                        buf = NULL;
                    }
                } else if (res == espCLOSED) {
                    /* Connection closed at this point */
                    printf("Connection closed!\r\n");
                }
            }
        }
    }
}

espr_t
esp_cayenne_create(esp_cayenne_t* c, esp_mqtt_client_api_p client_api, const esp_mqtt_client_info_t* client_info) {
    /* Check input parameters */

    c->api_c = client_api;
    c->info_c = client_info;

    /* Create semaphore */
    if (!esp_sys_sem_create(&c->sem, 1)) {
        return espERRMEM;
    }

    /* Create and wait for thread to start */
    esp_sys_sem_wait(&c->sem, 0);
    if (!esp_sys_thread_create(&c->thread, "cayenne", mqtt_thread, c, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO)) {
        esp_sys_sem_release(&c->sem);
        esp_sys_sem_delete(&c->sem);
        esp_sys_sem_invalid(&c->sem);
        return espERRMEM;
    }
    esp_sys_sem_wait(&c->sem, 0);
    esp_sys_sem_release(&c->sem);

    return espOK;
}

espr_t
esp_cayenne_subscribe(esp_cayenne_t* c, esp_cayenne_topic_t topic, uint16_t channel) {
    espr_t res;

    esp_sys_mutex_lock(&prot_mutex);
    build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel);
    if ((res = esp_mqtt_client_api_subscribe(c->api_c, topic_name, ESP_MQTT_QOS_EXACTLY_ONCE)) == espOK) {
        printf("Subscribed to %s topic\r\n", topic_name);
    } else {
        printf("Cannot subscribe to topic %s\r\n", topic_name);
    }
    esp_sys_mutex_unlock(&prot_mutex);
    return res;
}

espr_t
esp_cayenne_publish_data(esp_cayenne_t* c, esp_cayenne_topic_t topic, uint16_t channel,
                        const char* type, const char* unit, const char* data) {
    esp_sys_mutex_lock(&prot_mutex);
    build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel);

    payload_data[0] = 0;
    if (type != NULL) {
        strcat(payload_data, type);
    }
    if (type != NULL && unit != NULL) {
        strcat(payload_data, ",");
    }
    if (unit != NULL) {
        strcat(payload_data, unit);
    }
    if (strlen(payload_data)) {
        strcat(payload_data, "=");
    }
    strcat(payload_data, data);

    printf("Publishing on topic: %s\r\n", topic_name);
    printf("Publishing data: %s\r\n", payload_data);

    esp_mqtt_client_api_publish(c->api_c, topic_name, payload_data, strlen(payload_data), ESP_MQTT_QOS_AT_LEAST_ONCE, 1);

    esp_sys_mutex_unlock(&prot_mutex);
    return espOK;
}

