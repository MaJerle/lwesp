/**
 * \file            esp_ll_stm32f769i_discovery.c
 * \brief           Low-level communication with ESP device for STM32F769I-Discovery using DMA
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
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */

/*
 * STM32F769I-Discovery has connector CN2, dedicated for ESP-01 module.
 *
 * UART configuration is:
 *
 * UART: UART5
 * STM32 TX:            GPIOC, GPIO_PIN_12
 * STM32 RX:            GPIOD, GPIO_PIN_2
 * RESET:               GPIOJ, GPIO_PIN_14
 *
 * UART5_DMA:           DMA1
 * UART5_DMA_STREAM:    DMA_STREAM_0
 * UART5_DMA_CHANNEL:   DMA_CHANNEL_4
 *
 * When LL init function is called for first time,
 * driver will create a new thread. Thread will periodically (together with DMA + USART interrupts)
 * check for new incoming data and in case we have something to process,
 * it will use direct processing method without copying data to ESP internal receive buffers.
 *
 * \ref ESP_CFG_INPUT_USE_PROCESS must be enabled in order to use this driver.
 */
#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"
#include "system/esp_ll.h"

#if !__DOXYGEN__
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_usart.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_rcc.h"

#define ESP_USART                           UART5
#define ESP_USART_CLK                       LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5)
#define ESP_USART_CLK_SOURCE                LL_RCC_UART5_CLKSOURCE
#define ESP_USART_IRQ                       UART5_IRQn
#define ESP_USART_IRQHANDLER                UART5_IRQHandler

#define ESP_USART_TX_PORT_CLK               LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC)
#define ESP_USART_TX_PORT                   GPIOC
#define ESP_USART_TX_PIN                    LL_GPIO_PIN_12
#define ESP_USART_TX_PIN_AF                 LL_GPIO_AF_8

#define ESP_USART_RX_PORT_CLK               LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD)
#define ESP_USART_RX_PORT                   GPIOD
#define ESP_USART_RX_PIN                    LL_GPIO_PIN_2
#define ESP_USART_RX_PIN_AF                 LL_GPIO_AF_8

#define ESP_USART_RS_PORT_CLK               LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOJ)
#define ESP_USART_RS_PORT                   GPIOJ
#define ESP_USART_RS_PIN                    LL_GPIO_PIN_14

#define ESP_USART_DMA                       DMA1
#define ESP_USART_DMA_CLK                   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)
#define ESP_USART_DMA_RX_STREAM             LL_DMA_STREAM_0
#define ESP_USART_DMA_RX_CH                 LL_DMA_CHANNEL_4
#define ESP_USART_DMA_RX_STREAM_IRQ         DMA1_Stream0_IRQn
#define ESP_USART_DMA_RX_STREAM_IRQHANDLER  DMA1_Stream0_IRQHandler

#define IS_DMA_RX_STREAM_TC                 LL_DMA_IsActiveFlag_TC0(ESP_USART_DMA)
#define IS_DMA_RX_STREAM_HT                 LL_DMA_IsActiveFlag_HT0(ESP_USART_DMA)
#define DMA_RX_STREAM_CLEAR_TC              LL_DMA_ClearFlag_TC0(ESP_USART_DMA)
#define DMA_RX_STREAM_CLEAR_HT              LL_DMA_ClearFlag_HT0(ESP_USART_DMA)

/* USART memory */
static uint32_t usart_mem[0x1000 / 4] __attribute__((at(0x20010000)));
static uint16_t old_pos;
static uint8_t  is_running, initialized;

static void usart_ll_thread(void const * arg);
osThreadDef(usart_ll_thread, usart_ll_thread, osPriorityNormal, 0, 1024);
osThreadId usart_ll_thread_id;

osMessageQDef(usart_ll_mbox, 10, uint8_t);
osMessageQId usart_ll_mbox_id;

/**
 * \brief           USART data processing
 */
