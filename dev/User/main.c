/**
 * Keil project example for ESP8266 SERVER mode and RTOS support
 *
 * @note      Check defines.h file for configuration settings!
 * @note      When using Nucleo F411 board, example has set 8MHz external HSE clock!
 *
 * Before you start, select your target, on the right of the "Load" button
 *
 * @author    Tilen Majerle
 * @email     tilen@majerle.eu
 * @website   http://stm32f4-discovery.net
 * @ide       Keil uVision 5
 * @conf      PLL parameters are set in "Options for Target" -> "C/C++" -> "Defines"
 * @packs     STM32F4xx/STM32F7xx Keil packs are requred with HAL driver support
 * @stdperiph STM32F4xx/STM32F7xx HAL drivers required
 *
 * \par Description
 *
 * This examples shows how you can use ESP for basic server
 *
 * - Library is initialized using ESP_Init
 * - Device must connect to network. Check WIFINAME and WIFIPASS defines for proper settings for your wifi network
 * - On debug port, IP address will be written to you where you can connect with browser
 * - Magic will begin, you should see something on your screen on PC
 * - On debug output (PA2 pin) is printf targeted via UART at 921600 bauds
 *
 * \note  Example uses separate buffers for each connection, because multiple connections can be active at a time
 *
 * \par Pinout for example (Nucleo STM32F411)
 *
\verbatim
ESP         STM32F4xx           DESCRIPTION
 
RX          PA9                 TX from STM to RX from ESP
TX          PA10                RX from STM to RX from ESP
VCC         3.3V                Use external 3.3V regulator
GND         GND
RST         PA0                 Reset pin for ESP
CTS         PA3                 RTS from ST to CTS from ESP
            BUTTON(PA0, PC13)   Discovery/Nucleo button, depends on configuration
            
            PA2                 TX for debug purpose (connect to PC) with 921600 bauds
\endverbatim
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
//#include "cmsis_os.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX5
#include "cpu_utils.h"
#include "ff.h"

#include "apps/esp_http_server.h"
#include "mqtt.h"

void init_thread(void const* arg);
void client_thread(void const* arg);

osThreadId init_thread_id, server_thread_id, client_thread_id;
osThreadDef(init_thread, init_thread, osPriorityNormal, 0, 512);
osThreadDef(client_thread, client_thread, osPriorityNormal, 0, 512);
osThreadDef(mqtt_thread, mqtt_thread, osPriorityNormal, 0, 512);

char*       led_cgi_handler(http_param_t* params, size_t params_len);
char*       usart_cgi_handler(http_param_t* params, size_t params_len);

esp_ap_t aps[100];
size_t apf;
esp_sta_t stas[20];
size_t staf;
esp_datetime_t dt;

static size_t   http_ssi_cb(http_state_t* hs, const char* tag_name, size_t tag_len);

const http_cgi_t
cgi_handlers[] = {
    { "/led.cgi", led_cgi_handler },
    { "/usart.cgi", usart_cgi_handler },
};

static espr_t
http_post_start(http_state_t* hs, const char* uri, uint32_t content_len) {
    printf("POST started with %d length on URI: %s\r\n", (int)content_len, uri);
    return espOK;
}

static espr_t
http_post_data(http_state_t* hs, esp_pbuf_p pbuf) {
    printf("Data received: %d bytes\r\n", (int)esp_pbuf_length(pbuf, 1));
    return espOK;
}

static espr_t
http_post_end(http_state_t* hs) {
    printf("Post finished!\r\n");
    return espOK;
}

const http_init_t
http_init = {
#if HTTP_SUPPORT_POST
    .post_start_fn = http_post_start,
    .post_data_fn = http_post_data,
    .post_end_fn = http_post_end,
#endif /* HTTP_SUPPORT_POST */
    .cgi = cgi_handlers,
    .cgi_count = ESP_ARRAYSIZE(cgi_handlers),
    .ssi_fn = http_ssi_cb,
    
    .fs_open = http_fs_open,
    .fs_read = http_fs_read,
    .fs_close = http_fs_close,
};

int
main(void) {
    TM_RCC_InitSystem();                        /* Init system */
    HAL_Init();                                 /* Init HAL layer */
    TM_DISCO_LedInit();                         /* Init leds */
    TM_DISCO_ButtonInit();                      /* Init button */
    TM_DELAY_Init();                            /* Init delay */
    TM_USART_Init(DISCO_USART, DISCO_USART_PP, 921600); /* Init USART for debug purpose */
    
    osThreadCreate(osThread(init_thread), NULL);/* Create init thread */
    osKernelStart();                            /* Start OS kernel */
    
	while (1) {

	}
}

void
vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ) {
    printf("TASK OVERFLOW! %s\r\n", pcTaskName);
    while (1);
}

void
TM_DELAY_1msHandler(void) {
    osSystickHandler();                         /* Kernel systick handler processing */
}

static espr_t esp_cb(esp_cb_t* cb);

uint32_t time;

#define CONN_HOST       "example.org"
#define CONN_PORT       80

