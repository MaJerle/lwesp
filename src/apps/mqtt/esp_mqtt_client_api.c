/**
 * \file            esp_mqtt_client_api.c
 * \brief           MQTT client API
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
#include "esp/apps/esp_mqtt_client_api.h"
#include "esp/esp_mem.h"

/**
 * \brief           MQTT API client structure
 */
struct mqtt_client_api {
    mqtt_client_p mc;                           /*!< MQTT client handle */
    esp_sys_mbox_t rcv_mbox;                    /*!< Received data mbox */
    esp_sys_sem_t sync_sem;                     /*!< Synchronization semaphore */
    esp_sys_mutex_t mutex;                      /*!< Mutex handle */
    uint8_t release_sem;                        /*!< Set to 1 to release semaphore */
    mqtt_conn_status_t connect_resp;            /*!< Response when connecting to server */
    espr_t sub_pub_resp;                        /*!< Subscribe/Unsubscribe/Publish response */
} mqtt_client_api_t;

static uint8_t mqtt_closed = 0xFF;

/**
 * \brief           Release user semaphore
 * \param[in]       client: Client handle
 */
static void
release_sem(mqtt_client_api_p client) {
    if (client->release_sem) {
        client->release_sem = 0;
        esp_sys_sem_release(&client->sync_sem);
    }
}

/**
 * \brief           MQTT event callback function
 */
static void
mqtt_evt(mqtt_client_p client, mqtt_evt_t* evt) {
    mqtt_client_api_p api_client = mqtt_client_get_arg(client);
    if (api_client == NULL) {
        return;
    }
    switch (mqtt_client_evt_get_type(client, evt)) {
        case MQTT_EVT_CONNECT: {
            mqtt_conn_status_t status = mqtt_client_evt_connect_get_status(client, evt);

            api_client->connect_resp = status;
            release_sem(api_client);

            break;
        }
        case MQTT_EVT_PUBLISH_RECV: {
            /* Check valid receive mbox */
            if (esp_sys_mbox_isvalid(&api_client->rcv_mbox)) {
                mqtt_client_api_buf_p buf;
                size_t size, buf_size, topic_size, payload_size;

                /* Get event data */
                const char* topic = mqtt_client_evt_publish_recv_get_topic(client, evt);
                size_t topic_len = mqtt_client_evt_publish_recv_get_topic_len(client, evt);
                const uint8_t* payload = mqtt_client_evt_publish_recv_get_payload(client, evt);
                size_t payload_len = mqtt_client_evt_publish_recv_get_payload_len(client, evt);

                /* Calculate sizes */
                buf_size = sizeof(*buf);
                topic_size = sizeof(*topic) * (topic_len + 1);
                payload_size = sizeof(*payload) * (payload_len + 1);

                size = sizeof(*buf) + topic_size + payload_size;
                buf = esp_mem_alloc(size);
                if (buf != NULL) {
                    ESP_MEMSET(buf, 0x00, size);
                    buf->topic = (const void *)(buf + buf_size);
                    buf->payload = (const void *)(buf->topic + topic_size);
                    buf->topic_len = topic_len;
                    buf->payload_len = payload_len;

                    /* Copy content to new memory */
                    ESP_MEMCPY((void *)buf->topic, topic, sizeof(*topic) * topic_len);
                    ESP_MEMCPY((void *)buf->payload, payload, sizeof(*payload) * payload_len);

                    /* Write to receive queue */
                    if (!esp_sys_mbox_putnow(&api_client->rcv_mbox, buf)) {
                        esp_mem_free(buf);
                    }
                }
            }
            break;
        }
        case MQTT_EVT_PUBLISH: {
            api_client->sub_pub_resp = mqtt_client_evt_publish_get_result(client, evt);
            release_sem(api_client);            /* Release semaphore if forced */
            break;
        }
        case MQTT_EVT_SUBSCRIBE: {
            api_client->sub_pub_resp = mqtt_client_evt_subscribe_get_result(client, evt);
            release_sem(api_client);            /* Release semaphore if forced */
            break;
        }
        case MQTT_EVT_UNSUBSCRIBE: {
            api_client->sub_pub_resp = mqtt_client_evt_unsubscribe_get_result(client, evt);
            release_sem(api_client);            /* Release semaphore if forced */
            break;
        }
        case MQTT_EVT_DISCONNECT: {
            /* Disconnect event happened */
            api_client->sub_pub_resp = mqtt_client_evt_unsubscribe_get_result(client, evt);
            api_client->connect_resp = MQTT_CONN_STATUS_TCP_FAILED;

            /* Write to receive mbox to wakeup receive thread */
            if (esp_sys_mbox_isvalid(&api_client->rcv_mbox)) {
                esp_sys_mbox_putnow(&api_client->rcv_mbox, &mqtt_closed);
            }

            /* Release semaphore if forced */
            release_sem(api_client);
            break;
        }
        default: break;
    }
}

