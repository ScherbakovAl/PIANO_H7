// Заглушка HAL для сборки gui.cpp на ПК: только то, что gui.cpp зовёт сам. Реализация — в сцены.cpp
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { int unused; } stub_periph;
extern stub_periph stub_dma2, stub_spi3;
#define DMA2 (&stub_dma2)
#define SPI3 (&stub_spi3)

static inline void LL_SPI_EnableDMAReq_TX(stub_periph*) {}
static inline void LL_SPI_Enable(stub_periph*) {}
static inline void LL_SPI_StartMasterTransfer(stub_periph*) {}
static inline uint32_t LL_SPI_IsActiveFlag_TXC(stub_periph*) { return 1; }
static inline uint32_t LL_DMA_IsActiveFlag_TC1(stub_periph*) { return 0; }
static inline uint32_t LL_DMA_IsActiveFlag_TE1(stub_periph*) { return 0; }
static inline uint32_t LL_DMA_IsActiveFlag_HT1(stub_periph*) { return 0; }
static inline void LL_DMA_ClearFlag_TC1(stub_periph*) {}
static inline void LL_DMA_ClearFlag_TE1(stub_periph*) {}
static inline void LL_DMA_ClearFlag_HT1(stub_periph*) {}
static inline void SCB_CleanDCache_by_Addr(uint32_t*, int32_t) {}
static inline void SCB_DisableDCache(void) {}
static inline void SCB_DisableICache(void) {}
static inline void SCB_CleanInvalidateDCache(void) {}
static inline void NVIC_SystemReset(void) {}

uint32_t HAL_GetTick(void);
void request_power_off(void);

#ifdef __cplusplus
}
#endif
