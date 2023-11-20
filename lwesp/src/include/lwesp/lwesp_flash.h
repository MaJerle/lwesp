/**
 * \file            lwesp_flash.h
 * \brief           System flash API
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
#ifndef LWESP_FLASH_HDR_H
#define LWESP_FLASH_HDR_H

#include "lwesp/lwesp_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         LWESP
 * \defgroup        LWESP_FLASH System flash API
 * \brief           System flash API
 * \{
 */

lwespr_t lwesp_flash_erase(lwesp_flash_partition_t partition, uint32_t offset, uint32_t length,
                           const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
lwespr_t lwesp_flash_write(lwesp_flash_partition_t partition, uint32_t offset, const void* data, uint32_t length,
                           const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

lwespr_t lwesp_mfg_erase(lwesp_mfg_namespace_t namespace, const char* key, uint32_t offset, uint32_t length,
                         const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
lwespr_t lwesp_mfg_write(lwesp_mfg_namespace_t namespace, const char* key, lwesp_mfg_valtype_t valtype,
                         const void* data, uint32_t length, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg,
                         const uint32_t blocking);
lwespr_t lwesp_mfg_read(lwesp_mfg_namespace_t namespace, const char* key, const void* data, uint32_t len,
                const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_FLASH_HDR_H */
