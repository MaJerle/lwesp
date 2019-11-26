/**
 * \file            esp_ssl.c
 * \brief           Connection API
 */

/*
 * Copyright (c) 2019 Tilen MAJERLE
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
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         $_version_$
 */
#include "esp/esp_private.h"
#include "esp/esp_ssl.h"
#include "esp/esp_mem.h"

/**
 * \brief           Configure SSL parameters
 * \param[in]       link_id: ID of the connection (0~max), for multiple connections, if the value is max, it means all connections. By default, max is 5.
 * \param[in]       auth_mode: 0: no authorization. 1: load cert and private key for server authorization. 2: load CA for client authorize server cert and private key. 3: both authorization.
 * \param[in]       pki_number:  The index of cert and private key, if only one cert and private key, the value should be 0.
 * \param[in]       ca_number: The index of CA, if only one CA, the value should be 0.
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ssl_configure(uint8_t link_id, uint8_t auth_mode, uint8_t pki_number, uint8_t ca_number,
                    const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_SSLCCONF;
    ESP_MSG_VAR_REF(msg).msg.tcpip_ssl_cfg.link_id = link_id;
    ESP_MSG_VAR_REF(msg).msg.tcpip_ssl_cfg.auth_mode = auth_mode;
    ESP_MSG_VAR_REF(msg).msg.tcpip_ssl_cfg.pki_number = pki_number;
    ESP_MSG_VAR_REF(msg).msg.tcpip_ssl_cfg.ca_number = ca_number;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}
