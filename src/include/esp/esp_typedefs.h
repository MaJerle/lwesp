/**
 * \file            esp_typedefs.h
 * \brief           List of structures and enumerations for public usage
 */

/*
 * Copyright (c) 2018 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#ifndef __ESP_DEFS_H
#define __ESP_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/**
 * \addtogroup      ESP
 * \{
 */
 
/**
 * \defgroup        ESP_TYPEDEFS Structures and enumerations
 * \brief           List of core structures and enumerations
 * \{
 */

/**
 * \brief           Result enumeration used across application functions
 */
typedef enum {
    espOK = 0,                                  /*!< Function returned OK */
    espOKIGNOREMORE,                            /*!< Function succedded, should continue as espOK but ignore sending more data. This result is possible on connection data receive callback */
    espERR,
    espPARERR,                                  /*!< Wrong parameters on function call */
    espERRMEM,                                  /*!< Memory error occurred */
    espTIMEOUT,                                 /*!< Timeout occurred on command */
    espNOFREECONN,                              /*!< There is no free connection available to start */
    espCONT,                                    /*!< There is still some command to be processed in current command */
    espCLOSED,                                  /*!< Connection just closed */
    espINPROG,                                  /*!< Operation is in progress */
    
    espERRCONNTIMEOUT,                          /*!< Timeout received when connection to access point */
    espERRPASS,                                 /*!< Invalid password for access point */
    espERRNOAP,                                 /*!< No access point found with specific SSID and MAC address */
    espERRCONNFAIL,                             /*!< Connection failed to access point */
} espr_t;

/**
 * \brief           List of encryptions of access point
 */
typedef enum {
    ESP_ECN_OPEN = 0x00,                        /*!< No encryption on access point */
    ESP_ECN_WEP,                                /*!< WEP (Wired Equivalent Privacy) encryption */
    ESP_ECN_WPA_PSK,                            /*!< WPA (Wifi Protected Access) encryption */
    ESP_ECN_WPA2_PSK,                           /*!< WPA2 (Wifi Protected Access 2) encryption */
    ESP_ECN_WPA_WPA2_PSK,                       /*!< WPA/2 (Wifi Protected Access 1/2) encryption */
    ESP_ECN_WPA2_Enterprise                     /*!< Enterprise encryption. \note ESP is currently not able to connect to access point of this encryption type */
} esp_ecn_t;

/**
 * \brief           Access point data structure
 */
typedef struct {
    esp_ecn_t ecn;                              /*!< Encryption mode */
    char ssid[21];                              /*!< Access point name */
    int16_t rssi;                               /*!< Received signal strength indicator */
    uint8_t mac[6];                             /*!< MAC physical address */
    uint8_t ch;                                 /*!< WiFi channel used on access point */
    int8_t offset;                              /*!< Access point offset */
    uint8_t cal;                                /*!< Calibration value */
} esp_ap_t;

/**
 * \brief           IP structure
 */
typedef struct {
    uint8_t ip[4];                              /*!< IPv4 address */
} esp_ip_t;

/**
 * \brief           Port variable
 */
typedef uint16_t    esp_port_t;

/**
 * \brief           MAC address
 */
typedef struct {
    uint8_t mac[6];                             /*!< MAC address */
} esp_mac_t;

/**
 * \brief           Station data structure
 */
typedef struct {
    uint8_t ip[4];                              /*!< IP address of connected station */
    uint8_t mac[6];                             /*!< MAC address of connected station */
} esp_sta_t;

/**
 * \brief           Date and time structure
 */
typedef struct {
    uint8_t date;                               /*!< Day in a month, from 1 to up to 31 */
    uint8_t month;                              /*!< Month in a year, from 1 to 12 */
    uint16_t year;                              /*!< Year */
    uint8_t day;                                /*!< Day in a week, from 1 to 7 */
    uint8_t hours;                              /*!< Hours in a day, from 0 to 23 */
    uint8_t minutes;                            /*!< Minutes in a hour, from 0 to 59 */
    uint8_t seconds;                            /*!< Seconds in a minute, from 0 to 59 */
} esp_datetime_t;

/**
 * \}
 */

/**
 * \}
 */

#if defined(ESP_INTERNAL) || __DOXYGEN__

#if 0
#define ESP_MSG_VAR_DEFINE(name)                esp_msg_t name
#define ESP_MSG_VAR_ALLOC(name)                  
#define ESP_MSG_VAR_REF(name)                   name
#define ESP_MSG_VAR_FREE(name)                      
#else /* 1 */
#define ESP_MSG_VAR_DEFINE(name)                esp_msg_t* name
#define ESP_MSG_VAR_ALLOC(name)                 do {\
    (name) = esp_mem_alloc(sizeof(*(name)));          \
    ESP_DEBUGW(ESP_CFG_DBG_VAR | ESP_DBG_TYPE_TRACE, (name) != NULL, "MSG VAR: Allocated %d bytes at %p\r\n", sizeof(*(name)), (name)); \
    ESP_DEBUGW(ESP_CFG_DBG_VAR | ESP_DBG_TYPE_TRACE, (name) == NULL, "MSG VAR: Error allocating %d bytes\r\n", sizeof(*(name))); \
    if (!(name)) {                                  \
        return espERRMEM;                           \
    }                                               \
    memset(name, 0x00, sizeof(*(name)));            \
} while (0)
#define ESP_MSG_VAR_REF(name)                   (*(name))
#define ESP_MSG_VAR_FREE(name)                  do {\
    ESP_DEBUGF(ESP_CFG_DBG_VAR | ESP_DBG_TYPE_TRACE, "MSG VAR: Free memory: %p\r\n", (name)); \
    esp_mem_free(name);                             \
    (name) = NULL;                                  \
} while (0)
#endif /* !1 */

#endif /* defined(ESP_INTERNAL) || __DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_DEFS_H */
