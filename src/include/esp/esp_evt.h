/**	
 * \file            esp_evt.h
 * \brief           Event helper functions
 */
 
/*
 * Copyright (c) 2018 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#ifndef __ESP_EVT_H
#define __ESP_EVT_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

/**
 * \ingroup         ESP
 * \defgroup        ESP_EVT Event helpers
 * \brief           Event helper functions
 * \{
 */

/**
 * \name            ESP_EVT_RESET
 * \anchor          ESP_EVT_RESET
 * \brief           Event helper functions for \ref ESP_CB_RESET event
 */

/**
 * \brief           Check if reset was forced by user
 * \param[in]       cc: Event handle
 * \return          `1` if forced by user, `0` otherwise
 * \hideinitializer
 */
#define esp_evt_reset_is_forced(cc)             ESP_U8(!!((cc)->cb.reset.forced))

/**
 * \}
 */

/**
 * \name            ESP_EVT_AP_IP_STA
 * \anchor          ESP_EVT_AP_IP_STA
 * \brief           Event helper functions for \ref ESP_CB_AP_IP_STA event
 */

/**
 * \brief           Get MAC address from station
 * \param[in]       cc: Event handle
 * \return          MAC address
 * \hideinitializer
 */
#define esp_evt_ap_ip_sta_get_mac(cc)           ((cc)->cb.ap_ip_sta.mac)

/**
 * \brief           Get IP address from station
 * \param[in]       cc: Event handle
 * \return          IP address
 * \hideinitializer
 */
#define esp_evt_ap_ip_sta_get_ip(cc)            ((cc)->cb.ap_ip_sta.ip)
    
/**
 * \}
 */

/**
 * \name            ESP_EVT_AP_CONNECTED_STA
 * \anchor          ESP_EVT_AP_CONNECTED_STA
 * \brief           Event helper functions for \ref ESP_CB_AP_CONNECTED_STA event
 */

/**
 * \brief           Get MAC address from connected station
 * \param[in]       cc: Event handle
 * \return          MAC address
 * \hideinitializer
 */
#define esp_evt_ap_connected_sta_get_mac(cc)    ((cc)->cb.ap_conn_disconn_sta.mac)

/**
 * \}
 */

/**
 * \name            ESP_EVT_AP_DISCONNECTED_STA
 * \anchor          ESP_EVT_AP_DISCONNECTED_STA
 * \brief           Event helper functions for \ref ESP_CB_AP_DISCONNECTED_STA event
 */

/**
 * \brief           Get MAC address from disconnected station
 * \param[in]       cc: Event handle
 * \return          MAC address
 * \hideinitializer
 */
#define esp_evt_ap_disconnected_sta_get_mac(cc) ((cc)->cb.ap_conn_disconn_sta.mac)

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_DATA_RECV
 * \anchor          ESP_EVT_CONN_DATA_RECV
 * \brief           Event helper functions for \ref ESP_CB_CONN_DATA_RECV event
 */

/**
 * \brief           Get buffer from received data
 * \param[in]       cc: Event handle
 * \return          Buffer handle
 * \hideinitializer
 */
#define esp_evt_conn_data_recv_get_buff(cc)     ((cc)->cb.conn_data_recv.buff)

/**
 * \brief           Get connection handle for receive
 * \param[in]       cc: Event handle
 * \return          Connection handle
 * \hideinitializer
 */
#define esp_evt_conn_data_recv_get_conn(cc)     ((cc)->cb.conn_data_recv.conn)

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_DATA_SENT
 * \anchor          ESP_EVT_CONN_DATA_SENT
 * \brief           Event helper functions for \ref ESP_CB_CONN_DATA_SENT event
 */

/**
 * \brief           Get connection handle for data sent event
 * \param[in]       cc: Event handle
 * \return          Connection handle
 * \hideinitializer
 */
#define esp_evt_conn_data_sent_get_conn(cc)     ((cc)->cb.conn_data_sent.conn)

/**
 * \brief           Get number of bytes sent on connection
 * \param[in]       cc: Event handle
 * \return          Number of bytes sent
 * \hideinitializer
 */
#define esp_evt_conn_data_sent_get_length(cc)   ((cc)->cb.conn_data_sent.size)

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_DATA_SEND_ERR
 * \anchor          ESP_EVT_CONN_DATA_SEND_ERR
 * \brief           Event helper functions for \ref ESP_CB_CONN_DATA_SEND_ERR event
 */

/**
 * \brief           Get connection handle
 * \param[in]       cc: Event handle
 * \return          Connection handle
 * \hideinitializer
 */
#define esp_evt_conn_data_send_err_get_conn(cc)   ((cc)->cb.conn_data_send_err.conn)

/**
 * \brief           Get number of bytes successfully sent on failed send command
 * \param[in]       cc: Event handle
 * \return          Connection handle
 * \hideinitializer
 */
