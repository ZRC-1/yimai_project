/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "gpio.h"
#include "st7789.h"
#include "fonts.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "usart.h"
#include "drv_ft6336.h"
#include "string.h"
#include "brain_app.h"
#include "stm32f4xx_hal.h"          // 基础HAL库（必加）
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 重写fputc函数：将printf输出到USART1
#pragma import(__use_no_semihosting)
struct __FILE {
    int handle;
};
FILE __stdout;
void _sys_exit(int x) {
    x = x; // 空实现，避免链接错误
}
int fputc(int ch, FILE *f)
{
    // 阻塞式发送1个字节到串口，超时时间设为最大
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
void I2C_Scanner(void)
{
    HAL_StatusTypeDef status;
    uint8_t i;
    printf("Starting I2C Scanning...\r\n");

    for (i = 1; i < 128; i++)
    {
        //       ?    ??  
        status = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 3, 10); // ?   ?    1λ

        if (status == HAL_OK)
        {
            printf("Device found at address: 0x%02X (7-bit)\r\n", i);
        }
    }
    printf("Scanning finished.\r\n");
}
 

/* Private user code ---------------------------------------------------------*/


/* USER CODE END 0 */
/* USER CODE BEGIN 4 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI1_Init();
  #ifdef USE_DMA
  MX_DMA_Init();
  #endif
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  brain_driver_init();
  brain_app_init(acupoint_list);
  
  while (1)
  {
    brain_app_circulation();
    brain_driver_circulation(); 
    //HAL_Delay(10);
  }
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
      RCC_OscInitTypeDef RCC_OscInitStruct = {0};
      RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

      /** Configure the main internal regulator output voltage
      */
      __HAL_RCC_PWR_CLK_ENABLE();
      __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

      /** Initializes the RCC Oscillators according to the specified parameters
      * in the RCC_OscInitTypeDef structure.
      */
      RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
      RCC_OscInitStruct.HSIState = RCC_HSI_ON;
      RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
      RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
      RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
      RCC_OscInitStruct.PLL.PLLM = 8;
      RCC_OscInitStruct.PLL.PLLN = 160;
      RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
      RCC_OscInitStruct.PLL.PLLQ = 4;
      if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
      {
        Error_Handler();
      }

      /** Initializes the CPU, AHB and APB buses clocks
      */
      RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
      RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
      RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
      RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
      RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

      if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
      {
        Error_Handler();
      }
}

/* USER CODE BEGIN 4 */

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
    while (1)
    {
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
