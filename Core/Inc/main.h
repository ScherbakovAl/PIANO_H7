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
#define off_Pin LL_GPIO_PIN_2
#define off_GPIO_Port GPIOE
#define LED_Pin LL_GPIO_PIN_3
#define LED_GPIO_Port GPIOE
#define offE4_Pin LL_GPIO_PIN_4
#define offE4_GPIO_Port GPIOE
#define offE5_Pin LL_GPIO_PIN_5
#define offE5_GPIO_Port GPIOE
#define offE6_Pin LL_GPIO_PIN_6
#define offE6_GPIO_Port GPIOE
#define offC13_Pin LL_GPIO_PIN_13
#define offC13_GPIO_Port GPIOC
#define offC14_Pin LL_GPIO_PIN_14
#define offC14_GPIO_Port GPIOC
#define offC15_Pin LL_GPIO_PIN_15
#define offC15_GPIO_Port GPIOC
#define offC0_Pin LL_GPIO_PIN_0
#define offC0_GPIO_Port GPIOC
#define offC1_Pin LL_GPIO_PIN_1
#define offC1_GPIO_Port GPIOC
#define offC2_Pin LL_GPIO_PIN_2
#define offC2_GPIO_Port GPIOC
#define offC3_Pin LL_GPIO_PIN_3
#define offC3_GPIO_Port GPIOC
#define offA2_Pin LL_GPIO_PIN_2
#define offA2_GPIO_Port GPIOA
#define offA3_Pin LL_GPIO_PIN_3
#define offA3_GPIO_Port GPIOA
#define offA4_Pin LL_GPIO_PIN_4
#define offA4_GPIO_Port GPIOA
#define offA5_Pin LL_GPIO_PIN_5
#define offA5_GPIO_Port GPIOA
#define offA6_Pin LL_GPIO_PIN_6
#define offA6_GPIO_Port GPIOA
#define offA7_Pin LL_GPIO_PIN_7
#define offA7_GPIO_Port GPIOA
#define offC4_Pin LL_GPIO_PIN_4
#define offC4_GPIO_Port GPIOC
#define offC5_Pin LL_GPIO_PIN_5
#define offC5_GPIO_Port GPIOC
#define offB0_Pin LL_GPIO_PIN_0
#define offB0_GPIO_Port GPIOB
#define offB1_Pin LL_GPIO_PIN_1
#define offB1_GPIO_Port GPIOB
#define offB2_Pin LL_GPIO_PIN_2
#define offB2_GPIO_Port GPIOB
#define offE8_Pin LL_GPIO_PIN_8
#define offE8_GPIO_Port GPIOE
#define offE9_Pin LL_GPIO_PIN_9
#define offE9_GPIO_Port GPIOE
#define offE10_Pin LL_GPIO_PIN_10
#define offE10_GPIO_Port GPIOE
#define offE12_Pin LL_GPIO_PIN_12
#define offE12_GPIO_Port GPIOE
#define offE13_Pin LL_GPIO_PIN_13
#define offE13_GPIO_Port GPIOE
#define offE14_Pin LL_GPIO_PIN_14
#define offE14_GPIO_Port GPIOE
#define offE15_Pin LL_GPIO_PIN_15
#define offE15_GPIO_Port GPIOE
#define offB10_Pin LL_GPIO_PIN_10
#define offB10_GPIO_Port GPIOB
#define offB11_Pin LL_GPIO_PIN_11
#define offB11_GPIO_Port GPIOB
#define offB12_Pin LL_GPIO_PIN_12
#define offB12_GPIO_Port GPIOB
#define pin_test_8_Pin LL_GPIO_PIN_13
#define pin_test_8_GPIO_Port GPIOB
#define pin_test_7_Pin LL_GPIO_PIN_15
#define pin_test_7_GPIO_Port GPIOB
#define offD8_Pin LL_GPIO_PIN_8
#define offD8_GPIO_Port GPIOD
#define pin_test_6_Pin LL_GPIO_PIN_9
#define pin_test_6_GPIO_Port GPIOD
#define offD10_Pin LL_GPIO_PIN_10
#define offD10_GPIO_Port GPIOD
#define pin_test_5_Pin LL_GPIO_PIN_11
#define pin_test_5_GPIO_Port GPIOD
#define offD12_Pin LL_GPIO_PIN_12
#define offD12_GPIO_Port GPIOD
#define pin_test_4_Pin LL_GPIO_PIN_13
#define pin_test_4_GPIO_Port GPIOD
#define offD14_Pin LL_GPIO_PIN_14
#define offD14_GPIO_Port GPIOD
#define pin_test_3_Pin LL_GPIO_PIN_15
#define pin_test_3_GPIO_Port GPIOD
#define offC6_Pin LL_GPIO_PIN_6
#define offC6_GPIO_Port GPIOC
#define pin_test_2_Pin LL_GPIO_PIN_7
#define pin_test_2_GPIO_Port GPIOC
#define offC8_Pin LL_GPIO_PIN_8
#define offC8_GPIO_Port GPIOC
#define pin_test_1_Pin LL_GPIO_PIN_9
#define pin_test_1_GPIO_Port GPIOC
#define offA8_Pin LL_GPIO_PIN_8
#define offA8_GPIO_Port GPIOA
#define offA9_Pin LL_GPIO_PIN_9
#define offA9_GPIO_Port GPIOA
#define offA10_Pin LL_GPIO_PIN_10
#define offA10_GPIO_Port GPIOA
#define CTP_INT_Pin LL_GPIO_PIN_15
#define CTP_INT_GPIO_Port GPIOA
#define CTP_INT_EXTI_IRQn EXTI15_10_IRQn
#define CTP_I2C5_SDA_Pin LL_GPIO_PIN_10
#define CTP_I2C5_SDA_GPIO_Port GPIOC
#define CTP_I2C5_SCL_Pin LL_GPIO_PIN_11
#define CTP_I2C5_SCL_GPIO_Port GPIOC
#define offC12_Pin LL_GPIO_PIN_12
#define offC12_GPIO_Port GPIOC
#define CTP_RST_Pin LL_GPIO_PIN_0
#define CTP_RST_GPIO_Port GPIOD
#define offD1_Pin LL_GPIO_PIN_1
#define offD1_GPIO_Port GPIOD
#define offD2_Pin LL_GPIO_PIN_2
#define offD2_GPIO_Port GPIOD
#define offD3_Pin LL_GPIO_PIN_3
#define offD3_GPIO_Port GPIOD
#define offD4_Pin LL_GPIO_PIN_4
#define offD4_GPIO_Port GPIOD
#define offD5_Pin LL_GPIO_PIN_5
#define offD5_GPIO_Port GPIOD
#define LCD_LED_Pin LL_GPIO_PIN_6
#define LCD_LED_GPIO_Port GPIOD
#define offD7_Pin LL_GPIO_PIN_7
#define offD7_GPIO_Port GPIOD
#define LCD_SCK_Pin LL_GPIO_PIN_3
#define LCD_SCK_GPIO_Port GPIOB
#define offB4_Pin LL_GPIO_PIN_4
#define offB4_GPIO_Port GPIOB
#define LCD_SDI_Pin LL_GPIO_PIN_5
#define LCD_SDI_GPIO_Port GPIOB
#define offB6_Pin LL_GPIO_PIN_6
#define offB6_GPIO_Port GPIOB
#define LCD_DC_Pin LL_GPIO_PIN_7
#define LCD_DC_GPIO_Port GPIOB
#define offB8_Pin LL_GPIO_PIN_8
#define offB8_GPIO_Port GPIOB
#define LCD_RESET_Pin LL_GPIO_PIN_9
#define LCD_RESET_GPIO_Port GPIOB
#define offE0_Pin LL_GPIO_PIN_0
#define offE0_GPIO_Port GPIOE
#define LCD_CS_Pin LL_GPIO_PIN_1
#define LCD_CS_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
