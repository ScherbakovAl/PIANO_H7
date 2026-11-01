/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "piano_h7.hpp"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern void h7();
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// в слове над _bflag (флаг DFU): «после сброса уйти в выкл.»
#define POWER_OFF_FLAG 0xDEAD0FF0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// бутлоадер обнуляет только _bflag, а стеки бутлоадера и приложения начинаются ниже _bflag —
// это слово переживает сброс и прыжок из бутлоадера
extern uint32_t _bflag[]; // массив без длины: компилятор не считает доступ к слову за _bflag выходом за границы
static volatile uint32_t* const power_off_flag = &_bflag[1];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// SW1 (PC13) отпущена 50 мс подряд — дребезг закончился
static void power_button_wait_release(void) {
  for (int released_ms = 0; released_ms < 50;) {
    LL_mDelay(1);
    released_ms = LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_13) ? 0 : released_ms + 1;
  }
}

// «Выключено»: H7 в Stop, выводы держат заданные уровни (в Standby они ушли бы в Hi-Z).
// Выход — сбросом: по SW1 (ниже) или по NRST (TP2)
static void power_off(void) {
  // питание ядра — как в SystemClock_Config: пока ACTVOSRDY = 0, чип в ограниченном режиме Run*
  LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
  while (LL_PWR_IsActiveFlag_ACTVOS() == 0) {
  }
  LL_Init1msTick(64000000); // после сброса ядро на HSI 64 МГц
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB | LL_AHB4_GRP1_PERIPH_GPIOC
                         | LL_AHB4_GRP1_PERIPH_GPIOD | LL_AHB4_GRP1_PERIPH_GPIOE);
  LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);

  // шина G4 в покое, как между пакетами: U_TX (PB13) = 1, TIM_CLK = 0.
  // U_RX (PB12) и TIM_TRIGG (PE7) держат R18 и R11 — там работают выходы G4, эти пины не трогать
  LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_13);
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(TIM1_CLK_GPIO_Port, TIM1_CLK_Pin);
  LL_GPIO_SetPinMode(TIM1_CLK_GPIO_Port, TIM1_CLK_Pin, LL_GPIO_MODE_OUTPUT);

  // +5 В модуля дисплея не отключить: подсветку (PD6, на модуле подтянута вверх) держим в 0,
  // дисплей и тач — в сбросе
  LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_6);
  LL_GPIO_SetPinMode(GPIOD, LL_GPIO_PIN_6, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(LCD_RESET_GPIO_Port, LCD_RESET_Pin);
  LL_GPIO_SetPinMode(LCD_RESET_GPIO_Port, LCD_RESET_Pin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(CTP_RST_GPIO_Port, CTP_RST_Pin);
  LL_GPIO_SetPinMode(CTP_RST_GPIO_Port, CTP_RST_Pin, LL_GPIO_MODE_OUTPUT);

  // SW1: R20 на плате не стоит — подтяжка вниз внутренняя. В analog EXTI фронтов не видит
  LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);
  power_button_wait_release();
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13);
  LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_13);
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_13);
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
  NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
  NVIC_EnableIRQ(EXTI15_10_IRQn);

  __disable_irq(); // обработчик не нужен: WFI просыпается по прерыванию и при PRIMASK = 1
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

  power_button_wait_release(); // старт после отпускания — дребезг не даст лишних сбросов
  NVIC_SystemReset();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
//  // Включаем тактирование GPIOE и GPIOB для настройки пинов
//  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);
//  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);

//  // Переводим пины E11, E7, B12, B13 в режим аналогового вывода для обесточивания перед standby
//  // GPIOE/B_MODER: 00=input, 01=output, 10=alternate, 11=analog
//  // Сдвиг пина * 2, так как каждый пин занимает 2 бита
//  // ВАЖНО: сначала очищаем биты, потом устанавливаем нужное значение
 
//  // PE11 -> analog
//  GPIOE->MODER &= ~(0x03 << (11 * 2));  // Очищаем биты
//  GPIOE->MODER |= (0x03 << (11 * 2));   // Устанавливаем analog (11)
 
//  // PE7 -> analog
//  GPIOE->MODER &= ~(0x03 << (7 * 2));   // Очищаем биты
//  GPIOE->MODER |= (0x03 << (7 * 2));    // Устанавливаем analog (11)
 
//  // PB12 -> analog
//  GPIOB->MODER &= ~(0x03 << (12 * 2));  // Очищаем биты
//  GPIOB->MODER |= (0x03 << (12 * 2));   // Устанавливаем analog (11)
 
