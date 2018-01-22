#include "mqtt.h"
#include "esp/esp.h"
#include "esp/apps/esp_mqtt_client.h"
#include "esp/esp_timeout.h"

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

static espr_t
mqtt_esp_cb(esp_cb_t* cb) {
    switch (cb->type) {
        case ESP_CB_WIFI_GOT_IP: {
            example_do_connect(mqtt_client);    /* Start connection after we have a connection to network client */
            break;
        }
        default: break;
    }
    return espOK;
}

/**
 * \brief           MQTT client thread
 * \param[in]       arg: User argument
 */
void
mqtt_thread(void const* arg) {
    esp_cb_register(mqtt_esp_cb);
    
    /*
     * Create a new client with 256 bytes of RAW TX data
     * and 128 bytes of RAW incoming data
     */
    mqtt_client = mqtt_client_new(256, 128);
    if (esp_sta_joined() == espOK) {
        example_do_connect(mqtt_client);
    }
    
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
    
    if (mqtt_client_is_connected(client) == espOK) {
        if ((res = mqtt_client_publish(client, "stm32f7_topic0", "TEST DATA", 9, MQTT_QOS_EXACTLY_ONCE, 0, (void *)num)) == espOK) {
            printf("Publishing %d...\r\n", (int)num);
            num++;
        } else {
            printf("Cannot publish...: %d, client->state: %d\r\n", (int)res, (int)client->conn_state);
        }
    }
    esp_timeout_add(500, mqtt_timeout_cb, client);
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
            mqtt_conn_status_t status = evt->evt.connect.status;
            
            if (status == MQTT_CONN_STATUS_ACCEPTED) {
                printf("MQTT accepted!\r\n");
                /*
                 * Once we are accepted on server, 
                 * it is time to subscribe to differen topics
                 * We will subscrive to "mqtt_esp_example_topic" topic,
                 * and will also set the same name as subscribe argument for callback later
                 */
                mqtt_client_subscribe(client, "stm32f7_topic0", MQTT_QOS_EXACTLY_ONCE, "mqtt_esp_example_topic");
                
                esp_timeout_add(5000, mqtt_timeout_cb, client);
            } else {
                printf("MQTT server connection was not successful: %d\r\n", (int)status);
                /* Maybe close connection at this point and try again? */
                
                example_do_connect(client);
            }
            break;
        }
        
        /*
         * Subscribe event just happened.
         * Here it is time to check if it was successful or failed attempt
         */
        case MQTT_EVT_SUBSCRIBE: {
            const char* arg = evt->evt.sub_unsub_scribed.arg;   /* Get user argument */
            espr_t res = evt->evt.sub_unsub_scribed.res;    /* Get result of subscribe event */
            
            if (res == espOK) {
                printf("Successfully subscribed to %s topic\r\n", arg);
                if (!strcmp(arg, "mqtt_esp_example_topic")) {   /* Check topic name we were subscribed */
                    /* Subscribed to "example_topic" topic */
                    
                    /*
                     * Now publish an even on example topic
                     * and set QoS to minimal value which does not guarantee message delivery to received
                     */
                    mqtt_client_publish(client, "mqtt_esp_example_topic", "my_data", 7, MQTT_QOS_AT_MOST_ONCE, 0, (void *)1);
                }
            }
            
            break;
        }
        
        /*
         * Message published event occurred
         */
        case MQTT_EVT_PUBLISHED: {              /* MQTT publish was successful */
            uint32_t val = (uint32_t)evt->evt.published.arg;    /* Get user argument, which is in fact our custom number */
            
            printf("Publish was successful, user argument on message was: %d\r\n", (int)val);
            
            break;
        }
        
        /*
         * A new message was published to us
         * and now it is time to read the data
         */
        case MQTT_EVT_PUBLISH_RECV: {
            const char* topic = (const char *)evt->evt.publish_recv.topic;
            uint16_t topic_len = (uint16_t)evt->evt.publish_recv.topic_len;
            const uint8_t* payload = evt->evt.publish_recv.payload;
            uint16_t payload_len = (uint16_t)evt->evt.publish_recv.payload_len;
            
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
            example_do_connect(client);         /* Connect to server */
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

