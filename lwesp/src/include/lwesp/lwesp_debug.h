/**
 * \file            lwesp_debug.h
 * \brief           Debugging inside ESP stack
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.2-dev
 */
#ifndef LWESP_DEBUG_HDR_H
#define LWESP_DEBUG_HDR_H

#include <stdint.h>
#include <string.h>
#include "lwesp/lwesp_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         LWESP
 * \defgroup        LWESP_DEBUG Debug support
 * \brief           Debug support module to track library flow
 * \{
 */

#if LWESP_CFG_DBG && !defined(LWESP_CFG_DBG_OUT)
#warning "LWESP_CFG_DBG_OUT is not defined but debugging is enabled!"
#endif

#if (LWESP_CFG_DBG && defined(LWESP_CFG_DBG_OUT)) || __DOXYGEN__
/**
 * \brief           Print message to the debug "window" if enabled
 * \param[in]       c: Condition if debug of specific type is enabled
 * \param[in]       fmt: Formatted string for debug
 * \param[in]       ...: Variable parameters for formatted string
 */
#define LWESP_DEBUGF(c, fmt, ...)                                                                                      \
    do {                                                                                                               \
        if (((c) & (LWESP_DBG_ON)) && ((c) & (LWESP_CFG_DBG_TYPES_ON))                                                 \
            && ((c)&LWESP_DBG_LVL_MASK) >= (LWESP_CFG_DBG_LVL_MIN)) {                                                  \
            LWESP_CFG_DBG_OUT(fmt, ##__VA_ARGS__);                                                                     \
        }                                                                                                              \
    } while (0)

/**
 * \brief           Print message to the debug "window" if enabled when specific condition is met
 * \param[in]       c: Condition if debug of specific type is enabled
 * \param[in]       cond: Debug only if this condition is true
 * \param[in]       fmt: Formatted string for debug
 * \param[in]       ...: Variable parameters for formatted string
 */
#define LWESP_DEBUGW(c, cond, fmt, ...)                                                                                \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            LWESP_DEBUGF(c, fmt, ##__VA_ARGS__);                                                                       \
        }                                                                                                              \
    } while (0)
#else
#undef LWESP_CFG_DBG
#define LWESP_CFG_DBG LWESP_DBG_OFF
#define LWESP_DEBUGF(c, fmt, ...)
#define LWESP_DEBUGW(c, cond, fmt, ...)
#endif /* (LWESP_CFG_DBG && defined(LWESP_CFG_DBG_OUT)) || __DOXYGEN__ */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_DEBUG_HDR_H */
