/*
 * st7796.cpp
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#include "st7796.h"

const int LCD_W = 320;
const int LCD_H = 480;
// const int WHITE = 0xFFFF; // не используется
// uint16_t POINT_COLOR = 0x0000; // не используется

lcd_dev lcddev;

void Send_DMA_Data8(uint8_t* buff, uint16_t dataSize) {

	// LCD_DC_C(); // ? надо?
	// LCD_WR_REG(0x2c); // надо?
	LCD_DC_D();

	LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_1);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_1, dataSize / 4); // в словах; dataSize кратен 48 (lcd_rounder_cb)
	LL_DMA_SetPeriphAddress(DMA2, LL_DMA_STREAM_1, LL_SPI_DMA_GetTxRegAddr(SPI3));
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_1, (uint32_t)buff); // TODO (uint32_t*) а если + " * " ??
	// RM0468: перед EN все флаги потока должны быть сброшены (TC/HT/TE сбрасывает прерывание, FE/DME — нет)
	DMA2->LIFCR = DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;
	LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_1);
}

void LCD_Send_Data_8(uint8_t data) {
	while (!LL_SPI_IsActiveFlag_TXP(SPI3)) {
	};
	LL_SPI_TransmitData8(SPI3, data);
	while (!LL_SPI_IsActiveFlag_TXC(SPI3)) {
	};
}

// не используются (остались от RGB565 без DMA)
/*
void LCD_Send_Data_16(uint16_t* Data) {
	LCD_DC_D();
	while (!LL_SPI_IsActiveFlag_TXP(SPI3)) {
	};
	LL_SPI_TransmitData16(SPI3, *Data);
	while (!LL_SPI_IsActiveFlag_TXC(SPI3)) {
	};
	//	LCD_Send_Data_8(a >> 8);
	//	LCD_Send_Data_8(a);
}

void LCD_Send_Data_16_(uint16_t Data) {
	LCD_DC_D();
	while (!LL_SPI_IsActiveFlag_TXP(SPI3)) {
	};
	LL_SPI_TransmitData16(SPI3, Data);
	while (!LL_SPI_IsActiveFlag_TXC(SPI3)) {
	};
	//	LCD_Send_Data_8(Data >> 8);
	//	LCD_Send_Data_8(Data);
}
*/

void LCD_WR_REG(uint8_t data) {
	LCD_DC_C();
	LCD_Send_Data_8(data);
}

void LCD_WR_DATA(uint8_t data) {
	LCD_DC_D();
	LCD_Send_Data_8(data);
}

void LCD_WriteRAM_Prepare(void) {
	LCD_WR_REG(lcddev.wramcmd);
}

void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue) {
	LCD_WR_REG(LCD_Reg);
	LCD_WR_DATA(LCD_RegValue); // or LCD_Send_Data_16??
}

void LCD_direction(uint8_t direction) {
	lcddev.setxcmd = 0x2A; // CASET (2Ah): Column Address Set (42)
	lcddev.setycmd = 0x2B; // RASET (2Bh): Row Address Set (43)
	lcddev.wramcmd = 0x2C; // RAMWR (2Ch): Memory Write (44)
	// lcddev.rramcmd = 0x2E; // RAMRD (2Eh): Memory Read — не используется: SPI3 только на передачу
	lcddev.dir = direction % 4;
	switch (lcddev.dir) {
	case 0:
		lcddev.width = LCD_W;
		lcddev.height = LCD_H;
		// для цвета 565 - LCD_WriteReg(0x36, (1 << 3) | (1 << 6));((1 << 3) = RGB<>BRG)
		LCD_WriteReg(0x36, (1 << 6)); // MADCTL (36h): Memory Data Access Control
		break;
	case 1:
		lcddev.width = LCD_H;
		lcddev.height = LCD_W;
		LCD_WriteReg(0x36, (1 << 3) | (1 << 5)); // MADCTL (36h): Memory Data Access Control
		break;
	case 2:
		lcddev.width = LCD_W;
		lcddev.height = LCD_H;
		LCD_WriteReg(0x36, (1 << 3) | (1 << 7)); // MADCTL (36h): Memory Data Access Control
		break;
	case 3:
		lcddev.width = LCD_H;
		lcddev.height = LCD_W;
		LCD_WriteReg(0x36, (1 << 3) | (1 << 7) | (1 << 6) | (1 << 5));
		break;
	default:
		break;
	}
}

