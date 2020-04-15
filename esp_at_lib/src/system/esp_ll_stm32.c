/**
 * \file            esp_ll_stm32.c
 * \brief           Generic STM32 driver, included in various STM32 driver variants
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
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
 * Version:         $_version_$
 */

/*
 * How it works
 *
 * On first call to \ref esp_ll_init, new thread is created and processed in usart_ll_thread function.
 * USART is configured in RX DMA mode and any incoming bytes are processed inside thread function.
 * DMA and USART implement interrupt handlers to notify main thread about new data ready to send to upper layer.
 *
 * More about UART + RX DMA: https://github.com/MaJerle/stm32-usart-dma-rx-tx
 *
 * \ref ESP_CFG_INPUT_USE_PROCESS must be enabled in `esp_config.h` to use this driver.
 */
#include "main.h"
#include "cmsis_os.h"

#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"
#include "system/esp_ll.h"

#include "esp_ll_stm32f107_core.c"

static size_t send_data(const void* data, size_t len);

#if !__DOXYGEN__

#if !ESP_CFG_INPUT_USE_PROCESS
#error "ESP_CFG_INPUT_USE_PROCESS must be enabled in `esp_config.h` to use this driver."
#endif /* ESP_CFG_INPUT_USE_PROCESS */

#if !defined(ESP_USART_DMA_RX_BUFF_SIZE)
#define ESP_USART_DMA_RX_BUFF_SIZE      0x1000
#endif /* !defined(ESP_USART_DMA_RX_BUFF_SIZE) */

#if !defined(ESP_MEM_SIZE)
#define ESP_MEM_SIZE                    0x1000
#endif /* !defined(ESP_MEM_SIZE) */

#if !defined(ESP_USART_RDR_NAME)
#define ESP_USART_RDR_NAME              RDR
#endif /* !defined(ESP_USART_RDR_NAME) */

/* USART memory */
static uint8_t      usart_mem[ESP_USART_DMA_RX_BUFF_SIZE];
static uint8_t      is_running, initialized;
static size_t       old_pos;

/* USART thread */
static void usart_ll_thread(void* arg);
static osThreadId_t usart_ll_thread_id;

/* Message queue */
static osMessageQueueId_t usart_ll_mbox_id;

/**
 * \brief           USART data processing
 */
static void
usart_ll_thread(void* arg) {
    size_t pos;

    ESP_UNUSED(arg);
    
    while (1) {
        void* d;        
        
        /* Wait for the event message from DMA or USART */
        osMessageQueueGet(usart_ll_mbox_id, &d, NULL, osWaitForever);        

        /* Read data */
#if defined(ESP_USART_DMA_RX_STREAM)
        pos = sizeof(usart_mem) - LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
#else
        pos = sizeof(usart_mem) - LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
#endif /* defined(ESP_USART_DMA_RX_STREAM) */
        if (pos != old_pos && is_running) {
            if (pos > old_pos) {
                esp_input_process(&usart_mem[old_pos], pos - old_pos);
            } else {
                esp_input_process(&usart_mem[old_pos], sizeof(usart_mem) - old_pos);
                if (pos > 0) {
                    esp_input_process(&usart_mem[0], pos);
                }
            }
            old_pos = pos;
            if (old_pos == sizeof(usart_mem)) {
                old_pos = 0;
            }
        }
    }
}

/**
 * \brief           Configure UART using DMA for receive in double buffer mode and IDLE line detection
 */
static void
configure_uart(uint32_t baudrate) {

if (!initialized) {
        
    usart_init(baudrate);

    /* Create mbox and start thread */
    if (usart_ll_mbox_id == NULL) {
        usart_ll_mbox_id = osMessageQueueNew(10, sizeof(void *), NULL);        
    }
    if (usart_ll_thread_id == NULL) {
        const osThreadAttr_t attr = {
            .stack_size = 1024
        };
        usart_ll_thread_id = osThreadNew(usart_ll_thread, usart_ll_mbox_id, &attr);
    }
}
else {    
        osDelay(10);
        LL_USART_Disable(ESP_USART);                
        usart_init(baudrate);      
        LL_USART_Enable(ESP_USART);
    }
}

#if defined(ESP_RESET_PIN)
/**
 * \brief           Hardware reset callback
 */
static uint8_t
reset_device(uint8_t state) {
    if (state) {                                /* Activate reset line */
        LL_GPIO_ResetOutputPin(ESP_RESET_PORT, ESP_RESET_PIN);
    } else {
        LL_GPIO_SetOutputPin(ESP_RESET_PORT, ESP_RESET_PIN);
    }
    return 1;
}
#endif /* defined(ESP_RESET_PIN) */

/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
send_data(const void* data, size_t len) {
    const uint8_t* d = data;  
    
    for (size_t i = 0; i < len; ++i, ++d) {                
        LL_USART_TransmitData8(ESP_USART, *d);
        while (!LL_USART_IsActiveFlag_TXE(ESP_USART)) {}
    }
    return len;
}

