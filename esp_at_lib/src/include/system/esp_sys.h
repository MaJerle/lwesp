/**
 * \file            esp_sys.h
 * \brief           Main system include file which decides later include file
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
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
#ifndef ESP_HDR_MAIN_SYS_H
#define ESP_HDR_MAIN_SYS_H

#include "esp_config.h"
#include <stdint.h>

/* Include system port file from portable folder */
#include "esp_sys_port.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        ESP_SYS System functions
 * \brief           System based function for OS management, timings, etc
 * \{
 */

/**
 * \brief           Thread function prototype
 */
typedef void (*esp_sys_thread_fn)(void *);

/**
 * \anchor          ESP_SYS_CORE
 * \name            Main
 */

uint8_t     esp_sys_init(void);
uint32_t    esp_sys_now(void);

uint8_t     esp_sys_protect(void);
uint8_t     esp_sys_unprotect(void);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_MUTEX
 * \name            Mutex
 */

uint8_t     esp_sys_mutex_create(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_delete(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_lock(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_unlock(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_isvalid(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_invalid(esp_sys_mutex_t* p);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_SEM
 * \name            Semaphores
 */

uint8_t     esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt);
uint8_t     esp_sys_sem_delete(esp_sys_sem_t* p);
uint32_t    esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout);
uint8_t     esp_sys_sem_release(esp_sys_sem_t* p);
uint8_t     esp_sys_sem_isvalid(esp_sys_sem_t* p);
uint8_t     esp_sys_sem_invalid(esp_sys_sem_t* p);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_MBOX
 * \name            Message queues
 */

uint8_t     esp_sys_mbox_create(esp_sys_mbox_t* b, size_t size);
uint8_t     esp_sys_mbox_delete(esp_sys_mbox_t* b);
uint32_t    esp_sys_mbox_put(esp_sys_mbox_t* b, void* m);
uint32_t    esp_sys_mbox_get(esp_sys_mbox_t* b, void** m, uint32_t timeout);
uint8_t     esp_sys_mbox_putnow(esp_sys_mbox_t* b, void* m);
uint8_t     esp_sys_mbox_getnow(esp_sys_mbox_t* b, void** m);
uint8_t     esp_sys_mbox_isvalid(esp_sys_mbox_t* b);
uint8_t     esp_sys_mbox_invalid(esp_sys_mbox_t* b);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_THREAD
 * \name            Threads
 */

uint8_t     esp_sys_thread_create(esp_sys_thread_t* t, const char* name, esp_sys_thread_fn thread_func, void* const arg, size_t stack_size, esp_sys_thread_prio_t prio);
uint8_t     esp_sys_thread_terminate(esp_sys_thread_t* t);
uint8_t     esp_sys_thread_yield(void);

/**
 * \}
 */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ESP_HDR_MAIN_SYS_H */
