/*
 * MQTT client API example with ESP device to test server.
 * It utilizes sequential mode without callbacks in one user thread
 *
 * Once device is connected to network,
 * it will try to connect to mosquitto test server and start the MQTT.
 *
 * If successfully connected, it will publish data to "lwesp_mqtt_topic" topic every x seconds.
 *
 * To check if data are sent, you can use mqtt-spy PC software to inspect
 * test.mosquitto.org server and subscribe to publishing topic
 */

#include "mqtt_client_api.h"
#include "lwesp/apps/lwesp_mqtt_client_api.h"
#include "lwesp/lwesp_mem.h"

/**
 * \brief           Connection information for MQTT CONNECT packet
 */
static const lwesp_mqtt_client_info_t mqtt_client_info = {
    .keep_alive = 10,

    /* Server login data */
    .user = "8a215f70-a644-11e8-ac49-e932ed599553",
    .pass = "26aa943f702e5e780f015cd048a91e8fb54cca28",

    /* Device identifier address */
    .id = "869f5a20-af9c-11e9-b01f-db5cf74e7fb7",
};

static char mqtt_topic_str[256];  /*!< Topic string */
static char mqtt_topic_data[256]; /*!< Data string */

/**
 * \brief           Generate random number and write it to string
 * It utilizes simple pseudo random generator, super simple one
 * \param[out]      str: Output string with new number
 */
static void
prv_generate_random(char* str) {
    static uint32_t random_beg = 0x8916;
    random_beg = random_beg * 0x00123455 + 0x85654321;
    sprintf(str, "%u", (unsigned)((random_beg >> 8) & 0xFFFF));
}

/**
 * \brief           MQTT client API thread
 * \param[in]       arg: User argument
 */
void
lwesp_mqtt_client_api_thread(void const* arg) {
    lwesp_mqtt_client_api_p client;
    lwesp_mqtt_conn_status_t conn_status;
    lwesp_mqtt_client_api_buf_p buf;
    lwespr_t res;
    char random_str[10];

    LWESP_UNUSED(arg);

    /* Create new MQTT API */
    if ((client = lwesp_mqtt_client_api_new(256, 128)) == NULL) {
        goto terminate;
    }

    while (1) {
        /* Make a connection */
        printf("Joining MQTT server\r\n");

        /* Try to join */
        conn_status = lwesp_mqtt_client_api_connect(client, "mqtt.mydevices.com", 1883, &mqtt_client_info);
        if (conn_status == LWESP_MQTT_CONN_STATUS_ACCEPTED) {
            printf("Connected and accepted!\r\n");
            printf("Client is ready to subscribe and publish to new messages\r\n");
        } else {
            printf("Connect API response: %d\r\n", (int)conn_status);
            lwesp_delay(5000);
            continue;
        }

        /* Subscribe to topics */
        sprintf(mqtt_topic_str, "v1/%s/things/%s/cmd/#", mqtt_client_info.user, mqtt_client_info.id);
        if (lwesp_mqtt_client_api_subscribe(client, mqtt_topic_str, LWESP_MQTT_QOS_AT_LEAST_ONCE) == lwespOK) {
            printf("Subscribed to topic\r\n");
        } else {
            printf("Problem subscribing to topic!\r\n");
        }

        while (1) {
            /* Receive MQTT packet with 1000ms timeout */
            if ((res = lwesp_mqtt_client_api_receive(client, &buf, 5000)) == lwespOK) {
                if (buf != NULL) {
                    printf("Publish received!\r\n");
                    printf("Topic: %s, payload: %s\r\n", buf->topic, buf->payload);
                    lwesp_mqtt_client_api_buf_free(buf);
                    buf = NULL;
                }
            } else if (res == lwespCLOSED) {
                printf("MQTT connection closed!\r\n");
                break;
            } else if (res == lwespTIMEOUT) {
                printf("Timeout on MQTT receive function. Manually publishing.\r\n");

                /* Publish data on channel 1 */
                prv_generate_random(random_str);
                sprintf(mqtt_topic_str, "v1/%s/things/%s/data/1", mqtt_client_info.user, mqtt_client_info.id);
                sprintf(mqtt_topic_data, "temp,c=%s", random_str);
                lwesp_mqtt_client_api_publish(client, mqtt_topic_str, mqtt_topic_data, strlen(mqtt_topic_data),
                                              LWESP_MQTT_QOS_AT_LEAST_ONCE, 0);
            }
        }
        //goto terminate;
    }

terminate:
    lwesp_mqtt_client_api_delete(client);
    printf("MQTT client thread terminate\r\n");
    lwesp_sys_thread_terminate(NULL);
}
