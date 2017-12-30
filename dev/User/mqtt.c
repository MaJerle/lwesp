#include "mqtt.h"
#include "esp/esp.h"
#include "apps/esp_mqtt_client.h"

mqtt_client_t* mqtt_client;

const mqtt_client_info_t
mqtt_client_info = {
    .id = "test_client_id",
    // .user = "test_username",
    // .pass = "test_password",
    .keep_alive = 20,
};

static void mqtt_cb(mqtt_client_t* client, mqtt_evt_t* evt);

/**
 * \brief           MQTT client thread
 * \param[in]       arg: User argument
 */
void
mqtt_thread(void const* arg) {
    mqtt_client = mqtt_client_new(256, 128);
    mqtt_client_connect(mqtt_client, "test.mosquitto.org", 1883, mqtt_cb, &mqtt_client_info);
    
    while (1) {
        osDelay(10000);
        mqtt_client_publish(mqtt_client, "tilen_topic_test", "test_4", 6, MQTT_QOS_AT_MOST_ONCE, 0, (void *)((uint32_t)4));
    }
}

/**
 * \brief           MQTT event callback function
 */
static void
mqtt_cb(mqtt_client_t* client, mqtt_evt_t* evt) {
    switch (evt->type) {
        case MQTT_EVT_CONNECT: {                /* MQTT connect event occurred */
            mqtt_conn_status_t status = evt->evt.connect.status;
            if (status == MQTT_CONN_STATUS_ACCEPTED) {
                mqtt_client_subscribe(client, "tilen_topic_test", MQTT_QOS_AT_MOST_ONCE, "tilen_topic_test");
            } else {
                printf("MQTT server connection was not successful: %d\r\n", (int)status);
            }
            break;
        }
        
        case MQTT_EVT_SUBSCRIBE: {              /* MQTT subscribe to topic event */
            const char* arg = evt->evt.sub_unsub_scribed.arg;   /* Get user argument */
            
            printf("Successfully subscribed to %s topic\r\n", arg);
            if (!strcmp(arg, "tilen_topic_test")) {
                mqtt_client_subscribe(client, "tilen_another_topic", MQTT_QOS_AT_MOST_ONCE, "tilen_another_topic");
            } else if (!strcmp(arg, "tilen_another_topic")) {
                mqtt_client_publish(client, "tilen_topic_test", "test_1", 6, MQTT_QOS_AT_MOST_ONCE, 0, (void *)((uint32_t)1));
            }
            
            break;
        }
        
        /*
         * Message was successfully published
         */
        case MQTT_EVT_PUBLISHED: {              /* MQTT publish was successful */
            uint32_t val = (uint32_t)evt->evt.published.arg;    /* Get user argument */
            printf("Published val: %d\r\n", (int)val);
            if (val == 1) {
                mqtt_client_publish(client, "tilen_topic_test", "test_2", 6, MQTT_QOS_AT_MOST_ONCE, 0, (void *)((uint32_t)2));
            } else if (val == 2) {
                mqtt_client_publish(client, "tilen_topic_test", "test_3", 6, MQTT_QOS_AT_MOST_ONCE, 0, (void *)((uint32_t)3));
            } else if (val == 3) {
                mqtt_client_publish(client, "tilen_topic_test", "test_4", 6, MQTT_QOS_AT_MOST_ONCE, 0, (void *)((uint32_t)4));
            } else {
                printf("Everything was sent!\r\n");
            }
            break;
        }
        
        /*
         * A new message was published to us
         */
        case MQTT_EVT_PUBLISH_RECV: {
            if (!memcmp("tilen_topic_test", evt->evt.publish_recv.topic, evt->evt.publish_recv.topic_len)) {
                printf("MQTT publish received on topic: %.*s\r\n", evt->evt.publish_recv.topic_len, evt->evt.publish_recv.topic);
            }
            break;
        }
        
        case MQTT_EVT_DISCONNECT: {
            printf("MQTT client disconnected!\r\n");
            mqtt_client_connect(mqtt_client, "test.mosquitto.org", 1883, mqtt_cb, &mqtt_client_info);
            break;
        }
        
        default: 
            break;
    }
}