/**
 * \brief           Create new MQTT client API
 * \param[in]       tx_buff_len: Maximal TX buffer for maximal packet length
 * \param[in]       rx_buff_len: Maximal RX buffer
 * \return          Client handle on success, `NULL` otherwise
 */
mqtt_client_api_p
mqtt_client_api_new(size_t tx_buff_len, size_t rx_buff_len) {
    mqtt_client_api_p client;
    size_t size;

    size = ESP_MEM_ALIGN(sizeof(*client));      /* Get size of client itself */

    /* Create client APi structure */
    client = esp_mem_calloc(1, size);           /* Allocate client memory */
    if (client == NULL) {
        goto out;
    }
    /* Create MQTT raw client structure */
    client->mc = mqtt_client_new(tx_buff_len, rx_buff_len);
    if (client->mc == NULL) {
        goto out;
    }
    /* Create receive mbox queue */
    if (!esp_sys_mbox_create(&client->rcv_mbox, 5)) {
        goto out;
    }
    /* Create synchronization semaphore */
    if (!esp_sys_sem_create(&client->sync_sem, 5)) {
        goto out;
    }
    /* Create mutex */
    if (!esp_sys_mutex_create(&client->mutex)) {
        goto out;
    }
    mqtt_client_set_arg(client->mc, client);    /* Set client to mqtt client argument */
    return client;
out:
    mqtt_client_api_delete(client);
    client = NULL;
    return NULL;
}

/**
 * \brief           Delete client from memory
 * \param[in]       client: MQTT API client handle
 */
void
mqtt_client_api_delete(mqtt_client_api_p client) {
    if (client == NULL) {
        return;
    }
    if (esp_sys_sem_isvalid(&client->sync_sem)) {
        esp_sys_sem_delete(&client->sync_sem);
        esp_sys_sem_invalid(&client->sync_sem);
    }
    if (esp_sys_mutex_isvalid(&client->mutex)) {
        esp_sys_mutex_delete(&client->mutex);
        esp_sys_mutex_invalid(&client->mutex);
    }
    if (esp_sys_mbox_isvalid(&client->rcv_mbox)) {
        esp_sys_mbox_delete(&client->rcv_mbox);
        esp_sys_mbox_invalid(&client->rcv_mbox);
    }
    if (client->mc != NULL) {
        mqtt_client_delete(client->mc);
        client->mc = NULL;
    }
    esp_mem_free(client);
}

/**
 * \brief           Connect to MQTT broker
 * \param[in]       client: MQTT API client handle
 * \param[in]       host: TCP host
 * \param[in]       port: TCP port
 * \param[in]       info: MQTT client info
 * \return          \ref MQTT_CONN_STATUS_ACCEPTED on success, member of \ref mqtt_conn_status_t otherwise
 */
mqtt_conn_status_t
mqtt_client_api_connect(mqtt_client_api_p client, const char* host, esp_port_t port, const mqtt_client_info_t* info) {
    esp_sys_mutex_lock(&client->mutex);
    client->connect_resp = MQTT_CONN_STATUS_TCP_FAILED;
    esp_sys_sem_wait(&client->sync_sem, 0);
    client->release_sem = 1;
    if (mqtt_client_connect(client->mc, host, port, mqtt_evt, info) == espOK) {
        esp_sys_sem_wait(&client->sync_sem, 0);
    }
    client->release_sem = 0;
    esp_sys_sem_release(&client->sync_sem);
    esp_sys_mutex_unlock(&client->mutex);
    return client->connect_resp;
}

