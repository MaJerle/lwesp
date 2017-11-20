#ifndef __ESP_SYSTEM_H
#define __ESP_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdlib.h"

#include "esp_config.h"
#include "cmsis_os.h"

typedef osMutexId           esp_sys_mutex_t;
typedef osSemaphoreId       esp_sys_sem_t;
typedef osMessageQId        esp_sys_mbox_t;
typedef osThreadId          esp_sys_thread_t;
typedef osPriority          esp_sys_thread_prio_t;

#define ESP_SYS_MBOX_NULL   (osMessageQId)0
#define ESP_SYS_SEM_NULL    (osSemaphoreId)0
#define ESP_SYS_TIMEOUT     ((uint32_t)osWaitForever)
#define ESP_SYS_THREAD_PRIO (osPriorityNormal)
#define ESP_SYS_THREAD_SS   (512)

uint8_t     esp_sys_init(void);
uint8_t     esp_sys_protect(void);
uint8_t     esp_sys_unprotect(void);

uint8_t     esp_sys_mutex_create(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_delete(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_lock(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_unlock(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_isvalid(esp_sys_mutex_t* p);

uint8_t     esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt);
uint8_t     esp_sys_sem_delete(esp_sys_sem_t* p);
uint32_t    esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout);
uint8_t     esp_sys_sem_release(esp_sys_sem_t* p);
uint8_t     esp_sys_sem_isvalid(esp_sys_sem_t* p);
uint8_t     esp_sys_sem_invalid(esp_sys_sem_t* p);

uint8_t     esp_sys_mbox_create(esp_sys_mbox_t* b, size_t size);
uint8_t     esp_sys_mbox_delete(esp_sys_mbox_t* b);
uint32_t    esp_sys_mbox_put(esp_sys_mbox_t* b, void* m);
uint32_t    esp_sys_mbox_get(esp_sys_mbox_t* b, void** m, uint32_t timeout);
uint8_t     esp_sys_mbox_putnow(esp_sys_mbox_t* b, void* m);
uint8_t     esp_sys_mbox_getnow(esp_sys_mbox_t* b, void** m);
uint8_t     esp_sys_mbox_isvalid(esp_sys_mbox_t* b);
uint8_t     esp_sys_mbox_invalid(esp_sys_mbox_t* b);

uint8_t     esp_sys_thread_create(esp_sys_thread_t* t, const char* name, void(*thread_func)(void *), void* const arg, size_t stack_size, esp_sys_thread_prio_t prio);

uint32_t esp_sys_now(void);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __ESP_SYSTEM_H */
