#include "sntp.h"
#include "lwesp/lwesp.h"

/**
 * \brief           Run SNTP
 */
void
sntp_gettime(void) {
    lwesp_datetime_t dt;

    /* Enable SNTP with default configuration for NTP servers */
    if (lwesp_sntp_configure(1, 1, NULL, NULL, NULL, NULL, NULL, 1) == espOK) {
        lwesp_delay(5000);

        /* Get actual time and print it */
        if (lwesp_sntp_gettime(&dt, NULL, NULL, 1) == espOK) {
            printf("Date & time: %d.%d.%d, %d:%d:%d\r\n",
                   (int)dt.date, (int)dt.month, (int)dt.year,
                   (int)dt.hours, (int)dt.minutes, (int)dt.seconds);
        }
    }
}
