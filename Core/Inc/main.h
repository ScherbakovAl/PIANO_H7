/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_crs.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TIM1_trigger_Pin LL_GPIO_PIN_7
#define TIM1_trigger_GPIO_Port GPIOE
#define TIM1_CLK_Pin LL_GPIO_PIN_11
#define TIM1_CLK_GPIO_Port GPIOE
#define CTP_INT_Pin LL_GPIO_PIN_15
#define CTP_INT_GPIO_Port GPIOA
#define CTP_INT_EXTI_IRQn EXTI15_10_IRQn
#define CTP_I2C5_SDA_Pin LL_GPIO_PIN_10
#define CTP_I2C5_SDA_GPIO_Port GPIOC
#define CTP_I2C5_SCL_Pin LL_GPIO_PIN_11
#define CTP_I2C5_SCL_GPIO_Port GPIOC
#define CTP_RST_Pin LL_GPIO_PIN_0
#define CTP_RST_GPIO_Port GPIOD
#define LCD_LED_Pin LL_GPIO_PIN_6
#define LCD_LED_GPIO_Port GPIOD
#define LCD_SCK_Pin LL_GPIO_PIN_3
#define LCD_SCK_GPIO_Port GPIOB
#define LCD_SDI_Pin LL_GPIO_PIN_5
#define LCD_SDI_GPIO_Port GPIOB
#define LCD_DC_Pin LL_GPIO_PIN_7
#define LCD_DC_GPIO_Port GPIOB
#define LCD_RESET_Pin LL_GPIO_PIN_9
#define LCD_RESET_GPIO_Port GPIOB
#define LCD_CS_Pin LL_GPIO_PIN_1
#define LCD_CS_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
