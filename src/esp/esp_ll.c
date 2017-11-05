/**
 * \file            esp_int.c
 * \brief           Internal function shared between modules
 */

/*
 * Contains list of functions to parse different input strings
 *
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
#include "esp_ll.h"
#include "esp.h"
#include "esp_input.h"

#include "tm_stm32_usart.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_dma.h"

static uint8_t usart_mem[0x200];

#define ESP_USART                   USART1
#define ESP_USART_CLK               __HAL_RCC_USART1_CLK_ENABLE
#define ESP_USART_IRQ               USART1_IRQn
#define ESP_USART_IRQHANDLER        USART1_IRQHandler

#define ESP_USART_TX_PORT_CLK       __HAL_RCC_GPIOA_CLK_ENABLE
#define ESP_USART_TX_PORT           GPIOA
#define ESP_USART_TX_PIN            LL_GPIO_PIN_9
#define ESP_USART_RX_PORT_CLK       __HAL_RCC_GPIOA_CLK_ENABLE
#define ESP_USART_RX_PORT           GPIOA
#define ESP_USART_RX_PIN            LL_GPIO_PIN_10

#define ESP_USART_DMA               DMA2
#define ESP_USART_DMA_CLK           __HAL_RCC_DMA2_CLK_ENABLE
#define ESP_USART_DMA_RX_STREAM     LL_DMA_STREAM_5
#define ESP_USART_DMA_RX_CH         LL_DMA_CHANNEL_4
#define ESP_USART_DMA_RX_STREAM_IRQ DMA2_Stream5_IRQn
#define ESP_USART_DMA_RX_STREAM_IRQHANDLER  DMA2_Stream5_IRQHandler

#define IS_DMA_RX_STREAM_TC         LL_DMA_IsActiveFlag_TC5(ESP_USART_DMA)
#define IS_DMA_RX_STREAM_HT         LL_DMA_IsActiveFlag_HT5(ESP_USART_DMA)
#define DMA_RX_STREAM_CLEAR_TC      LL_DMA_ClearFlag_TC5(ESP_USART_DMA);
#define DMA_RX_STREAM_CLEAR_HT      LL_DMA_ClearFlag_HT5(ESP_USART_DMA);


/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static uint16_t
send_data(const uint8_t* data, uint16_t len) {
    TM_USART_Send(USART1, data, len);           /* Send actual data via UART using blocking method */
    return len;
}

/**
 * \brief           Configure UART using DMA for receive in double buffer mode and IDLE line detection
 */
static void
configure_uart(uint32_t baudrate) {
    LL_DMA_InitTypeDef dma_init;
    LL_USART_InitTypeDef usart_init;
    LL_GPIO_InitTypeDef gpio_init;
    
    ESP_USART_CLK();              
    ESP_USART_TX_PORT_CLK();
    ESP_USART_RX_PORT_CLK();
    ESP_USART_DMA_CLK();
    
    /* Configure GPIO pins */
    gpio_init.Alternate = LL_GPIO_AF_7;
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_UP;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    
    gpio_init.Pin = ESP_USART_TX_PIN;
    LL_GPIO_Init(ESP_USART_TX_PORT, &gpio_init);
    gpio_init.Pin = ESP_USART_RX_PIN;
    LL_GPIO_Init(ESP_USART_RX_PORT, &gpio_init);
    
    /* Configure UART */
    usart_init.BaudRate = baudrate;
    usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
    usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    usart_init.OverSampling = LL_USART_OVERSAMPLING_8;
    usart_init.Parity = LL_USART_PARITY_NONE;
    usart_init.StopBits = LL_USART_STOPBITS_1;
    usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
    LL_USART_Init(ESP_USART, &usart_init);
    
    LL_USART_Enable(ESP_USART);
    LL_USART_EnableDMAReq_RX(ESP_USART);
    LL_USART_EnableIT_IDLE(ESP_USART);
    
    /* Set DMA_InitStruct fields to default values */
    dma_init.Channel = ESP_USART_DMA_RX_CH;
    dma_init.PeriphOrM2MSrcAddress = (uint32_t)&ESP_USART->DR;
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
    
    HAL_NVIC_SetPriority(ESP_USART_IRQ, 1, 1);
    HAL_NVIC_EnableIRQ(ESP_USART_IRQ);
    
    HAL_NVIC_SetPriority(ESP_USART_DMA_RX_STREAM_IRQ, 1, 0);
    HAL_NVIC_EnableIRQ(ESP_USART_DMA_RX_STREAM_IRQ);
}

uint16_t old_pos;

/**
 * \brief           UART global interrupt handler
 */
void
ESP_USART_IRQHANDLER(void) {
    uint16_t ndtr, curr_pos, to_read, start_pos;
    
    if (LL_USART_IsActiveFlag_IDLE(ESP_USART)) {
        start_pos = old_pos;
        ndtr = LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);    /* Remaining data to receive */
        curr_pos = sizeof(usart_mem) - ndtr;    /* Current position in receive buffer array */
        
        LL_USART_ClearFlag_IDLE(ESP_USART);
        to_read = curr_pos - old_pos;           /* Number of bytes to read */
        old_pos += to_read;                     /* Set new position of new valid data */
        esp_input(&usart_mem[start_pos], to_read);  /* Send to stack */
    }
}

/**
 * \brief           UART DMA stream handler
 */
void
ESP_USART_DMA_RX_STREAM_IRQHANDLER(void) {
    uint16_t ndtr, curr_pos, to_read, start_pos;
    
    start_pos = old_pos;
    ndtr = LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);    /* Remaining data to receive */
    if (IS_DMA_RX_STREAM_TC && ndtr == sizeof(usart_mem)) {
        ndtr = 0;
    }
    curr_pos = sizeof(usart_mem) - ndtr;        /* Current position in receive buffer array */
    
    if (IS_DMA_RX_STREAM_TC) {
        DMA_RX_STREAM_CLEAR_TC;                 /* Clear flag */
        to_read = curr_pos - old_pos;           /* Number of bytes to read */
        old_pos = 0;                            /* Set new position of new valid data */
    } else if (IS_DMA_RX_STREAM_HT) {
        DMA_RX_STREAM_CLEAR_HT;
        to_read = curr_pos - old_pos;           /* Number of bytes to read */
        old_pos += to_read;                     /* Set new position of new valid data */
    }
    esp_input(&usart_mem[start_pos], to_read);  /* Send to stack */
}

/**
 * \brief           Callback function called from initialization process
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_ll_init(esp_ll_t* ll, uint32_t baudrate) {
    static uint8_t memory[0x1000];
    esp_mem_region_t mem_regions[] = {
        {memory, sizeof(memory)}
    };
    static uint8_t initialized = 0;
    
    if (!initialized) {
        ll->send = send_data;                   /* Set callback function to send data */
    
        esp_mem_assignmemory(mem_regions, 1);   /* Assign memory for allocations */
    }

    configure_uart(baudrate);                   /* Initialize UART for communication */
    initialized = 1;
    return espOK;
}
