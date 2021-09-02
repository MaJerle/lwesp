/**
 * \file            lwesp_sys_threadx.c
 * \brief           System dependant functions
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
 * subject to the follwing conditions:
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Author:          Neo Xiong <xiongyu0523@gmail.com>
 * Version:         v1.1.0-dev
 */
#include "system/lwesp_sys.h"
#include "lwesp/lwesp_mem.h"
#include "tx_api.h"

#if !__DOXYGEN__

#if !defined(LWESP_MEM_SIZE)
#define LWESP_MEM_SIZE                    0x1000
#endif

#define LWESP_TICK_PERIOD                 (1000 / TX_TIMER_TICKS_PER_SECOND)
#define LWESP_MS_TO_TICKS(ms)             ((ms) * TX_TIMER_TICKS_PER_SECOND / 1000 )

static UCHAR        byte_pool_mem[LWESP_MEM_SIZE];
TX_BYTE_POOL        lwesp_byte_tool;
static TX_MUTEX     _sys_mutex;

uint8_t
lwesp_sys_init(void) {

    UINT status;
    
    status = tx_byte_pool_create(&lwesp_byte_tool, "byte pool", byte_pool_mem, LWESP_MEM_SIZE);
    if (status == TX_SUCCESS) {
        status = tx_mutex_create(&_sys_mutex, "sys mutex", TX_INHERIT);
    }

    return status == TX_SUCCESS ? 1 : 0;
}

uint32_t
lwesp_sys_now(void) {
    return tx_time_get() * LWESP_TICK_PERIOD;
}

