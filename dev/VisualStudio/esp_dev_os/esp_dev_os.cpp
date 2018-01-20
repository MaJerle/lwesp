// esp_dev_os.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "esp/esp.h"
#include "apps/esp_http_server.h"

#include "mqtt.h"
#include "http_server.h"
#include "station_manager.h"
#include "netconn_client.h"
#include "netconn_server.h"

static void main_thread(void* arg);
DWORD main_thread_id;

static espr_t esp_cb(esp_cb_t* cb);

/**
 * \brief           Program entry point
 */
int
main() {
    /* Create start main thread */
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main_thread, NULL, 0, &main_thread_id);

    /* Do nothing anymore at this point */
	while (1) {
        esp_delay(1000);
	}
}

/**
 * \brief           Main thread for init purposes
 */
static void
main_thread(void* arg) {
    /*
     * Init ESP library
     */
    esp_init(esp_cb);
    
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
 * \brief           Global ESP event function callback
 * \param[in]       cb: Event information
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_cb(esp_cb_t* cb) {
    switch (cb->type) {
        case ESP_CB_INIT_FINISH: {

        }
        default: break;
    }
	return espOK;
}
