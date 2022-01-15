/*
 * Simple MQTT aynchronous cayenne connectivitiy for publish-mode only.
 * Received events are not parsed and not processed by the lib
 */
#include "cayenne_async_mqtt.h"
#include "lwesp/apps/lwesp_mqtt_client.h"
#include "lwesp/apps/lwesp_cayenne.h"
#include "lwesp/lwesp_timeout.h"

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

/* Client object */
static lwesp_mqtt_client_p mqtt_client;

/* Data buffer */
lwesp_buff_t cayenne_async_data_buff;

/* Private functions */
static lwespr_t prv_evt_fn(lwesp_evt_t* evt);
static void prv_try_connect(void);

/* Entry function mode */
uint8_t
cayenne_async_mqtt_init(void) {
    lwesp_evt_register(prv_evt_fn);             /* Register event function to receive system messages */

    /* Create buffer */
    if (!lwesp_buff_init(&cayenne_async_data_buff, 16 * sizeof(cayenne_async_data_t))) {
        return 0;
    }

    /* Create object and try to connect */
    if ((mqtt_client = lwesp_mqtt_client_new(1024, 256)) == NULL) {
        lwesp_buff_free(&cayenne_async_data_buff);
        return 0;
    }
    if (lwesp_sta_is_joined()) {
        prv_try_connect();
    }
    return 1;
}

/**
 * \brief           Try to send data over MQTT
 */
static void
prv_try_send(void) {
    static char topic[256];
    static char tx_data[20];
    lwespr_t res;
    const cayenne_async_data_t* dptr;
    uint8_t try_next = 1;

    /* Get full linear entry */
    while (try_next && lwesp_buff_get_linear_block_read_length(&cayenne_async_data_buff) >= sizeof(*dptr)) {
        dptr = lwesp_buff_get_linear_block_read_address(&cayenne_async_data_buff);
        try_next = 0;

        /* Build topic */
        sprintf(topic, "%s/%s/things/%s/data/%d", LWESP_CAYENNE_API_VERSION, mqtt_client_info.user, mqtt_client_info.id, dptr->channel);

        /* Build data */
        if (dptr->type == CAYENNE_DATA_TYPE_TEMP) {
            sprintf(tx_data, "temp,c=%f", dptr->data.flt);
        } else if (dptr->type == CAYENNE_DATA_TYPE_OUTPUT_STATUS) {
            sprintf(tx_data, "digital=%d", (int)dptr->data.u32);
        }

        /* Now try to publish message */
        if (lwesp_mqtt_client_is_connected(mqtt_client)) {
            if ((res = lwesp_mqtt_client_publish(mqtt_client, topic, tx_data, LWESP_U16(strlen(tx_data)), LWESP_MQTT_QOS_AT_LEAST_ONCE, 0, NULL)) == lwespOK) {
                printf("Publishing...\r\n");
                lwesp_buff_skip(&cayenne_async_data_buff, sizeof(*dptr));
                try_next = 1;
            } else {
                printf("Cannot publish...: %d\r\n", (int)res);
            }
        }
    }
}

/**
 * \brief           Timeout callback for MQTT events
 * \param[in]       arg: User argument
 */
void
prv_mqtt_timeout_cb(void* arg) {
    prv_try_send();
    lwesp_timeout_add(1000, prv_mqtt_timeout_cb, arg);
}

/**
 * \brief           MQTT event callback function
 * \param[in]       client: MQTT client where event occurred
 * \param[in]       evt: Event type and data
 */
static void
prv_mqtt_cb(lwesp_mqtt_client_p client, lwesp_mqtt_evt_t* evt) {
    switch (lwesp_mqtt_client_evt_get_type(client, evt)) {
        /*
         * Connect event
         * Called if user successfully connected to MQTT server
         * or even if connection failed for some reason
         */
        case LWESP_MQTT_EVT_CONNECT: {            /* MQTT connect event occurred */
            lwesp_mqtt_conn_status_t status = lwesp_mqtt_client_evt_connect_get_status(client, evt);

            if (status == LWESP_MQTT_CONN_STATUS_ACCEPTED) {
                printf("Connection accepted, starting timeout for publishing\r\n");
                /* Subscribe here if necessary */
                //lwesp_mqtt_client_subscribe(client, "esp8266_mqtt_topic", LWESP_MQTT_QOS_EXACTLY_ONCE, "esp8266_mqtt_topic");
                lwesp_timeout_add(1000, prv_mqtt_timeout_cb, client);
            } else {
                printf("Not accepted, trying again..\r\n");
                prv_try_connect();
            }
            break;
        }

        /*
         * Subscribe event just happened.
         * Here it is time to check if it was successful or failed attempt
         */
        case LWESP_MQTT_EVT_SUBSCRIBE: {
            const char* arg = lwesp_mqtt_client_evt_subscribe_get_argument(client, evt);  /* Get user argument */
            lwespr_t res = lwesp_mqtt_client_evt_subscribe_get_result(client, evt); /* Get result of subscribe event */

            if (res == lwespOK) {
                printf("Successfully subscribed to %s topic\r\n", arg);
                if (!strcmp(arg, "esp8266_mqtt_topic")) {   /* Check topic name we were subscribed */
                    /* Subscribed to "esp8266_mqtt_topic" topic */

                    /*
                     * Now publish an even on example topic
                     * and set QoS to minimal value which does not guarantee message delivery to received
                     */
                    lwesp_mqtt_client_publish(client, "esp8266_mqtt_topic", "test_data", 9, LWESP_MQTT_QOS_AT_MOST_ONCE, 0, NULL);
                }
            }
            break;
        }

        /* Message published event occurred */
        case LWESP_MQTT_EVT_PUBLISH: {
            printf("Publish event\r\n");
            break;
        }

        /*
         * A new message was published to us
         * and now it is time to read the data
         */
        case LWESP_MQTT_EVT_PUBLISH_RECV: {
            const char* topic = lwesp_mqtt_client_evt_publish_recv_get_topic(client, evt);
            size_t topic_len = lwesp_mqtt_client_evt_publish_recv_get_topic_len(client, evt);
            const uint8_t* payload = lwesp_mqtt_client_evt_publish_recv_get_payload(client, evt);
            size_t payload_len = lwesp_mqtt_client_evt_publish_recv_get_payload_len(client, evt);

            LWESP_UNUSED(payload);
            LWESP_UNUSED(payload_len);
            LWESP_UNUSED(topic);
            LWESP_UNUSED(topic_len);
            break;
        }

        /* Client is fully disconnected from MQTT server */
        case LWESP_MQTT_EVT_DISCONNECT: {
            printf("MQTT client disconnected!\r\n");
            prv_try_connect();
            break;
        }
        default:
            break;
    }
}

/**
 * \brief           Try to start client connection with MQTT server
 */
static void
prv_try_connect(void) {
    if (mqtt_client == NULL) {
        return;
    }

    /* Start a simple connection to open source */
    lwesp_timeout_remove(prv_mqtt_timeout_cb);
    lwesp_mqtt_client_connect(mqtt_client, LWESP_CAYENNE_HOST, LWESP_CAYENNE_PORT, prv_mqtt_cb, &mqtt_client_info);
}

/**
 * \brief           LwESP system callback function
 * \param           evt: Event object with data 
 * \return          Success status 
 */
static lwespr_t
prv_evt_fn(lwesp_evt_t* evt) {
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_WIFI_GOT_IP: {
            prv_try_connect();                  /* Start connection after we have a connection to network client */
            break;
        }
        default:
            break;
    }
    return lwespOK;
}
