/*
 * A simple example to get current time using SNTP protocol
 * thanks to AT commands being supported by Espressif
 */
#include "sntp.h"
#include "lwesp/lwesp.h"

/**
 * \brief           Run SNTP
 */
void
sntp_gettime(void) {
    struct tm dt;

    /* Enable SNTP with default configuration for NTP servers */
    if (lwesp_sntp_set_config(1, 1, NULL, NULL, NULL, NULL, NULL, 1) == lwespOK) {
        lwesp_delay(5000);

        /* Get actual time and print it */
        if (lwesp_sntp_gettime(&dt, NULL, NULL, 1) == lwespOK) {
            printf("Date & time: %d.%d.%d, %d:%d:%d\r\n", (int)dt.tm_mday, (int)(dt.tm_mon + 1),
                   (int)(dt.tm_year + 1900), (int)dt.tm_hour, (int)dt.tm_min, (int)dt.tm_sec);
        }
    }
}
