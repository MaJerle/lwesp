/*
 * MQTT client example with ESP device.
 *
 * Once the device is ocnnected to network,
 * it will try to connect to mosquitto test server and start the MQTT.
 *
 * If successful, it will publish data to "esp8266_mqtt_topic" topic every second.
 *
 * To check if data are sent, you can use mqtt-spy PC software to inspect
 * test.mosquitto.org server if you can receive the data
 */

#include "esp/apps/esp_mqtt_client.h"
#include "esp/esp.h"
#include "esp/esp_timeout.h"
#include "mqtt_client.h"

mqtt_client_t* mqtt_client;

/**
 * \brief           Connection information for MQTT CONNECT packet
 */
const mqtt_client_info_t
mqtt_client_info = {
    .id = "test_client_id",                     /* The only required field for connection! */
    
    .keep_alive = 10,
    // .user = "test_username",
    // .pass = "test_password",
};

static void mqtt_cb(mqtt_client_t* client, mqtt_evt_t* evt);
static void example_do_connect(mqtt_client_t* client);

/**
 * \brief           Custom callback function for ESP events
 */
static espr_t
mqtt_esp_cb(esp_evt_t* evt) {
    switch (evt->type) {
#if ESP_CFG_MODE_STATION
        case ESP_CB_WIFI_GOT_IP: {
            example_do_connect(mqtt_client);    /* Start connection after we have a connection to network client */
            break;
        }
#endif /* ESP_CFG_MODE_STATION */
        default: break;
    }
    return espOK;
}

/**
 * \brief           MQTT client thread
 * \param[in]       arg: User argument
 */
void
mqtt_client_thread(void const* arg) {
    esp_evt_register(mqtt_esp_cb);              /* Register new callback for general events from ESP stack */
    
    /*
     * Create a new client with 256 bytes of RAW TX data
     * and 128 bytes of RAW incoming data
     */
    mqtt_client = mqtt_client_new(256, 128);    /* Create new MQTT client */
    if (esp_sta_is_joined()) {                  /* If ESP is already joined to network */
        example_do_connect(mqtt_client);        /* Start connection to MQTT server */
    }
    
    /*
     * Make dummy delay of thread
     */
    while (1) {
        esp_delay(1000);
    }
}

/**
 * \brief           Timeout callback for MQTT events
 * \param[in]       arg: User argument
 */
void
mqtt_timeout_cb(void* arg) {
    static uint32_t num = 10;
    mqtt_client_t* client = arg;
    espr_t res;
    
    if (mqtt_client_is_connected(client)) {
        if ((res = mqtt_client_publish(client, "esp8266_mqtt_topic", "TEST DATA", 9, MQTT_QOS_EXACTLY_ONCE, 0, (void *)num)) == espOK) {
            printf("Publishing %d...\r\n", (int)num);
            num++;
        } else {
            printf("Cannot publish...: %d, client->state: %d\r\n", (int)res, (int)client->conn_state);
        }
    }
    esp_timeout_add(1000, mqtt_timeout_cb, client);
}

/**
 * \brief           MQTT event callback function
 * \param[in]       client: MQTT client where event occurred
 * \param[in]       evt: Event type and data
 */
static void
mqtt_cb(mqtt_client_t* client, mqtt_evt_t* evt) {
    switch (evt->type) {
        /*
         * Connect event
         * Called if user successfully connected to MQTT server
         * or even if connection failed for some reason
         */
        case MQTT_EVT_CONNECT: {                /* MQTT connect event occurred */
            mqtt_conn_status_t status = mqtt_client_evt_connect_get_status(client, evt);
            
            if (status == MQTT_CONN_STATUS_ACCEPTED) {
                printf("MQTT accepted!\r\n");
                /*
                 * Once we are accepted by server, 
                 * it is time to subscribe to different topics
                 * We will subscrive to "mqtt_esp_example_topic" topic,
                 * and will also set the same name as subscribe argument for callback later
                 */
                mqtt_client_subscribe(client, "esp8266_mqtt_topic", MQTT_QOS_EXACTLY_ONCE, "esp8266_mqtt_topic");
                
                /*
                 * Start timeout timer after 5000ms and call mqtt_timeout_cb function
                 */
                esp_timeout_add(5000, mqtt_timeout_cb, client);
            } else {
                printf("MQTT server connection was not successful: %d\r\n", (int)status);
                
                /*
                 * Try to connect all over again
                 */
                example_do_connect(client);
            }
            break;
        }
        
        /*
         * Subscribe event just happened.
         * Here it is time to check if it was successful or failed attempt
         */
        case MQTT_EVT_SUBSCRIBE: {
            const char* arg = mqtt_client_evt_subscribe_get_argument(client, evt);  /* Get user argument */
            espr_t res = mqtt_client_evt_subscribe_get_result(client, evt); /* Get result of subscribe event */
            
            if (res == espOK) {
                printf("Successfully subscribed to %s topic\r\n", arg);
                if (!strcmp(arg, "esp8266_mqtt_topic")) {   /* Check topic name we were subscribed */
                    /* Subscribed to "esp8266_mqtt_topic" topic */
                    
                    /*
                     * Now publish an even on example topic
                     * and set QoS to minimal value which does not guarantee message delivery to received
                     */
                    mqtt_client_publish(client, "esp8266_mqtt_topic", "test_data", 9, MQTT_QOS_AT_MOST_ONCE, 0, (void *)1);
                }
            }
            break;
        }
        
        /*
         * Message published event occurred
         */
        case MQTT_EVT_PUBLISHED: {              /* MQTT publish was successful */
            uint32_t val = (uint32_t)mqtt_client_evt_published_get_argument(client, evt);   /* Get user argument, which is in fact our custom number */
            
            printf("Publish was successful, user argument on message was: %d\r\n", (int)val);
            break;
        }
        
        /*
         * A new message was published to us
         * and now it is time to read the data
         */
        case MQTT_EVT_PUBLISH_RECV: {
            const char* topic = mqtt_client_evt_publish_recv_get_topic(client, evt);
            size_t topic_len = mqtt_client_evt_publish_recv_get_topic_len(client, evt);
            const uint8_t* payload = mqtt_client_evt_publish_recv_get_payload(client, evt);
            size_t payload_len = mqtt_client_evt_publish_recv_get_payload_len(client, evt);
            
            ESP_UNUSED(payload);
            ESP_UNUSED(payload_len);
            ESP_UNUSED(topic);
            ESP_UNUSED(topic_len);
            break;
        }
        
        /*
         * Client is fully disconnected from MQTT server
         */
        case MQTT_EVT_DISCONNECT: {
            printf("MQTT client disconnected!\r\n");
            example_do_connect(client);         /* Connect to server all over again */
            break;
        }
        
        default: 
            break;
    }
}

/** Make a connection to MQTT server in non-blocking mode */
static void
example_do_connect(mqtt_client_t* client) {
    if (client == NULL) {
        return;
    }
    
    /*
     * Start a simple connection to open source
     * MQTT server on mosquitto.org
     */
    esp_timeout_remove(mqtt_timeout_cb);
    mqtt_client_connect(mqtt_client, "test.mosquitto.org", 1883, mqtt_cb, &mqtt_client_info);
}
