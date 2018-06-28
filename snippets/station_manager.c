#include "station_manager.h"
#include "esp/esp.h"

/*
 * List of preferred access points for ESP device
 * SSID and password
 * 
 * ESP will try to scan for access points 
 * and then compare them with the one on the list below
 */
ap_entry_t
ap_list[] = {
    //{ "SSID name", "SSID password" },
    { "TilenM_ST", "its private" },
    { "Majerle AMIS", "majerle_internet_private" },
    { "Majerle WiFi", "majerle_internet" },
    { "Slikop.", "slikop2012" },
};

/**
 * \brief           List of access points found by ESP device
 */
static
esp_ap_t aps[100];

/**
 * \brief           Number of valid access points in \ref aps array
 */
static
size_t apf;

/**
 * \brief           Connect to preferred access point
 *
 * \note            List of access points should be set by user in \ref ap_list structure
 * \param[in]       unlimited: When set to 1, function will block until SSID is found and connected
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
connect_to_preferred_access_point(uint8_t unlimited) {
    size_t i, j;
    espr_t eres;
    uint8_t tried;

    /*
     * Scan for network access points
     * In case we have access point,
     * try to connect to known AP
     */
    do {
        /*
         * Scan for access points visible to ESP device
         */
        printf("Scanning access points...\r\n");
        if ((eres = esp_sta_list_ap(NULL, aps, ESP_ARRAYSIZE(aps), &apf, 1)) == espOK) {
            tried = 0;
            /* Print all access points found by ESP */
            for (i = 0; i < apf; i++) {
                printf("AP found: %s\r\n", aps[i].ssid);
            }

            /*
             * Process array of preferred access points with array of found points
             */
            for (j = 0; j < ESP_ARRAYSIZE(ap_list); j++) {
                for (i = 0; i < apf; i++) {
                    if (!strcmp(aps[i].ssid, ap_list[j].ssid)) {
                        tried = 1;
                        printf("Connecting to \"%s\" network...\r\n", ap_list[j].ssid);
                        /* Try to join to access point */
                        if ((eres = esp_sta_join(ap_list[j].ssid, ap_list[j].pass, NULL, 1, 1)) == espOK) {
                            esp_ip_t ip;
                            esp_sta_copy_ip(&ip, NULL, NULL);

                            printf("Connected to %s network!\r\n", ap_list[j].ssid);
                            printf("Station IP address: %d.%d.%d.%d\r\n",
                                (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
                            return espOK;
                        } else {
                            printf("Connection error: %d\r\n", (int)eres);
                        }
                    }
                }
            }
            if (!tried) {
                printf("No access points available with preferred SSID!\r\nPlease check station_manager.c file and edit preferred SSID access points!\r\n");
            }
        } else if (eres == espERRNODEVICE) {
            printf("Device is not present!\r\n");
            break;
        } else {
            printf("Error on WIFI scan procedure!\r\n");
        }
        if (!unlimited) {
            break;
        }
    } while (1);
    return espERR;
}

static size_t last_index = 0;
static uint8_t is_listing = 0, is_connected;

static void
scan_access_points(void) {
    if (!is_listing) {
        if (esp_sta_list_ap(NULL, aps, ESP_ARRAYSIZE(aps), &apf, 0) == espOK) {
            printf("Access point scan started\r\n");
            is_listing = 1;                 /* Start scan procedure in async way */
        } else {
            printf("Access point scan failed\r\n");
        }
    }
}

static void
join_to_next_ap(void) {
    size_t i;
    if (esp_sta_is_joined()) {
        last_index = 0;
        return;
    }
    if (last_index >= apf) {
        last_index = 0;
        scan_access_points();               /* Scan access points */
        return;
    }

    /* Continue with other access points */
    for (; last_index < apf; last_index++) {
        for (i = 0; i < ESP_ARRAYSIZE(ap_list); i++) {
            if (!strcmp(aps[last_index].ssid, ap_list[i].ssid)) {
                printf("Start connection to %s access point\r\n", aps[last_index].ssid);
                if (esp_sta_join(ap_list[i].ssid, ap_list[i].pass, NULL, 0, 0) == espOK) {
                    last_index++;               /* Manually increase index */
                    return;
                }
            }
        }
    }
}

/**
 * \brief           Callback function for access points operation
 */
static espr_t
access_points_cb(esp_cb_t* cb) {
    switch (cb->type) {
        case ESP_CB_STA_LIST_AP: {
            is_listing = 0;
            printf("Access points listed!\r\n");
            last_index = 0;
            join_to_next_ap();
            break;
        }
        case ESP_CB_WIFI_CONNECTED: {
            is_connected = 1;
            printf("Wifi connected!\r\n");
            break;
        }
        case ESP_CB_WIFI_GOT_IP: {
            printf("Wifi got IP!\r\n");
            is_connected = 1;
            break;
        }
        case ESP_CB_WIFI_DISCONNECTED: {
            if (is_connected) {
                scan_access_points();
            }
            is_connected = 0;
            join_to_next_ap();
            break;
        }
        case ESP_CB_STA_JOIN_AP: {
            espr_t status = esp_evt_sta_join_ap_get_status(cb);
            if (status != espOK) {
                printf("Join NOT OK.\r\n");
                join_to_next_ap();
            }
            break;
        }
    }
    return espOK;
}

/**
 * \brief           Start async scan of access points and connect to preferred.
 *                  If station gets disconnected from access point, start procedure again
 */
void
start_access_point_scan_and_connect_procedure(void) {
    esp_cb_register(access_points_cb);          /* Register access points */
    scan_access_points();                       /* Scan for access points */
}
