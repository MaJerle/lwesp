/**
 * \file            lwesp_debug_types.h
 * \brief           Debugging types
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
#ifndef LWESP_DEBUG_TYPES_HDR_H
#define LWESP_DEBUG_TYPES_HDR_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         LWESP_DEBUG
 * \{
 */

#define LWESP_DBG_ON          0x80 /*!< Indicates debug is enabled */
#define LWESP_DBG_OFF         0    /*!< Indicates debug is disabled */

/**
 * \anchor          LWESP_DBG_LVL
 * \name            Debug levels
 * \brief           List of debug levels
 * \{
 */

#define LWESP_DBG_LVL_ALL     0x00 /*!< Print all messages of all types */
#define LWESP_DBG_LVL_WARNING 0x01 /*!< Print warning and upper messages */
#define LWESP_DBG_LVL_DANGER  0x02 /*!< Print danger errors */
#define LWESP_DBG_LVL_SEVERE  0x03 /*!< Print severe problems affecting program flow */
#define LWESP_DBG_LVL_MASK    0x03 /*!< Mask for getting debug level */

/**
 * \}
 */

/**
 * \anchor          LWESP_DBG_TYPE
 * \name            Debug types
 * \brief           List of debug types
 * \{
 */

#define LWESP_DBG_TYPE_TRACE  0x40 /*!< Debug trace messages for program flow */
#define LWESP_DBG_TYPE_STATE  0x20 /*!< Debug state messages (such as state machines) */
#define LWESP_DBG_TYPE_ALL    (LWESP_DBG_TYPE_TRACE | LWESP_DBG_TYPE_STATE) /*!< All debug types */

/**
 * \}
 */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_DEBUG_TYPES_HDR_H */
