/**
 * \file            lwesp_wps.c
 * \brief           WPS API
 */

/*
 * Copyright (c) 2023 Tilen MAJERLE
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
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.2-dev
 */
#include "lwesp/lwesp_wps.h"
#include "lwesp/lwesp_private.h"

#if LWESP_CFG_WPS || __DOXYGEN__

/**
 * \brief           Configure WPS function on ESP device
 * \note            WPS does not support WEP encryption
 * \param[in]       en: Set to `1` to enable WPS or `0` to disable WPS
 * \param[in]       min_ecn: Minimum security level ESP will look for.
 *                      It will not connect to the level below selected parameter
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
lwesp_wps_set_config(uint8_t en, lwesp_ecn_t min_ecn, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg,
                     const uint32_t blocking) {
    LWESP_MSG_VAR_DEFINE(msg);

    /* Check valid entries */
    LWESP_ASSERT(min_ecn <= LWESP_ECN_WPA2_WPA3_PSK);

    LWESP_MSG_VAR_ALLOC(msg, blocking);
    LWESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_WIFI_WPS;
    LWESP_MSG_VAR_REF(msg).msg.wps_cfg.en = en;
    LWESP_MSG_VAR_REF(msg).msg.wps_cfg.min_ecn = min_ecn;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_initiate_cmd, 10000);
}

#endif /* LWESP_CFG_WPS || __DOXYGEN__ */
