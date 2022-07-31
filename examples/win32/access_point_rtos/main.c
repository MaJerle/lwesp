/**
 * \file            main.c
 * \brief           Main file
 */

/*
 * Copyright (c) 2022 Tilen MAJERLE
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
 * Before you start using WIN32 implementation with USB and VCP,
 * check lwesp_ll_win32.c implementation and choose your COM port!
 */
#include "lwesp/lwesp.h"
#include "station_manager.h"
#include "utils.h"
#include "examples_common_lwesp_callback_func.h"

/* Callback function for example related events */
static lwespr_t lwesp_callback_func(lwesp_evt_t* evt);

/**
 * \brief           Program entry point
 */
int
main(void) {
    lwespr_t res;

    printf("Starting ESP application!\r\n");

    /* Initialize ESP with common callback for all examples */
    printf("Initializing LwESP\r\n");
    if (lwesp_init(examples_common_lwesp_callback_func, 1) != lwespOK) {
        printf("Cannot initialize LwESP!\r\n");
    } else {
        printf("LwESP initialized!\r\n");
    }
    lwesp_evt_register(lwesp_callback_func);

    /* Enable access point only mode */
    if ((res = lwesp_set_wifi_mode(LWESP_MODE_AP, NULL, NULL, 1)) == lwespOK) {
        printf("ESP set to access-point-only mode\r\n");
    } else {
        printf("Problems setting ESP to access-point-only mode: %d\r\n", (int)res);
    }

    /* Configure access point */
    res = lwesp_ap_set_config("LWESP_AccessPoint", "ap_password", 13, LWESP_ECN_WPA2_PSK, 5, 0, NULL, NULL, 1);
    if (res == lwespOK) {
        printf("Access point configured!\r\n");
    } else {
        printf("Cannot configure access point!\r\n");
    }

    /* The rest is handled in event function */

    /*
     * Do not stop program here.
     * New threads were created for ESP processing
     */
    while (1) {
        lwesp_delay(1000);
    }

    return 0;
}

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
lwesp_callback_func(lwesp_evt_t* evt) {
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_AP_CONNECTED_STA: {
            lwesp_mac_t* mac = lwesp_evt_ap_connected_sta_get_mac(evt);
            utils_print_mac("New station connected to access point with MAC address: ", mac, "\r\n");
            break;
        }
        case LWESP_EVT_AP_IP_STA: {
            lwesp_mac_t* mac = lwesp_evt_ap_ip_sta_get_mac(evt);
            lwesp_ip_t* ip = lwesp_evt_ap_ip_sta_get_ip(evt);

            utils_print_ip("IP ", ip, " assigned to station with MAC address: ");
            utils_print_mac(NULL, mac, "\r\n");
            break;
        }
        case LWESP_EVT_AP_DISCONNECTED_STA: {
            lwesp_mac_t* mac = lwesp_evt_ap_disconnected_sta_get_mac(evt);
            utils_print_mac("Station disconnected from access point with MAC address: ", mac, "\r\n");
            break;
        }
        default: break;
    }
    return lwespOK;
}
