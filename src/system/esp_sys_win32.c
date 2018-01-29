/**	
 * \file            esp_sys_win32.c
 * \brief           System dependant functions for WIN32
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
 * This file is part of ESP-AT.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#include "system/esp_sys.h"
#include "string.h"
#include "stdlib.h"

/**
 * \brief           Custom message queue implementation for WIN32
 */
typedef struct {
    esp_sys_sem_t sem_not_empty;                /*!< Semaphore indicates not empty */
	esp_sys_sem_t sem_not_full;                 /*!< Semaphore indicates not full */
	esp_sys_sem_t sem;                          /*!< Semaphore to lock access */
    size_t in, out, size;
    void* entries[1];
} win32_mbox_t;

static LARGE_INTEGER freq, sys_start_time;
static esp_sys_mutex_t sys_mutex;				/* Mutex ID for main protection */

/**
 * \brief           Check if message box is full
 * \param[in]       m: Message box handle
 * \return          1 if full, 0 otherwise
 */
static uint8_t
mbox_is_full(win32_mbox_t* m) {
    size_t size = 0;
    if (m->in > m->out) {
        size = (m->in - m->out);
    } else if (m->out > m->in) {
        size = m->size - m->out + m->in;
    }
    return size == m->size - 1;
}

/**
 * \brief           Check if message box is empty
 * \param[in]       m: Message box handle
 * \return          1 if empty, 0 otherwise
 */
static uint8_t
mbox_is_empty(win32_mbox_t* m) {
    return m->in == m->out;
}

/**
 * \brief           Get current kernel time in units of milliseconds
 */
static uint32_t
osKernelSysTick(void) {
	LONGLONG ret;
	LARGE_INTEGER now;

	QueryPerformanceCounter(&now);              /* Get current time */
	ret = now.QuadPart - sys_start_time.QuadPart;
	return (uint32_t)(((ret) * 1000) / freq.QuadPart);
}

/**
 * \brief           Init system dependant parameters
 * \note            Called from high-level application layer when required
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_init(void) {
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&sys_start_time);   /* Get start time */

    esp_sys_mutex_create(&sys_mutex);           /* Create system mutex */
    return 1;
}

/**
 * \brief           Get current time in units of milliseconds
 * \return          Current time in units of milliseconds
 */
uint32_t
esp_sys_now(void) {
    return osKernelSysTick();                   /* Get current tick in units of milliseconds */
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
	*p = CreateMutex(NULL, FALSE, NULL);
	return !!*p;
}

/**
 * \brief           Delete mutex from OS
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_delete(esp_sys_mutex_t* p) {
	return CloseHandle(*p);
}

/**
 * \brief           Wait forever to lock the mutex
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_lock(esp_sys_mutex_t* p) {
	DWORD ret;
	ret = WaitForSingleObject(*p, INFINITE);
	if (ret != WAIT_OBJECT_0) {
        return 0;
	}
	return 1;
}

/**
 * \brief           Unlock mutex
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_unlock(esp_sys_mutex_t* p) {
	return !!ReleaseMutex(*p);
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
	HANDLE h;
	h = CreateSemaphore(NULL, !!cnt, 1, NULL);
	*p = h;
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
	return CloseHandle(*p);
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
	DWORD ret;
    uint32_t tick = osKernelSysTick();          /* Get start tick time */
	
	if (!timeout) {
		ret = WaitForSingleObject(*p, INFINITE);
		return 1;
	} else {
		ret = WaitForSingleObject(*p, timeout);
		if (ret == WAIT_OBJECT_0) {
			return 1;
		} else {
			return ESP_SYS_TIMEOUT;
		}
	}
}

