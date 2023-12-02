/*
 * Example to connect to Cayenne cloud using MQTT API module.
 *
 * Connection is implemented from separate thread in sequential mode.
 * No callbacks are being used in such mode
 */

#include "lwesp/apps/lwesp_mqtt_client_api.h"
#include "mqtt_client_api.h"

/* Override safeprintf function */
#define safeprintf          printf

/**
 * \brief           MQTT client info for server
 */
static const lwesp_mqtt_client_info_t
mqtt_client_info = {
    /* Device ID */
    .id = "869f5a20-af9c-11e9-b01f-db5cf74e7fb7",

    /* User credentials */
    .user = "8a215f70-a644-11e8-ac49-e932ed599553",
    .pass = "26aa943f702e5e780f015cd048a91e8fb54cca28",

    .keep_alive = 60,
};

static char
mqtt_client_str[256];
static char
mqtt_client_data[256];

/**
 * \brief           MQTT thread
 */
void
lwesp_mqtt_client_api_cayenne_thread(void const* arg) {
    lwesp_mqtt_client_api_p client = NULL;
    lwesp_mqtt_conn_status_t status;
    lwesp_mqtt_client_api_buf_p buf;
    lwespr_t res;

    LWESP_UNUSED(arg);

beg:
    while (1) {
        /* Wait IP and connected to network */
        while (!lwesp_sta_has_ip()) {
            lwesp_delay(1000);
        }

        if (client == NULL) {
            client = lwesp_mqtt_client_api_new(256, 256);
        }
        if (client != NULL) {
            safeprintf("[MQTT] Connecting to MQTT broker...\r\n");
            status = lwesp_mqtt_client_api_connect(client, "mqtt.mydevices.com", 1883, &mqtt_client_info);
            if (status == LWESP_MQTT_CONN_STATUS_ACCEPTED) {
                safeprintf("[MQTT] Connected to MQTT broker and ready to publish/subscribe to topics...\r\n");

                /* Subscribe to topic */
                sprintf(mqtt_client_str, "v1/%s/things/%s/cmd/#", mqtt_client_info.user, mqtt_client_info.id);
                if (lwesp_mqtt_client_api_subscribe(client, mqtt_client_str, LWESP_MQTT_QOS_AT_LEAST_ONCE) == lwespOK) {
                    safeprintf("[MQTT] Subscribed to topic: %s\r\n", mqtt_client_str);
                } else {
                    safeprintf("[MQTT] Problems subscribing to topic!\r\n");
                }

                /* Start accepting and publishing data */
                while (1) {
                    res = lwesp_mqtt_client_api_receive(client, &buf, 1000);
                    if (res == lwespOK) {
                        safeprintf("[MQTT] Receive OK\r\n");
                        if (buf != NULL) {
                            const char* s;
                            safeprintf("[MQTT] Publish received. Topic: %s, payload: %s\r\n", buf->topic, buf->payload);
                            safeprintf("[MQTT] Topic_Len: %d, Payload_len: %d\r\n", (int)buf->topic_len, (int)buf->payload_len);

                            /* Find out reason */
                            if ((s = strstr((void*)buf->topic, "cmd/2")) != NULL) {
                                s = strstr((void*)buf->payload, ",");
                                if (s != NULL) {
                                    s++;
                                    if (*s == '0') {

                                    } else {

                                    }
                                    sprintf(mqtt_client_str, "v1/%s/things/%s/data/2", mqtt_client_info.user, mqtt_client_info.id);
                                    sprintf(mqtt_client_data, "%c", *s);
                                    lwesp_mqtt_client_api_publish(client, mqtt_client_str, mqtt_client_data, strlen(mqtt_client_data), LWESP_MQTT_QOS_AT_LEAST_ONCE, 0);
                                }
                            }

                            lwesp_mqtt_client_api_buf_free(buf);
                        }
                    } else if (res == lwespCLOSED) {
                        safeprintf("[MQTT] Connection closed!\r\n");
                        goto beg;
                    } else if (res == lwespTIMEOUT) {
                        static uint32_t temp;
                        safeprintf("[MQTT] Receive timeout!\r\n");

                        sprintf(mqtt_client_str, "v1/%s/things/%s/data/1", mqtt_client_info.user, mqtt_client_info.id);
                        sprintf(mqtt_client_data, "temp,c=%u", (unsigned)temp++);
                        safeprintf("[MQTT] CLIENT DATA: %s, length: %d\r\n", mqtt_client_data, (int)strlen(mqtt_client_data));
                        lwesp_mqtt_client_api_publish(client, mqtt_client_str, mqtt_client_data, strlen(mqtt_client_data), LWESP_MQTT_QOS_AT_LEAST_ONCE, 0);
                    }
                }
            } else {
                printf("[MQTT] Connect error: %d\r\n", (int)status);
            }
        }
        lwesp_delay(1000);
    }
    if (client != NULL) {
        lwesp_mqtt_client_api_delete(client);
        client = NULL;
    }
    lwesp_sys_thread_terminate(NULL);
}
