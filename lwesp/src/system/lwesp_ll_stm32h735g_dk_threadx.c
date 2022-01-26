/**
 * \file            lwesp_ll_stm32h735g_dk_threadx.c
 * \brief           STM32H735-DK driver with ThreadX
 */

/*
 * Copyright (c) 2022 Tilen MAJERLE
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
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.2-dev
 */

/*
 * This is special driver for STM32H735G-DK utilizing ThreadX operating system
 * and full DMA support for TX and RX operation.
 */
#include "lwesp/lwesp.h"
#include "lwesp/lwesp_mem.h"
#include "lwesp/lwesp_input.h"
#include "system/lwesp_ll.h"
#include "lwrb/lwrb.h"
#include "tx_api.h"
#include "mcu.h"

#if !__DOXYGEN__

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"

#if !LWESP_CFG_INPUT_USE_PROCESS
#error "LWESP_CFG_INPUT_USE_PROCESS must be enabled in `lwesp_opts.h` to use this driver."
#endif /* LWESP_CFG_INPUT_USE_PROCESS */
#if !LWESP_CFG_MEM_CUSTOM
#error "LWESP_CFG_MEM_CUSTOM must be used instead. This driver does not set memory regions for LwESP."
#endif /* !LWESP_CFG_MEM_CUSTOM */

/* USART setup */
#define LWESP_USART                                 USART1
#define LWESP_USART_CLK_EN                          LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1)
#define LWESP_USART_IRQ                             USART1_IRQn
#define LWESP_USART_IRQ_HANDLER                     USART1_IRQHandler

/* TX DMA */
#define LWESP_USART_DMA_TX                          DMA1
#define LWESP_USART_DMA_TX_STREAM                   LL_DMA_STREAM_5
#define LWESP_USART_DMA_TX_REQUEST                  LL_DMAMUX1_REQ_USART1_TX
#define LWESP_USART_DMA_TX_CLK_EN                   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)
#define LWESP_USART_DMA_TX_IRQ                      DMA1_Stream5_IRQn
#define LWESP_USART_DMA_TX_IRQ_HANDLER              DMA1_Stream5_IRQHandler
#define LWESP_USART_DMA_TX_IS_TC                    LL_DMA_IsActiveFlag_TC5(LWESP_USART_DMA_TX)
#define LWESP_USART_DMA_TX_CLEAR_TC                 LL_DMA_ClearFlag_TC5(LWESP_USART_DMA_TX)
#define LWESP_USART_DMA_TX_CLEAR_HT                 LL_DMA_ClearFlag_HT5(LWESP_USART_DMA_TX)
#define LWESP_USART_DMA_TX_CLEAR_TE                 LL_DMA_ClearFlag_TE5(LWESP_USART_DMA_TX)

/* RX DMA */
#define LWESP_USART_DMA_RX                          DMA1
#define LWESP_USART_DMA_RX_STREAM                   LL_DMA_STREAM_4
#define LWESP_USART_DMA_RX_REQUEST                  LL_DMAMUX1_REQ_USART1_RX
#define LWESP_USART_DMA_RX_CLK_EN                   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)
#define LWESP_USART_DMA_RX_IRQ                      DMA1_Stream4_IRQn
#define LWESP_USART_DMA_RX_IRQ_HANDLER              DMA1_Stream4_IRQHandler
#define LWESP_USART_DMA_RX_IS_TC                    LL_DMA_IsActiveFlag_TC4(LWESP_USART_DMA_RX)
#define LWESP_USART_DMA_RX_IS_HT                    LL_DMA_IsActiveFlag_HT4(LWESP_USART_DMA_RX)
#define LWESP_USART_DMA_RX_IS_TE                    LL_DMA_IsActiveFlag_TE4(LWESP_USART_DMA_RX)
#define LWESP_USART_DMA_RX_CLEAR_TC                 LL_DMA_ClearFlag_TC4(LWESP_USART_DMA_RX)
#define LWESP_USART_DMA_RX_CLEAR_HT                 LL_DMA_ClearFlag_HT4(LWESP_USART_DMA_RX)
#define LWESP_USART_DMA_RX_CLEAR_TE                 LL_DMA_ClearFlag_TE4(LWESP_USART_DMA_RX)