void LCD_Clear(uint32_t Color) {
	LCD_Fill(0, 0, lcddev.width - 1, lcddev.height - 1, Color);
}

// Заливка окна (включительно) цветом 0xRRGGBB. Байты идут в порядке B, G, R — как в буфере
// LVGL (RGB888), с которым цвета на этом дисплее верные. DC не меняется, поэтому ждём только
// места в FIFO, а TXC — один раз в конце.
void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t Color) {
	LCD_SetWindows(x1, y1, x2, y2);
	LCD_DC_D();
	const uint32_t pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
	for (uint32_t i = 0; i < pixels; i++) {
		while (!LL_SPI_IsActiveFlag_TXP(SPI3)) {
		};
		LL_SPI_TransmitData8(SPI3, Color); // B
		while (!LL_SPI_IsActiveFlag_TXP(SPI3)) {
		};
		LL_SPI_TransmitData8(SPI3, Color >> 8); // G
		while (!LL_SPI_IsActiveFlag_TXP(SPI3)) {
		};
		LL_SPI_TransmitData8(SPI3, Color >> 16); // R
	}
	while (!LL_SPI_IsActiveFlag_TXC(SPI3)) {
	};
}

// не используется
// void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos) {
// 	LCD_SetWindows(Xpos, Ypos, Xpos, Ypos);
// }

void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd,
	uint16_t yEnd) {
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(xStar >> 8);
	LCD_WR_DATA(0x00FF & xStar);
	LCD_WR_DATA(xEnd >> 8);
	LCD_WR_DATA(0x00FF & xEnd);

	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(yStar >> 8);
	LCD_WR_DATA(0x00FF & yStar);
	LCD_WR_DATA(yEnd >> 8);
	LCD_WR_DATA(0x00FF & yEnd);

	LCD_WriteRAM_Prepare();
}

void LCD_RESET() {
	LCD_RES_L();
	LL_mDelay(100);
	LCD_RES_H();
	LL_mDelay(50);
}

