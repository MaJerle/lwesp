#include "utils.h"
#include "lwesp/lwesp.h"

#if WIN32
#define utils_printf            printf
#else
#define utils_printf            printf
#endif /* WIN32 */

/**
 * \brief           Print IP string to the output
 * \param[in]       str_b: Text to print before IP address
 * \param[in]       ip: IP to print
 * \param[in]       str_a: Text to print after IP address
 */
void
utils_print_ip(const char* str_b, const lwesp_ip_t* ip, const char* str_a) {
    if (str_b != NULL) {
        utils_printf("%s", str_b);
    }

    if (0) {
#if LWESP_CFG_IPV6
    } else if (ip->type == LWESP_IPTYPE_V6) {
        utils_printf("%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X\r\n",
            (unsigned)ip->addr.ip6.addr[0], (unsigned)ip->addr.ip6.addr[1], (unsigned)ip->addr.ip6.addr[2],
            (unsigned)ip->addr.ip6.addr[3], (unsigned)ip->addr.ip6.addr[4], (unsigned)ip->addr.ip6.addr[5],
            (unsigned)ip->addr.ip6.addr[6], (unsigned)ip->addr.ip6.addr[7]);
#endif /* LWESP_CFG_IPV6 */
    } else {
        utils_printf("%d.%d.%d.%d\r\n",
            (int)ip->addr.ip4.addr[0], (int)ip->addr.ip4.addr[1],
            (int)ip->addr.ip4.addr[2], (int)ip->addr.ip4.addr[3]);
    }
    if (str_a != NULL) {
        utils_printf("%s", str_a);
    }
}

/**
 * \brief           Print MAC string to the output
 * \param[in]       str_b: Text to print before MAC address
 * \param[in]       mac: MAC to print
 * \param[in]       str_a: Text to print after MAC address
 */
void
utils_print_mac(const char* str_b, const lwesp_mac_t* mac, const char* str_a) {
    if (str_b != NULL) {
        utils_printf("%s", str_b);
    }
    utils_printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",
        (unsigned)mac->mac[0], (unsigned)mac->mac[1], (unsigned)mac->mac[2],
        (unsigned)mac->mac[3], (unsigned)mac->mac[4], (unsigned)mac->mac[5]
    );
    if (str_a != NULL) {
        utils_printf("%s", str_a);
    }
}
