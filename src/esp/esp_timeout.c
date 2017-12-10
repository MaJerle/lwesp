/**	
 * \file            esp_timeout.c
 * \brief           Timeout manager
 */
 
/*
 * Copyright (c) 2017 Tilen Majerle
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "include/esp_private.h"
#include "include/esp_timeout.h"
#include "include/esp_mem.h"

static esp_timeout_t* first_timeout;
static uint32_t last_timeout_time;

/**
 * \brief           Get time we have to wait before we can process next timeout
 * \return          Time in units of milliseconds to wait
 */
static uint32_t
get_next_timeout_diff(void) {
    uint32_t diff;
    if (!first_timeout) {
        return 0;
    }
    diff = esp_sys_now() - last_timeout_time;   /* Get difference between current time and last process time */
    if (diff >= first_timeout->time) {          /* Are we over already? */
        return 0;                               /* We have to immediatelly process this timeout */
    }
    return first_timeout->time - diff;          /* Return remaining time for sleep */
}

/**
 * \brief           Process next timeout in a linked list
 */
static void
process_next_timeout(void) {
    uint32_t time;
    time = esp_sys_now();
    
    if (first_timeout) {
        esp_timeout_t* to = first_timeout;
        
        to->cb(to->arg);                        /* Call user callback function */
        
        first_timeout = first_timeout->next;    /* Set next timeout on a list as first timeout */
        esp_mem_free(to);                       /* Free timeout memory */
    }
    
    last_timeout_time = time;                   /* Reset variable when we were last processed */
}

/**
 * \brief           Get next entry from message queue
 * \param[in]       b: Pointer to message queue to get element
 * \param[out]      m: Pointer to pointer to output variable
 * \return          Time in milliseconds required for next message
 */
uint32_t
espi_get_from_mbox_with_timeout_checks(esp_sys_mbox_t* b, void** m) {
    uint32_t time, wait_time;
    do {
        if (!first_timeout) {                   /* We have no timeouts ready? */
            return esp_sys_mbox_get(b, m, 0);   /* Get entry from message queue */
        }
        wait_time = get_next_timeout_diff();    /* Get time to wait for next timeout execution */
        if (wait_time == 0 || esp_sys_mbox_get(b, m, wait_time) == ESP_SYS_TIMEOUT) {
            process_next_timeout();             /* Process with next timeout */
        }
        break;
    } while (1);
    return time;
}

/**
 * \brief           Add new timeout to processing list
 * \param[in]       time: Time in units of milliseconds for timeout execution
 * \param[in]       cb: Callback function to call when timeout expires
 */
espr_t
esp_timeout_add(uint32_t time, void (*cb)(void *), void* arg) {
    esp_timeout_t* to;
    
    to = esp_mem_calloc(1, sizeof(*to));        /* Allocate memory for timeout structure */
    if (!to) {
        return espERR;
    }
    to->time = time;
    to->arg = arg;
    to->cb = cb;
    
    /**
     * Add new timeout to proper place on linked list
     * and align times to have corrected times between timeouts
     */
    if (!first_timeout) {
        first_timeout = to;                     /* Set as first element */
        last_timeout_time = esp_sys_now();      /* Reset last timeout time to current time */
    } else {                                    /* Find where to place a new timeout */
        /**
         * \todo: Actual implementation
         */
    }
}