#define esp_evt_conn_data_send_err_get_length(cc)   ((cc)->cb.conn_data_send_err.size)

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_ACTIVE
 * \anchor          ESP_EVT_CONN_ACTIVE
 * \brief           Event helper functions for \ref ESP_CB_CONN_ACTIVE event
 */

/**
 * \brief           Get connection handle
 * \param[in]       cc: Event handle
 * \return          Connection handle
 * \hideinitializer
 */
#define esp_evt_conn_active_get_conn(cc)        ((cc)->cb.conn_active_closed.conn)

/**
 * \brief           Check if new connection is client
 * \param[in]       cc: Event handle
 * \return          `1` if client, `0` otherwise
 * \hideinitializer
 */
#define esp_evt_conn_active_is_client(cc)       ESP_U8(!!((cc)->cb.conn_active_closed.client))

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_CLOSED
 * \anchor          ESP_EVT_CONN_CLOSED
 * \brief           Event helper functions for \ref ESP_CB_CONN_CLOSED event
 */

/**
 * \brief           Get connection handle
 * \param[in]       cc: Event handle
 * \return          Connection handle
 * \hideinitializer
 */
#define esp_evt_conn_closed_get_conn(cc)        ((cc)->cb.conn_active_closed.conn)

/**
 * \brief           Check if just closed connection was client
 * \param[in]       cc: Event handle
 * \return          `1` if client, `0` otherwise
 * \hideinitializer
 */
#define esp_evt_conn_closed_is_client(cc)       ((cc)->cb.conn_active_closed.client)

/**
 * \brief           Check if connection close even was forced by user
 * \param[in]       cc: Event handle
 * \return          `1` if forced, `0` otherwise
 * \hideinitializer
 */
#define esp_evt_conn_closed_is_forced(cc)       ((cc)->cb.conn_active_closed.forced)

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_POLL
 * \anchor          ESP_EVT_CONN_POLL
 * \brief           Event helper functions for \ref ESP_CB_CONN_POLL event
 */

/**
 * \brief           Get connection handle
 * \param[in]       cc: Event handle
 * \return          Connection handle
 * \hideinitializer
 */
#define esp_evt_conn_poll_get_conn(cc)          ((cc)->cb.conn_poll.conn)

/**
 * \}
 */

/**
 * \name            ESP_EVT_STA_LIST_AP
 * \anchor          ESP_EVT_STA_LIST_AP
 * \brief           Event helper functions for \ref ESP_CB_STA_LIST_AP event
 */

/**
 * \brief           Get command success status
 * \param[in]       cc: Event handle
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 * \hideinitializer
 */
#define esp_evt_sta_list_ap_get_status(cc)      ((cc)->cb.sta_list_ap.status)

/**
 * \brief           Get command success status
 * \param[in]       cc: Event handle
 * \return          Pointer to \ref esp_ap_t with first access point description
 * \hideinitializer
 */
#define esp_evt_sta_list_ap_get_aps(cc)         ((cc)->cb.sta_list_ap.aps)

/**
 * \brief           Get number of access points found
 * \param[in]       cc: Event handle
 * \return          Number of access points found
 * \hideinitializer
 */
#define esp_evt_sta_list_ap_get_length(cc)      ((cc)->cb.sta_list_ap.len)

/**
 * \}
 */

/**
 * \name            ESP_EVT_STA_JOIN_AP
 * \anchor          ESP_EVT_STA_JOIN_AP
 * \brief           Event helper functions for \ref ESP_CB_STA_JOIN_AP event
 */

/**
 * \brief           Get command success status
 * \param[in]       cc: Event handle
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 * \hideinitializer
 */
#define esp_evt_sta_join_ap_get_status(cc)      ((cc)->cb.sta_join_ap.status)

/**
 * \}
 */

/**
 * \name            ESP_EVT_DNS_HOSTBYNAME
 * \anchor          ESP_EVT_DNS_HOSTBYNAME
 * \brief           Event helper functions for \ref ESP_CB_DNS_HOSTBYNAME event
 */

/**
 * \brief           Get resolve status
 * \param[in]       cc: Event handle
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 * \hideinitializer
 */
#define esp_evt_dns_hostbyname_get_status(cc)   ((cc)->cb.dns_hostbyname.status)

/**
 * \brief           Get hostname used to resolve IP address
 * \param[in]       cc: Event handle
 * \return          Hostname
 * \hideinitializer
 */
#define esp_evt_dns_hostbyname_get_host(cc)     ((cc)->cb.dns_hostbyname.host)

/**
 * \brief           Get IP address from DNS function
 * \param[in]       cc: Event handle
 * \return          IP address
 * \hideinitializer
 */
#define esp_evt_dns_hostbyname_get_ip(cc)       ((cc)->cb.dns_hostbyname.ip)

/**
 * \}
 */

/**
 * \}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __ESP_PING_H */
