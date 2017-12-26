/**
 * \file            esp_config.c
 * \brief           Configuration for ESP
 */

/*
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
 * \author          Tilen MAJERLE <tilen@majerle.eu>
 */
#ifndef __ESP_CONFIG_H
#define __ESP_CONFIG_H  100

#include "esp/esp_debug.h"

/**
 * User specific config which overwrites setup from esp_config_default.h file
 */

#if !__DOXYGEN__
#define ESP_NETCONN                         1
#define ESP_DBG_IPD                         ESP_DBG_OFF
#define ESP_DBG_SERVER                      ESP_DBG_OFF
#define ESP_DBG_MQTT                        ESP_DBG_ON
#define ESP_DBG_MEM                         ESP_DBG_ON
#define ESP_RCV_BUFF_SIZE                   0x1000

#define ESP_IPD_MAX_BUFF_SIZE               1460

#define ESP_INPUT_USE_PROCESS               0

#define ESP_AT_ECHO                         1

#define ESP_MAX_CONNS                       20

#define ESP_SNTP                            1
#endif /* !__DOXYGEN__ */

/* Include default configuration setup */
#include "esp/esp_config_default.h"
 
/**
 * \}
 */

#endif /* __ESP_CONFIG_H */
