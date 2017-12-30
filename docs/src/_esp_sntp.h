/**
 * \addtogroup      ESP_SNTP
 * \{
 *
 * SNTP protocol allows you to get current time from network servers.
 * 
 * \code{c}
esp_datetime_t dt;

/* Configure SNTP parameters: enable, timezone +1 and use default network servers */
if (esp_sntp_configure(1, 1, NULL, NULL, NULL, 1) == espOK) {
    /* Try to get time from network servers */
    if (esp_sntp_gettime(&dt, 1) == espOK) {
        printf("We have a date and time: %d.%d.%d: %d:%d:%d\r\n", 
            (int)dt.date, (int)dt.month, (int)dt.year, 
            (int)dt.hours, (int)dt.minutes, (int)dt.seconds
        );
    }
}
\endcode
 *
 * \}
 */
/**
 * \addtogroup      ESP_SNTP
 * \{
 * 
 * Or if you have your custom NTP servers, you can apply them on configuration part of SNTP.
 * 
 * \code{c}
/* Set custom NTP servers. You may apply up to 3 servers, all are optional */
esp_sntp_configure(1, 1, "server1.myntp.com", "server2.myntp.com", "server3.myntp.com", 1)
\endcode
 *
 * \}
 */