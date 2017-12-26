/**
 * \file            esp_ll.c
 * \brief           Low-level communication with ESP device
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
#include "esp/esp.h"
#include "esp/esp_input.h"
#include "system/esp_ll.h"

#if !__DOXYGEN__

#include "tm_stm32_usart.h"
#include "tm_stm32_disco.h"

#if defined(STM32F7xx)
#include "stm32f7xx_ll_usart.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_dma.h"
#else /* defined(STM32F769_DISCOVERY) */
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_dma.h"
#endif /* !defined(STM32F769_DISCOVERY) */

#if defined(STM32F769_DISCOVERY)
#define ESP_USART                   UART5
#define ESP_USART_CLK               __HAL_RCC_UART5_CLK_ENABLE
#define ESP_USART_IRQ               UART5_IRQn
#define ESP_USART_IRQHANDLER        UART5_IRQHandler

#define ESP_USART_TX_PORT_CLK       __HAL_RCC_GPIOC_CLK_ENABLE
#define ESP_USART_TX_PORT           GPIOC
#define ESP_USART_TX_PIN            LL_GPIO_PIN_12
#define ESP_USART_TX_PIN_AF         LL_GPIO_AF_8
#define ESP_USART_RX_PORT_CLK       __HAL_RCC_GPIOD_CLK_ENABLE
#define ESP_USART_RX_PORT           GPIOD
#define ESP_USART_RX_PIN            LL_GPIO_PIN_2
#define ESP_USART_RX_PIN_AF         LL_GPIO_AF_8
#define ESP_USART_RS_PORT_CLK       __HAL_RCC_GPIOD_CLK_ENABLE
#define ESP_USART_RS_PORT           GPIOD
#define ESP_USART_RS_PIN            LL_GPIO_PIN_2

#define ESP_USART_DMA               DMA1
#define ESP_USART_DMA_CLK           __HAL_RCC_DMA1_CLK_ENABLE
#define ESP_USART_DMA_RX_STREAM     LL_DMA_STREAM_0
#define ESP_USART_DMA_RX_CH         LL_DMA_CHANNEL_4
#define ESP_USART_DMA_RX_STREAM_IRQ DMA1_Stream0_IRQn
#define ESP_USART_DMA_RX_STREAM_IRQHANDLER  DMA1_Stream0_IRQHandler

#define IS_DMA_RX_STREAM_TC         LL_DMA_IsActiveFlag_TC0(ESP_USART_DMA)
#define IS_DMA_RX_STREAM_HT         LL_DMA_IsActiveFlag_HT0(ESP_USART_DMA)
#define DMA_RX_STREAM_CLEAR_TC      LL_DMA_ClearFlag_TC0(ESP_USART_DMA)
#define DMA_RX_STREAM_CLEAR_HT      LL_DMA_ClearFlag_HT0(ESP_USART_DMA)
#else /* defined(STM32F769_DISCOVERY) */
#define ESP_USART                   USART1
#define ESP_USART_CLK               __HAL_RCC_USART1_CLK_ENABLE
#define ESP_USART_IRQ               USART1_IRQn
#define ESP_USART_IRQHANDLER        USART1_IRQHandler

#define ESP_USART_TX_PORT_CLK       __HAL_RCC_GPIOA_CLK_ENABLE
#define ESP_USART_TX_PORT           GPIOA
#define ESP_USART_TX_PIN            LL_GPIO_PIN_9
#define ESP_USART_TX_PIN_AF         LL_GPIO_AF_7
#define ESP_USART_RX_PORT_CLK       __HAL_RCC_GPIOA_CLK_ENABLE
#define ESP_USART_RX_PORT           GPIOA
#define ESP_USART_RX_PIN            LL_GPIO_PIN_10
#define ESP_USART_RX_PIN_AF         LL_GPIO_AF_7
#define ESP_USART_RS_PORT_CLK       __HAL_RCC_GPIOA_CLK_ENABLE
#define ESP_USART_RS_PORT           GPIOA
#define ESP_USART_RS_PIN            LL_GPIO_PIN_0

