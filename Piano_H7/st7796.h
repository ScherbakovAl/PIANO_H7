/*
 * st7796.h
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#pragma once

#include <stdint.h>
#include "spi.h"

 //#define WHITE       0xFFFF
#define BLACK      	0x0000
#define BLUE       	0x001F
#define BRED        0XF81F
#define GRED 			 	0XFFE0
#define GBLUE			 	0X07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define BROWN 			0XBC40
#define BRRED 			0XFC07
#define GRAY  			0X8430
#define DARKBLUE      	 0X01CF
#define LIGHTBLUE      	 0X7D7C
#define GRAYBLUE       	 0X5458
#define LIGHTGREEN     	0X841F
#define LIGHTGRAY     0XEF5B
#define LGRAY 			 		0XC618
#define LGRAYBLUE      	0XA651
#define LBBLUE          0X2B12

const int DIRECTION = 3;
struct lcd_dev {
	uint16_t width;
	uint16_t height;
	uint16_t id;
	uint8_t dir;
	uint16_t wramcmd;
	uint16_t rramcmd;
	uint16_t setxcmd;
	uint16_t setycmd;
};

void Send_DMA_Data8(uint16_t* buff, uint16_t dataSize);
void Send_DMA_Data16(uint16_t* buff, uint16_t dataSize);
void LCD_Send_Data_8(uint8_t data);
void LCD_Send_Data_16(uint16_t* Data);
void LCD_Send_Data_16_(uint16_t Data);
void LCD_WR_REG(uint8_t data);
void LCD_WR_DATA(uint8_t data);
void LCD_WriteRAM_Prepare(void);
void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue);
void LCD_direction(uint8_t direction);
void LCD_Clear(uint16_t Color);
void LCD_rect_test(int x1, int x2, int y1, int y2, uint16_t Color);
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos);
void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd);
void LCD_RESET(void);
void LCD_Init(void);

void LCD_RES_H();
void LCD_RES_L();
void LCD_DC_D();
void LCD_DC_C();
void LCD_CS_H();
void LCD_CS_L();
void LCD_BL_H();
void LCD_BL_L();

