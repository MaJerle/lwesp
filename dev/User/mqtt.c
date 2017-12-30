#include "mqtt.h"
#include "esp/esp.h"
#include "apps/esp_mqtt_client.h"

mqtt_client_t* mqtt_client;

const mqtt_client_info_t
mqtt_client_info = {
    .id = "test_client_id",
    // .user = "test_username",
    // .pass = "test_password",
    .keep_alive = 10,
};

static void mqtt_cb(mqtt_evt_t* evt);

/**
 * \brief           MQTT client thread
 * \param[in]       arg: User argument
 */
void
mqtt_thread(void const* arg) {
    mqtt_client = mqtt_client_new(256, 128);
    mqtt_client_connect(mqtt_client, "test.mosquitto.org", 1883, mqtt_cb, &mqtt_client_info);
    
    while (1) {
        osDelay(1000);
    }
}

/**
 * \brief           MQTT event callback function
 */
static void
mqtt_cb(mqtt_evt_t* evt) {
    switch (evt->type) {
        case MQTT_EVT_CONNECTED: {              /* MQTT connected to server */
            mqtt_client_subscribe(mqtt_client, "test_topic", MQTT_QOS_AT_LEAST_ONCE);
            break;
        }
        case MQTT_EVT_SUBSCRIBED: {             /* MQTT subscribed to topic */
            mqtt_client_publish(mqtt_client, "test_topic", "krneki", 6, 2, 0);
            break;
        }
    }
}
