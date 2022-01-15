#ifndef SNIPPET_HDR_CAYENNE_ASYNC_MQTT_H
#define SNIPPET_HDR_CAYENNE_ASYNC_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "lwesp/lwesp.h"
#include "lwesp/apps/lwesp_mqtt_client.h"
#include "lwesp/apps/lwesp_cayenne.h"

/**
 * \brief           Data type for publishing
 */
typedef enum {
    CAYENNE_DATA_TYPE_TEMP,
    CAYENNE_DATA_TYPE_OUTPUT_STATUS,
    CAYENNE_DATA_TYPE_END,
} cayenne_data_type_t;

/**
 * \brief           Data structure for cayenne buffer entry
 */
typedef struct {
    cayenne_data_type_t type;       /* Message format */
    uint32_t channel;               /* Channel number to update */
    union {
        float flt;                  /* Float format. used for Temperature */
        uint32_t u32;               /* Uint32 for output status */
        int32_t i32;
    } data;
} cayenne_async_data_t;

/* Data buffer */
extern lwesp_buff_t cayenne_async_data_buff;

/* Entry function mode */
uint8_t cayenne_async_mqtt_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SNIPPET_HDR_CAYENNE_H */
