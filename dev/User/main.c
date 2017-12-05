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
#include "esp.h"
#include "fs_data.h"
//#include "cmsis_os.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX5

void init_thread(void const* arg);
void server_thread(void const* arg);
void client_thread(void const* arg);

osThreadId init_thread_id, server_thread_id, client_thread_id;
osThreadDef(init_thread, init_thread, osPriorityNormal, 0, 512);
osThreadDef(server_thread, server_thread, osPriorityNormal, 0, 512);
osThreadDef(client_thread, client_thread, osPriorityNormal, 0, 512);

int
main(void) {
    TM_RCC_InitSystem();                        /* Init system */
    HAL_Init();                                 /* Init HAL layer */
    TM_DISCO_LedInit();                         /* Init leds */
    TM_DISCO_ButtonInit();                      /* Init button */
    TM_DELAY_Init();                            /* Init delay */
    TM_USART_Init(DISCO_USART, DISCO_USART_PP, 921600); /* Init USART for debug purpose */
    
    osThreadCreate(osThread(init_thread), NULL);/* Create init thread */
    osKernelStart();                            /* Start operating system */
    
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
static espr_t esp_conn_client_cb(esp_cb_t* cb);
static espr_t esp_conn_server_cb(esp_cb_t* cb);

uint32_t time;

#define CONN_HOST       "192.168.0.201"
#define CONN_PORT       80

esp_ap_t aps[100];
size_t apf;

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
    size_t sent, i, j;
    int val;
    esp_conn_p conn;
    uint8_t is_default = 0;
    uint8_t ip[4];
    
    TM_GPIO_Init(GPIOC, GPIO_PIN_3, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
    
    printf("Initialization thread started!\r\n");
    esp_init(esp_cb);                           /* Init ESP stack */
    
    time = osKernelSysTick();
  
    /**
     * Scan for network access points
     * In case we have access point,
     * try to connect to known Ap
     */
    if (esp_ap_list(NULL, aps, sizeof(aps) / sizeof(aps[0]), &apf, 1) == espOK) {
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
    if (TM_GPIO_GetInputPinValue(GPIOC, GPIO_PIN_3)) {  
        client_thread_id = osThreadCreate(osThread(client_thread), NULL);
        printf("Client mode!\r\n");
    } else {
        uint8_t ip[] = {192, 168, 0, 201};
        esp_sta_setip(ip, NULL, NULL, 0, 1);
        server_thread_id = osThreadCreate(osThread(server_thread), NULL);
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
        printf("Could not connect to any WiFi network!\r\n");
        printf("Closing down!\r\n");
        while (1);
    }
    
    printf("Init finished!\r\n");
    
    while (1) {
      
        osDelay(50);
        
//        if (TM_DISCO_ButtonOnPressed()) {
//            //esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "example.net", CONN_PORT, NULL, esp_conn_client_cb, 0);
//            esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "example.org", CONN_PORT, NULL, esp_conn_client_cb, 0);
//        }
    }
}

size_t resp_sent;
size_t sent;

/**
 * \brief           Thread for processing connections acting as server
 */
void
server_thread(void const* arg) {
    esp_netconn_p conn, new_conn;
    espr_t res;
    esp_pbuf_p pbuf;
    
    printf("API server thread started\r\n");
    
    conn = esp_netconn_new(ESP_NETCONN_TYPE_TCP);   /* Prepare a new connection */
    if (conn) {
        printf("API connection created\r\n");
        res = esp_netconn_bind(conn, 80);       /* Bind a connection on port 80 */
        if (res == espOK) {
            printf("API connection binded\r\n");
            res = esp_netconn_listen(conn);     /* Start listening on a connection */
            
            while (1) {
                printf("API waiting connection\r\n");
                res = esp_netconn_accept(conn, &new_conn);  /* Accept for a new connection */
                if (res == espOK) {             /* Do we have a new connection? */
                    printf("API new connection accepted: %d\r\n", (int)esp_netconn_getconnnum(new_conn));
                    
                    /**
                     * Receive data blocking from network
                     * We assume everything will be received in single packet
                     */
                    res = esp_netconn_receive(new_conn, &pbuf);
                    if (res == espOK) {         /* Do we have actual packet of data? */
                        printf("API data read: %d\r\n", (int)esp_netconn_getconnnum(new_conn));
                        fs_file_t* file = fs_data_open_file(pbuf);
                        if (file) {
                            esp_netconn_write(new_conn, file->data, file->len);
                            fs_data_close_file(file);
                        }
                        esp_pbuf_free(pbuf);    /* Free used memory */
                        esp_netconn_close(new_conn);    /* And close connection */
                    } else if (res == espCLOSED) {  /* Or was connection closed by remote automatically? */
                        printf("Connection already closed!\r\n");
                    }
                    esp_netconn_close(new_conn);    /* And close connection */
                    esp_netconn_delete(new_conn);   /* Delete everything */
                }
            }
        }
    }
}

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
    if (conn) {
        while (1) {                                 /* Infinite loop */            
            /**
             * Connect to external server as client
             */
            res = esp_netconn_connect(conn, CONN_HOST, CONN_PORT);
            if (res == espOK) {                     /* Are we successfully connected? */
                printf("API client: connected! Writing data...\r\n");
                res = esp_netconn_write(conn, requestData, sizeof(requestData) - 1);    /* Send data to server */
                if (res == espOK) {                 /* Were data sent? */
                    printf("API client: data were written, waiting response\r\n");
                    uint32_t time = osKernelSysTick();
                    do {
                        res = esp_netconn_receive(conn, &pbuf); /* Receive single packet of data from +IPD response */
                        if (res == espCLOSED) {     /* Was the connection closed? This can be checked by return status of receive function */
                            printf("API client: connection closed by remote server!\r\n");
                            break;
                        }
                        esp_pbuf_free(pbuf);        /* Free processed data, must be done by user to clear memory leaks */
                    } while (1);
                    printf("Total receive time: %d ms\r\n", (int)(osKernelSysTick() - time));
                } else {
                    printf("API client: data write error!\r\n");
                }
                if (res != espCLOSED) {             /* If it was not closed by server, close it manually */
                    esp_netconn_close(conn);        /* Close the connection */
                }
            } else {
                printf("API client: cannot connect!\r\n");
            }
            
            
            osDelay(1000);                          /* Wait a little before starting a new connection */
        }
    }
}

/**
 * \brief           Client connection based callback
 * \param[in]       cb: Pointer to callback data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_conn_client_cb(esp_cb_t* cb) {
    switch (cb->type) {
        case ESP_CB_CONN_ACTIVE: {
            //printf("Conn active: N: %d\r\n", cb->cb.conn_active_closed.conn->num);
            esp_conn_send(cb->cb.conn_active_closed.conn, requestData, sizeof(requestData), &sent, 0);
            break;
        }
        case ESP_CB_CONN_ERROR: {
            //printf("Could not connect to server %s:%d\r\n", cb->cb.conn_error.host, (int)cb->cb.conn_error.port);
            //esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, CONN_PORT, NULL, esp_conn_client_cb, 0);  
            break;
        }
        case ESP_CB_CONN_CLOSED: {
            printf("Conn closed: N: %d\r\n", esp_conn_getnum(cb->cb.conn_active_closed.conn));
            //esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, CONN_PORT, NULL, esp_conn_client_cb, 0);
            break;
        }
        case ESP_CB_DATA_RECV: {
            size_t i;
            size_t pbuf_len;
            const uint8_t* pbuf_data;
            
            pbuf_len = esp_pbuf_length(cb->cb.conn_data_recv.buff);
            pbuf_data = esp_pbuf_data(cb->cb.conn_data_recv.buff);
            
            //printf("Data received: N: %d; L: %d\r\n", cb->cb.conn_data_recv.conn->num, cb->cb.conn_data_recv.buff->len);
            for (i = 0; i < pbuf_len; i++) {
                printf("%c", pbuf_data[i]);
            }
            return espOKMEM;
        }
        case ESP_CB_DATA_SENT: {
            //printf("Data sent: N: %d\r\n", cb->cb.conn_data_recv.conn->num);
            break;
        }
        case ESP_CB_DATA_SEND_ERR: {
            //printf("Data sent error: N: %d\r\n", cb->cb.conn_data_send_err.conn->num);
            esp_conn_close(cb->cb.conn_data_send_err.conn, 0);
            break;
        }
        default:
            break;
    }
    return espOK;
}

/**
 * \brief           Server connection based callback
 * \param[in]       cb: Pointer to callback data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_conn_server_cb(esp_cb_t* cb) {
    switch (cb->type) {
        case ESP_CB_CONN_ACTIVE: {
            printf("Server conn active: N: %d\r\n", esp_conn_getnum(cb->cb.conn_active_closed.conn));
            break;
        }
        case ESP_CB_CONN_CLOSED: {
            printf("Server conn closed: N: %d\r\n", esp_conn_getnum(cb->cb.conn_active_closed.conn));
            break;
        }
        case ESP_CB_DATA_RECV: {
            printf("Server data received: N: %d; L: %d\r\n", esp_conn_getnum(cb->cb.conn_data_recv.conn), esp_pbuf_length(cb->cb.conn_data_recv.buff));
            if (!strncmp((const char *)cb->cb.conn_data_recv.buff, "GET /favicon.ico", 16)) {
                esp_conn_close(cb->cb.conn_data_recv.conn, 0);
//            } else if (!strncmp((const char *)cb->cb.conn_data_recv.buff, "GET /style.css", 14)) {
//                esp_conn_send(cb->cb.conn_data_recv.conn, responseData_css, sizeof(responseData_css) - 1, &resp_sent, 0);
//            } else {
//                esp_conn_send(cb->cb.conn_data_recv.conn, responseData, sizeof(responseData) - 1, &resp_sent, 0);
            }
            break;
        }
        case ESP_CB_DATA_SENT: {
            printf("Server data sent: N: %d\r\n", esp_conn_getnum(cb->cb.conn_data_recv.conn));
            esp_conn_close(cb->cb.conn_data_sent.conn, 0);
            break;
        }
        case ESP_CB_DATA_SEND_ERR: {
            printf("Data sent error: N: %d\r\n", esp_conn_getnum(cb->cb.conn_data_send_err.conn));
            esp_conn_close(cb->cb.conn_data_sent.conn, 0);
            break;
        }
        default:
            break;
    }
    return espOK;
}

/**
 * \brief           Global ESP callback for general info
 * \param[in]       cb: Pointer to callback data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_cb(esp_cb_t* cb) {
    switch (cb->type) {
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
