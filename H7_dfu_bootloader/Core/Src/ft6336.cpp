/*
 * ft6336.cpp
 *
 *  Created on: Apr 9, 2025
 *      Author: sche
 */

#include "ft6336.h"
#include "stm32h7xx_ll_i2c.h"
#include "stm32h7xx_ll_dma.h"

// Глобальный флаг для синхронизации DMA
volatile uint8_t ft6336_dma_rx_complete = 0;
volatile uint8_t ft6336_dma_error = 0;

void FT6336_RST_L() {
	LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_0);
}

void FT6336_RST_H() {
	LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_0);
}

TouchPoints_HandleTypeDef TouchPoints;

uint8_t FT6336_WriteRegister(uint8_t RegAddress, uint8_t* pData, uint16_t Size) {
	// Ожидание готовности шины I2C
	while (LL_I2C_IsActiveFlag_BUSY(I2C5));

	// Настройка передачи
	LL_I2C_HandleTransfer(I2C5, FT6X36_ADDR, LL_I2C_ADDRSLAVE_7BIT,
		Size + 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

	// Ожидание готовности TX буфера и отправка адреса регистра
	while (!LL_I2C_IsActiveFlag_TXIS(I2C5));
	LL_I2C_TransmitData8(I2C5, RegAddress);

	// Отправка данных
	for (uint16_t i = 0; i < Size; i++) {
		while (!LL_I2C_IsActiveFlag_TXIS(I2C5));
		LL_I2C_TransmitData8(I2C5, pData[i]);
	}

	// Ожидание завершения передачи
	while (!LL_I2C_IsActiveFlag_STOP(I2C5));
	LL_I2C_ClearFlag_STOP(I2C5);

	return 0; // Успех
}

uint8_t FT6336_ReadRegister(uint8_t RegAddress, uint8_t* pData, uint16_t Size) {
	// Ожидание готовности шины I2C
	while (LL_I2C_IsActiveFlag_BUSY(I2C5));

	// Отправка адреса регистра (запись)
	LL_I2C_HandleTransfer(I2C5, FT6X36_ADDR, LL_I2C_ADDRSLAVE_7BIT,
		1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

	while (!LL_I2C_IsActiveFlag_TXIS(I2C5));
	LL_I2C_TransmitData8(I2C5, RegAddress);

	while (!LL_I2C_IsActiveFlag_TC(I2C5));

	// Чтение данных
	LL_I2C_HandleTransfer(I2C5, FT6X36_ADDR, LL_I2C_ADDRSLAVE_7BIT,
		Size, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

	for (uint16_t i = 0; i < Size; i++) {
		while (!LL_I2C_IsActiveFlag_RXNE(I2C5));
		pData[i] = LL_I2C_ReceiveData8(I2C5);
	}

	// Ожидание завершения приема
	while (!LL_I2C_IsActiveFlag_STOP(I2C5));
	LL_I2C_ClearFlag_STOP(I2C5);

	return 0; // Успех
}

uint8_t FT6336_ReadRegister_DMA(uint8_t RegAddress, uint8_t* pData, uint16_t Size) {
	// Сброс флагов
	ft6336_dma_rx_complete = 0;
	ft6336_dma_error = 0;
	SCB_CleanInvalidateDCache(); // or

	// Ожидание готовности шины I2C
	while (LL_I2C_IsActiveFlag_BUSY(I2C5));

	// Отправка адреса регистра (запись)
	LL_I2C_HandleTransfer(I2C5, FT6X36_ADDR, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

	while (!LL_I2C_IsActiveFlag_TXIS(I2C5));
	LL_I2C_TransmitData8(I2C5, RegAddress);

	// while (!LL_I2C_IsActiveFlag_TC(I2C5));

	// Очистка флагов DMA
	// LL_DMA_ClearFlag_TC3(DMA2);
	// LL_DMA_ClearFlag_TE3(DMA2);
	// LL_DMA_ClearFlag_HT3(DMA2);
	// LL_DMA_ClearFlag_DME3(DMA2);

	// Настройка DMA для приема
	LL_DMA_ConfigAddresses(DMA2, LL_DMA_STREAM_3,
		(uint32_t)&I2C5->RXDR, (uint32_t)pData,
		LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_3, Size);

	// Включение DMA для I2C
	// LL_I2C_EnableDMAReq_RX(I2C5);

	// Включение DMA Stream
	LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_3);

	// Запуск чтения с рестартом
	LL_I2C_HandleTransfer(I2C5, FT6X36_ADDR, LL_I2C_ADDRSLAVE_7BIT,
		Size, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

	// Ожидание завершения DMA или ошибки
	while (!ft6336_dma_rx_complete && !ft6336_dma_error);

	// Ожидание STOP флага
	while (!LL_I2C_IsActiveFlag_STOP(I2C5));
	LL_I2C_ClearFlag_STOP(I2C5);

	// Отключение DMA запросов
	// LL_I2C_DisableDMAReq_RX(I2C5);

	// Отключение DMA Stream
	LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_3);

	// if (ft6336_dma_error) {
		// return 1; // Ошибка
	// }

	return 0; // Успех
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


	LL_I2C_EnableDMAReq_RX(I2C5);
	LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_3);
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
		// AdjustTouchCoordinates(&touchPoints.point1_x, &touchPoints.point1_y);

		if (touchCount > 1) {
			FT6336_ReadRegister(FT6336_P2_XH, &touchData[4], 4);
			touchPoints.point2_x = ((touchData[4] & 0x0F) << 8) | touchData[5];
			touchPoints.point2_y = ((touchData[6] & 0x0F) << 8) | touchData[7];
			// AdjustTouchCoordinates(&touchPoints.point2_x, &touchPoints.point2_y);
		}
	}
	return touchPoints;
}
