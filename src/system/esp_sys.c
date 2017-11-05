#define esp_INTERNAL
#include "esp_sys.h"
#include "stm32f4xx_hal.h"

#define VAL_MUTEX_INVALID           (osMutexId)0
#define VAL_SEM_INVALID             (osSemaphoreId)0
#define VAL_MBOX_INVALID            (osMessageQId)0

static osMutexId sys_mutex;                     /* Mutex ID for main protection */
 
uint32_t
esp_sys_now(void) {
    return HAL_GetTick();                       /* Get current tick in units of milliseconds */
}

uint8_t
esp_sys_init(void) {
    esp_sys_mutex_create(&sys_mutex);           /* Create system mutex */
    return 1;
}

uint8_t
esp_sys_protect(void) {
    esp_sys_mutex_lock(&sys_mutex);             /* Lock system and protect it */
    return 1;
}

uint8_t
esp_sys_unprotect(void) {
    esp_sys_mutex_unlock(&sys_mutex);           /* Release lock */
    return 1;
}

uint8_t
esp_sys_mutex_create(esp_sys_mutex_t* p) {
    osMutexDef(MUT);                            /* Define a mutex */
    *p = osRecursiveMutexCreate(osMutex(MUT));  /* Create recursive mutex */
    return !!*p;                                /* Create mutex from init */
}

uint8_t
esp_sys_mutex_delete(esp_sys_mutex_t* p) {
    return osMutexDelete(*p) == osOK;           /* Delete mutex */
}

uint8_t
esp_sys_mutex_lock(esp_sys_mutex_t* p) {
    return osRecursiveMutexWait(*p, osWaitForever) == osOK; /* Wait forever for mutex */
}
  
uint8_t
esp_sys_mutex_unlock(esp_sys_mutex_t* p) {
    return osRecursiveMutexRelease(*p) == osOK; /* Release mutex */
}

uint8_t
esp_sys_mutex_isvalid(esp_sys_mutex_t* p) {
    return !!*p;                                /* Check if mutex is valid */
}

uint8_t
esp_sys_mutex_invalid(esp_sys_mutex_t* p) {
    *p = VAL_MUTEX_INVALID;                     /* Set mutex as invalid */
    return 1;
}

uint8_t
esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt) {
    osSemaphoreDef(SEM);                        /* Define semaphore info */
    *p = osSemaphoreCreate(osSemaphore(SEM), 1);    /* Create semaphore with one token */
    
    if (*p && !cnt) {                           /* We have valid entry */
        osSemaphoreWait(*p, 0);                 /* Lock semaphore immediatelly */
    }
    return !!*p;
}

uint8_t
esp_sys_sem_delete(esp_sys_sem_t* p) {
    return osSemaphoreDelete(*p) == osOK;       /* Delete semaphore */
}

uint32_t
esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout) {
    uint32_t tick = osKernelSysTick();          /* Get start tick time */
    return (osSemaphoreWait(*p, !timeout ? osWaitForever : timeout) == osOK) ? (osKernelSysTick() - tick) : ESP_SYS_TIMEOUT;    /* Wait for semaphore with specific time */
}

uint8_t
esp_sys_sem_release(esp_sys_sem_t* p) {
    return osSemaphoreRelease(*p) == osOK;      /* Release semaphore */
}

uint8_t
esp_sys_sem_isvalid(esp_sys_sem_t* p) {
    return !!*p;                                /* Check if valid */
}

uint8_t
esp_sys_sem_invalid(esp_sys_sem_t* p) {
    *p = VAL_SEM_INVALID;                       /* Invaldiate semaphore */
    return 1;
}

uint8_t
esp_sys_mbox_create(esp_sys_mbox_t* b, size_t size) {
    osMessageQDef(MBOX, size, void *);          /* Define message box */
    *b = osMessageCreate(osMessageQ(MBOX), NULL);   /* Create message box */
    return !!*b;
}

uint8_t
esp_sys_mbox_delete(esp_sys_mbox_t* b) {
    if (osMessageWaiting(*b)) {                 /* We still have messages in queue, should not delete queue */
        return 0;                               /* Return error as we still have entries in message queue */
    }
    return osMessageDelete(*b) == osOK;         /* Delete message queue */
}

uint32_t
esp_sys_mbox_put(esp_sys_mbox_t* b, void* m) {
    uint32_t tick = osKernelSysTick();          /* Get start time */
    return osMessagePut(*b, (uint32_t)m, osWaitForever) == osOK ? (osKernelSysTick() - tick) : ESP_SYS_TIMEOUT; /* Put new message with forever timeout */
}

uint32_t
esp_sys_mbox_get(esp_sys_mbox_t* b, void** m, uint32_t timeout) {
    osEvent evt;
    uint32_t time = osKernelSysTick();          /* Get current time */
    
    evt = osMessageGet(*b, !timeout ? osWaitForever : timeout); /* Get message event */
    if (evt.status == osEventMessage) {         /* Did we get a message? */
        *m = evt.value.p;                       /* Set value */
        return osKernelSysTick() - timeout;     /* Return time required for reading message */
    }
    return ESP_SYS_TIMEOUT;
}

uint8_t
esp_sys_mbox_putnow(esp_sys_mbox_t* b, void* m) {
    return osMessagePut(*b, (uint32_t)m, 0) == osOK;   /* Put new message without timeout */
}

uint8_t
esp_sys_mbox_getnow(esp_sys_mbox_t* b, void** m) {
    osEvent evt;
    
    evt = osMessageGet(*b, 0);                  /* Get message event */
    if (evt.status == osEventMessage) {         /* Did we get a message? */
        *m = evt.value.p;                       /* Set value */
        return 1;
    }
    return 0;
}

uint8_t
esp_sys_mbox_isvalid(esp_sys_mbox_t* b) {
    return !!*b;                                /* Return status if message box is valid */
}

uint8_t
esp_sys_mbox_invalid(esp_sys_mbox_t* b) {
    *b = VAL_MBOX_INVALID;                      /* Invalidate message box */
    return 1;
}
    
esp_sys_thread_t
esp_sys_thread_create(const char* name, void (*thread_func)(void *), void* const arg, size_t stack_size, esp_sys_thread_prio_t prio) {
    const osThreadDef_t thread_def = {(char *)name, (os_pthread)thread_func, (osPriority)prio, 0, stack_size};  /* Create thread description */
    return osThreadCreate(&thread_def, arg);    /* Create thread */
}
