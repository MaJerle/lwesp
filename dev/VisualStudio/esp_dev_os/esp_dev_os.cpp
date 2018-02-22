// esp_dev_os.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "esp/esp.h"
#include "esp/apps/esp_http_server.h"

#include "mqtt_client.h"
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
    printf("App start!\r\n");

    /* Create start main thread */
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main_thread, NULL, 0, &main_thread_id);

    /* Do nothing at this point but do not close the program */
	while (1) {
        esp_delay(1000);
	}
}

/**
 * \brief           Main thread for init purposes
 */
static void
main_thread(void* arg) {
    esp_netconn_p c1;
    espr_t res;
    esp_pbuf_p p;

    /*
     * Init ESP library
     */
    esp_init(esp_cb);
    esp_sta_autojoin(1, 1);
    
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

    /*
     * Check if device has set IP address
     *
     * This should always pass
     */
    if (esp_sta_has_ip() == espOK) {
        esp_ip_t ip;
        esp_sta_copy_ip(&ip, NULL, NULL);
        printf("Connected to WIFI!\r\n");
        printf("Device IP: %d.%d.%d.%d\r\n", ip.ip[0], ip.ip[1], ip.ip[2], ip.ip[3]);
    }

    /*
     * Terminate thread
     */
    esp_sys_thread_terminate(NULL);
}

/**
 * \brief           Global ESP event function callback
 * \param[in]       cb: Event information
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_cb(esp_cb_t* cb) {
    switch (cb->type) {
        case ESP_CB_INIT_FINISH: {
            esp_set_at_baudrate(115200, 0);
            break;
        }
        case ESP_CB_RESET: {
            printf("Device reset!\r\n");
            break;
        }
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CB_AP_CONNECTED_STA: {
            esp_mac_t* mac = cb->cb.ap_conn_disconn_sta.mac;
            printf("New station connected to ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
            break;
        }
        case ESP_CB_AP_DISCONNECTED_STA: {
            esp_mac_t* mac = cb->cb.ap_conn_disconn_sta.mac;
            printf("Station disconnected from ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
            break;
        }
        case ESP_CB_AP_IP_STA: {
            esp_mac_t* mac = cb->cb.ap_ip_sta.mac;
            esp_ip_t* ip = cb->cb.ap_ip_sta.ip;
            printf("Station received IP address from ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X and IP: %d.%d.%d.%d\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5],
                ip->ip[0], ip->ip[1], ip->ip[2], ip->ip[3]);
            break;
        }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        default: break;
    }
	return espOK;
}
