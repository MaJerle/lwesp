/**	
 * \file            esp_sys_freertos_os.c
 * \brief           System dependant functions
 */
 
/*
 * Copyright (c) 2018 Tilen Majerle
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
 * Author:          Adrian Carpenter (FreeRTOS port)
 */

#include "system/esp_sys.h"
#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*******************************************/
/*******************************************/
/**   Modify this file for your system    **/
/*******************************************/
/*******************************************/

static SemaphoreHandle_t sys_mutex;                     /* Mutex ID for main protection */

typedef struct freertos_mbox
{
    void *d;
} freertos_mbox;

/**
 * \brief           Init system dependant parameters
 * \note            Called from high-level application layer when required
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_init(void) {
    sys_mutex = xSemaphoreCreateMutex();
    return sys_mutex == NULL ? 0 : 1;
}

/**
 * \brief           Get current time in units of milliseconds
 * \return          Current time in units of milliseconds
 */
uint32_t
esp_sys_now(void) {
    return xTaskGetTickCount();                   /* Get current tick in units of milliseconds */
}

/**
 * \brief           Protect stack core
 * \note            This function is required with OS
 *
 * \note            This function may be called multiple times, recursive protection is required
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_protect(void) {
    esp_sys_mutex_lock(&sys_mutex);             /* Lock system and protect it */
    return 1;
}

/**
 * \brief           Protect stack core
 * \note            This function is required with OS
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_unprotect(void) {
    esp_sys_mutex_unlock(&sys_mutex);           /* Release lock */
    return 1;
}

/**
 * \brief           Create a new mutex and pass it to input pointer
 * \note            This function is required with OS
 * \note            Recursive mutex must be created as it may be locked multiple times before unlocked
 * \param[out]      p: Pointer to mutex structure to save result to
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_create(esp_sys_mutex_t* p) {
    *p = xSemaphoreCreateRecursiveMutex();
    return !!*p;                                /* Return status */
}

/**
 * \brief           Delete mutex from OS
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_delete(esp_sys_mutex_t* p) {
    vSemaphoreDelete(*p);           /* Delete mutex */
    return(1);
}

/**
 * \brief           Wait forever to lock the mutex
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_lock(esp_sys_mutex_t* p) {
    return xSemaphoreTakeRecursive(*p, portMAX_DELAY) == pdPASS;
}

/**
 * \brief           Unlock mutex
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_unlock(esp_sys_mutex_t* p) {
    return xSemaphoreGiveRecursive(*p) == pdPASS;
}

/**
 * \brief           Check if mutex structure is valid OS entry
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_isvalid(esp_sys_mutex_t* p) {
    return !!*p;                                /* Check if mutex is valid */
}

/**
 * \brief           Set mutex structure as invalid
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_invalid(esp_sys_mutex_t* p) {
    *p = ESP_SYS_MUTEX_NULL;                    /* Set mutex as invalid */
    return 1;
}

/**
 * \brief           Create a new binary semaphore and set initial state
 * \note            Semaphore may only have 1 token available
 * \note            This function is required with OS
 * \param[out]      p: Pointer to semaphore structure to fill with result
 * \param[in]       cnt: Count indicating default semaphore state:
 *                     0: Lock it immediteally
 *                     1: Leave it unlocked
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt) {
    *p = xSemaphoreCreateBinary();
    
    if (*p && !cnt) {                           /* We have valid entry */
        xSemaphoreTake(*p, 0);
    }
    return !!*p;
}

/**
 * \brief           Delete binary semaphore
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_delete(esp_sys_sem_t* p) {
    vSemaphoreDelete(*p);
    return 1;
}

/**
 * \brief           Wait for semaphore to be available
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \param[in]       timeout: Timeout to wait in milliseconds. When 0 is applied, wait forever
 * \return          Number of milliseconds waited for semaphore to become available
 */
uint32_t
esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout) {
    return xSemaphoreTake(*p, timeout == 0 ? portMAX_DELAY : timeout);
}

/**
 * \brief           Release semaphore
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_release(esp_sys_sem_t* p) {
    return xSemaphoreGive(*p) == pdPASS;
}

/**
 * \brief           Check if semaphore is valid
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_isvalid(esp_sys_sem_t* p) {
    return !!*p;                                /* Check if valid */
}

/**
 * \brief           Invalid semaphore
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_invalid(esp_sys_sem_t* p) {
    *p = ESP_SYS_SEM_NULL;                      /* Invaldiate semaphore */
    return 1;
}

