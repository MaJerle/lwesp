/**
 * \file            lwesp_sys_port.h
 * \brief           FreeRTOS native port
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Author:          Adrian Carpenter (FreeRTOS port)
 * Version:         v1.1.2-dev
 */
#ifndef LWESP_SYSTEM_PORT_HDR_H
#define LWESP_SYSTEM_PORT_HDR_H

#include <stdint.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "lwesp/lwesp_opt.h"
#include "semphr.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if LWESP_CFG_OS && !__DOXYGEN__

typedef SemaphoreHandle_t lwesp_sys_mutex_t;
typedef SemaphoreHandle_t lwesp_sys_sem_t;
typedef QueueHandle_t lwesp_sys_mbox_t;
typedef TaskHandle_t lwesp_sys_thread_t;
typedef UBaseType_t lwesp_sys_thread_prio_t;

#define LWESP_SYS_MUTEX_NULL  ((SemaphoreHandle_t)0)
#define LWESP_SYS_SEM_NULL    ((SemaphoreHandle_t)0)
#define LWESP_SYS_MBOX_NULL   ((QueueHandle_t)0)
#define LWESP_SYS_TIMEOUT     ((TickType_t)portMAX_DELAY)
#define LWESP_SYS_THREAD_PRIO (configMAX_PRIORITIES - 1)
#define LWESP_SYS_THREAD_SS   (1024)

#endif /* LWESP_CFG_OS && !__DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_SYSTEM_PORT_HDR_H */
