/**
 * \file            esp_ll_stm32f107_core.c
 * \brief           Low-level communication with ESP device for STM32F429ZI-Nucleo using DMA
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
 * Author:          Vladimir V. <askfind@ya.ru>
 * Version:         $_version_$
 */

/*
 * Default UART configuration is:
 *
 * UART:                USART3
 * STM32 TX (ESP RX):   GPIOD, GPIO_PIN_8
 * STM32 RX (ESP TX):   GPIOD, GPIO_PIN_9
 * RESET:               GPIOE, GPIO_PIN_11 // used
 * GPIO0:               GPIOE, GPIO_PIN_13 // no used
 * GPIO2:               GPIOE, GPIO_PIN_13 // no used
 * CH_PD:               GPIOE, GPIO_PIN_13 // no used
 *
 * USART_DMA:           DMA1 
 * USART_DMA_CHANNEL:   DMA_CHANNEL_3
 */
#ifndef ESP_LL_STM32F107_CORE_H
#define ESP_LL_STM32F107_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * \brief           Calculate length of statically allocated array
 */
#define ARRAY_LEN(x)            (sizeof(x) / sizeof((x)[0]))

/* USART */
#define ESP_USART                           USART3
#define ESP_USART_CLK                       LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3)
#define ESP_USART_IRQ                       USART3_IRQn
#define ESP_USART_IRQHANDLER                USART3_IRQHandler
#define ESP_USART_RDR_NAME                  DR

/* DMA settings */
#define ESP_USART_DMA                       DMA1
#define ESP_USART_DMA_CLK                   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)
#define ESP_USART_DMA_RX_CH                 LL_DMA_CHANNEL_3
#define ESP_USART_DMA_RX_IRQ                DMA1_Channel3_IRQn
#define ESP_USART_DMA_RX_IRQHANDLER         DMA1_Stream3_IRQHandler

/* DMA flags management */
#define ESP_USART_DMA_RX_IS_TC              LL_DMA_IsActiveFlag_TC5(ESP_USART_DMA)
#define ESP_USART_DMA_RX_IS_HT              LL_DMA_IsActiveFlag_HT5(ESP_USART_DMA)
#define ESP_USART_DMA_RX_CLEAR_TC           LL_DMA_ClearFlag_TC5(ESP_USART_DMA)
#define ESP_USART_DMA_RX_CLEAR_HT           LL_DMA_ClearFlag_HT5(ESP_USART_DMA)

/* USART TX PIN */
#define ESP_USART_TX_PORT_CLK               __HAL_RCC_GPIOD_CLK_ENABLE()
#define ESP_USART_TX_PORT                   GPIOD
#define ESP_USART_TX_PIN                    LL_GPIO_PIN_9
#define ESP_USART_TX_PIN_AF                 LL_GPIO_MODE_ALTERNATE

/* USART RX PIN */
#define ESP_USART_RX_PORT_CLK               __HAL_RCC_GPIOD_CLK_ENABLE()
#define ESP_USART_RX_PORT                   GPIOD
#define ESP_USART_RX_PIN                    LL_GPIO_PIN_8
#define ESP_USART_RX_PIN_AF                 LL_GPIO_MODE_ALTERNATE

/* RESET PIN */
#define ESP_RESET_PORT_CLK                  __HAL_RCC_GPIOE_CLK_ENABLE()
#define ESP_RESET_PORT                      GPIOE
#define ESP_RESET_PIN                       LL_GPIO_PIN_11

/* GPIO0 PIN */
#define ESP_GPIO0_PORT_CLK                 __HAL_RCC_GPIOE_CLK_ENABLE()
#define ESP_GPIO0_PORT                      GPIOE
#define ESP_GPIO0_PIN                       LL_GPIO_PIN_13

/* GPIO2 PIN */
#define ESP_GPIO2_PORT_CLK                  __HAL_RCC_GPIOE_CLK_ENABLE()
#define ESP_GPIO2_PORT                      GPIOE
#define ESP_GPIO2_PIN                       LL_GPIO_PIN_13

/* CH_PD PIN */
#define ESP_CH_PD_PORT_CLK                  __HAL_RCC_GPIOE_CLK_ENABLE()
#define ESP_CH_PD_PORT                      GPIOE
#define ESP_CH_PD_PIN                       LL_GPIO_PIN_13

#ifdef __cplusplus
}
#endif

#endif    /*  ESP_LL_STM32F107_CORE_H */
