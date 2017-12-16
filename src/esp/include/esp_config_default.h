/**
 * \file            esp_config.c
 * \brief           Default configuration for ESP
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
#ifndef __ESP_DEFAULT_CONFIG_H
#define __ESP_DEFAULT_CONFIG_H  100

/**
 * \addtogroup      ESP
 * \{
 */

/**
 * \defgroup        ESP_CONF Configuration
 * \brief           Configuration parameters for ESP8266 library
 * \{
 */
 
/**
 * \brief           Enables (1) or disables (0) operating system support for ESP
 *
 * \note            Value must be set to 1 in the current revision
 *
 * \note            Check \ref ESP_CONF_OS group for more configuration related to operating system
 */
#ifndef ESP_OS
#define ESP_OS                              1
#endif

/**
 * \brief           Memory alignment for dynamic memory allocations
 * \note            Some CPUs can work faster if memory is aligned, usually 4 or 8 bytes.
 *                  To speed up this possibilities, you can set memory alignment and library
 *                  will try to allocate memory on aligned position.
 *
 * \note            Some CPUs such ARM Cortex-M0 don't support unaligned memory access.
 *                  This CPUs must have set correct memory alignment value. 
 *
 * \note            This value must be power of 2
 */
#ifndef ESP_MEM_ALIGNMENT
#define ESP_MEM_ALIGNMENT                   4
#endif

/**
 * \brief           Maximal number of connections AT software can support on ESP device
 * \note            In case of official AT software, leave this on default value (5)
 */
#ifndef ESP_MAX_CONNS
#define ESP_MAX_CONNS                       5
#endif

/**
 * \brief           Maximal number of bytes we can send at single command to ESP
 * \note            Value can not exceed 2048 bytes or no data will be ever send
 */
#ifndef ESP_CONN_MAX_DATA_LEN
#define ESP_CONN_MAX_DATA_LEN               2048
#endif

/**
 * \brief           Set number of retries for send data command.
 *
 *                  Sometimes can happen that AT+SEND fails due to different problems.
 *                  In that case, try again can help and solve the problem.
 */
#ifndef ESP_MAX_SEND_RETRIES
#define ESP_MAX_SEND_RETRIES                3
#endif

/**
 * \brief           Maximal buffer size for entries in +IPD statement from ESP
 *                  If +IPD length is larger that this setting, 
 *                  multiple pbuf entries will be created to hold entire +IPD
 */
#ifndef ESP_IPD_MAX_BUFF_SIZE
#define ESP_IPD_MAX_BUFF_SIZE               1460
#endif

/**
 * \brief           Default baudrate used for AT port
 *
 * \note            Later, user may call API function to change to desired baudrate if necessary
 */
#ifndef ESP_AT_PORT_BAUDRATE
#define ESP_AT_PORT_BAUDRATE                115200
#endif

/**
 * \brief           Enables (1) or disables (1) ESP acting as station
 * 
 * \note            When device is in station mode, it can connect to other access points
 */
#ifndef ESP_MODE_STATION
#define ESP_MODE_STATION                    1
#endif

/**
 * \brief           Enables (1) or disables (1) ESP acting as access point
 * 
 * \note            When device is in access point mode, it can accept connections from other stations
 */
#ifndef ESP_MODE_ACCESS_POINT
#define ESP_MODE_ACCESS_POINT               1
#endif

/**
 * \brief           Buffer size for received data waiting to be processed
 * \note            When server mode is active and a lot of connections are in queue
 *                  this should be set high otherwise your buffer may overflow
 * 
 * \note            Buffer size also depends on TX user driver if it uses DMA or blocking mode
 *                  In case of DMA (CPU can work other tasks), buffer may be smaller as CPU
 *                  will have more time to process all the incoming bytes
 *
 * \note            This parameter has no meaning when \ref ESP_INPUT_USE_PROCESS is enabled
 */
#ifndef ESP_RCV_BUFF_SIZE
#define ESP_RCV_BUFF_SIZE                   0x400
#endif

/**
 * \defgroup        ESP_CONF_DBG Debugging
 * \brief           Debugging configurations
 * \{
 */

