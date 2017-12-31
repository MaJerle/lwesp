#ifndef __ESP_CONFIG_H
#define __ESP_CONFIG_H  100

/*
 * Rename this file to "esp_config.h" for your application
 */

/* First include debug */
#include "esp/esp_debug.h"

/*
 * Open "include/esp/esp_config_default.h" and 
 * copy & replace settings you want to change here
 */


/* After user configuration, call default config to merge config together */
#include "esp/esp_config_default.h"


#endif /* __ESP_CONFIG_H */ 