/**
 * \brief           Close MQTT connection
 * \param[in]       client: MQTT API client handle
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
mqtt_client_api_close(mqtt_client_api_p client) {
    espr_t res = espERR;

    esp_sys_mutex_lock(&client->mutex);
    esp_sys_sem_wait(&client->sync_sem, 0);
    client->release_sem = 1;
    if (mqtt_client_disconnect(client->mc) == espOK) {
        res = espOK;
        esp_sys_sem_wait(&client->sync_sem, 0);
    }
    client->release_sem = 0;
    esp_sys_sem_release(&client->sync_sem);
    esp_sys_mutex_unlock(&client->mutex);
    return res;
}

/**
 * \brief           Subscribe to topic
 * \param[in]       client: MQTT API client handle
 * \param[in]       topic: Topic to subscribe on
 * \param[in]       qos: Quality of service. This parameter can be a value of \ref ESP_APP_MQTT_CLIENT_QOS
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
mqtt_client_api_subscribe(mqtt_client_api_p client, const char* topic, uint8_t qos) {
    espr_t res = espERR;

    esp_sys_mutex_lock(&client->mutex);
    esp_sys_sem_wait(&client->sync_sem, 0);
    client->release_sem = 1;
    if (mqtt_client_subscribe(client->mc, topic, qos, NULL) == espOK) {
        esp_sys_sem_wait(&client->sync_sem, 0);
        res = client->sub_pub_resp;
    }
    client->release_sem = 0;
    esp_sys_sem_release(&client->sync_sem);
    esp_sys_mutex_unlock(&client->mutex);

    return res;
}

/**
 * \brief           Unsubscribe from topic
 * \param[in]       client: MQTT API client handle
 * \param[in]       topic: Topic to unsubscribe from
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
mqtt_client_api_unsubscribe(mqtt_client_api_p client, const char* topic) {
    espr_t res = espERR;

    esp_sys_mutex_lock(&client->mutex);
    esp_sys_sem_wait(&client->sync_sem, 0);
    client->release_sem = 1;
    if (mqtt_client_unsubscribe(client->mc, topic, NULL) == espOK) {
        esp_sys_sem_wait(&client->sync_sem, 0);
        res = client->sub_pub_resp;
    }
    client->release_sem = 0;
    esp_sys_sem_release(&client->sync_sem);
    esp_sys_mutex_unlock(&client->mutex);

    return res;
}

/**
 * \brief           Publish new packet to MQTT network
 * \param[in]       client: MQTT API client handle
 * \param[in]       topic: Topic to publish on
 * \param[in]       data: Data to send
 * \param[in]       btw: Number of bytes to send for data parameter
 * \param[in]       qos: Quality of service. This parameter can be a value of \ref ESP_APP_MQTT_CLIENT_QOS
 * \param[in]       retain: Set to `1` for retain flag, `0` otherwise
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
mqtt_client_api_publish(mqtt_client_api_p client, const char* topic, const void* data,
                        size_t btw, uint8_t qos, uint8_t retain) {
    espr_t res = espERR;

    esp_sys_mutex_lock(&client->mutex);
    esp_sys_sem_wait(&client->sync_sem, 0);
    client->release_sem = 1;
    if (mqtt_client_publish(client->mc, topic, data, ESP_U16(btw), qos, 1, NULL) == espOK) {
        esp_sys_sem_wait(&client->sync_sem, 0);
        res = client->sub_pub_resp;
    }
    client->release_sem = 0;
    esp_sys_sem_release(&client->sync_sem);
    esp_sys_mutex_unlock(&client->mutex);

    return res;
}

/**
 * \brief           Receive next packet in specific timeout time
 * \note            This function can be called from separate thread
 *                      than the rest of API function, which allows you to 
 *                      handle receive data separated with custom timeout
 * \param[in]       client: MQTT API client handle
 * \param[in]       p: Pointer to output buffer
 * \param[in]       timeout: Maximal time to wait before function returns timeout
 * \return          \ref espOK on success, \ref espCLOSED if MQTT is closed, \ref espTIMEOUT on timeout
 */
espr_t
mqtt_client_api_receive(mqtt_client_api_p client, mqtt_client_api_buf_p* p, uint32_t timeout) {
    *p = NULL;

    /* Get new entry from mbox */
    if (timeout == 0) {
        if (!esp_sys_mbox_getnow(&client->rcv_mbox, (void **)p)) {
            return espTIMEOUT;
        }
    } else if (esp_sys_mbox_get(&client->rcv_mbox, (void **)p, timeout) == ESP_SYS_TIMEOUT) {
        return espTIMEOUT;
    }

    /* Check for MQTT closed event */
    if ((uint8_t *)(*p) == (uint8_t *)&mqtt_closed) {
        *p = NULL;
        return espCLOSED;
    }
    return espOK;
}

/**
 * \brief           Free buffer memory after usage
 * \param[in]       p: Buffer to free
 */
void
mqtt_client_api_buf_free(mqtt_client_api_buf_p p) {
    esp_mem_free(p);
}