static void
usart_ll_thread(void const * arg) {
    osEvent evt;
    size_t pos;

    while (1) {
        /*
         * Wait for the event message from:
         *  - DMA (half)transfer complete
         *  - IDLE line detection on UART line
         */
        evt = osMessageGet(usart_ll_mbox_id, osWaitForever);
        if (evt.status != osEventMessage) {
            continue;
        }

        /* Read data from buffer and process them */
        pos = LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM); /* Get current DMA position */
        pos = sizeof(usart_mem) - pos;          /* Get position in correct order */
        if (pos != old_pos && is_running) {
            /*
             * At this point, user may implement
             * RTS pin functionality to block ESP to
             * send more data until processing is finished
             */

            if (pos > old_pos) {                /* Are we in linear section? */
                /*
                 * In linear section, simply process difference between pointers
                 */
                esp_input_process(&((uint8_t *)usart_mem)[old_pos], pos - old_pos);  /* Process input data in linear buffer phase */
            } else {                            /* We are in overflow section */
                /*
                 * In overflow mode we have to process twice:
                 *  - Process data until end of buffer
                 *  - Process data until current position on top of buffer
                 */
                esp_input_process(&((uint8_t *)usart_mem)[old_pos], sizeof(usart_mem) - old_pos);
                esp_input_process(&((uint8_t *)usart_mem)[0], pos);
            }
            old_pos = pos;
            if (old_pos == sizeof(usart_mem)) {
                old_pos = 0;
            }

            /*
             * At this point, user may implement
             * RTS pin functionality to again allow ESP to
             * send more data until processing is finished
             */
        }
    }
}

/**
 * \brief           Configure UART using DMA for receive in double buffer mode and IDLE line detection
 */