/**
 * \brief           Create a new message queue with entry type of "void *"
 * \note            This function is required with OS
 * \param[out]      b: Pointer to message queue structure
 * \param[in]       size: Number of entries for message queue to hold
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mbox_create(esp_sys_mbox_t* b, size_t size) {
    *b = xQueueCreate(size, sizeof(freertos_mbox));
    return !!*b;
}

/**
 * \brief           Delete message queue
 * \note            This function is required with OS
 * \param[in]       b: Pointer to message queue structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mbox_delete(esp_sys_mbox_t* b) {
    if (uxQueueMessagesWaiting(*b)) {                 /* We still have messages in queue, should not delete queue */
        return 0;                               /* Return error as we still have entries in message queue */
    }
    vQueueDelete(*b);
    return 1;         /* Delete message queue */
}

/**
 * \brief           Put a new entry to message queue and wait until memory available
 * \note            This function is required with OS
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to entry to insert to message queue
 * \return          Time in units of milliseconds needed to put a message to queue
 */
uint32_t
esp_sys_mbox_put(esp_sys_mbox_t* b, void* m) {
    uint32_t t = xTaskGetTickCount();
    freertos_mbox mb;

    mb.d = m;
    xQueueSend(*b, &mb, portMAX_DELAY);
    return xTaskGetTickCount()-t;
}

/**
 * \brief           Get a new entry from message queue with timeout
 * \note            This function is required with OS
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \param[in]       timeout: Maximal timeout to wait for new message. When 0 is applied, wait for unlimited time
 * \return          Time in units of milliseconds needed to put a message to queue
 */
uint32_t
esp_sys_mbox_get(esp_sys_mbox_t* b, void** m, uint32_t timeout) {
    uint32_t t = xTaskGetTickCount();
    freertos_mbox mb;

    if (xQueueReceive(*b, &mb, timeout == 0 ? portMAX_DELAY : timeout)) {
	   *m = mb.d;
	   return xTaskGetTickCount()-t;	
    }
    return ESP_SYS_TIMEOUT;
}

/**
 * \brief           Put a new entry to message queue without timeout (now or fail)
 * \note            This function is required with OS
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to message to save to queue
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mbox_putnow(esp_sys_mbox_t* b, void* m) {
    freertos_mbox mb;

    mb.d = m;
    return xQueueSendFromISR(*b, &mb, 0) == pdPASS;
}

/**
 * \brief           Get an entry from message queue immediatelly
 * \note            This function is required with OS
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mbox_getnow(esp_sys_mbox_t* b, void** m) {
    freertos_mbox mb;

    if (xQueueReceive(*b, &mb,0)) {
	   *m = mb.d;
	   return 1;
    }
    return 0;
}

/**
 * \brief           Check if message queue is valid
 * \note            This function is required with OS
 * \param[in]       b: Pointer to message queue structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mbox_isvalid(esp_sys_mbox_t* b) {
    return !!*b;                                /* Return status if message box is valid */
}

/**
 * \brief           Invalid message queue
 * \note            This function is required with OS
 * \param[in]       b: Pointer to message queue structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mbox_invalid(esp_sys_mbox_t* b) {
    *b = ESP_SYS_MBOX_NULL;                     /* Invalidate message box */
    return 1;
}

/**
 * \brief           Create a new thread
 * \note            This function is required with OS
 * \param[out]      t: Pointer to thread identifier if create was successful
 * \param[in]       name: Name of a new thread
 * \param[in]       thread_func: Thread function to use as thread body
 * \param[in]       arg: Thread function argument
 * \param[in]       stack_size: Size of thread stack in uints of bytes. If set to 0, reserve default stack size
 * \param[in]       prio: Thread priority 
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_create(esp_sys_thread_t* t, const char* name, esp_sys_thread_fn thread_func, void* const arg, size_t stack_size, esp_sys_thread_prio_t prio) {
    return xTaskCreate(thread_func, name, stack_size, arg, prio, t) == pdPASS ? 1 : 0;
}

/**
 * \brief           Terminate thread (shut it down and remove)
 * \note            This function is required with OS
 * \param[in]       t: Pointer to thread handle to terminate. If set to NULL, terminate current thread (thread from where function is called)
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_terminate(esp_sys_thread_t* t) {
    vTaskDelete(*t);
    return 1;
}

/**
 * \brief           Yield current thread
 * \note            This function is required with OS
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_yield(void) {
    taskYIELD();
    return 1;
}