/**
 * \brief           Set debug level for memory manager.
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_MEM
#define ESP_DBG_MEM                         ESP_DBG_OFF
#endif

/**
 * \brief           Set debug level for input module
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_INPUT
#define ESP_DBG_INPUT                       ESP_DBG_OFF
#endif

/**
 * \brief           Set debug level for ESP threads
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_THREAD
#define ESP_DBG_THREAD                      ESP_DBG_OFF
#endif

/**
 * \brief           Set debug level for asserting of input variables
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_ASSERT
#define ESP_DBG_ASSERT                      ESP_DBG_OFF
#endif

/**
 * \brief           Set debug level for incoming data received from device
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_IPD
#define ESP_DBG_IPD                         ESP_DBG_OFF
#endif

/**
 * \brief           Set debug level for netconn sequential API
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_NETCONN
#define ESP_DBG_NETCONN                     ESP_DBG_OFF
#endif

/**
 * \brief           Set debug level for packet buffer manager
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_PBUF
#define ESP_DBG_PBUF                        ESP_DBG_OFF
#endif

/**
 * \brief           Set debug level for dynamic variable allocations
 *                  Possible values are \ref ESP_DBG_ON or \ref ESP_DBG_OFF
 */
#ifndef ESP_DBG_VAR
#define ESP_DBG_VAR                         ESP_DBG_OFF
#endif
 
/**
 * \}
 */

/**
 * \defgroup        ESP_CONF_OS
 * \brief           Operating system dependant configuration
 * \{
 */
 
/**
 * \brief           Set number of message queue entries for procuder thread
 *                  Message queue is used for storing memory address to command data
 */
#ifndef ESP_THREAD_PRODUCER_MBOX_SIZE
#define ESP_THREAD_PRODUCER_MBOX_SIZE       10
#endif

/**
 * \brief           Set number of message queue entries for processing thread
 *                  Message queue is used to notify processing thread about new received data on AT port
 *
 * \note            This parameter has no meaning when \ref ESP_INPUT_USE_PROCESS is enabled
 */
#ifndef ESP_THREAD_PROCESS_MBOX_SIZE
#define ESP_THREAD_PROCESS_MBOX_SIZE        10
#endif

/**
 * \brief           Enables (1) or disables (0) direct support for processing input data
 *                  When this mode is enabled, no overhead is included for copying data
 *                  to receive buffer because bytes are processed directly.
 *
 * \note            This mode can only be used when \ref ESP_OS is enabled
 *
 * \note            When using this mode, separate thread must be dedicated only 
 *                  for reading data on AT port
 *
 * \note            Best case for using this mode is if DMA receive is supported by host device
 */
#ifndef ESP_INPUT_USE_PROCESS
#define ESP_INPUT_USE_PROCESS               0
#endif
 
/**
 * \}
 */

/**
 * \defgroup        ESP_CONF_MODULES
 * \brief           Configuration of specific modules
 * \{
 */

/**
 * \brief           Enables (1) or disables (0) NETCONN sequential API support for OS systems
 *
 * \note            To use this feature, OS support is mandatory. 
 * \sa              ESP_OS
 */
#ifndef ESP_NETCONN
#define ESP_NETCONN                         0
#endif
 
/**
 * \brief           Enables (1) or disables (0) support for DNS functions
 *
 */
#ifndef ESP_DNS
#define ESP_DNS                             0
#endif
 
/**
 * \brief           Enables (1) or disables (0) support for ping functions
 *
 */
#ifndef ESP_PING
#define ESP_PING                            0
#endif

/**
 * \brief           Enables (1) or disables (0) support for SNTP protocol with AT commands
 *
 */
#ifndef ESP_SNTP
#define ESP_SNTP                            0
#endif

/**
 * \}
 */

/**
 * \}
 */
 
/**
 * \}
 */
 
#if !__DOXYGEN__

/* Define group mode value */
#define ESP_MODE_STATION_ACCESS_POINT   (ESP_MODE_STATION && ESP_MODE_ACCESS_POINT)

/* At least one of them must be enabled */
#if !ESP_MODE_STATION && !ESP_MODE_ACCESS_POINT
#error "Invalid ESP configuration. ESP_MODE_STATION and ESP_MODE_STATION cannot be disabled at the same time!"
#endif

#if !ESP_OS
    #if ESP_INPUT_USE_PROCESS
    #error "ESP_INPUT_USE_PROCESS may only be enabled when OS is used"
    #endif /* ESP_INPUT_USE_PROCESS */
#endif /* !ESP_OS */

#endif /* !__DOXYGEN__ */

#endif /* __ESP_DEFAULT_CONFIG_H */
