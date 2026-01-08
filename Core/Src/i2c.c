/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
#include "i2c.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* I2C5 init function */
void MX_I2C5_Init(void)
{

  /* USER CODE BEGIN I2C5_Init 0 */

  /* USER CODE END I2C5_Init 0 */

  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetI2CClockSource(LL_RCC_I2C123_CLKSOURCE_PCLK1);

  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
  /**I2C5 GPIO Configuration
  PC10   ------> I2C5_SDA
  PC11   ------> I2C5_SCL
  */
  GPIO_InitStruct.Pin = CTP_I2C5_SDA_Pin|CTP_I2C5_SCL_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C5);

  /* I2C5 DMA Init */

  /* I2C5_RX Init */
  LL_DMA_SetPeriphRequest(DMA2, LL_DMA_STREAM_3, LL_DMAMUX1_REQ_I2C5_RX);

  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_3, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA2, LL_DMA_STREAM_3, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_3, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_3, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_3, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_3, LL_DMA_MDATAALIGN_BYTE);

  LL_DMA_DisableFifoMode(DMA2, LL_DMA_STREAM_3);

  /* USER CODE BEGIN I2C5_Init 1 */

  /* USER CODE END I2C5_Init 1 */

  /** I2C Initialization
  */
  LL_I2C_EnableAutoEndMode(I2C5);
  LL_I2C_SetOwnAddress2(I2C5, 0, LL_I2C_OWNADDRESS2_NOMASK);
  LL_I2C_DisableOwnAddress2(I2C5);
  LL_I2C_DisableGeneralCall(I2C5);
  LL_I2C_EnableClockStretching(I2C5);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x00D049FB;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C5, &I2C_InitStruct);
  /* USER CODE BEGIN I2C5_Init 2 */

  /* USER CODE END I2C5_Init 2 */

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