#define ESP_USART_DMA               DMA2
#define ESP_USART_DMA_CLK           __HAL_RCC_DMA2_CLK_ENABLE
#define ESP_USART_DMA_RX_STREAM     LL_DMA_STREAM_5
#define ESP_USART_DMA_RX_CH         LL_DMA_CHANNEL_4
#define ESP_USART_DMA_RX_STREAM_IRQ DMA2_Stream5_IRQn
#define ESP_USART_DMA_RX_STREAM_IRQHANDLER  DMA2_Stream5_IRQHandler

#define IS_DMA_RX_STREAM_TC         LL_DMA_IsActiveFlag_TC5(ESP_USART_DMA)
#define IS_DMA_RX_STREAM_HT         LL_DMA_IsActiveFlag_HT5(ESP_USART_DMA)
#define DMA_RX_STREAM_CLEAR_TC      LL_DMA_ClearFlag_TC5(ESP_USART_DMA)
#define DMA_RX_STREAM_CLEAR_HT      LL_DMA_ClearFlag_HT5(ESP_USART_DMA)
#endif /* !defined(STM32F769_DISCOVERY) */

#define USART_USE_DMA               1

#if USART_USE_DMA
static uint8_t usart_mem[0x400];
uint16_t old_pos;
#endif /* USART_USE_DMA */

static uint8_t initialized;
static uint8_t is_running;

#if ESP_INPUT_USE_PROCESS
void usart_ll_thread(void const * arg);

osMessageQDef(usart_ll_mbox, 10, uint8_t);
osMessageQId usart_ll_mbox_id;
osThreadDef(usart_ll_thread, usart_ll_thread, osPriorityNormal, 0, 1024);
osThreadId usart_ll_thread_id;
#endif /* ESP_INPUT_USE_PROCESS */

/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static uint16_t
send_data(const void* data, uint16_t len) {
    TM_USART_Send(ESP_USART, data, len);        /* Send actual data via UART using blocking method */
    return len;
}

/**
 * \brief           Configure UART using DMA for receive in double buffer mode and IDLE line detection
 */
