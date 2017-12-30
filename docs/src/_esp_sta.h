/**
 * \addtogroup      ESP_STA
 * \{
 * 
 * When device is in station mode, it is able to search for and connect to other access points.
 *
 * \par             Example
 *
 * Example shows how to scan for access points available around device
 *
 * \code{c}
size_t i, apf;
esp_ap_t aps[100];
 
/* Search for access points around ESP station */
if (esp_sta_list_ap(NULL, aps, ESP_ARRAYSIZE(aps), &apf, 1) == espOK) {
    for (i = 0; i < apf; i++) {
        printf("AP found: %s\r\n", aps[i].ssid);
    }
}
\endcode
 *
 * \}
 */
/**
 * \addtogroup      ESP_STA
 * \{
 *
 * \}
 */