/**
 * \brief           Release semaphore
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_release(esp_sys_sem_t* p) {
	return ReleaseSemaphore(*p, 1, NULL);
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
    win32_mbox_t* mbox;
    
    *b = 0;
    
    mbox = malloc(sizeof(*mbox) + size * sizeof(void *));
    if (mbox != NULL) {
        memset(mbox, 0x00, sizeof(*mbox));
        mbox->size = size + 1;                  /* Set it to 1 more as cyclic buffer has only one less than size */
        esp_sys_sem_create(&mbox->sem, 1);
        esp_sys_sem_create(&mbox->sem_not_empty, 0);
        esp_sys_sem_create(&mbox->sem_not_full, 0);
        *b = mbox;
    }
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
    win32_mbox_t* mbox = *b;
    esp_sys_sem_delete(&mbox->sem);
    esp_sys_sem_delete(&mbox->sem_not_full);
    esp_sys_sem_delete(&mbox->sem_not_empty);
    free(mbox);
    return 1;
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
    win32_mbox_t* mbox = *b;
    uint32_t time = osKernelSysTick();          /* Get start time */
  
    esp_sys_sem_wait(&mbox->sem, 0);            /* Wait for access */
    
    /*
     * Since function is blocking until ready to write something to queue,
     * wait and release the semaphores to allow other threads
     * to process the queue before we can write new value.
     */
    while (mbox_is_full(mbox)) {
        esp_sys_sem_release(&mbox->sem);        /* Release semaphore */
        esp_sys_sem_wait(&mbox->sem_not_full, 0);   /* Wait for semaphore indicating not full */
        esp_sys_sem_wait(&mbox->sem, 0);        /* Wait availability again */
    }
    mbox->entries[mbox->in] = m;
    if (mbox->in == mbox->out) {                /* Was the previous state empty? */
        esp_sys_sem_release(&mbox->sem_not_empty);  /* Signal non-empty state */
    }
    if (++mbox->in >= mbox->size) {
        mbox->in = 0;
    }
    esp_sys_sem_release(&mbox->sem);            /* Release access for other threads */
    return osKernelSysTick() - time;
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
    win32_mbox_t* mbox = *b;
    uint32_t time = osKernelSysTick();          /* Get current time */
    uint32_t spent_time;
    
    /*
     * Get exclusive access to message queue
     */
    if ((spent_time = esp_sys_sem_wait(&mbox->sem, timeout)) == ESP_SYS_TIMEOUT) {
        return spent_time;
    }
    
    /*
     * Make sure we have something to read from queue.
     */
    while (mbox_is_empty(mbox)) {
        esp_sys_sem_release(&mbox->sem);        /* Release semaphore and allow other threads to write something */
        /*
         * Timeout = 0 means unlimited time
         * Wait either unlimited time or for specific timeout
         */
        if (!timeout) {
            esp_sys_sem_wait(&mbox->sem_not_empty, 0);
        } else {
            spent_time = esp_sys_sem_wait(&mbox->sem_not_empty, timeout);
            if (spent_time == ESP_SYS_TIMEOUT) {
                return spent_time;
            }
        }
        spent_time = esp_sys_sem_wait(&mbox->sem, timeout); /* Wait again for exclusive access */
    }
    
    /*
     * At this point, semaphore is not empty and
     * we have exclusive access to content
     */
    *m = mbox->entries[mbox->out];
    if (++mbox->out >= mbox->size) {
        mbox->out = 0;
    }
    
    /* Release it only if waiting for it */
    esp_sys_sem_release(&mbox->sem_not_full);   /* Release semaphore as it is not full */
    esp_sys_sem_release(&mbox->sem);            /* Release exclusive access to mbox */
    
    return osKernelSysTick() - time;
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
    win32_mbox_t* mbox = *b;

    esp_sys_sem_wait(&mbox->sem, 0);
    if (mbox_is_full(mbox)) {
        esp_sys_sem_release(&mbox->sem);
        return 0;
    }
    mbox->entries[mbox->in] = m;
    if (mbox->in == mbox->out) {
        esp_sys_sem_release(&mbox->sem_not_empty);
    }
    mbox->in++;
    if (mbox->in >= mbox->size) {
        mbox->in = 0;
    }
    esp_sys_sem_release(&mbox->sem);
    return 1;
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
    win32_mbox_t* mbox = *b;
    
    esp_sys_sem_wait(&mbox->sem, 0);            /* Wait exclusive access */
    if (mbox->in == mbox->out) {
        esp_sys_sem_release(&mbox->sem);        /* Release access */
        return 0;
    }
    
    *m = mbox->entries[mbox->out];
    mbox->out++;
    if (mbox->out >= mbox->size) {
        mbox->out = 0;
    }
    esp_sys_sem_release(&mbox->sem_not_full);   /* Queue not full anymore */
    esp_sys_sem_release(&mbox->sem);            /* Release semaphore */
    return 1;
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
    HANDLE h;
    DWORD id;
    h = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_func, arg, 0, &id);
    if (t != NULL) {
        *t = h;
    }
    return t != NULL;
}

/**
 * \brief           Terminate thread (shut it down and remove)
 *
 *                  Implementation will not shutdown any thread, it will just make huge delay to leave resources
 *
 * \note            This function is required with OS
 * \param[in]       t: Thread handle to terminate. If set to NULL, terminate current thread (thread from where function is called)
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_terminate(esp_sys_thread_t* t) {
    HANDLE h = NULL;

    if (t == NULL) {                            /* Shall we terminate ourself? */
        h = GetCurrentThread();                 /* Get current thread handle */
    } else {                                    /* We have known thread, find handle by looking at ID */
        h = *t;
    }
    TerminateThread(h, 0);
	return 1;
}

/**
 * \brief           Yield current thread
 * \note            This function is required with OS
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_yield(void) {
    /* Not implemented */
	return 1;
}
