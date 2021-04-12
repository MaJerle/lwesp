#ifndef SNIPPET_HDR_UTILS_H
#define SNIPPET_HDR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "lwesp/lwesp.h"

void    utils_print_ip(const char* str_b, const lwesp_ip_t* ip, const char* str_a);
void    utils_print_mac(const char* str_b, const lwesp_mac_t* mac, const char* str_a);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
