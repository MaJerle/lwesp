#include "dns.h"
#include "esp/esp.h"

/* Host to resolve */
#define DNS_HOST1           "example.com"
#define DNS_HOST2           "example.net"

/**
 * \brief           Variable to hold result of DNS resolver
 */
static esp_ip_t ip;

/**
 * \brief           Event callback function for API call, 
 *                  called when API command finished with execution
 */
static void
dns_resolve_evt(espr_t res, void* arg) {
    /* Check result of command */
    if (res == espOK) {
        /* DNS resolver has IP address */
        printf("DNS record for %s (from API callback): %d.%d.%d.%d\r\n",
            (const char *)arg, (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
    }
}

/**
 * \brief           Start DNS resolver
 */
void
dns_start(void) {
    /* Use DNS protocol to get IP address of domain name */

    /* Get IP with non-blocking mode */
    if (esp_dns_gethostbyname(DNS_HOST2, &ip, dns_resolve_evt, DNS_HOST2, 0) == espOK) {
        printf("Request for DNS record for " DNS_HOST2 " has started\r\n");
    } else {
        printf("Could not start command for DNS\r\n");
    }

    /* Get IP with blocking mode */
    if (esp_dns_gethostbyname(DNS_HOST1, &ip, dns_resolve_evt, DNS_HOST1, 1) == espOK) {
        printf("DNS record for " DNS_HOST1 " (from lin code): %d.%d.%d.%d\r\n",
            (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
    } else {
        printf("Could not retrieve IP address for " DNS_HOST1 "\r\n");
    }
}
