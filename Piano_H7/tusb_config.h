/*
 * tusb_config.h
 *
 *  Created on: Mar 15, 2025
 *      Author: sche
 */

#ifndef INC_TUSB_CONFIG_H_
#define INC_TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT            0
#endif

    // RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED         OPT_MODE_DEFAULT_SPEED
#endif

// ^^^ <...> omitted stuff above, generic C language boilerplate crap, see any example in tinyusb

#define CFG_TUSB_MCU                OPT_MCU_STM32H7
#define CFG_TUSB_OS                 OPT_OS_NONE
#define CFG_TUSB_DEBUG        		0
#define CFG_TUD_ENABLED       		1
#define CFG_TUD_MAX_SPEED           BOARD_TUD_MAX_SPEED
//#define BOARD_DEVICE_RHPORT_SPEED   OPT_MODE_FULL_SPEED  // 480mbps
#define BOARD_DEVICE_RHPORT_NUM     0
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// H723: один контроллер OTG_HS со встроенным FS PHY, dwc2_stm32.h сажает на него и порт 0, и порт 1.
// Везде порт 0: tusb_init, tud_int_handler(BOARD_DEVICE_RHPORT_NUM) и midi_device (write_flush берёт rhport 0 жёстко).

// vvv<...> (see tinyusb examples for remainder, configuring USB Class and buffer size basics)

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
#endif






#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

//------------- CLASS -------------//
#define CFG_TUD_CDC               0
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              1
#define CFG_TUD_VENDOR            0

// MIDI FIFO size of TX and RX
#define CFG_TUD_MIDI_RX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_MIDI_TX_BUFSIZE   512 // FS тоже: 64 Б = 8 нот (CC88+нота), аккорд с подъёмами демпферов не влезал



#ifdef __cplusplus
}
#endif
#endif /* INC_TUSB_CONFIG_H_ */
