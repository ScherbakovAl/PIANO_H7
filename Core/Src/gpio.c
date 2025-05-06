/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOD);

  /**/
  LL_GPIO_ResetOutputPin(GPIOE, off_Pin|LED_Pin|offE4_Pin|offE5_Pin
                          |offE6_Pin|offE8_Pin|offE9_Pin|offE10_Pin
                          |offE12_Pin|offE13_Pin|offE14_Pin|offE15_Pin
                          |offE0_Pin|LCD_CS_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOC, offC13_Pin|offC14_Pin|offC15_Pin|offC0_Pin
                          |offC1_Pin|offC2_Pin|offC3_Pin|offC4_Pin
                          |offC5_Pin|offC6_Pin|pin_test_2_Pin|offC8_Pin
                          |pin_test_1_Pin|offC12_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOA, offA2_Pin|offA3_Pin|offA4_Pin|offA5_Pin
                          |offA6_Pin|offA7_Pin|offA8_Pin|offA9_Pin
                          |offA10_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOB, offB0_Pin|offB1_Pin|offB2_Pin|offB10_Pin
                          |offB11_Pin|offB12_Pin|pin_test_8_Pin|pin_test_7_Pin
                          |offB4_Pin|offB6_Pin|offB8_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOD, offD8_Pin|pin_test_6_Pin|offD10_Pin|pin_test_5_Pin
                          |offD12_Pin|pin_test_4_Pin|offD14_Pin|pin_test_3_Pin
                          |CTP_RST_Pin|offD1_Pin|offD2_Pin|offD3_Pin
                          |offD4_Pin|offD5_Pin|offD7_Pin);

  /**/
  LL_GPIO_SetOutputPin(LCD_LED_GPIO_Port, LCD_LED_Pin);

  /**/
  LL_GPIO_SetOutputPin(GPIOB, LCD_DC_Pin|LCD_RESET_Pin);

  /**/
  GPIO_InitStruct.Pin = off_Pin|LED_Pin|offE4_Pin|offE5_Pin
                          |offE6_Pin|offE8_Pin|offE9_Pin|offE10_Pin
                          |offE12_Pin|offE13_Pin|offE14_Pin|offE15_Pin
                          |offE0_Pin|LCD_CS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offC13_Pin|offC14_Pin|offC15_Pin|offC0_Pin
                          |offC1_Pin|offC2_Pin|offC3_Pin|offC4_Pin
                          |offC5_Pin|offC6_Pin|pin_test_2_Pin|offC8_Pin
                          |pin_test_1_Pin|offC12_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offA2_Pin|offA3_Pin|offA4_Pin|offA5_Pin
                          |offA6_Pin|offA7_Pin|offA8_Pin|offA9_Pin
                          |offA10_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB0_Pin|offB1_Pin|offB2_Pin|offB10_Pin
                          |offB11_Pin|offB12_Pin|pin_test_8_Pin|pin_test_7_Pin
                          |offB4_Pin|offB6_Pin|offB8_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offD8_Pin|pin_test_6_Pin|offD10_Pin|pin_test_5_Pin
                          |offD12_Pin|pin_test_4_Pin|offD14_Pin|pin_test_3_Pin
                          |offD1_Pin|offD2_Pin|offD3_Pin|offD4_Pin
                          |offD5_Pin|offD7_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = CTP_RST_Pin|LCD_LED_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_RESET_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(LCD_RESET_GPIO_Port, &GPIO_InitStruct);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE15);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_15;
  EXTI_InitStruct.Line_32_63 = LL_EXTI_LINE_NONE;
  EXTI_InitStruct.Line_64_95 = LL_EXTI_LINE_NONE;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  LL_GPIO_SetPinPull(CTP_INT_GPIO_Port, CTP_INT_Pin, LL_GPIO_PULL_UP);

  /**/
  LL_GPIO_SetPinMode(CTP_INT_GPIO_Port, CTP_INT_Pin, LL_GPIO_MODE_INPUT);

  /* EXTI interrupt init*/
  NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