void LCD_Init() {

	LCD_RESET();

	LCD_WR_REG(0x11); // Sleep Out - выход из спящего режима (после аппаратного сброса дисплей уже в Sleep In)

	LL_mDelay(120);       //Delay 120ms

	LCD_direction(0); // MADCTL (0x36) – ориентация, зеркалирование и порядок цветов

	LCD_WR_REG(0x3A); // COLMOD (Interface Pixel Format, Формат цвета)
	LCD_WR_DATA(0x77); // было 0x55 //0x55 = 16 бит (RGB565) - 0x66 = 18 бит (RGB666) - 0x77 = 24 бит (RGB888)

	//	/*
		// с этими настройками немного цвет меняется
	LCD_WR_REG(0xF0); // CSCON (Command Set Control) ??
	LCD_WR_DATA(0xC3); // C3h enable command 2 part I

	LCD_WR_REG(0xF0); // CSCON (Command Set Control) ??
	LCD_WR_DATA(0x96); // 96h enable command 2 part II

	LCD_WR_REG(0xB4); // DIC (B4): Display Inversion Control ??
	LCD_WR_DATA(0x02); // 2-dot inversion (0x02) ?? хз

	LCD_WR_REG(0xB7); // EM(B7): Entry Mode Set ??
	LCD_WR_DATA(0xC6);

	LCD_WR_REG(0xC0); // PWR1(C0h): Power Control 1
	LCD_WR_DATA(0xC0);
	LCD_WR_DATA(0x00);

	LCD_WR_REG(0xC1); // PWR2 (C1h): Power Control 2
	LCD_WR_DATA(0x13); // 4.5+

	LCD_WR_REG(0xC2); // PWR3 (C2h): Power Control 3
	LCD_WR_DATA(0x7); // Source driving current level - Low, Source driving current level - High (0xA7)

	LCD_WR_REG(0xC5); // VCMPCTL(C5h): VCOM Control
	LCD_WR_DATA(0x21); // 0.825

	LCD_WR_REG(0xE8); // DOCA (E8h): Display Output Ctrl Adjust ??
	LCD_WR_DATA(0x40);
	LCD_WR_DATA(0x8A);
	LCD_WR_DATA(0x00); // (0x1B)
	LCD_WR_DATA(0x00); // (0x1B)
	LCD_WR_DATA(0x23); // Source timing Control(us) = 13.5
	LCD_WR_DATA(0x0A); // G_START Gate timing Control (Tclk) = 10
	LCD_WR_DATA(0xAC); // G_END  Gate timing Control (Tclk) = 44 & Gate driver EQ function ON/OFF. '0' OFF, '1' ON. Default is OFF. = ON
	LCD_WR_DATA(0x33);

	LCD_WR_REG(0xE0); // PGC (E0h): Positive Gamma Control
	LCD_WR_DATA(0xD2);
	LCD_WR_DATA(0x05);
	LCD_WR_DATA(0x08);
	LCD_WR_DATA(0x06);
	LCD_WR_DATA(0x05);
	LCD_WR_DATA(0x02);
	LCD_WR_DATA(0x2A);
	LCD_WR_DATA(0x44);
	LCD_WR_DATA(0x46);
	LCD_WR_DATA(0x39);
	LCD_WR_DATA(0x15);
	LCD_WR_DATA(0x15);
	LCD_WR_DATA(0x2D);
	LCD_WR_DATA(0x32);

	LCD_WR_REG(0xE1); // NGC (E1h): Negative Gamma Control
	LCD_WR_DATA(0x96);
	LCD_WR_DATA(0x08);
	LCD_WR_DATA(0x0C);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x25);
	LCD_WR_DATA(0x2E);
	LCD_WR_DATA(0x43);
	LCD_WR_DATA(0x42);
	LCD_WR_DATA(0x35);
	LCD_WR_DATA(0x11);
	LCD_WR_DATA(0x11);
	LCD_WR_DATA(0x28);
	LCD_WR_DATA(0x2E);

	LCD_WR_REG(0xF0); // CSCON (F0h): Command Set Control
	LCD_WR_DATA(0x3C); // 3Ch disable command 2 part I

	LCD_WR_REG(0xF0); // CSCON (F0h): Command Set Control
	LCD_WR_DATA(0x69); // 69h disable command 2 part II

	LL_mDelay(120);

	// LCD_WR_REG(0x20); // INVOFF (20h): Display Inversion Off
	LCD_WR_REG(0x21); // INVON (21h): Display Inversion On

	// LCD_WR_REG(0x28); // DISPOFF (28h): Display Off
	LCD_WR_REG(0x29); // DISPON (29h): Display On

	LCD_Clear(0x000000);

	// DMA (TXDMAEN и порог FIFO SPI — до SPE в init_LCD_and_touch).
	// Словами с обеих сторон: SPI сам раскладывает 32-битную запись на 4 байта, младший первым;
	// память читается пачками по 16 байт через FIFO потока — в 4 раза меньше обращений к шине.
	LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_1, LL_DMA_MDATAALIGN_WORD);
	LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_1, LL_DMA_PDATAALIGN_WORD);
	LL_DMA_EnableFifoMode(DMA2, LL_DMA_STREAM_1);
	LL_DMA_SetFIFOThreshold(DMA2, LL_DMA_STREAM_1, LL_DMA_FIFOTHRESHOLD_FULL);
	LL_DMA_SetMemoryBurstxfer(DMA2, LL_DMA_STREAM_1, LL_DMA_MBURST_INC4);
	LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_1);
	LL_DMA_EnableIT_TE(DMA2, LL_DMA_STREAM_1);

	// тест при старте: драйвер и дисплей ожили, код дошёл до инициализации
	for (int a = 0; a < 50; ++a) {
		LCD_Fill(50 + a, 50 + a, 100 + a, 100 + a, 0xFFFFFF);
		LCD_Fill(100 + a, 100 + a, 150 + a, 150 + a, 0xFFFFFF);
		LCD_Fill(150 + a, 150 + a, 200 + a, 200 + a, 0xFFFFFF);
	}
}

void LCD_RES_H() {
	LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9);
}

void LCD_RES_L() {
	LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_9);
}

// LCD_RS_SET;
void LCD_DC_D() {
	LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7);
}

// LCD_RS_CLR;
void LCD_DC_C() {
	LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_7);
}

// не используются: CS (PE1) прижат к 0 в MX_GPIO_Init — дисплей один на шине
// // LCD_CS_SET;
// void LCD_CS_H() {
// 	LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_1);
// }
//
// // LCD_CS_CLR;
// void LCD_CS_L() {
// 	LL_GPIO_ResetOutputPin(GPIOE, LL_GPIO_PIN_1);
// }

// не используются: подсветка светится всегда, PD6 настроен как аналоговый
// void LCD_BL_H() {
// 	LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_6);
// }
//
// void LCD_BL_L() {
// 	LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_6);
// }

