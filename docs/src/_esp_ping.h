/**
 * \addtogroup      ESP_PING
 * \{
 *
 * By pinging external server, you can get response time in units of milliseconds
 * 
 * \code{c}
uint32_t time;

/* Try to ping domain example.com and print time */
if (esp_ping("example.com", &time, 1) == espOK) {
    printf("Ping successful. Time: %d ms\r\n", (int)time);
}
\endcode
 *
 * \}
 */