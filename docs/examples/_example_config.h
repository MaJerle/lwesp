#ifndef __ESP_CONFIG_H
#define __ESP_CONFIG_H  100

/*
 * Rename this file to "esp_config.h" for your application
 */

/* First include debug */
#include "esp/esp_debug.h"

/*
 * Increase default receive buffer length
 */
#define ESP_RCV_BUFF_SIZE                   0x800
 
/* After user configuration, call default config to merge config together */
#include "esp/esp_config_default.h"

#endif /* __ESP_CONFIG_H */ 