/**
 * @file           : main.c
 * @brief          : Main program body
 **/

#include "main.h"
#include "ssd1306.h"

#include <stdbool.h>

typedef enum
{
   MODE_OFF = 0,
   MODE_SLOW,
   MODE_FAST
} LedMode_t;

typedef enum
{
   BTN_EVENT_NONE = 0,
   BTN_EVENT_SHORT_PRESS,
   BTN_EVENT_LONG_PRESS
} BtnEvent_t;

#define LONG_PRESS_MS 1000U
#define SLOW_PERIOD_MS 800U
#define FAST_PERIOD_MS 200U
#define DEBOUNCE_MS 20U
#define DISPLAY_TIMEOUT_MS 4000U

/* Status LED on PC13 is active-low on the Black Pill board */
#define LED_OFF_STATE GPIO_PIN_SET

I2C_HandleTypeDef hi2c1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void button_init(uint32_t now);
static BtnEvent_t button_poll(uint32_t now);
static void led_tick(LedMode_t mode, uint32_t now);
static void display_update(LedMode_t mode, uint32_t now, bool event);

/**
 * @brief  Application entry point.
 * @retval int
 */
int main(void)
{
   HAL_Init();
   SystemClock_Config();
   MX_GPIO_Init();
   MX_I2C1_Init();

   if (ssd1306_init(&hi2c1) != HAL_OK)
   {
      Error_Handler();
   }

   button_init(HAL_GetTick());
   LedMode_t mode = MODE_OFF;

   while (1)
   {
      uint32_t now = HAL_GetTick();

      BtnEvent_t evt = button_poll(now);
      switch (evt)
      {
      case BTN_EVENT_SHORT_PRESS:
         mode = (mode == MODE_OFF) ? MODE_SLOW : MODE_OFF;
         break;
      case BTN_EVENT_LONG_PRESS:
         mode = MODE_FAST;
         break;
      case BTN_EVENT_NONE:
         break;
      }

      led_tick(mode, now);
      display_update(mode, now, evt != BTN_EVENT_NONE);

      /* Sleep until the next SysTick or button edge wakes us. */
      __WFI();
   }
}

static bool s_button_pressed;
static bool s_last_raw;
static bool s_long_press_fired;
static uint32_t s_last_edge_tick;
static uint32_t s_press_start;

static void button_init(uint32_t now)
{
   s_button_pressed = false;
   s_last_raw = false;
   s_long_press_fired = false;
   s_last_edge_tick = now;
   s_press_start = 0;
}

static BtnEvent_t button_poll(uint32_t now)
{
   bool raw = (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_RESET);

   if (raw != s_last_raw)
   {
      s_last_raw = raw;
      s_last_edge_tick = now;
   }
   else if ((now - s_last_edge_tick) >= DEBOUNCE_MS && raw != s_button_pressed)
   {
      s_button_pressed = raw;

      if (s_button_pressed)
      {
         s_press_start = now;
         s_long_press_fired = false;
      }
      else if (!s_long_press_fired)
      {
         return BTN_EVENT_SHORT_PRESS;
      }
   }

   if (s_button_pressed && !s_long_press_fired && (now - s_press_start) >= LONG_PRESS_MS)
   {
      s_long_press_fired = true;
      return BTN_EVENT_LONG_PRESS;
   }

   return BTN_EVENT_NONE;
}

static void led_tick(LedMode_t mode, uint32_t now)
{
   static LedMode_t prev_mode = MODE_OFF;
   static uint32_t last_toggle = 0;

   if (mode != prev_mode)
   {
      prev_mode = mode;
      last_toggle = now;
      if (mode == MODE_OFF)
      {
         HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, LED_OFF_STATE);
      }
      return;
   }

   if (mode == MODE_OFF)
   {
      return;
   }

   uint32_t period = (mode == MODE_FAST) ? FAST_PERIOD_MS : SLOW_PERIOD_MS;

   if ((now - last_toggle) >= period)
   {
      HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
      last_toggle = now;
   }
}

static void display_update(LedMode_t mode, uint32_t now, bool event)
{
   static bool on = false;
   static uint32_t last_active;

   if (event)
   {
      const char *text;
      switch (mode)
      {
      case MODE_SLOW: text = "LED: SLOW"; break;
      case MODE_FAST: text = "LED: FAST"; break;
      case MODE_OFF:
      default:        text = "LED: OFF";  break;
      }
      ssd1306_clear();
      ssd1306_draw_text(1, 1, text);
      ssd1306_flush();
      if (!on)
      {
         ssd1306_display_on();
         on = true;
      }
      last_active = now;
      return;
   }

   if (on && (now - last_active) >= DISPLAY_TIMEOUT_MS)
   {
      ssd1306_display_off();
      on = false;
   }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
static void SystemClock_Config(void)
{
   RCC_OscInitTypeDef RCC_OscInitStruct = {0};
   RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

   /* Configure the main internal regulator output voltage */
   __HAL_RCC_PWR_CLK_ENABLE();
   __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

   /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
   RCC_OscInitStruct.HSIState = RCC_HSI_ON;
   RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
   {
      Error_Handler();
   }

   /** Initializes the CPU, AHB and APB buses clocks
    */
   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
   {
      Error_Handler();
   }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

   /* GPIO Ports Clock Enable */
   __HAL_RCC_GPIOC_CLK_ENABLE();
   __HAL_RCC_GPIOH_CLK_ENABLE();
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();

   /* Drive the LED to its off state before enabling the output */
   HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, LED_OFF_STATE);

   /* Configure GPIO pin : STATUS_LED_Pin */
   GPIO_InitStruct.Pin = STATUS_LED_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(STATUS_LED_GPIO_Port, &GPIO_InitStruct);

   /* Configure GPIO pins : PC14 PC15 */
   GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

   /* Configure GPIO pins : PA1..PA15 (PA0 is the user button, configured below) */
   GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

   /* Configure GPIO pins : PB0 PB1 PB2 PB10
                            PB12 PB13 PB14 PB15
                            PB3 PB4 PB5 PB8 PB9
                            (PB6 and PB7 are driven by HAL_I2C_MspInit) */
   GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9;
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

   /* User button on PA0: EXTI on both edges to wake the CPU from WFI */
   GPIO_InitStruct.Pin = USER_BUTTON_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
   GPIO_InitStruct.Pull = GPIO_PULLUP;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

   HAL_NVIC_SetPriority(USER_BUTTON_EXTI_IRQn, 2, 0);
   HAL_NVIC_EnableIRQ(USER_BUTTON_EXTI_IRQn);
}

/**
 * @brief I2C1 Initialization: 100 kHz standard mode on PB6/PB7.
 */
static void MX_I2C1_Init(void)
{
   hi2c1.Instance             = I2C1;
   hi2c1.Init.ClockSpeed      = 100000;
   hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
   hi2c1.Init.OwnAddress1     = 0;
   hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
   hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
   hi2c1.Init.OwnAddress2     = 0;
   hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
   hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
   if (HAL_I2C_Init(&hi2c1) != HAL_OK)
   {
      Error_Handler();
   }
}

/**
 * @brief  This function is executed on error
 * @retval None
 */
void Error_Handler(void)
{
   /* Add your own implementation to report the HAL error return state */
   __disable_irq();

   while (1)
   {
   }
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
   /* Add your own implementation to report the file name and line number,
      eg: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
