/*
 * st6336.h
 *
 *  Created on: Apr 9, 2025
 *      Author: sche
 */

#pragma once

#include <stdint.h>
#include "i2c.h"
#include "gpio.h"
#include "stm32h7xx_ll_i2c.h"

#define FT6X36_ADDR 0x38 << 1
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 320;

#define FT6336_I2C_PORT hi2c5

//struct TouchPoints_HandleTypeDef {
//	uint16_t point1_x;
//	uint16_t point1_y;
//	uint16_t point2_x;
//	uint16_t point2_y;
//};
typedef struct {
	uint16_t point1_x;
	uint16_t point1_y;
	uint16_t point2_x;
	uint16_t point2_y;
} TouchPoints_HandleTypeDef;

#define FT6336_DEV_MODE 0x00
#define FT6336_GEST_ID 0x01
#define FT6336_TD_STATUS 0x02
#define FT6336_P1_XH 0x03
#define FT6336_P1_XL 0x04
#define FT6336_P1_YH 0x05
#define FT6336_P1_YL 0x06
#define FT6336_P1_WEIGHT 0x07
#define FT6336_P1_MISC 0x08
#define FT6336_P2_XH 0x09
#define FT6336_P2_XL 0x0A
#define FT6336_P2_YH 0x0B
#define FT6336_P2_YL 0x0C
#define FT6336_P2_WEIGHT 0x0D
#define FT6336_P2_MISC 0x0E
#define FT6336_TH_GROUP 0x80
#define FT6336_TH_DIFF 0x85
#define FT6336_CTRL 0x86
#define FT6336_TIMEENTERM 0x87
#define FT6336_PERIODACTIVE 0x88
#define FT6336_PERIODMONITOR 0x89
#define FT6336_RADIAN_VALUE 0x91
#define FT6336_OFFSET_LEFT_RIGHT 0x92
#define FT6336_OFFSET_UP_DOWN 0x93
#define FT6336_DISTANCE_LEFT_RIGHT 0x94
#define FT6336_DISTANCE_UP_DOWN 0x95
#define FT6336_DISTANCE_ZOOM 0x96
#define FT6336_LIB_VER_H 0xA1
#define FT6336_LIB_VER_L 0xA2
#define FT6336_CIPHER 0xA3
#define FT6336_G_MODE 0xA4
#define FT6336_PWR_MODE 0xA5
#define FT6336_FIRMID 0xA6
#define FT6336_FOCALTECH_ID 0xA8
#define FT6336_RELEASE_CODE_ID 0xAF
#define FT6336_STATE 0xBC

void FT6336_RST_L();
void FT6336_RST_H();

void FT6336_Init();
TouchPoints_HandleTypeDef FT6336_GetTouchPoint();
uint8_t FT6336_WriteRegister(uint8_t RegAddress, uint8_t *pData, uint16_t Size);
uint8_t FT6336_ReadRegister(uint8_t RegAddress, uint8_t *pData, uint16_t Size);

