/**
 * \file            main.c
 * \brief           
 *
 */
/* Include core modules */
#include "stm32fxxx_hal.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32_disco.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_usart.h"
#include "stm32f7xx_ll_gpio.h"
#include "esp/esp.h"
#include "esp/esp_sntp.h"
#include "cmsis_os.h"
#include "cpu_utils.h"
#include "ff.h"

#include "esp/apps/esp_http_server.h"
#include "esp/esp_hostname.h"

#include "mqtt_client.h"
#include "station_manager.h"
#include "netconn_client.h"
#include "netconn_server.h"
#include "http_server.h"
#include "netconn_server_1thread.h"

static void init_thread(void const* arg);

osThreadId init_thread_id, server_thread_id, client_thread_id;

esp_datetime_t dt;

uint8_t
is_device_present(void) {
    uint8_t pin_state = !!(GPIOJ->IDR & GPIO_PIN_3);
    return pin_state;
}

/**
 * \brief           Application entry point
 */
int
main(void) {
    TM_RCC_InitSystem();                        /* Init system */
    HAL_Init();                                 /* Init HAL layer */
    TM_DISCO_LedInit();                         /* Init leds */
    TM_DISCO_ButtonInit();                      /* Init button */
    TM_DELAY_Init();                            /* Init delay */
    TM_USART_Init(DISCO_USART, DISCO_USART_PP, 921600); /* Init USART for debug purpose */
    
    esp_sys_thread_create(NULL, "init", (esp_sys_thread_fn)init_thread, NULL, 512, (esp_sys_thread_prio_t)0);
    osKernelStart();                            /* Start OS kernel */
    
    while (1) {

    }
}

void
TM_DELAY_1msHandler(void) {
    osSystickHandler();                         /* Kernel systick handler processing */
}

void
vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ) {
    printf("TASK OVERFLOW! %s\r\n", pcTaskName);
    while (1);
}

static espr_t esp_evt(esp_evt_t* evt);


/**
 * \brief           Initialization thread for entire process
 */
static void
init_thread(void const* arg) {
    GPIO_InitTypeDef init;
    printf("Initialization thread started!\r\n");
    
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    init.Pin = GPIO_PIN_3;
    init.Mode = GPIO_MODE_IT_RISING_FALLING;
    init.Pull = GPIO_PULLDOWN;
    init.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOJ, &init);
    
    HAL_NVIC_SetPriority(EXTI3_IRQn, 2, 4);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    
    esp_init(esp_evt, 1);                       /* Init ESP stack */
    
//    if (is_device_present()) {
//        printf("Device connected...starting with reset!\r\n");
//        esp_delay(2000);
//        esp_reset(1);
//    } else {
//        printf("Device not connected!\r\n");
//        esp_device_set_present(0, 1);
//    }
    
    /*
     * Start MQTT thread
     */
    //esp_sys_thread_create(NULL, "MQTT", (esp_sys_thread_fn)mqtt_thread, NULL, 512, ESP_SYS_THREAD_PRIO);
    
    /*
     * Try to connect to preferred access point
     *
     * Follow function implementation for more info
     * on how to setup preferred access points for fast connection
     */
    connect_to_preferred_access_point(1);

    /*
     * Start server on port 80
     */
    //http_server_start();
    //printf("Server mode!\r\n");
    
    /*
     * Check if device has set IP address
     *
     * This should always pass
     */
    if (esp_sta_has_ip() == espOK) {
        esp_ip_t ip;
        esp_sta_copy_ip(&ip, NULL, NULL);
        printf("Connected to WIFI!\r\n");
        printf("Device IP: %d.%d.%d.%d\r\n", ip.ip[0], ip.ip[0], ip.ip[0], ip.ip[0]);
    }
    
    esp_sys_thread_create(NULL, "netconn_server_single", (esp_sys_thread_fn)netconn_server_1thread_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    esp_sys_thread_create(NULL, "mqtt_client", (esp_sys_thread_fn)mqtt_client_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    
    /*
     * Terminate thread
     */
    esp_sys_thread_terminate(NULL);
}

/**
 * \brief           Global ESP callback for general info
 * \param[in]       evt: Pointer to callback data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_evt(esp_evt_t* evt) {
    switch (esp_evt_get_type(evt)) {
        case ESP_EVT_RESET: {
            printf("Device reset!\r\n");
            break;
        }
        case ESP_EVT_INIT_FINISH: {                                                
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_EVT_STA_LIST_AP: {
            printf("List AP finished!\r\n");                        
            break;
        }
        case ESP_EVT_WIFI_GOT_IP: {
            printf("WIFI got IP!\r\n");
            break;
        }
        case ESP_EVT_WIFI_CONNECTED: {
            printf("WIFI connected!\r\n");
            break;
        }
        case ESP_EVT_WIFI_DISCONNECTED: {
            printf("WIFI disconnected!\r\n");
            break;
        }
#endif /* ESP_CFG_MODE_STATION */
        case ESP_EVT_CONN_ACTIVE: {
            printf("Connection active, time: %d, conn: %p\r\n", (int)esp_sys_now(), evt->evt.conn_active_closed.conn);
            break;
        }
        case ESP_EVT_CONN_POLL: {
            printf("Connection poll, time: %d, conn: %p\r\n", (int)esp_sys_now(), evt->evt.conn_poll.conn);
            break;
        }
        case ESP_EVT_CONN_CLOSED: {
            printf("Connection closed, time: %d, conn: %p\r\n", (int)esp_sys_now(), evt->evt.conn_poll.conn);
            break;
        }
        default:
            break;
    }
    return espOK;
}

/**
 * \brief           Interrupt handler for EXTI 3 line
 */
void
EXTI3_IRQHandler(void) {
    if (is_device_present()) {
        printf("Device present!\r\n");
        esp_device_set_present(1, 0);           /* Notify stack about device present */
        esp_reset_with_delay(2000, 0);
    } else {
        printf("Device disconnected!\r\n");
        esp_device_set_present(0, 0);           /* Notify stack about device not connected anymore */
    }
    EXTI->PR = GPIO_PIN_3;
}

/* printf handler */
#ifdef __GNUC__
int __io_putchar(int ch) {
#else
int fputc(int ch, FILE* fil) {
#endif
    TM_USART_Putc(DISCO_USART, ch);             /* Send over debug USART */
    return ch;                                  /* Return OK */
}
