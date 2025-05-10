/*
 * ft6336.cpp
 *
 *  Created on: Apr 9, 2025
 *      Author: sche
 */

#include "ft6336.h"

void FT6336_RST_L() {
	LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_0);
}

void FT6336_RST_H() {
	LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_0);
}

//#define asd
#ifdef asd
TouchPoints_HandleTypeDef TouchPoints;

// #define FT6336_I2C_PORT hi2c1
// extern SPI_HandleTypeDef FT6336_I2C_PORT;


void FT6336_WriteRegister(uint8_t RegAddress, uint8_t* pData, uint16_t Size) {
	//	 LL_I2C_HandleTransfer(I2C4, SLAVE_OWN_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
	uint8_t deviceAddress = FT6X36_ADDR; // 0x70
	LL_I2C_ClearFlag_STOP(I2C5);
	LL_I2C_ClearFlag_NACK(I2C5);
	LL_I2C_ClearFlag_TXE(I2C5);

	LL_I2C_SetTransferSize(I2C5, Size);
	LL_I2C_SetTransferRequest(I2C5, LL_I2C_REQUEST_WRITE);
	LL_I2C_SetSlaveAddr(I2C5, deviceAddress);
	LL_I2C_GenerateStartCondition(I2C5);

	while (!LL_I2C_IsActiveFlag_TXIS(I2C5)) {
		if (LL_I2C_IsActiveFlag_NACK(I2C5)) {
			break;
		}
	}
	LL_I2C_TransmitData8(I2C5, RegAddress);
	LL_mDelay(1);
	LL_I2C_TransmitData8(I2C5, *pData);
	LL_mDelay(1);

	while (!LL_I2C_IsActiveFlag_STOP(I2C5)) {
		if (LL_I2C_IsActiveFlag_NACK(I2C5)) {
			break;
		}
	}
	LL_I2C_ClearFlag_STOP(I2C5);
}

void FT6336_ReadRegister(uint8_t RegAddress, uint8_t* pData, uint16_t Size) {
	uint8_t deviceAddress = FT6X36_ADDR; // 0x70
	LL_I2C_SetTransferSize(I2C5, Size);
	LL_I2C_SetTransferRequest(I2C5, LL_I2C_REQUEST_WRITE);
	LL_I2C_SetSlaveAddr(I2C5, deviceAddress);
	LL_I2C_GenerateStartCondition(I2C5);
	LL_I2C_TransmitData8(I2C5, RegAddress);
	LL_mDelay(1);
	//	LL_I2C_ClearFlag_STOP(I2C5);

	LL_I2C_SetTransferSize(I2C5, Size);
	LL_I2C_SetTransferRequest(I2C5, LL_I2C_REQUEST_READ);
	LL_I2C_SetSlaveAddr(I2C5, deviceAddress);
	LL_I2C_GenerateStartCondition(I2C5);
	uint8_t a = LL_I2C_ReceiveData8(I2C5);
	LL_mDelay(1);
	LL_I2C_ClearFlag_STOP(I2C5);
}

//HAL_StatusTypeDef FT6336_WriteRegister( uint8_t RegAddress, uint8_t *pData, uint16_t Size) {
//    return HAL_I2C_Mem_Write(&FT6336_I2C_PORT, FT6X36_ADDR, RegAddress, I2C_MEMADD_SIZE_8BIT, pData, Size, HAL_MAX_DELAY);
//}
//
//HAL_StatusTypeDef FT6336_ReadRegister( uint8_t RegAddress, uint8_t *pData, uint16_t Size) {
//    return HAL_I2C_Mem_Read(&FT6336_I2C_PORT, FT6X36_ADDR, RegAddress, I2C_MEMADD_SIZE_8BIT, pData, Size, HAL_MAX_DELAY);
//}

void FT6336_Init(void) {
	FT6336_RST_L();
	LL_mDelay(10);
	FT6336_RST_H();
	LL_mDelay(50);

	FT6336_WriteRegister(0x00, 0x00, 1); // normal mode
	FT6336_WriteRegister(0xA4, 0x00, 1); // enable interrupt out
	uint8_t id = 0;
	FT6336_ReadRegister(FT6336_FOCALTECH_ID, &id, 1);
	//    HAL_Delay(1000);

	//used to debug IIC
	//    for (uint8_t addr = 0x08; addr < 0x78; ++addr) {  // I2C 地址范围是 0x08 到 0x77
	//           if (HAL_I2C_Mem_Read(&FT6336_I2C_PORT, addr << 1, 0x00, I2C_MEMADD_SIZE_8BIT, &id, 1, 100) == HAL_OK) {
	//                // 如果扫描到设备并且读取成功，进入处理函数
	//                HAL_Delay(100);
	//            }
	//            HAL_Delay(10);
	//        }
	//    HAL_I2C_Mem_Read(&FT6336_I2C_PORT, 0xA0, 0x00, I2C_MEMADD_SIZE_8BIT, &id, 1, HAL_MAX_DELAY);
	//    HAL_Delay(1000);
		//used to debug IIC
}
#endif

extern I2C_HandleTypeDef FT6336_I2C_PORT;

TouchPoints_HandleTypeDef TouchPoints;

HAL_StatusTypeDef FT6336_WriteRegister(uint8_t RegAddress, uint8_t* pData,
	uint16_t Size) {
	return HAL_I2C_Mem_Write(&FT6336_I2C_PORT, FT6X36_ADDR, RegAddress,
		I2C_MEMADD_SIZE_8BIT, pData, Size, HAL_MAX_DELAY);
}

HAL_StatusTypeDef FT6336_ReadRegister(uint8_t RegAddress, uint8_t* pData,
	uint16_t Size) {
	return HAL_I2C_Mem_Read(&FT6336_I2C_PORT, FT6X36_ADDR, RegAddress,
		I2C_MEMADD_SIZE_8BIT, pData, Size, HAL_MAX_DELAY);
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