uint8_t
lwesp_sys_protect(void) {
    return tx_mutex_get(&_sys_mutex, TX_WAIT_FOREVER) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_unprotect(void) {
    return tx_mutex_put(&_sys_mutex) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_mutex_create(lwesp_sys_mutex_t* p) {
    return tx_mutex_create(p, TX_NULL, TX_INHERIT) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_mutex_delete(lwesp_sys_mutex_t* p) {
    return tx_mutex_delete(p) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_mutex_lock(lwesp_sys_mutex_t* p) {
    return tx_mutex_get(p, TX_WAIT_FOREVER) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_mutex_unlock(lwesp_sys_mutex_t* p) {
    return tx_mutex_put(p) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_mutex_isvalid(lwesp_sys_mutex_t* p) {
    return p->tx_mutex_id != TX_CLEAR_ID ? 1 : 0;
}

uint8_t
lwesp_sys_mutex_invalid(lwesp_sys_mutex_t* p) {
    /* No need actions since all invalid are following delete, and delete make sure it is invalid */
    return 1;
}

uint8_t
lwesp_sys_sem_create(lwesp_sys_sem_t* p, uint8_t cnt) {
    return tx_semaphore_create(p, TX_NULL, cnt) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_sem_delete(lwesp_sys_sem_t* p) {
    return tx_semaphore_delete(p) == TX_SUCCESS ? 1 : 0;
}

uint32_t
lwesp_sys_sem_wait(lwesp_sys_sem_t* p, uint32_t timeout) {
    ULONG start = tx_time_get();
    return tx_semaphore_get(p, !timeout ? TX_WAIT_FOREVER : LWESP_MS_TO_TICKS(timeout)) == TX_SUCCESS ? (tx_time_get() - start) * LWESP_TICK_PERIOD : LWESP_SYS_TIMEOUT;
}

uint8_t
lwesp_sys_sem_release(lwesp_sys_sem_t* p) {
    return tx_semaphore_put(p) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_sem_isvalid(lwesp_sys_sem_t* p) {
    return p->tx_semaphore_id != TX_CLEAR_ID ? 1 : 0;
}

uint8_t
lwesp_sys_sem_invalid(lwesp_sys_sem_t* p) {
    /* No need actions since all invalid are following delete, and delete make sure it is invalid */
    return 1;
}

uint8_t
lwesp_sys_mbox_create(lwesp_sys_mbox_t* b, size_t size) {

    uint8_t rt = 0;
    ULONG queue_total_size = size * sizeof(void *);

    void *queue_mem = lwesp_mem_malloc(queue_total_size);
    if (queue_mem != NULL) {
        if (tx_queue_create(b, TX_NULL, sizeof(void *) / sizeof(ULONG), queue_mem, queue_total_size) == TX_SUCCESS) {
            rt = 1;
        } else {
            lwesp_mem_free(queue_mem);
        }
    }

    return rt;
}

uint8_t
lwesp_sys_mbox_delete(lwesp_sys_mbox_t* b) {

    (VOID)tx_queue_delete(b);
    lwesp_mem_free(b->tx_queue_start);
    return 1;
}

uint32_t
lwesp_sys_mbox_put(lwesp_sys_mbox_t* b, void* m) {

    ULONG start = tx_time_get();
    (VOID)tx_queue_send(b, &m, TX_WAIT_FOREVER);
    return tx_time_get() - start;
}

uint32_t
lwesp_sys_mbox_get(lwesp_sys_mbox_t* b, void** m, uint32_t timeout) {

    ULONG start = tx_time_get();
    return tx_queue_receive(b, m, !timeout ? TX_WAIT_FOREVER : LWESP_MS_TO_TICKS(timeout)) == TX_SUCCESS ? (tx_time_get() - start) * LWESP_TICK_PERIOD : LWESP_SYS_TIMEOUT;
}

uint8_t
lwesp_sys_mbox_putnow(lwesp_sys_mbox_t* b, void* m) {
    return tx_queue_send(b, &m, TX_NO_WAIT) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_mbox_getnow(lwesp_sys_mbox_t* b, void** m) {
    return tx_queue_receive(b, m, TX_NO_WAIT) == TX_SUCCESS ? 1 : 0;
}

uint8_t
lwesp_sys_mbox_isvalid(lwesp_sys_mbox_t* b) {
    return b->tx_queue_id != TX_CLEAR_ID ? 1 : 0;
}

uint8_t
lwesp_sys_mbox_invalid(lwesp_sys_mbox_t* b) {
    /* No need actions since all invalid are following delete, and delete make sure it is invalid */
    return 1;
}

uint8_t
lwesp_sys_thread_create(lwesp_sys_thread_t* t, const char* name, lwesp_sys_thread_fn thread_func, void* const arg, size_t stack_size, lwesp_sys_thread_prio_t prio) {

typedef VOID (*threadx_entry_t)(ULONG);
    uint8_t rt = 0;

    void *stack_mem = lwesp_mem_malloc(stack_size);
    if (stack_mem != NULL) {
        if (tx_thread_create(t, (CHAR *)name, (VOID (*)(ULONG))(thread_func), (ULONG)arg, stack_mem, stack_size, prio, prio, TX_NO_TIME_SLICE, TX_AUTO_START) == TX_SUCCESS) {
            rt = 1;
        } else {
            lwesp_mem_free(stack_mem);
        }
    }

    return rt;
}

uint8_t
lwesp_sys_thread_terminate(lwesp_sys_thread_t* t) {
    uint8_t rt = 0;
    
    /*  t == NULL means temrinate itself. 
        Here termination of a thread requires deleting thread (free RCB) and releasing stack memory
        ThreadX does not support deleting itself, so I left this feature not supported (when t == NULL) */

    if ((t != NULL) && (t != tx_thread_identify())) {
        if (tx_thread_terminate(t) == TX_SUCCESS) {
            if (tx_thread_delete(t) == TX_SUCCESS) {
                lwesp_mem_free(t->tx_thread_stack_start);
                rt = 1;
            }
        }
    }

    return rt;
}

uint8_t
lwesp_sys_thread_yield(void) {
    /* Not supported by ThreadX, also this is not used by LWESP */
    return 0;
}

#endif /* !__DOXYGEN__ */
