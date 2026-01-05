/*
 * ft6336.cpp
 *
 *  Created on: Apr 9, 2025
 *      Author: sche
 */

#include "ft6336.h"
#include "stm32h7xx_ll_i2c.h"

void FT6336_RST_L() {
	LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_0);
}

void FT6336_RST_H() {
	LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_0);
}

TouchPoints_HandleTypeDef TouchPoints;

#define FT6336_I2C I2C5
#define FT6336_TIMEOUT 10000

static void FT6336_I2C_WaitUntilReady(void) {
	uint32_t timeout = FT6336_TIMEOUT;
	// Wait until BUSY flag is cleared
	while (LL_I2C_IsActiveFlag_BUSY(FT6336_I2C) && timeout > 0) {
		timeout--;
	}
}

static void FT6336_I2C_Start(void) {
	LL_I2C_GenerateStartCondition(FT6336_I2C);
}

static void FT6336_I2C_Stop(void) {
	LL_I2C_GenerateStopCondition(FT6336_I2C);
}

static void FT6336_I2C_SendAddress(uint8_t Address, uint8_t Direction) {
	LL_I2C_TransmitData8(FT6336_I2C, (Address << 1) | Direction);
}

static void FT6336_I2C_WriteByte(uint8_t Data) {
	LL_I2C_TransmitData8(FT6336_I2C, Data);
	// Wait until TXE flag is set (data transferred)
	uint32_t timeout = FT6336_TIMEOUT;
	while (!LL_I2C_IsActiveFlag_TXE(FT6336_I2C) && timeout > 0) {
		timeout--;
	}
}

static uint8_t FT6336_I2C_ReadByte(void) {
	// Wait until RXNE flag is set
	uint32_t timeout = FT6336_TIMEOUT;
	while (!LL_I2C_IsActiveFlag_RXNE(FT6336_I2C) && timeout > 0) {
		timeout--;
	}
	return LL_I2C_ReceiveData8(FT6336_I2C);
}

void FT6336_WriteRegister(uint8_t RegAddress, uint8_t* pData, uint16_t Size) {
	FT6336_I2C_WaitUntilReady();

	// Generate START
	LL_I2C_GenerateStartCondition(FT6336_I2C);

	// Send device address with write direction
	FT6336_I2C_SendAddress(FT6X36_ADDR, LL_I2C_DIRECTION_WRITE);

	// Wait for ADDR flag (address sent, ACK received)
	uint32_t timeout = FT6336_TIMEOUT;
	while (!LL_I2C_IsActiveFlag_ADDR(FT6336_I2C) && timeout > 0) {
		timeout--;
	}
	LL_I2C_ClearFlag_ADDR(FT6336_I2C);

	// Send register address
	FT6336_I2C_WriteByte(RegAddress);

	// Send data
	for (uint16_t i = 0; i < Size; i++) {
		FT6336_I2C_WriteByte(pData[i]);
	}

	// Generate STOP
	FT6336_I2C_Stop();
}

void FT6336_ReadRegister(uint8_t RegAddress, uint8_t* pData, uint16_t Size) {
	FT6336_I2C_WaitUntilReady();

	// Generate START
	LL_I2C_GenerateStartCondition(FT6336_I2C);

	// Send device address with write direction
	FT6336_I2C_SendAddress(FT6X36_ADDR, LL_I2C_DIRECTION_WRITE);

	// Wait for ADDR flag
	uint32_t timeout = FT6336_TIMEOUT;
	while (!LL_I2C_IsActiveFlag_ADDR(FT6336_I2C) && timeout > 0) {
		timeout--;
	}
	LL_I2C_ClearFlag_ADDR(FT6336_I2C);

	// Send register address
	FT6336_I2C_WriteByte(RegAddress);

	// Generate RESTART
	LL_I2C_GenerateStartCondition(FT6336_I2C);

	// Send device address with read direction
	FT6336_I2C_SendAddress(FT6X36_ADDR, LL_I2C_DIRECTION_READ);

	// Wait for ADDR flag
	timeout = FT6336_TIMEOUT;
	while (!LL_I2C_IsActiveFlag_ADDR(FT6336_I2C) && timeout > 0) {
		timeout--;
	}
	LL_I2C_ClearFlag_ADDR(FT6336_I2C);

	// Read data
	for (uint16_t i = 0; i < Size; i++) {
		if (i == (Size - 1)) {
			// Last byte: disable ACK
			LL_I2C_AcknowledgeNextData(FT6336_I2C, LL_I2C_NACK);
			// Generate STOP
			FT6336_I2C_Stop();
		}
		pData[i] = FT6336_I2C_ReadByte();
	}

	// Re-enable ACK for future transfers
	LL_I2C_AcknowledgeNextData(FT6336_I2C, LL_I2C_ACK);
}

void FT6336_Init(void) {
	FT6336_RST_L();
	LL_mDelay(10);
	FT6336_RST_H();
	LL_mDelay(50);

	//    // 设置 DEVICE_MODE 为 0x00（正常模式）
	FT6336_WriteRegister(0x00, 0x00, 1);
	//
	//    // 设置 ID_G_MODE 为 0x00（启用中断输出）
	FT6336_WriteRegister(0xA4, 0x00, 1);

	uint8_t id = 0;
	FT6336_ReadRegister(FT6336_FOCALTECH_ID, &id, 1);
	LL_mDelay(1);
}

static void AdjustTouchCoordinates(uint16_t* x, uint16_t* y) {
	uint16_t rawX = *x;
	uint16_t rawY = *y;
	*x = SCREEN_WIDTH - rawY;
	*y = rawX;
}

TouchPoints_HandleTypeDef FT6336_GetTouchPoint() {
	TouchPoints_HandleTypeDef touchPoints;
	TouchPoints.point1_x = 0;
	TouchPoints.point1_y = 0;
	TouchPoints.point2_x = 0;
	TouchPoints.point2_y = 0;
	uint8_t touchStatus = 0;
	uint8_t touchData[8];

	FT6336_ReadRegister(FT6336_TD_STATUS, &touchStatus, 1);
	uint8_t touchCount = touchStatus & 0x0F; // получить количество точек

	if (touchCount > 0) {
		FT6336_ReadRegister(FT6336_P1_XH, touchData, 4); // первая точка
		touchPoints.point1_x = ((touchData[0] & 0x0F) << 8) | touchData[1];
		touchPoints.point1_y = ((touchData[2] & 0x0F) << 8) | touchData[3];
		AdjustTouchCoordinates(&touchPoints.point1_x, &touchPoints.point1_y);

		if (touchCount > 1) {
			FT6336_ReadRegister(FT6336_P2_XH, &touchData[4], 4);
			touchPoints.point2_x = ((touchData[4] & 0x0F) << 8) | touchData[5];
			touchPoints.point2_y = ((touchData[6] & 0x0F) << 8) | touchData[7];
			AdjustTouchCoordinates(&touchPoints.point2_x,
				&touchPoints.point2_y);
		}
	}
	return touchPoints;
}
