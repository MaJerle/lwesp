// esp_dev_os.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "esp/esp.h"
#include "apps/esp_http_server.h"
#include "mqtt.h"

static void main_thread(void* arg);
DWORD main_thread_id;

static espr_t esp_cb(esp_cb_t* cb);

esp_ap_t aps[100];
size_t apf;

const http_init_t
http_init = {0};

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
    { "Tilen\xE2\x80\x99s iPhone", "ni dostopa" },
    { "Hoteldeshorlogers", "roomclient16" },
    { "scandic_easy", "" },
    { "HOTEL-VEGA", "hotelvega" },
    { "Slikop.", "slikop2012" },
    { "Danai Hotel", "danai2017!" },
    { "Amis3789606848", "majerle_internet_private" },
};

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
	size_t i, j;
	espr_t eres;

    /*
     * Init ESP library
     */
    esp_init(esp_cb);

    /*
     * Create new thread for MQTT client
     */
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)mqtt_thread, NULL, 0, NULL);

	/*
	 * Scan for network access points
	 * In case we have access point,
	 * try to connect to known AP
	 */
	do {
		if (esp_sta_list_ap(NULL, aps, sizeof(aps) / sizeof(aps[0]), &apf, 1) == espOK) {
			for (i = 0; i < apf; i++) {
				printf("AP found: %s\r\n", aps[i].ssid);
			}
			for (j = 0; j < sizeof(ap_list) / sizeof(ap_list[0]); j++) {
				for (i = 0; i < apf; i++) {
					if (!strcmp(aps[i].ssid, ap_list[j].ssid)) {
						printf("Trying to connect to \"%s\" network\r\n", ap_list[j].ssid);
						if ((eres = esp_sta_join(ap_list[j].ssid, ap_list[j].pass, NULL, 0, 1)) == espOK) {
							goto cont;
						}
						else {
							printf("Connection error: %d\r\n", (int)eres);
						}
					}
				}
			}
		}
		else {
			printf("No WIFI to connect!\r\n");
		}
	} while (1);
cont:

    /* Start HTTP server */
	esp_http_server_init(&http_init, 80);

	while (1) {
		esp_delay(1000);
	}
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
            esp_set_at_baudrate(921600, 0);
        }
    }
	return espOK;
}
