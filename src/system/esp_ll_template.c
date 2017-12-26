/**
 * \file            esp_ll_template.c
 * \brief           Low-level communication with ESP device template
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
#define ESP_INTERNAL
#include "system/esp_ll.h"
#include "esp/esp.h"
#include "esp/esp_input.h"

/**
 * \addtogroup      ESP_LL
 * \{
 *
 * Low-level communication part is responsible to make sure
 * all bytes received from ESP device are properly 
 * sent to upper layer stack and that all bytes are sent to ESP
 * when requested by upper layer ESP stack
 *
 * When initializing low-level part, following steps are important and must be done when \ref esp_ll_init function is called:
 *
 * 1. Assign memory for dynamic allocations required by ESP library
 * 2. Configure AT send function to use when we have data to be transmitted
 * 3. Configure AT port to be able to send/receive any data
 *
 * \par             Example
 * 
 * Example shows basic functionality what user must do in order to prepare stack properly.
 *
 * \code{c}
uint8_t initialized = 0;

// Prepare send function to send the data to AT port
static uint16_t
send_data(const void* data, uint16_t len) {
    return len;
}

// Implement esp_ll_init callback function
// It is called when stack is initialized
espr_t
esp_ll_init(esp_ll_t* ll, uint32_t baudrate) {
    // Step 1
    static uint8_t memory[0x10000];
    // Create a lookup table of memory regions for dynamic memory allocator
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions)); 
    }
    
    // Step 2
    if (!initialized) {
        ll->send_fn = send_data;
    }
    
    // Step 3: User must configure UART with specific baudrate
    configure_uart(baudrate);
    
    initialized = 1;
}
\endcode
 *
 * \}
 */

static uint8_t initialized = 0;

/**
 * \brief           Send data to ESP device, function called from ESP stack when we have data to send
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static uint16_t
send_data(const void* data, uint16_t len) {
    /**
     * Implement send function here
     */
    
    
    return len;                                 /* Return number of bytes actually sent to AT port */
}

/**
 * \brief           Callback function called from initialization process
 *
 * \note            This function may be called multiple times if AT baudrate is changed from application.
 *                  It is important that every configuration except AT baudrate is configured only once!
 *
 * \note            This function may be called from different threads in ESP stack when using OS.
 *                  When \ref ESP_INPUT_USE_PROCESS is set to 1, this function may be called from user UART thread.
 *
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \param[in]       baudrate: Baudrate to use on AT port
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ll_init(esp_ll_t* ll, uint32_t baudrate) {
    /*
     * Step 1: Configure memory for dynamic allocations
     */
    static uint8_t memory[0x10000];             /* Create memory for dynamic allocations with specific size */

    /*
     * Create memory region(s) of memory.
     * If device has internal/external memory available,
     * multiple memories may be used
     */
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations to ESP library */
    }
    
    /*
     * Step 2: Set AT port send function to use when we have data to transmit
     */
    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
    }

    /*
     * Step 3: Configure AT port to be able to send/receive data to/from ESP device
     */
    configure_uart(baudrate);                   /* Initialize UART for communication */
    initialized = 1;
    return espOK;
}
