#include "station_manager.h"
#include "utils.h"
#include "lwesp/lwesp.h"

/**
 * \brief           Private access-point and station management system
 * 
 * This is used for asynchronous connection to access point
 */
typedef struct {
    size_t index_preferred_list;                /*!< Current index position of preferred array */
    size_t index_found_list;                    /*!< Current index position in array of found APs */
} prv_ap_data_t;

/*
 * List of preferred access points for ESP device
 * SSID and password
 *
 * ESP will try to scan for access points
 * and then compare them with the one on the list below
 */
static const ap_entry_t preferred_ap_list[] = {
    //{ "SSID name", "SSID password" },
    { .ssid = "TilenM_ST", .pass = "its private" },
    { .ssid = "Kaja", .pass = "kajagin2021" },
    { .ssid = "Majerle WIFI", .pass = "majerle_internet_private" },
    { .ssid = "Majerle AMIS", .pass = "majerle_internet_private" },
};
static lwesp_ap_t aps[100];                     /* Array of found access points */
static size_t apf = 0;                          /* Number of found access points */
static prv_ap_data_t ap_async_data;             /* Asynchronous data structure */

/**
 * \brief           Private event function for asynchronous scanning
 * \param[in]       evt: Event information
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
prv_evt_fn(lwesp_evt_t* evt) {
    switch (evt->type) {
        case LWESP_EVT_WIFI_GOT_IP: {
            if (lwesp_sta_has_ipv6_local()) {
                
            }
            if (lwesp_sta_has_ipv6_global()) {
                
            }
            break;
        }
        case LWESP_EVT_RESET: {
            break;
        }
        case LWESP_EVT_WIFI_CONNECTED: {
            break;
        }
        case LWESP_EVT_WIFI_DISCONNECTED: {
            /* Start scan mode in non-blocking mode */
            lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &apf, NULL, NULL, 0);
            break;
        }
        case LWESP_EVT_STA_LIST_AP: {
            /* Network scanning has completed */
            if (lwesp_evt_sta_list_ap_get_result(evt) == lwespOK) {
                const lwesp_ap_t *aps_found =  lwesp_evt_sta_list_ap_get_aps(evt);
                size_t aps_found_len = lwesp_evt_sta_list_ap_get_length(evt);
            
                /* Process complete list and try to find suitable match */
                for (size_t j = 0; j < LWESP_ARRAYSIZE(preferred_ap_list); j++) {
                    for (size_t i = 0; i < aps_found_len; i++) {
                        if (strncmp(aps_found[i].ssid, preferred_ap_list[j].ssid, strlen(preferred_ap_list[j].ssid)) == 0) {
                            /* Try to connect to the network */
                            lwesp_sta_join(preferred_ap_list[j].ssid, preferred_ap_list[j].pass, NULL, NULL, NULL, 0);
                        }
                    }
                }
            }
            break;
        }
        case LWESP_EVT_STA_INFO_AP: {
            break;
        }
        case LWESP_EVT_WIFI_IP_ACQUIRED: {
            break;
        }
        default: break;
    }
    return lwespOK;
}

/**
 * \brief           Initialize asynchronous mode to connect to preferred station
 * 
 * Function relies on LwESP callback to notify "disconnected wifi mode",
 * triggering the system to scan for access point and connection to appropriate one
 */
lwespr_t
station_manager_connect_to_access_point_async_init(void) {
    /* Register new function */
    lwesp_evt_register(prv_evt_fn);

    /* Start scanning process in non-blocking mode */
    return lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &apf, NULL, NULL, 0);
}

/**
 * \brief           Connect to preferred access point in blocking mode
 * 
 * This functionality can only be used if non-blocking approach is not used
 * 
 * \note            List of access points should be set by user in \ref ap_list structure
 * \param[in]       unlimited: When set to 1, function will block until SSID is found and connected
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
connect_to_preferred_access_point(uint8_t unlimited) {
    lwespr_t eres;
    uint8_t tried;

    /*
     * Scan for network access points
     * In case we have access point,
     * try to connect to known AP
     */
    do {
        if (lwesp_sta_has_ip()) {
            return lwespOK;
        }

        /* Scan for access points visible to ESP device */
        printf("Scanning access points...\r\n");
        if ((eres = lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &apf, NULL, NULL, 1)) == lwespOK) {
            tried = 0;
            /* Print all access points found by ESP */
            for (size_t i = 0; i < apf; i++) {
                printf("AP found: %s, CH: %d, RSSI: %d\r\n", aps[i].ssid, aps[i].ch, aps[i].rssi);
            }

            /* Process array of preferred access points with array of found points */
            for (size_t j = 0; j < LWESP_ARRAYSIZE(preferred_ap_list); j++) {
                for (size_t i = 0; i < apf; i++) {
                    if (strncmp(aps[i].ssid, preferred_ap_list[j].ssid, strlen(aps[i].ssid)) == 0) {
                        tried = 1;
                        printf("Connecting to \"%s\" network...\r\n", preferred_ap_list[j].ssid);

                        /* Try to join to access point */
                        if ((eres = lwesp_sta_join(preferred_ap_list[j].ssid, preferred_ap_list[j].pass, NULL, NULL, NULL, 1)) == lwespOK) {
                            lwesp_ip_t ip;
                            uint8_t is_dhcp;

                            printf("Connected to %s network!\r\n", preferred_ap_list[j].ssid);

                            lwesp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
                            utils_print_ip("Station IP address: ", &ip, "\r\n");
                            printf("; Is DHCP: %d\r\n", (int)is_dhcp);
                            return lwespOK;
                        } else {
                            printf("Connection error: %d\r\n", (int)eres);
                        }
                    }
                }
            }
            if (!tried) {
                printf("No access points available with preferred SSID!\r\nPlease check station_manager.c file and edit preferred SSID access points!\r\n");
            }
        } else if (eres == lwespERRNODEVICE) {
            printf("Device is not present!\r\n");
            break;
        } else {
            printf("Error on WIFI scan procedure!\r\n");
        }
        if (!unlimited) {
            break;
        }
    } while (1);
    return lwespERR;
}
