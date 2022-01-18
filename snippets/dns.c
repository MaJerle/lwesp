/*
 * This snippet shows how to use ESP's DNS module to 
 * obtain IP address from domain name
 */
#include "dns.h"
#include "lwesp/lwesp.h"

/* Host to resolve */
#define DNS_HOST1           "example.com"
#define DNS_HOST2           "example.net"

/**
 * \brief           Variable to hold result of DNS resolver
 */
static lwesp_ip_t ip;

/**
 * \brief           Function to print actual resolved IP address
 */
static void
prv_print_ip(void) {
    if (0) {
#if LWESP_CFG_IPV6
    } else if (ip.type == LWESP_IPTYPE_V6) {
        printf("IPv6: %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X\r\n",
            (unsigned)ip.addr.ip6.addr[0], (unsigned)ip.addr.ip6.addr[1], (unsigned)ip.addr.ip6.addr[2],
            (unsigned)ip.addr.ip6.addr[3], (unsigned)ip.addr.ip6.addr[4], (unsigned)ip.addr.ip6.addr[5],
            (unsigned)ip.addr.ip6.addr[6], (unsigned)ip.addr.ip6.addr[7]);
#endif /* LWESP_CFG_IPV6 */
    } else {
        printf("IPv4: %d.%d.%d.%d\r\n",
            (int)ip.addr.ip4.addr[0], (int)ip.addr.ip4.addr[1], (int)ip.addr.ip4.addr[2], (int)ip.addr.ip4.addr[3]);
    }
}

/**
 * \brief           Event callback function for API call,
 *                  called when API command finished with execution
 */
static void
prv_dns_resolve_evt(lwespr_t res, void* arg) {
    /* Check result of command */
    if (res == lwespOK) {
        /* Print actual resolved IP */
        prv_print_ip();
    }
}

/**
 * \brief           Start DNS resolver
 */
void
dns_start(void) {
    /* Use DNS protocol to get IP address of domain name */

    /* Get IP with non-blocking mode */
    if (lwesp_dns_gethostbyname(DNS_HOST2, &ip, prv_dns_resolve_evt, DNS_HOST2, 0) == lwespOK) {
        printf("Request for DNS record for " DNS_HOST2 " has started\r\n");
    } else {
        printf("Could not start command for DNS\r\n");
    }

    /* Get IP with blocking mode */
    if (lwesp_dns_gethostbyname(DNS_HOST1, &ip, prv_dns_resolve_evt, DNS_HOST1, 1) == lwespOK) {
        /* Print actual resolved IP */
        prv_print_ip();
    } else {
        printf("Could not retrieve IP address for " DNS_HOST1 "\r\n");
    }
}
