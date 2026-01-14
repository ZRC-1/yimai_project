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

FT6336_HandleTypeDef hft6336=
{
	.hi2c      = &hi2c1,       // 使用 I2C1
	.Address   = FT6336_I2C_ADDR_GND,	// 使用地址 0x38 (最常见)
	.RST_Port  = GPIOG,
	.RST_Pin   = GPIO_PIN_15,
	.INT_Port  = GPIOB,
	.INT_Pin   = GPIO_PIN_1, // 假设INT引脚连接到PB1
};
TouchState_t touch_state[2] = {0}; // 支持两个触摸点
void MyTouchCallback(TouchPoint_t* touch_point);
void FT6336_LONGPRESS_DETECTION(TouchState_t touch_state[2]);
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

uint16_t color_test=0;    
/* Private user code ---------------------------------------------------------*/


/* USER CODE END 0 */
/* USER CODE BEGIN 4 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/
    
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    #ifdef USE_DMA
    MX_DMA_Init();
    #endif
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    /////////////////
///////////////////////////////////
    ST7789_Init();
    ST7789_Test();
    ST7789_Fill_Color(BLACK);
    
	ST7789_DrawImage(0, 0, 128, 128, (uint16_t *)saber);
	HAL_Delay(3000);

    ST7789_Fill_Color(BLACK);
    ST7789_DrawImage(0, 0, 240, 240, (uint16_t *)knky);
    HAL_Delay(1000);
    ST7789_DrawImage(0, 0, 240, 240, (uint16_t *)tek);
    HAL_Delay(1000);
    ST7789_DrawImage(0, 0, 240, 240, (uint16_t *)adi1);
    HAL_Delay(1000);
    // I2C_Scanner();
    // 3. 调用初始化函数
    if (FT6336_Init(&hft6336) != FT6336_OK)
    {
        Error_Handler();
    }
    FT6336_RegisterCallback(MyTouchCallback);
    
      HAL_Delay(10);
      //I2C_Scanner();
      /* USER CODE BEGIN WHILE */
      while (1)
      {
        FT6336_Process();
        FT6336_LONGPRESS_DETECTION(touch_state);
        HAL_Delay(10);
        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */
      }
    /* USER CODE END 3 */
}

//长按检测函数
void FT6336_LONGPRESS_DETECTION(TouchState_t touch_state[2])
{
  for (int i = 0; i < 2; i++)
  {
    // 检查条件：正在触摸 && 尚未移动 && 尚未触发长按
    if (touch_state[i].is_touching && !touch_state[i].is_moving && !touch_state[i].is_long_press_triggered)
    {
      // 检查是否超过长按时间阈值
      if ((HAL_GetTick() - touch_state[i].press_start_tick) > TOUCH_LONG_PRESS_MS)
      {
              // 触发长按事件
              printf("Touch %d: LONG PRESS\r\n", i + 1);
              touch_state[i].is_long_press_triggered = 1; // 置1，防止重复触发
      }
    }
  }
}

/**
 * @brief  用户自定义的触摸事件回调函数 (包含所有逻辑)
 * @param  touch_point: 包含触摸点信息的结构体
 * @retval None
 */
void MyTouchCallback(TouchPoint_t* touch_point)
{
    if (touch_point->id > 2) return;
    uint8_t point_id = touch_point->id - 1;

    switch (touch_point->event)
    {
        case TOUCH_EVENT_PRESS:
        // 记录按下时的所有初始状态
        touch_state[point_id].press_start_x = touch_point->x;
        touch_state[point_id].press_start_y = touch_point->y;
        touch_state[point_id].press_start_tick = HAL_GetTick();
        touch_state[point_id].is_touching = 1;
        touch_state[point_id].is_long_press_triggered = 0;
        touch_state[point_id].is_moving = 0;
        printf("Touch %d: Pressed at (%d, %d)\r\n", touch_point->id, touch_point->x, touch_point->y);
        break;

        case TOUCH_EVENT_CONTACT:
        // 如果还未进入移动状态，检查移动距离
        if (!touch_state[point_id].is_moving)
        {
            int dx = abs(touch_point->x - touch_state[point_id].press_start_x);
            int dy = abs(touch_point->y - touch_state[point_id].press_start_y);
            
            if (dx > MOVE_THRESHOLD_PX || dy > MOVE_THRESHOLD_PX)
            {
                // 超过阈值，判定为移动开始
                touch_state[point_id].is_moving = 1;
                printf("Touch %d: MOVE STARTED\r\n", touch_point->id);
            }
        }
        break;

        case TOUCH_EVENT_RELEASE:
        // 在抬起时，根据整个过程的标志位，最终判定事件类型
        if (touch_state[point_id].is_moving)
        {
            printf("Touch %d: MOVE ENDED\r\n", touch_point->id);
        }
        else if (touch_state[point_id].is_long_press_triggered)
        {
            // 长按已经打印过，这里可以什么都不做，或者打印一个结束事件
            printf("Touch %d: LONG PRESS RELEASED\r\n", touch_point->id);
        }
        else
        {
            uint32_t press_duration = HAL_GetTick() - touch_state[point_id].press_start_tick;
            printf("Touch %d: SHORT CLICK (Duration: %d ms)\r\n", touch_point->id, press_duration);
            color_test=color_test+2000;
            if(color_test>65535)color_test=0;
            ST7789_Fill_Color(color_test);
            ST7789_WriteNumber_Simple(120, 100, color_test, Font_16x26, RED, WHITE); 
        }
        
        // 清理该点的所有状态
        memset(&touch_state[point_id], 0, sizeof(TouchState_t));
        break;

        default:
        break;
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