//  // PB13 -> analog
//  GPIOB->MODER &= ~(0x03 << (13 * 2));  // Очищаем биты
//  GPIOB->MODER |= (0x03 << (13 * 2));   // Устанавливаем analog (11)

//  // Отключаем подтяжку для этих пинов (сбрасываем в 0)
//  GPIOE->PUPDR &= ~(0x03 << (11 * 2));  // PE11 - no pull
//  GPIOE->PUPDR &= ~(0x03 << (7 * 2));   // PE7 - no pull
//  GPIOB->PUPDR &= ~(0x03 << (12 * 2));  // PB12 - no pull
//  GPIOB->PUPDR &= ~(0x03 << (13 * 2));  // PB13 - no pull
 
//  // Сбрасываем выходные регистры в 0 (на всякий случай)
//  GPIOE->ODR &= ~((1 << 11) | (1 << 7));
//  GPIOB->ODR &= ~((1 << 12) | (1 << 13));

	// if (!__HAL_PWR_GET_FLAG(PWR_FLAG_SB)) {
	// 	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4); //pin4 == кнопка К1 на плате
	// 	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
	// 	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4);
	// 	HAL_PWR_EnterSTANDBYMode();
	// }
	// else {
	// 	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
	// }

  // H7 всегда в USB: подача питания и «off» в меню — в Stop до нажатия SW1.
  // Бутлоадер прыгает сюда, не трогая RCC, — флаги сброса доходят целыми. Они копятся до RMVF:
  // без сброса флагов любой сброс выглядел бы как подача питания
  const int power_up = LL_RCC_IsActiveFlag_PORRST() || LL_RCC_IsActiveFlag_BORRST();
  LL_RCC_ClearResetFlags();
  if (power_up || *power_off_flag == POWER_OFF_FLAG) {
    *power_off_flag = 0;
    power_off();
  }
  /* USER CODE END 1 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_UART5_Init();
  MX_USB_OTG_HS_PCD_Init();
  MX_I2C5_Init();
  MX_SPI3_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  h7();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_3)
  {
  }
  LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_HSI48_Enable();

   /* Wait till HSI48 is ready */
  while(LL_RCC_HSI48_IsReady() != 1)
  {

  }
  LL_RCC_HSE_EnableCSS();
  LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSE);
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1R_Enable();
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_4_8);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(5);
  LL_RCC_PLL1_SetN(110);
  LL_RCC_PLL1_SetP(1);
  LL_RCC_PLL1_SetQ(2);
  LL_RCC_PLL1_SetR(1);
  LL_RCC_PLL1_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL1_IsReady() != 1)
  {
  }

   /* Intermediate AHB prescaler 2 when target frequency clock is higher than 80 MHz */
   LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1)
  {

  }
  LL_RCC_SetSysPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_2);
  LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_2);
  LL_SetSystemCoreClock(550000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  LL_RCC_HSE_EnableCSS();
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  LL_RCC_PLL2Q_Enable();
  LL_RCC_PLL2_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_1_2);
  LL_RCC_PLL2_SetVCOOutputRange(LL_RCC_PLLVCORANGE_MEDIUM);
  LL_RCC_PLL2_SetM(25);
  LL_RCC_PLL2_SetN(272);
  LL_RCC_PLL2_SetP(1);
  LL_RCC_PLL2_SetQ(2);
  LL_RCC_PLL2_SetR(2);
  LL_RCC_PLL2_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL2_IsReady() != 1)
  {
  }

  LL_RCC_PLL3P_Enable();
  LL_RCC_PLL3_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_2_4);
  LL_RCC_PLL3_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL3_SetM(10);
  LL_RCC_PLL3_SetN(150);
  LL_RCC_PLL3_SetP(2);
  LL_RCC_PLL3_SetQ(1);
  LL_RCC_PLL3_SetR(2);
  LL_RCC_PLL3_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL3_IsReady() != 1)
  {
  }

}

/* USER CODE BEGIN 4 */

// «off» в меню: выключение через сброс — из чистого состояния, тем же путём, что при подаче питания.
// Сброс сам отключает USB: у хоста устройство пропадает
void request_power_off(void) {
  *power_off_flag = POWER_OFF_FLAG;
  SCB_CleanDCache(); // иначе флаг останется в кэше и пропадёт при сбросе
  NVIC_SystemReset();
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
