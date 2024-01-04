/**
 * \file            main.c
 * \brief           Main file
 */

/*
 * Copyright (c) 2024 Tilen MAJERLE
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
#include "netconn_client.h"
#include "examples_common_lwesp_callback_func.h"

/**
 * \brief           Program entry point
 */
int
main(void) {
    printf("Starting ESP application!\r\n");

    /* Initialize ESP with common callback for all examples */
    printf("Initializing LwESP\r\n");
    if (lwesp_init(examples_common_lwesp_callback_func, 1) != lwespOK) {
        printf("Cannot initialize LwESP!\r\n");
    } else {
        printf("LwESP initialized!\r\n");
    }

    /*
     * Connect to access point.
     *
     * Try unlimited time until access point accepts us.
     * Check for station_manager.c to define preferred access points ESP should connect to
     */
    station_manager_connect_to_preferred_access_point(1);

    /*
     * Start netconn client processing in separate thread
     *
     * Use netconn_client_thread function as thread entry point
     */
    lwesp_sys_thread_create(NULL, "netconn_client", (lwesp_sys_thread_fn)netconn_client_thread, NULL, 0, 0);

    /*
     * Do not stop program here.
     * New threads were created for ESP processing
     */
    while (1) {
        lwesp_delay(1000);
    }

    return 0;
}
