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
#include "esp/esp.h"
#include "esp/esp_sntp.h"
#include "cmsis_os.h"
#include "cpu_utils.h"
#include "ff.h"

#include "apps/esp_http_server.h"
#include "esp/esp_hostname.h"

#include "mqtt.h"
#include "station_manager.h"
#include "netconn_client.h"
#include "netconn_server.h"
#include "http_server.h"

static void init_thread(void const* arg);

osThreadId init_thread_id, server_thread_id, client_thread_id;

esp_datetime_t dt;

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

static espr_t esp_cb(esp_cb_t* cb);

/**
 * \brief           Initialization thread for entire process
 */
static void
init_thread(void const* arg) {
    printf("Initialization thread started!\r\n");
    esp_init(esp_cb);                           /* Init ESP stack */
    
    /*
     * Start MQTT thread
     */
    esp_sys_thread_create(NULL, "MQTT", (esp_sys_thread_fn)mqtt_thread, NULL, 512, ESP_SYS_THREAD_PRIO);
    
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
    http_server_start();
    printf("Server mode!\r\n");
    
    /*
     * Check if device has set IP address
     *
     * This should always pass
     */
    if (esp_sta_has_ip() == espOK) {
        uint8_t ip[4];
        esp_sta_copy_ip(ip, NULL, NULL);
        printf("Connected to WIFI!\r\n");
        printf("Device IP: %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);
    }
    
    /*
     * Terminate thread
     */
    esp_sys_thread_terminate(NULL);
}

/**
 * \brief           Global ESP callback for general info
 * \param[in]       cb: Pointer to callback data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_cb(esp_cb_t* cb) {
    switch (cb->type) {
        case ESP_CB_RESET: {
            printf("Device reset!\r\n");
            break;
        }
        case ESP_CB_INIT_FINISH:
            esp_set_at_baudrate(115200, 0);     /* Init ESP stack */
                                                
            break;
        case ESP_CB_STA_LIST_AP:
            printf("List AP finished!\r\n");                        
            break;
        case ESP_CB_WIFI_GOT_IP: {
            printf("WIFI GOT IP FROM MAIN!\r\n");
            break;
        }
        default:
            break;
    }
    return espOK;
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
