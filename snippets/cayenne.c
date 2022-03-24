/*
 * Cayenne cloud connectivity with MQTT client asynchronous connection
 */
#include "lwesp/apps/lwesp_cayenne.h"
#include "lwesp/apps/lwesp_cayenne_evt.h"

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
    /* User & device credentials */
    .user = "8a215f70-a644-11e8-ac49-e932ed599553",
    .pass = "26aa943f702e5e780f015cd048a91e8fb54cca28",
#if defined(STM32H7xx)
    .id = "a6537950-7637-11ec-8da3-474359af83d7",
#else
    .id = "869f5a20-af9c-11e9-b01f-db5cf74e7fb7",
#endif /* defined(STM32H7xx) */

    .keep_alive = 10,
};

/**
 * \brief           Cayenne handle
 */
static lwesp_cayenne_t cayenne = {
    .client_buff_tx_len = 1024,
    .tx_buff_count = 32
};

/**
 * \brief           Cayenne event function
 * \param[in]       c: Cayenne instance
 * \param[in]       evt: Event data
 */
static lwespr_t
prv_cayenne_evt(lwesp_cayenne_t* c, lwesp_cayenne_evt_t* evt) {
    switch (lwesp_cayenne_evt_get_type(evt)) {
        case LWESP_CAYENNE_EVT_CONNECT: {
            lwesp_cayenne_tx_msg_t tx_msg = {
                .channel = LWESP_CAYENNE_NO_CHANNEL,
                .data_type_unit = LWESP_CAYENNE_DATA_TYPE_END_UNIT_END,
                .topic = LWESP_CAYENNE_TOPIC_DATA,
            };

            /* We are connected, build schema */
            debug_printf("[CAYENNE APP] Just now connected...sending up default (or up-to-date) data\r\n");

            /* Send device description */
            tx_msg.topic = LWESP_CAYENNE_TOPIC_SYS_MODEL;
            tx_msg.data_format = LWESP_CAYENNE_DATA_FORMAT_STRING;
            tx_msg.data.str = "My custom model";
            lwesp_cayenne_publish_ex(&cayenne, &tx_msg);
            tx_msg.topic = LWESP_CAYENNE_TOPIC_SYS_CPU_SPEED;
            tx_msg.data_format = LWESP_CAYENNE_DATA_FORMAT_STRING;
            tx_msg.data.str = "550000000";
            lwesp_cayenne_publish_ex(&cayenne, &tx_msg);

            /* Sensors.. */
            tx_msg.topic = LWESP_CAYENNE_TOPIC_DATA;
            tx_msg.data_format = LWESP_CAYENNE_DATA_FORMAT_FLOAT;
            tx_msg.data_type_unit = LWESP_CAYENNE_DATA_TYPE_TEMPERATURE_UNIT_CELSIUS;
            tx_msg.channel = 1;
            tx_msg.data.flt = 20.7;
            lwesp_cayenne_publish_ex(&cayenne, &tx_msg);
            tx_msg.channel = 2;
            tx_msg.data.flt = 23.7;
            lwesp_cayenne_publish_ex(&cayenne, &tx_msg);
            tx_msg.channel = 3;
            tx_msg.data.flt = 26.7;
            lwesp_cayenne_publish_ex(&cayenne, &tx_msg);

            break;
        }
        case LWESP_CAYENNE_EVT_DATA: {
            /* Reply with the same */
            debug_printf("[CAYENNE APP] data received: %s\r\n", evt->evt.data.msg->values[0].value);
            lwesp_cayenne_publish_response(c, evt->evt.data.msg, LWESP_CAYENNE_RESP_OK, "0");
            //lwesp_cayenne_publish_data(c, LWESP_CAYENNE_TOPIC_DATA, evt->evt.data.msg->channel, , evt->evt.data.msg->values[0].value);
            //evt->evt.data.msg->channel
            break;
        }
        case LWESP_CAYENNE_EVT_DISCONNECT: {
            debug_printf("[CAYENNE APP] Just now disconnected...that's sad story\r\n");
            break;
        }
        default:
            break;
    }
    return lwespOK;
}

/**
 * \brief           MQTT cayenne thread
 * \param[in]       arg: User argument
 */
void
cayenne_thread(void const* arg) {
    char s[20];
    float temp = 0.1f;

    while (!lwesp_sta_has_ip()) {
        lwesp_delay(1000);
    }

    /* Start cayenne instance */
    lwesp_cayenne_init();
    if (lwesp_cayenne_create(&cayenne, &mqtt_client_info, prv_cayenne_evt) != lwespOK) {
        debug_printf("[CAYENNE] Cannot create cayenne instance..\r\n");
    }

    static lwesp_cayenne_tx_msg_t tx_msg;
    static lwesp_cayenne_data_type_unit_t type_unit = (lwesp_cayenne_data_type_unit_t)0;
    while (1) {
        for (size_t i = 0; i < 3; ++i) {
            tx_msg.channel = 100 + type_unit;
            tx_msg.data_format = LWESP_CAYENNE_DATA_FORMAT_FLOAT;
            tx_msg.data_type_unit = type_unit;
            tx_msg.data.flt = temp;
            tx_msg.topic = LWESP_CAYENNE_TOPIC_DATA;
            //lwesp_cayenne_publish_ex(&cayenne, &tx_msg);

            /* Calculate new values */
            temp = temp * 1.05;
            ++type_unit;
            if (type_unit >= LWESP_CAYENNE_DATA_TYPE_END_UNIT_END) {
                type_unit = (lwesp_cayenne_data_type_unit_t)0;
            }
        }
        lwesp_delay(5000);
    }
    lwesp_sys_thread_terminate(NULL);
}