static void
configure_uart(uint32_t baudrate) {
    LL_USART_InitTypeDef usart_init;
    LL_GPIO_InitTypeDef gpio_init;
#if USART_USE_DMA
    LL_DMA_InitTypeDef dma_init;
#endif /* USART_USE_DMA */
    
    ESP_USART_CLK();              
    ESP_USART_TX_PORT_CLK();
    ESP_USART_RX_PORT_CLK();
    ESP_USART_RS_PORT_CLK();
#if USART_USE_DMA
    ESP_USART_DMA_CLK();
#endif /* USART_USE_DMA */
    
    if (!initialized) {
        /* Global pin configuration */
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull = LL_GPIO_PULL_UP;
        gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        
        /* Configure reset pin */
        gpio_init.Mode = LL_GPIO_MODE_OUTPUT;
        gpio_init.Pin = ESP_USART_RS_PIN;
        gpio_init.Pin = ESP_USART_RS_PIN;
        LL_GPIO_Init(ESP_USART_RS_PORT, &gpio_init);
        
        /* Configure GPIO pins */
        gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
        gpio_init.Alternate = ESP_USART_TX_PIN_AF;
        gpio_init.Pin = ESP_USART_TX_PIN;
        LL_GPIO_Init(ESP_USART_TX_PORT, &gpio_init);
        gpio_init.Alternate = ESP_USART_RX_PIN_AF;
        gpio_init.Pin = ESP_USART_RX_PIN;
        LL_GPIO_Init(ESP_USART_RX_PORT, &gpio_init);
    }
    
    /* Configure UART */
    if (!initialized) {
        LL_USART_DeInit(ESP_USART);
        usart_init.BaudRate = baudrate;
        usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
        usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
        usart_init.OverSampling = LL_USART_OVERSAMPLING_8;
        usart_init.Parity = LL_USART_PARITY_NONE;
        usart_init.StopBits = LL_USART_STOPBITS_1;
        usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
        LL_USART_Init(ESP_USART, &usart_init);
        LL_USART_Enable(ESP_USART);
    } else {
        LL_USART_Disable(ESP_USART);
        LL_USART_SetBaudRate(ESP_USART, 100000000, LL_USART_OVERSAMPLING_8, baudrate);
        LL_USART_Enable(ESP_USART);
    }

    
    
    
#if !USART_USE_DMA
    LL_USART_EnableIT_RXNE(ESP_USART);
#endif /* USART_USE_DMA */

#if USART_USE_DMA
    LL_USART_EnableDMAReq_RX(ESP_USART);
    LL_USART_EnableIT_IDLE(ESP_USART);
    
    /* Set DMA_InitStruct fields to default values */
    if (!initialized) {
        is_running = 0;
        LL_DMA_DeInit(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        dma_init.Channel = ESP_USART_DMA_RX_CH;
#if defined(STM32F769_DISCOVERY)
        dma_init.PeriphOrM2MSrcAddress = (uint32_t)&ESP_USART->RDR;
#else /* defined(STM32F769_DISCOVERY) */
        dma_init.PeriphOrM2MSrcAddress = (uint32_t)&ESP_USART->DR;
#endif /* !defined(STM32F769_DISCOVERY) */
        dma_init.MemoryOrM2MDstAddress = (uint32_t)usart_mem;
        dma_init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
        dma_init.Mode = LL_DMA_MODE_CIRCULAR;
        dma_init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
        dma_init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
        dma_init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
        dma_init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
        dma_init.NbData = sizeof(usart_mem);
        dma_init.Priority = LL_DMA_PRIORITY_LOW;
        dma_init.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
        dma_init.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_1_4;
        dma_init.MemBurst = LL_DMA_MBURST_SINGLE;
        dma_init.PeriphBurst = LL_DMA_PBURST_SINGLE;
        LL_DMA_Init(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM, &dma_init);
        LL_DMA_EnableIT_HT(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TC(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableStream(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        
        HAL_NVIC_SetPriority(ESP_USART_DMA_RX_STREAM_IRQ, 1, 0);
        HAL_NVIC_EnableIRQ(ESP_USART_DMA_RX_STREAM_IRQ);
        
        old_pos = 0;
        is_running = 1;
    }
#endif /* USART_USE_DMA */
    
    HAL_NVIC_SetPriority(ESP_USART_IRQ, 1, 1);
    HAL_NVIC_EnableIRQ(ESP_USART_IRQ);

#if ESP_INPUT_USE_PROCESS
    if (!usart_ll_thread_id) {
        usart_ll_thread_id = osThreadCreate(osThread(usart_ll_thread), NULL);
    }
    if (!usart_ll_mbox_id) {
        usart_ll_mbox_id = osMessageCreate(osMessageQ(usart_ll_mbox), NULL);
    }
#endif /* ESP_INPUT_USE_PROCESS */
    
    if (!initialized) {
        LL_GPIO_ResetOutputPin(ESP_USART_RS_PORT, ESP_USART_RS_PIN);
        osDelay(1);
        LL_GPIO_SetOutputPin(ESP_USART_RS_PORT, ESP_USART_RS_PIN);
        osDelay(200);
    }
}

/**
 * \brief           UART global interrupt handler
 */
void
ESP_USART_IRQHANDLER(void) {
#if USART_USE_DMA
    uint16_t pos;
    
    if (LL_USART_IsActiveFlag_IDLE(ESP_USART) && is_running) {
        LL_USART_ClearFlag_IDLE(ESP_USART);
#if ESP_INPUT_USE_PROCESS
        osMessagePut(usart_ll_mbox_id, 0, 0);
        (void)pos;
#else
        pos = sizeof(usart_mem) - LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM); /* Remaining data to receive */
        
        if ((pos - old_pos) > 0) {
            esp_input(&usart_mem[old_pos], pos - old_pos);  /* Send to stack */
        }
        old_pos = pos;
#endif /* !ESP_INPUT_USE_PROCESS */
    }
#else /* USART_USE_DMA */
    if (LL_USART_IsActiveFlag_RXNE(ESP_USART)) {
        uint8_t val = LL_USART_ReceiveData8(ESP_USART);
        esp_input(&val, 1);
    }
#endif /* !USART_USE_DMA */
    LL_USART_ClearFlag_ORE(ESP_USART);
}

#if USART_USE_DMA
/**
 * \brief           UART DMA stream handler
 */
void
ESP_USART_DMA_RX_STREAM_IRQHANDLER(void) {
#if !ESP_INPUT_USE_PROCESS
    uint16_t pos;
#endif /* !ESP_INPUT_USE_PROCESS */
    
    if (!is_running) {                          /* Ignore if not running */
        DMA_RX_STREAM_CLEAR_TC;
        DMA_RX_STREAM_CLEAR_HT;
    }
#if ESP_INPUT_USE_PROCESS
    DMA_RX_STREAM_CLEAR_TC;                     /* Clear transfer complete interrupt */
    DMA_RX_STREAM_CLEAR_HT;                     /* Clear half-transfer interrupt */
    osMessagePut(usart_ll_mbox_id, 0, 0);       /* Notify thread only */
#else
    if (IS_DMA_RX_STREAM_TC) {
        pos = 0;
    } else {
        pos = LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM); /* Remaining data to receive */
    }
    pos = sizeof(usart_mem) - pos;              /* Current position in receive buffer array */
    
    if (IS_DMA_RX_STREAM_TC) {
        DMA_RX_STREAM_CLEAR_TC;
        esp_input(&usart_mem[old_pos], pos - old_pos);  /* Send to stack */
        old_pos = 0;                            /* Set new position of new valid data */
    } else if (IS_DMA_RX_STREAM_HT) {
        DMA_RX_STREAM_CLEAR_HT;
        esp_input(&usart_mem[old_pos], pos - old_pos);  /* Send to stack */
        old_pos = pos;                          /* Set new position of new valid data */
    }
#endif /* !ESP_INPUT_USE_PROCESS */
}
#endif /* USART_USE_DMA */

#if ESP_INPUT_USE_PROCESS
/**
 * \brief           USART read thread
 */
void
usart_ll_thread(void const * arg) {
    osEvent evt;
    size_t pos;
    
    while (1) {
        evt = osMessageGet(usart_ll_mbox_id, osWaitForever);
        if (evt.status == osEventMessage) {     /* We have a valid message */
            /* Read data from buffer and process them */
            pos = LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM); /* Get current DMA position */
            pos = sizeof(usart_mem) - pos;      /* Get position in correct order */
            
            if (pos > old_pos) {                /* Are we in linear section? */
                esp_input_process(&usart_mem[old_pos], pos - old_pos);
                old_pos = pos;
            } else {                            /* We are in overflow section */
                esp_input_process(&usart_mem[old_pos], sizeof(usart_mem) - old_pos);
                old_pos = 0;
                esp_input_process(&usart_mem[old_pos], pos - old_pos);
                old_pos = pos;
            }
            if (old_pos == sizeof(usart_mem)) {
                old_pos = 0;
            }
        }
    }
}
#endif /* ESP_INPUT_USE_PROCESS */

/**
 * \brief           Callback function called from initialization process
 * \note            This function may be called multiple times if AT baudrate is changed from application
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \param[in]       baudrate: Baudrate to use on AT port
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_ll_init(esp_ll_t* ll, uint32_t baudrate) {
    static uint8_t memory[0x10000];
    esp_mem_region_t mem_regions[] = {
        {memory, sizeof(memory)}
    };
    
    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
    
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations */
    }

    configure_uart(baudrate);                   /* Initialize UART for communication */
    initialized = 1;
    return espOK;
}
#endif /* !__DOXYGEN__ */