void
configure_uart(uint32_t baudrate) {
    LL_USART_InitTypeDef usart_init;
    LL_GPIO_InitTypeDef gpio_init;
    LL_DMA_InitTypeDef dma_init;
    
    if (!initialized) {
        /* Enable clock */
        ESP_USART_CLK;              
        ESP_USART_TX_PORT_CLK;
        ESP_USART_RX_PORT_CLK;
        ESP_USART_RS_PORT_CLK;
        ESP_USART_DMA_CLK;
        
        /* Global pin configuration */
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull = LL_GPIO_PULL_UP;
        gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        
        /* Configure RESET pin */
        gpio_init.Mode = LL_GPIO_MODE_OUTPUT;
        gpio_init.Pin = ESP_USART_RS_PIN;
        LL_GPIO_Init(ESP_USART_RS_PORT, &gpio_init);
        
        /* Configure USART pins */
        gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
        
        gpio_init.Alternate = ESP_USART_TX_PIN_AF;
        gpio_init.Pin = ESP_USART_TX_PIN;
        LL_GPIO_Init(ESP_USART_TX_PORT, &gpio_init);
        gpio_init.Alternate = ESP_USART_RX_PIN_AF;
        gpio_init.Pin = ESP_USART_RX_PIN;
        LL_GPIO_Init(ESP_USART_RX_PORT, &gpio_init);
        
        LL_USART_DeInit(ESP_USART);
        LL_USART_StructInit(&usart_init);
        usart_init.BaudRate = baudrate;
        usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
        usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
        usart_init.OverSampling = LL_USART_OVERSAMPLING_16;
        usart_init.Parity = LL_USART_PARITY_NONE;
        usart_init.StopBits = LL_USART_STOPBITS_1;
        usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
        LL_USART_Init(ESP_USART, &usart_init);
        LL_USART_Enable(ESP_USART);
    
        NVIC_SetPriority(ESP_USART_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x0F, 0x00));
        NVIC_EnableIRQ(ESP_USART_IRQ);
        
        /* Configure DMA */
        is_running = 0;
        LL_DMA_DeInit(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        dma_init.Channel = ESP_USART_DMA_RX_CH;
        dma_init.PeriphOrM2MSrcAddress = (uint32_t)&ESP_USART->RDR;
        dma_init.MemoryOrM2MDstAddress = (uint32_t)usart_mem;
        dma_init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
        dma_init.Mode = LL_DMA_MODE_CIRCULAR;
        dma_init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
        dma_init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
        dma_init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
        dma_init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
        dma_init.NbData = sizeof(usart_mem);
        dma_init.Priority = LL_DMA_PRIORITY_VERYHIGH;
        dma_init.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
        dma_init.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
        dma_init.MemBurst = LL_DMA_MBURST_INC4;
        dma_init.PeriphBurst = LL_DMA_PBURST_INC4;
        LL_DMA_Init(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM, &dma_init);
        LL_DMA_EnableIT_HT(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TC(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TE(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_FE(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_DME(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableStream(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        
        /* Enable DMA interrupts */
        NVIC_SetPriority(ESP_USART_DMA_RX_STREAM_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x0F, 0x00));
        NVIC_EnableIRQ(ESP_USART_DMA_RX_STREAM_IRQ);
        
        old_pos = 0;
        is_running = 1;
    } else {
        osDelay(10);
        LL_USART_Disable(ESP_USART);
        LL_USART_SetBaudRate(ESP_USART, LL_RCC_GetUARTClockFreq(ESP_USART_CLK_SOURCE), LL_USART_OVERSAMPLING_16, baudrate);
        LL_USART_Enable(ESP_USART);
    }
    
    /*
     * Configure USART RX DMA for data reception on AT port
     */
    LL_USART_EnableIT_IDLE(ESP_USART);
    LL_USART_EnableIT_PE(ESP_USART);
    LL_USART_EnableIT_ERROR(ESP_USART);
    LL_USART_EnableDMAReq_RX(ESP_USART);

    /* Create mbox and start thread */
    if (usart_ll_mbox_id == NULL) {
        usart_ll_mbox_id = osMessageCreate(osMessageQ(usart_ll_mbox), NULL);
    }
    if (usart_ll_thread_id == NULL) {
        usart_ll_thread_id = osThreadCreate(osThread(usart_ll_thread), usart_ll_mbox_id);
    }
    
    /*
     * Force ESP hardware reset
     * after initialization to make sure device is ready and
     * not in undefined state from previous AT usage
     */
    if (!initialized) {
        LL_GPIO_ResetOutputPin(ESP_USART_RS_PORT, ESP_USART_RS_PIN);
        osDelay(1);
        LL_GPIO_SetOutputPin(ESP_USART_RS_PORT, ESP_USART_RS_PIN);
        osDelay(200);
    }
}

/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static uint16_t
send_data(const void* data, uint16_t len) {
    uint16_t i;
    const uint8_t* d = data;
    
    for (i = 0; i < len; i++, d++) {
        LL_USART_TransmitData8(ESP_USART, *d);
        while (!LL_USART_IsActiveFlag_TXE(ESP_USART));
    }
    return len;
}

/**
 * \brief           Callback function called from initialization process
 * \note            This function may be called multiple times if AT baudrate is changed from application
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \param[in]       baudrate: Baudrate to use on AT port
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
    static uint8_t memory[0x10000];
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };
    
    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
    
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations */
    }

    configure_uart(ll->uart.baudrate);          /* Initialize UART for communication */
    initialized = 1;
    return espOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ll_deinit(esp_ll_t* ll) {
    if (usart_ll_mbox_id != NULL) {
        osMessageDelete(usart_ll_mbox_id);      /* Delete message queue */
        usart_ll_mbox_id = NULL;
    }
    if (usart_ll_thread_id != NULL) {
        osThreadTerminate(usart_ll_thread_id);  /* Terminate thread */
        usart_ll_thread_id = NULL;
    }
    initialized = 0;                            /* Clear initialized flag */
    return espOK;
}

/**
 * \brief           UART global interrupt handler
 */
void
ESP_USART_IRQHANDLER(void) {
    if (LL_USART_IsActiveFlag_IDLE(ESP_USART)) {/*  */
        LL_USART_ClearFlag_IDLE(ESP_USART);     /* Clear IDLE flag */
        if (usart_ll_mbox_id != NULL) {
            osMessagePut(usart_ll_mbox_id, 0, 0);   /* Send IDLE event to queue */
        }
    }

    LL_USART_ClearFlag_PE(ESP_USART);
    LL_USART_ClearFlag_FE(ESP_USART);
    LL_USART_ClearFlag_ORE(ESP_USART);
    LL_USART_ClearFlag_NE(ESP_USART);
}

/**
 * \brief           UART DMA stream handler
 */
void
ESP_USART_DMA_RX_STREAM_IRQHANDLER(void) {
    DMA_RX_STREAM_CLEAR_TC;                     /* Clear transfer complete interrupt */
    DMA_RX_STREAM_CLEAR_HT;                     /* Clear half-transfer interrupt */
    if (usart_ll_mbox_id != NULL) {
        osMessagePut(usart_ll_mbox_id, 0, 0);   /* Send event to queue */
    }
}

#endif /* !__DOXYGEN__ */