/**
 * \brief           Callback function called from initialization process
 */
espr_t
esp_ll_init(esp_ll_t* ll) {

#if !ESP_CFG_MEM_CUSTOM
    static uint8_t memory[ESP_MEM_SIZE];
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };

    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations */
    }
#endif /* !ESP_CFG_MEM_CUSTOM */
    
    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
#if defined(ESP_RESET_PIN)
        ll->reset_fn = reset_device;            /* Set callback for hardware reset */
#endif /* defined(ESP_RESET_PIN) */
    }

    configure_uart(ll->uart.baudrate);          /* Initialize UART for communication */
    initialized = 1;
    return espOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 */
espr_t
esp_ll_deinit(esp_ll_t* ll) {
    if (usart_ll_mbox_id != NULL) {
        osMessageQueueId_t tmp = usart_ll_mbox_id;
        usart_ll_mbox_id = NULL;
        osMessageQueueDelete(tmp);
    }
    if (usart_ll_thread_id != NULL) {
        osThreadId_t tmp = usart_ll_thread_id;
        usart_ll_thread_id = NULL;
        osThreadTerminate(tmp);
    }
    initialized = 0;
    ESP_UNUSED(ll);
    return espOK;
}

/**
 * \brief           UART global interrupt handler
 */
void
ESP_USART_IRQHANDLER(void) {

    LL_USART_ClearFlag_IDLE(ESP_USART);
    LL_USART_ClearFlag_PE(ESP_USART);
    LL_USART_ClearFlag_FE(ESP_USART);
    LL_USART_ClearFlag_ORE(ESP_USART);
    LL_USART_ClearFlag_NE(ESP_USART);

    if (usart_ll_mbox_id != NULL) {
        void* d = (void *)1;
        osMessageQueuePut(usart_ll_mbox_id, &d, 0, 0);
    }

}

/**
 * \brief           UART DMA stream/channel handler
 */
void
ESP_USART_DMA_RX_IRQHANDLER(void) {

    ESP_USART_DMA_RX_CLEAR_TC;
    ESP_USART_DMA_RX_CLEAR_HT;

    if (usart_ll_mbox_id != NULL) {
        void* d = (void *)1;
        
        osMessageQueuePut(usart_ll_mbox_id, &d, 0, 0);
    }

}


// ---------------- ESP INIT LL USART DMA1 ---------------- 

/**
 * \brief           USART3 Initialization Function
 */
void
usart_init( uint32_t baudrate ) {
    
    LL_USART_InitTypeDef USART_InitStruct;
    LL_GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);    
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOD);

    /*
     * USART1 GPIO Configuration
     *
     * PD8  ------> USART3_TX
     * PD9  ------> USART3_RX
     */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;    
    GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    LL_GPIO_AF_EnableRemap_USART3();

    /* Configure DMA */
    is_running = 0;
    
    LL_DMA_DeInit(ESP_USART_DMA, ESP_USART_DMA_RX_CH);

    /* USART1 DMA Init */
        
    /* USART1_RX Init */
    LL_DMA_SetDataTransferDirection(ESP_USART_DMA, ESP_USART_DMA_RX_CH, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(ESP_USART_DMA, ESP_USART_DMA_RX_CH, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(ESP_USART_DMA, ESP_USART_DMA_RX_CH, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(ESP_USART_DMA, ESP_USART_DMA_RX_CH, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(ESP_USART_DMA, ESP_USART_DMA_RX_CH, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(ESP_USART_DMA, ESP_USART_DMA_RX_CH, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(ESP_USART_DMA, ESP_USART_DMA_RX_CH, LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_SetPeriphAddress(ESP_USART_DMA, ESP_USART_DMA_RX_CH, (uint32_t)&ESP_USART->ESP_USART_RDR_NAME);
    LL_DMA_SetMemoryAddress(ESP_USART_DMA, ESP_USART_DMA_RX_CH, (uint32_t)usart_mem);
    LL_DMA_SetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_CH, ARRAY_LEN(usart_mem));

    /* Enable HT & TC interrupts */
    LL_DMA_EnableIT_HT(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
    LL_DMA_EnableIT_TC(ESP_USART_DMA, ESP_USART_DMA_RX_CH);

    /* Init with LL driver */
    /* DMA controller clock enable */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    /* DMA1_Channel3_IRQn interrupt configuration */
    NVIC_SetPriority(ESP_USART_DMA_RX_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(ESP_USART_DMA_RX_IRQ);

    /* USART configuration */
    USART_InitStruct.BaudRate = baudrate;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    LL_USART_Init(ESP_USART, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(ESP_USART);
    LL_USART_EnableDMAReq_RX(ESP_USART);

    /* USART interrupt */
    NVIC_SetPriority(ESP_USART_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(ESP_USART_IRQ);

    old_pos = 0;
    is_running = 1;

    /* Enable USART and DMA */
    LL_DMA_EnableChannel(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
    LL_USART_Enable(ESP_USART);

}

#endif /* !__DOXYGEN__ */
