#include "station_manager.h"
#include "esp/esp.h"

/**
 * List of preferred access points for ESP device
 * SSID and password
 * 
 * ESP will try to scan for access points 
 * and then compare them with the one on the list below
 */
ap_entry_t
ap_list[] = {
    { "Tilen\xE2\x80\x99s iPhone", "ni dostopa" },
    { "Majerle WiFi", "majerle_internet" },
    { "Slikop.", "slikop2012" },
    { "Amis3789606848", "majerle_internet_private" },
};

esp_ap_t aps[100];
size_t apf;

/**
 * \brief           Connect to preferred access point
 *
 * \note            List of access points should be set by user in \ref ap_list structure
 * \param[in]       unlimited: When set to 1, function will block until SSID is found and connected
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
connect_to_preferred_access_point(uint8_t unlimited) {
    size_t i, j;
    espr_t eres;
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
                            return espOK;
                        } else {
                            printf("Connection error: %d\r\n", (int)eres);
                        }
                    }
                }
            }
        } else {
            printf("No WIFI to connect!\r\n");
        }
        if (!unlimited) {
            break;
        }
    } while (1);
    return espERR;
}
