/**	
 * \file            esp_sntp.c
 * \brief           Connection API
 */
 
/*
 * Copyright (c) 2017 Tilen Majerle
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "esp/esp_private.h"
#include "esp/esp_sntp.h"
#include "esp/esp_mem.h"

/**
 * \addtogroup      ESP_SNTP
 * \{
 *
 * SNTP protocol allows you to get current time from network servers.
 * 
 * \code{c}
esp_datetime_t dt;

//Configure SNTP parameters: enable, timezone +1 and use default network servers
if (esp_sntp_configure(1, 1, NULL, NULL, NULL, 1) == espOK) {
    //Try to get time from network servers
    if (esp_sntp_gettime(&dt, 1) == espOK) {
        printf("We have a date and time: %d.%d.%d: %d:%d:%d\r\n", 
            (int)dt.date, (int)dt.month, (int)dt.year, 
            (int)dt.hours, (int)dt.minutes, (int)dt.seconds
        );
    }
}
\endcode
 *
 * \}
 */

#if ESP_SNTP || __DOXYGEN__

/**
 * \brief           Configure SNTP mode parameters
 * \param[in]       en: Status whether SNTP mode is enabled or disabled on ESP device
 * \param[in]       tz: Timezone to use when SNTP acquires time, between -11 and 13
 * \param[in]       h1: Optional first SNTP server for time. Set to NULL if not used
 * \param[in]       h2: Optional second SNTP server for time. Set to NULL if not used
 * \param[in]       h3: Optional third SNTP server for time. Set to NULL if not used
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_sntp_configure(uint8_t en, int8_t tz, const char* h1, const char* h2, const char* h3, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSNTPCFG;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sntp_cfg.en = en;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sntp_cfg.tz = tz;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sntp_cfg.h1 = h1;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sntp_cfg.h2 = h2;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sntp_cfg.h3 = h3;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

/**
 * \brief           Get time from SNTP servers
 * \param[out]      dt: Pointer to \ref esp_datetime_t structure to fill with date and time values
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_sntp_gettime(esp_datetime_t* dt, uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);                    /* Define variable for message */
    
    ESP_MSG_VAR_ALLOC(msg);                     /* Allocate memory for variable */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSNTPTIME;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sntp_time.dt = dt;
    
    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, blocking);  /* Send message to producer queue */
}

#endif /* ESP_SNTP || __DOXYGEN__ */