typedef struct {
    const char* ssid;
    const char* pass;
} ap_entry_t;

/**
 * List of preferred access points for ESP device
 * SSID and password
 * In case ESP finds some of these SSIDs on startup, 
 * it will use the match by match to get connection
 */
ap_entry_t ap_list[] = {
    { "Majerle WiFi", "majerle_internet" },
    { "Hoteldeshorlogers", "roomclient16" },
    { "scandic_easy", "" },
    { "HOTEL-VEGA", "hotelvega" },
    { "Slikop.", "slikop2012" },
    { "Danai Hotel", "danai2017!" },
    { "Amis3789606848", "majerle_internet_private" },
};

const uint8_t requestData[] = ""
"GET / HTTP/1.1\r\n"
"Host: " CONN_HOST "\r\n"
"Connection: close\r\n"
"\r\n";

/**
 * \brief           Initialization thread for entire process
 */
static void
init_thread(void const* arg) {
    size_t i, j;
    
    TM_GPIO_Init(GPIOC, GPIO_PIN_3, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
    
    printf("Initialization thread started!\r\n");
    esp_init(esp_cb);                           /* Init ESP stack */
    
    time = osKernelSysTick();
    
    esp_ap_configure("Tilenov WiFi", "ni dostopa", 5, ESP_ECN_WPA_WPA2_PSK, 8, 0, 1, 1);
    
    /**
     * Scan for network access points
     * In case we have access point,
     * try to connect to known Ap
     */
    if (esp_sta_list_ap(NULL, aps, sizeof(aps) / sizeof(aps[0]), &apf, 1) == espOK) {
        for (i = 0; i < apf; i++) {
            printf("AP found: %s\r\n", aps[i].ssid);
        }
        for (j = 0; j < sizeof(ap_list) / sizeof(ap_list[0]); j++) {
            for (i = 0; i < apf; i++) {
                if (!strcmp(aps[i].ssid, ap_list[j].ssid)) {
                    printf("Trying to connect to \"%s\" network\r\n", ap_list[j].ssid);
                    if (esp_sta_join(ap_list[j].ssid, ap_list[j].pass, NULL, 0, 1) == espOK) {
                        goto cont;
                    }
                }
            }
        }
    } else {
        printf("No WIFI to connect!\r\n");
    }
cont:

    /**
     * Determine if this nucleo is client or server
     */
    if (0 && TM_GPIO_GetInputPinValue(GPIOC, GPIO_PIN_3)) {  
        client_thread_id = osThreadCreate(osThread(client_thread), NULL);
        printf("Client mode!\r\n");
    } else {
        esp_http_server_init(&http_init, 80);
        printf("Server mode!\r\n");
    }
    
    /**
     * Check if device has set IP address
     */
    if (esp_sta_has_ip() == espOK) {
        uint8_t ip[4];
        esp_sta_copy_ip(ip, NULL, NULL);
        printf("Connected to WIFI!\r\n");
        printf("Device IP: %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);
    } else {
        if (client_thread_id != NULL) {
            osThreadTerminate(client_thread_id);
        }
        printf("Could not connect to any WiFi network!\r\n");
        printf("Closing down!\r\n");
        while (1) {
            osDelay(1000);
        }
    }
    
    client_thread_id = osThreadCreate(osThread(mqtt_thread), NULL);
    
    while (1) {
        if (0 && esp_ap_list_sta(stas, sizeof(stas) / sizeof(stas[0]), &staf, 1) == espOK) {
            printf("- - - - - - - - -\r\n");
            for (i = 0; i < staf; i++) {
                printf("STA: IP: %d.%d.%d.%d; MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                    stas[i].ip[0], stas[i].ip[1], stas[i].ip[2], stas[i].ip[3], 
                    stas[i].mac[0], stas[i].mac[1], stas[i].mac[2], 
                    stas[i].mac[3], stas[i].mac[4], stas[i].mac[5]
                );
            }
        }
        osDelay(60000);
        //esp_sta_list_ap(NULL, aps, sizeof(aps) / sizeof(aps[0]), &apf, 0);
        //printf("CPU USAGE: %d\r\n", (int)osGetCPUUsage());
        
//        if (TM_DISCO_ButtonOnPressed()) {
//            //esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "example.net", CONN_PORT, NULL, esp_conn_client_cb, 0);
//            esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "example.org", CONN_PORT, NULL, esp_conn_client_cb, 0);
//        }
    }
}

size_t resp_sent;
size_t sent;

/**
 * \brief           Client netconn thread
 */
void
client_thread(void const* arg) {
    esp_netconn_p conn;
    espr_t res;
    esp_pbuf_p pbuf;
    
    printf("API client: thread started\r\n");
    
    printf("Waiting first button press!\r\n");
    while (!TM_DISCO_ButtonPressed()) {
        osDelay(1);
    }
    while (TM_DISCO_ButtonPressed()) {
        osDelay(1);
    }
                                                
    conn = esp_netconn_new(ESP_NETCONN_TYPE_TCP);   /* Create new instance */
    if (conn != NULL) {
        while (1) {                             /* Infinite loop */            
            /**
             * Connect to external server as client
             */
            res = esp_netconn_connect(conn, CONN_HOST, CONN_PORT);
            if (res == espOK) {                 /* Are we successfully connected? */
                printf("API client: connected! Writing data...\r\n");
                res = esp_netconn_write(conn, requestData, sizeof(requestData) - 1);    /* Send data to server */
                if (res == espOK) {             /* Were data sent? */
                    printf("API client: data were written, waiting response\r\n");
                    uint32_t time = osKernelSysTick();
                    do {
                        res = esp_netconn_receive(conn, &pbuf); /* Receive single packet of data from +IPD response */
                        if (res == espCLOSED) { /* Was the connection closed? This can be checked by return status of receive function */
                            printf("API client: connection closed by remote server!\r\n");
                            break;
                        }
                        printf("Received PBUF: %d\r\n", (int)esp_pbuf_length(pbuf, 0));
                        esp_pbuf_free(pbuf);    /* Free processed data, must be done by user to clear memory leaks */
                    } while (1);
                    printf("Total receive time: %d ms\r\n", (int)(osKernelSysTick() - time));
                } else {
                    printf("API client: data write error!\r\n");
                }
                if (res != espCLOSED) {         /* If it was not closed by server, close it manually */
                    esp_netconn_close(conn);    /* Close the connection */
                }
            } else {
                printf("API client: cannot connect!\r\n");
            }
            
            
            osDelay(1000);                      /* Wait a little before starting a new connection */
        }
    }
}

char *
led_cgi_handler(http_param_t* params, size_t params_len) {
    int type = -1, value = -1;
    while (params_len--) {
        if (!strcmp(params->name, "led")) {
            if (!strcmp(params->value, "green")) {
                type = 0;
            } else if (!strcmp(params->value, "red")) {
                type = 1;
            }
        } else if (!strcmp(params->name, "val")) {
            if (!strcmp(params->value, "on")) {
                value = 1;
            } else if (!strcmp(params->value, "off")) {
                value = 0;
            } else if (!strcmp(params->value, "toggle")) {
                value = 2;
            }
        }
        params++;
    }
    if (type >= 0 && value >= 0) {
        if (value == 0) {
            TM_DISCO_LedOff(type == 0 ? LED_GREEN : LED_RED);
        } else if (value == 1) {
            TM_DISCO_LedOn(type == 0 ? LED_GREEN : LED_RED);
        } else if (value == 2) {
            TM_DISCO_LedToggle(type == 0 ? LED_GREEN : LED_RED);
        }
    }
    return "/index.html";
}

char *
usart_cgi_handler(http_param_t* params, size_t params_len) {
    printf("USART!\r\n");
    return "/index.html";
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
        default:
            break;
    }
    return espOK;
}

/* printf handler */
int
fputc(int ch, FILE* fil) {
    TM_USART_Putc(DISCO_USART, ch);             /* Send over debug USART */
    return ch;                                  /* Return OK */
}

char ssi_buffer[25];

static size_t
http_ssi_cb(http_state_t* hs, const char* tag_name, size_t tag_len) {
    if (!strncmp(tag_name, "title", tag_len)) {
        esp_http_server_write_string(hs, "ESP8266 SSI TITLE");
    } else if (!strncmp(tag_name, "led_status", tag_len)) {
        if (TM_DISCO_LedIsOn(LED_GREEN)) {
            esp_http_server_write_string(hs, "Led is on");
        } else {
            esp_http_server_write_string(hs, "Led is off");
        }
    } else if (!strncmp(tag_name, "wifi_list", tag_len)) {
        size_t i;

        esp_http_server_write_string(hs, "<table class=\"table\">");
        esp_http_server_write_string(hs, "<thead><tr><th>#</th><th>SSID</th><th>MAC</th><th>RSSI</th></tr></thead><tbody>");
        
        for (i = 0; i < apf; i++) {
            esp_http_server_write_string(hs, "<tr><td>");
            sprintf(ssi_buffer, "%d", (int)i);
            esp_http_server_write_string(hs, ssi_buffer);
            esp_http_server_write_string(hs, "</td><td>");
            esp_http_server_write_string(hs, aps[i].ssid);
            esp_http_server_write_string(hs, "</td><td>");
            sprintf(ssi_buffer, "%02X:%02X:%02X:%02X:%02X:%02X", aps[i].mac[0], aps[i].mac[1], aps[i].mac[2], aps[i].mac[3], aps[i].mac[4], aps[i].mac[5]);
            esp_http_server_write_string(hs, ssi_buffer);
            esp_http_server_write_string(hs, "</td><td>");
            sprintf(ssi_buffer, "%d", (int)aps[i].rssi);
            esp_http_server_write_string(hs, ssi_buffer);
            esp_http_server_write_string(hs, "</td></tr>");
        }
        esp_http_server_write_string(hs, "</tbody></table>");
    }
    return 0;
}

