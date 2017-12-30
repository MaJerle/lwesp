/**
* \file            esp_typedefs.h
*
* Copyright (c) 2017, Tilen MAJERLE
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in the
*      documentation and/or other materials provided with the distribution.
*  * Neither the name of the author nor the
*      names of its contributors may be used to endorse or promote products
*      derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* \author           Tilen MAJERLE <tilen@majerle.eu>
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
    esp_ecn_t ecn;                              /*!< Ecryption mode */
    char ssid[21];                              /*!< Access point name */
    int16_t rssi;                               /*!< Received signal strength indicator */
    uint8_t mac[6];                             /*!< MAC physical address */
    uint8_t ch;                                 /*!< WiFi channel used on access point */
    int8_t offset;                              /*!< Access point offset */
    uint8_t cal;                                /*!< Calibration value */
} esp_ap_t;

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
#define ESP_MSG_VAR_ALLOC(name)                 do { name = esp_mem_alloc(sizeof(*(name))); if (!(name)) { ESP_DEBUGF(ESP_DBG_VAR, "Error allocating: %d bytes\r\n", sizeof(*(name))); return espERRMEM; } memset(name, 0x00, sizeof(*(name))); } while (0)
#define ESP_MSG_VAR_REF(name)                   (*(name))
#define ESP_MSG_VAR_FREE(name)                  esp_mem_free(name)
#endif /* !1 */

#endif /* defined(ESP_INTERNAL) || __DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_DEFS_H */