/* GPIO configuration */
#define LWESP_USART_TX_PORT                         GPIOB
#define LWESP_USART_TX_PIN                          LL_GPIO_PIN_14
#define LWESP_USART_TX_PORT_CLK_EN                  LL_AHB1_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB)
#define LWESP_USART_TX_PIN_AF                       LL_GPIO_AF_4
#define LWESP_USART_RX_PORT                         GPIOB
#define LWESP_USART_RX_PIN                          LL_GPIO_PIN_15
#define LWESP_USART_RX_PORT_CLK_EN                  LL_AHB1_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB)
#define LWESP_USART_RX_PIN_AF                       LL_GPIO_AF_4

/* USART memory */
ALIGN_32BYTES(static uint8_t __attribute__((section(".dma_buffer"))) usart_rx_dma_buffer[0x100]);
static uint8_t      is_running, initialized = 0;
static size_t       old_pos = 0;

/* TX data buffers, must be 32-bytes aligned (cache) and in dma buffer section to make sure DMA has access to the memory region */
ALIGN_32BYTES(static uint8_t __attribute__((section(".dma_buffer"))) tx_rb_data[4096]);
static lwrb_t tx_rb;
volatile size_t tx_len;

/* USART thread */
static void prv_read_thread_entry(ULONG arg);
static TX_THREAD read_thread;
static UCHAR read_thread_stack[4 * LWESP_SYS_THREAD_SS];

/* Message queue */
#define LL_QUEUE_NUM_OF_ENTRY               10
static UCHAR    usart_ll_mbox_mem[LL_QUEUE_NUM_OF_ENTRY * sizeof(ULONG)];
static TX_QUEUE usart_ll_mbox;

/**
 * \brief           USART data processing thread
 * This is the thread used to enter received data from UART to the LwESP stack for further processing
 * \param[in]       arg: User argument
 */
static void
prv_read_thread_entry(ULONG arg) {
    size_t pos;

    LWESP_UNUSED(arg);

    while (1) {
        void* d;
        /* Wait for the event message from DMA or USART */
        tx_queue_receive(&usart_ll_mbox, &d, TX_WAIT_FOREVER);

        /* Read data */
        pos = sizeof(usart_rx_dma_buffer) - LL_DMA_GetDataLength(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);
        SCB_InvalidateDCache_by_Addr(usart_rx_dma_buffer, sizeof(usart_rx_dma_buffer));
        if (pos != old_pos && is_running) {
            if (pos > old_pos) {
                lwesp_input_process(&usart_rx_dma_buffer[old_pos], pos - old_pos);
            } else {
                lwesp_input_process(&usart_rx_dma_buffer[old_pos], sizeof(usart_rx_dma_buffer) - old_pos);
                if (pos > 0) {
                    lwesp_input_process(&usart_rx_dma_buffer[0], pos);
                }
            }
            old_pos = pos;
        }
    }
}

/**
 * \brief           Try to send more data with DMA
 */
static void
prv_start_tx_transfer(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (tx_len == 0
        && (tx_len = lwrb_get_linear_block_read_length(&tx_rb)) > 0) {
        const void* d = lwrb_get_linear_block_read_address(&tx_rb);

        /* Limit tx len up to some size to optimize buffer reading process */
        tx_len = LWESP_MIN(tx_len, 64);

        /* Cleanup cache first to make sure we have latest data in memory */
        SCB_CleanDCache_by_Addr((void *)d, tx_len);

        /* Disable channel if enabled */
        LL_DMA_DisableStream(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM);

        /* Clean flags */
        LWESP_USART_DMA_TX_CLEAR_TC;
        LWESP_USART_DMA_TX_CLEAR_HT;
        LWESP_USART_DMA_TX_CLEAR_TE;

        /* Configure DMA */
        LL_DMA_SetMemoryAddress(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, (uint32_t)d);
        LL_DMA_SetDataLength(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, tx_len);

        /* Enable instances */
        LL_DMA_EnableStream(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM);
    }
    __set_PRIMASK(primask);
}

