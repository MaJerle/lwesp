/*
 * Simple MQTT aynchronous cayenne connectivitiy for publish-mode only.
 * Received events are not parsed and not processed by the lib
 */
#include "cayenne_async_mqtt.h"
#include "lwesp/apps/lwesp_mqtt_client.h"
#include "lwesp/apps/lwesp_cayenne.h"
#include "lwesp/lwesp_timeout.h"
#include "lwc/lwc.h"

/* For STM32H7xx project, include "debug.h" file which implements debug_printf debugging messages.. */
#if defined(STM32H7xx)
#include "debug.h"
#else
#define debug_printf                    printf
#endif /* defined(STM32H7xx) */

/**
 * \brief           MQTT client info for server
 */
static const lwesp_mqtt_client_info_t
mqtt_client_info = {
    /* Device ID */
#if defined(STM32H7xx)
    .id = "a6537950-7637-11ec-8da3-474359af83d7",
#else
    .id = "869f5a20-af9c-11e9-b01f-db5cf74e7fb7",
#endif /* defined(STM32H7xx) */

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
    if (!lwesp_buff_init(&cayenne_async_data_buff, 128 * sizeof(cayenne_async_data_t))) {
        return 0;
    }

    /* Create object and try to connect */
    if ((mqtt_client = lwesp_mqtt_client_new(2048, 256)) == NULL) {
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
        sprintf(topic, "%s/%s/things/%s/data/%d", LWESP_CAYENNE_API_VERSION, mqtt_client_info.user, mqtt_client_info.id, (int)(dptr->channel));

        /* Build data */
        if (dptr->type == CAYENNE_DATA_TYPE_TEMP) {
            sprintf(tx_data, "temp,c=%d.%03d", (int)(dptr->data.flt), (int)((dptr->data.flt - (float)((int)dptr->data.flt)) * 1000));
        } else if (dptr->type == CAYENNE_DATA_TYPE_OUTPUT_STATUS_DIGITAL) {
            sprintf(tx_data, "digital_sensor=%d", (int)dptr->data.u32);
        } else if (dptr->type == CAYENNE_DATA_TYPE_OUTPUT_STATUS_ANALOG) {
            sprintf(tx_data, "analog_sensor=%d.%03d", (int)(dptr->data.flt), (int)((dptr->data.flt - (float)((int)dptr->data.flt)) * 1000));
        }

        /* Now try to publish message */
        if (lwesp_mqtt_client_is_connected(mqtt_client)) {
            if ((res = lwesp_mqtt_client_publish(mqtt_client, topic, tx_data, LWESP_U16(strlen(tx_data)), LWESP_MQTT_QOS_AT_LEAST_ONCE, 0, NULL)) == lwespOK) {
                debug_printf("[MQTT Cayenne] Publishing: Channel: %d, data: %s\r\n", (int)dptr->channel, tx_data);
                lwesp_buff_skip(&cayenne_async_data_buff, sizeof(*dptr));
                try_next = 1;
            } else {
                debug_printf("[MQTT Cayenne] Cannot publish now, will try later. Error code: %d\r\n", (int)res);
            }
        }
    }
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
                debug_printf("[MQTT Cayenne] Connection accepted, starting transmitting\r\n");
                /* Subscribe here if necessary */
                prv_try_send();                 /* Start sending data */
            } else {
                debug_printf("[MQTT Cayenne] Not accepted, trying again..\r\n");
                prv_try_connect();
            }
            break;
        }

        /* Message published event occurred */
        case LWESP_MQTT_EVT_PUBLISH: {
            prv_try_send();
            break;
        }

        /* Message received = we don't care for it */
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
            debug_printf("[MQTT Cayenne] MQTT client disconnected!\r\n");
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

    debug_printf("[MQTT Cayenne] Trying to connect to server\r\n");

    /* Start a simple connection to open source */
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
        case LWESP_EVT_KEEP_ALIVE: {
            prv_try_send();                     /* Try to send data */
            break;
        }
        case LWESP_EVT_WIFI_GOT_IP: {
            debug_printf("[MQTT Cayenne] Wifi got IP, let's gooo\r\n");
            prv_try_connect();                  /* Start connection after we have a connection to network client */
            break;
        }
        default:
            break;
    }
    return lwespOK;
}
