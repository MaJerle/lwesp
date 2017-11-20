/**
 * \file            esp_at.h
 * \brief           AT commands parser
 */

/*
 * Copyright (c) 2017, Tilen MAJERLE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *  * Neither the name of the author nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \author          Tilen MAJERLE <tilen@majerle.eu>
 */
#ifndef __ESP_AT_H
#define __ESP_AT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef ESP_INTERNAL

espr_t      espi_process(void);
    
espr_t      espi_basic_uart(esp_msg_t* msg);
    
espr_t      espi_set_wifi_mode(esp_msg_t* msg);
espr_t      espi_reset(esp_msg_t* msg);
espr_t      espi_get_conns_status(esp_msg_t* msg);
    
espr_t      espi_sta_join_quit(esp_msg_t* msg);
    
espr_t      espi_cip_sta_ap_cmd(esp_msg_t* msg);
    
espr_t      espi_tcpip_misc(esp_msg_t* msg);
espr_t      espi_tcpip_server(esp_msg_t* msg);
    
espr_t      espi_tcpip_conn(esp_msg_t* msg);
    
espr_t      espi_tcpip_process_send_data(void);
    
#endif /* ESP_INTERNAL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESP_AT_H */