/**
 * \brief           Configure UART using DMA for receive in double buffer mode and IDLE line detection
 */
static void
prv_configure_uart(uint32_t baudrate) {
    LL_GPIO_InitTypeDef gpio_init = {0};
    LL_USART_InitTypeDef usart_init = {0};

    if (!initialized) {
        /* Enable peripheral clocks */
        LWESP_USART_CLK_EN;
        LWESP_USART_DMA_RX_CLK_EN;
        LWESP_USART_DMA_TX_CLK_EN;
        LWESP_USART_TX_PORT_CLK_EN;
        LWESP_USART_RX_PORT_CLK_EN;

        /* Global pin configuration */
        LL_GPIO_StructInit(&gpio_init);
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull = LL_GPIO_PULL_UP;
        gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;

        /* TX PIN */
        gpio_init.Pin = LWESP_USART_TX_PIN;
        gpio_init.Alternate = LWESP_USART_TX_PIN_AF;
        LL_GPIO_Init(LWESP_USART_TX_PORT, &gpio_init);

        /* RX PIN */
        gpio_init.Pin = LWESP_USART_RX_PIN;
        gpio_init.Alternate = LWESP_USART_RX_PIN_AF;
        LL_GPIO_Init(LWESP_USART_RX_PORT, &gpio_init);

        /*******************/
        /*** UART RX DMA ***/
        /*******************/

        /* Enable DMA interrupts */
        NVIC_SetPriority(LWESP_USART_DMA_RX_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x05, 0x00));
        NVIC_EnableIRQ(LWESP_USART_DMA_RX_IRQ);

        /* Configure DMA */
        LL_DMA_SetPeriphRequest(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LWESP_USART_DMA_RX_REQUEST);
        LL_DMA_SetDataTransferDirection(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
        LL_DMA_SetStreamPriorityLevel(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_DMA_PRIORITY_MEDIUM);
        LL_DMA_SetMode(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_DMA_MODE_CIRCULAR);
        LL_DMA_SetPeriphIncMode(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_DMA_MDATAALIGN_BYTE);
        LL_DMA_DisableFifoMode(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);
        LL_DMA_SetPeriphAddress(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, LL_USART_DMA_GetRegAddr(LWESP_USART, LL_USART_DMA_REG_DATA_RECEIVE));
        LL_DMA_SetMemoryAddress(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, (uint32_t)usart_rx_dma_buffer);
        LL_DMA_SetDataLength(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM, sizeof(usart_rx_dma_buffer));

        /* Enable DMA interrupts */
        LL_DMA_EnableIT_HT(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TC(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TE(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_FE(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_DME(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);

        /*******************/
        /*** UART TX DMA ***/
        /*******************/

        /* Enable DMA interrupts */
        NVIC_SetPriority(LWESP_USART_DMA_TX_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x05, 0x00));
        NVIC_EnableIRQ(LWESP_USART_DMA_TX_IRQ);

        /* Configure DMA */
        LL_DMA_SetPeriphRequest(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LWESP_USART_DMA_TX_REQUEST);
        LL_DMA_SetDataTransferDirection(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
        LL_DMA_SetStreamPriorityLevel(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_DMA_PRIORITY_MEDIUM);
        LL_DMA_SetMode(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_DMA_MODE_NORMAL);
        LL_DMA_SetPeriphIncMode(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_DMA_MDATAALIGN_BYTE);
        LL_DMA_DisableFifoMode(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM);
        LL_DMA_SetPeriphAddress(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM, LL_USART_DMA_GetRegAddr(LWESP_USART, LL_USART_DMA_REG_DATA_TRANSMIT));

        /* Enable DMA interrupts */
        LL_DMA_EnableIT_TC(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM);
        LL_DMA_EnableIT_TE(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM);

        /*******************/
        /***    UART     ***/
        /*******************/

        /* Enable USART interrupts */
        NVIC_SetPriority(LWESP_USART_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x05, 0x00));
        NVIC_EnableIRQ(LWESP_USART_IRQ);

        /* Configure UART */
        LL_USART_DeInit(LWESP_USART);
        LL_USART_StructInit(&usart_init);
        usart_init.BaudRate = baudrate;
        usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
        usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
        usart_init.OverSampling = LL_USART_OVERSAMPLING_16;
        usart_init.Parity = LL_USART_PARITY_NONE;
        usart_init.StopBits = LL_USART_STOPBITS_1;
        usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
        LL_USART_Init(LWESP_USART, &usart_init);

        /* Enable USART interrupts and DMA request */
        LL_USART_EnableIT_IDLE(LWESP_USART);
        LL_USART_EnableIT_ERROR(LWESP_USART);
        LL_USART_EnableDMAReq_RX(LWESP_USART);
        LL_USART_EnableDMAReq_TX(LWESP_USART);

        /* Reset DMA position */
        old_pos = 0;

        /* Start DMA and USART */
        LL_DMA_EnableStream(LWESP_USART_DMA_RX, LWESP_USART_DMA_RX_STREAM);
        LL_USART_Enable(LWESP_USART);

        is_running = 1;
    } else {
        //tx_thread_sleep(10);
        //LL_USART_Disable(LWESP_USART);
        //usart_init.BaudRate = baudrate;
        //LL_USART_Init(LWESP_USART, &usart_init);
        //LL_USART_Enable(LWESP_USART);
    }

    /* Create mbox and threads */
    if (usart_ll_mbox.tx_queue_id == TX_CLEAR_ID) {
        tx_queue_create(&usart_ll_mbox, "lwesp_ll_queue", sizeof(void *) / sizeof(ULONG), usart_ll_mbox_mem, sizeof(usart_ll_mbox_mem));
    }
    if (read_thread.tx_thread_id == TX_CLEAR_ID) {
        tx_thread_create(&read_thread, "lwesp_read_thread", prv_read_thread_entry, 0,
                read_thread_stack, sizeof(read_thread_stack),
                TX_MAX_PRIORITIES / 2 - 1, TX_MAX_PRIORITIES / 2 - 1,
                TX_NO_TIME_SLICE, TX_AUTO_START);
    }
}

#if defined(LWESP_RX_PIN)

/**
 * \brief           Hardware reset callback
 * \param[in]       state: Set to `1` to enable reset, `0` to release
 */
static uint8_t
prv_reset_device(uint8_t state) {
    if (state) {                                /* Activate reset line */
        //LL_GPIO_ResetOutputPin(LWESP_RESET_PORT, LWESP_RESET_PIN);
    } else {
        //LL_GPIO_SetOutputPin(LWESP_RESET_PORT, LWESP_RESET_PIN);
    }
    return 1;
}

#endif /* defined(LWESP_RX_PIN) */

/**
 * \brief           Send data to ESP device over UART
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
prv_send_data(const void* data, size_t len) {
    const uint8_t* d = data;
    uint8_t use_dma = 1;

    /*
     * When in DMA TX mode, application writes 
     * TX data to ring buffer for which DMA certainly has access to.
     * 
     * As it is a non-blocking TX (we don't wait for finish),
     * writing to buffer is faster than writing over UART hence
     * we need to find a mechanism to be able to still write as much as fast,
     * if such event happens.
     * 
     * Writes to buffer are checked, and when no memory is available to write full data:
     * - Try to force transfer (if not already on-going)
     * - Yield thread and wait for next-time run
     * 
     * In the meantime, DMA will trigger TC complete interrupt
     * and clean-up used memory, ready for next transfers.
     * 
     * To avoid such complications, allocate > 1kB memory for buffer
     */
    if (use_dma) {
        size_t written = 0;
        do {
            written += lwrb_write(&tx_rb, &d[written], len - written);
            if (written < len) {
                prv_start_tx_transfer();
                tx_thread_relinquish();
            }
        } while (written < len);
        prv_start_tx_transfer();
    } else {   
        for (size_t i = 0; i < len; ++i, ++d) {
            LL_USART_TransmitData8(LWESP_USART, *d);
            while (!LL_USART_IsActiveFlag_TXE(LWESP_USART)) {}
        }
    }
    return len;
}

/**
 * \brief           Callback function called from initialization process
 */
lwespr_t
lwesp_ll_init(lwesp_ll_t* ll) {
    if (!initialized) {
        ll->send_fn = prv_send_data;            /* Set callback function to send data */
#if defined(LWESP_RX_PIN)
        ll->reset_fn = prv_reset_device;        /* Set callback for hardware reset */
#endif /* defined(LWESP_RX_PIN) */

        /* Initialize buffer for TX */
        tx_len = 0;
        lwrb_init(&tx_rb, tx_rb_data, sizeof(tx_rb_data));
    }
    prv_configure_uart(ll->uart.baudrate);      /* Initialize UART for communication */
    initialized = 1;
    return lwespOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 */
lwespr_t
lwesp_ll_deinit(lwesp_ll_t* ll) {
    LL_USART_Disable(LWESP_USART);
    tx_queue_delete(&usart_ll_mbox);
    tx_thread_delete(&read_thread);

    initialized = 0;
    LWESP_UNUSED(ll);
    return lwespOK;
}

/**
 * \brief           UART global interrupt handler
 */
void
LWESP_USART_IRQ_HANDLER(void) {
    /* Clear all potential flags */
    LL_USART_ClearFlag_IDLE(LWESP_USART);
    LL_USART_ClearFlag_PE(LWESP_USART);
    LL_USART_ClearFlag_FE(LWESP_USART);
    LL_USART_ClearFlag_ORE(LWESP_USART);
    LL_USART_ClearFlag_NE(LWESP_USART);

    /* Write message to thread to wake-it up */
    if (usart_ll_mbox.tx_queue_id != TX_CLEAR_ID) {
        void* d = (void*)1;
        tx_queue_send(&usart_ll_mbox, &d, TX_NO_WAIT);
    }
}

/**
 * \brief           UART DMA RX stream handler
 */
void
LWESP_USART_DMA_RX_IRQ_HANDLER(void) {
    LWESP_USART_DMA_RX_CLEAR_TC;
    LWESP_USART_DMA_RX_CLEAR_HT;
    LWESP_USART_DMA_RX_CLEAR_TE;

    /* Write message to thread to wake-it up */
    if (usart_ll_mbox.tx_queue_id != TX_CLEAR_ID) {
        void* d = (void*)1;
        tx_queue_send(&usart_ll_mbox, &d, TX_NO_WAIT);
    }
}

/**
 * \brief           UART DMA RX stream handler
 */
void
LWESP_USART_DMA_TX_IRQ_HANDLER(void) {
    /* React on TC event only */
    if (LL_DMA_IsEnabledIT_TC(LWESP_USART_DMA_TX, LWESP_USART_DMA_TX_STREAM) && LWESP_USART_DMA_TX_IS_TC) {
        LWESP_USART_DMA_TX_CLEAR_TC;

        lwrb_skip(&tx_rb, tx_len);
        tx_len = 0;
        prv_start_tx_transfer();
    }
}

#endif /* !__DOXYGEN__ */
