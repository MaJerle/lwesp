/**
 * \file            lwesp_flash_partitions.h
 * \brief           List of flash partitions
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

/*
 * Define LWESP_FLASH_PARTITION and LWESP_MFG_NAMESPACE before you include the file
 *
 * #define LWESP_FLASH_PARTITION(enum_type, at_string)
 * #define LWESP_MFG_NAMESPACE(enum_type, at_string)
 */

/* Factory related device partitions */
LWESP_FLASH_PARTITION(MFG_NVS, "mfg_nvs")
LWESP_FLASH_PARTITION(FATFS, "fatfs")

/* Default user non-volatile storage sections, part of mfg_nvs */
LWESP_MFG_NAMESPACE(SERVER_CERT, "server_cert")
LWESP_MFG_NAMESPACE(SERVER_KEY, "server_key")
LWESP_MFG_NAMESPACE(SERVER_CA, "server_ca")
LWESP_MFG_NAMESPACE(CLIENT_CERT, "client_cert")
LWESP_MFG_NAMESPACE(CLIENT_KEY, "client_key")
LWESP_MFG_NAMESPACE(CLIENT_CA, "client_ca")
LWESP_MFG_NAMESPACE(MQTT_CERT, "mqtt_cert")
LWESP_MFG_NAMESPACE(MQTT_KEY, "mqtt_key")
LWESP_MFG_NAMESPACE(MQTT_CA, "mqtt_ca")
LWESP_MFG_NAMESPACE(BLE_DATA, "ble_data")
LWESP_MFG_NAMESPACE(FACTORY_PARAM, "factory_param")

#undef LWESP_FLASH_PARTITION
#undef LWESP_MFG_NAMESPACE