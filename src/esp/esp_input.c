/**
 * \file            esp_input.c
 * \brief           Wrapper for passing input data to stack
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
#define ESP_INTERNAL
#include "esp/esp_private.h"
#include "esp/esp.h"
#include "esp/esp_input.h"
#include "esp/esp_buff.h"

static uint32_t esp_recv_total_len;
static uint32_t esp_recv_calls;

#if !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__

/**
 * \brief           Write data to input buffer
 * \note            \ref ESP_CFG_INPUT_USE_PROCESS must be disabled to use this function
 * \param[in]       data: Pointer to data to write
 * \param[in]       len: Number of data elements in units of bytes
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_input(const void* data, size_t len) {
    if (!esp.buff.buff) {
        return espERR;
    }
    esp_buff_write(&esp.buff, data, len);       /* Write data to buffer */
    esp_sys_mbox_putnow(&esp.mbox_process, NULL);   /* Write empty box */
    esp_recv_total_len += len;                  /* Update total number of received bytes */
    esp_recv_calls++;                           /* Update number of calls */
    return espOK;
}

#endif /* !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__ */

#if ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__

/**
 * \brief           Process input data directly without writing it to input buffer
 * \note            This function may only be used when in OS mode,
 *                  where single thread is dedicated for input read of AT receive
 * 
 * \note            \ref ESP_CFG_INPUT_USE_PROCESS must be enabled to use this function
 *
 * \param[in]       data: Pointer to received data to be processed
 * \param[in]       len: Length of data to process in units of bytes
 */
espr_t
esp_input_process(const void* data, size_t len) {
    espr_t res;
    esp_recv_total_len += len;                  /* Update total number of received bytes */
    esp_recv_calls++;                           /* Update number of calls */
    
    if (!esp.status.f.initialized) {
        return espERR;
    }
    ESP_CORE_PROTECT();                         /* Protect core */
    res = espi_process(data, len);              /* Process input data */
    ESP_CORE_UNPROTECT();                       /* Release core */
    return res;
}

#endif /* ESP_CFG_OS || __DOXYGEN__ */
