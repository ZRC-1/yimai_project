/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
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
#include "dma.h"
#include "spi.h"
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

    
      /* 修复4：初始化SPI1_TX的DMA通道参数 */
  hdma_spi1_tx.Instance = DMA2_Stream3;
  hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3; // SPI1_TX对应DMA2_Stream3_Channel3（必须匹配硬件）
  hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH; // 内存→外设（SPI发送）
  hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE; // 外设地址（SPI DR）不递增
  hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE; // 内存地址递增（遍历缓冲区）
  hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE; // 外设数据宽度8位
  hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE; // 内存数据宽度8位
  hdma_spi1_tx.Init.Mode = DMA_NORMAL; // 普通模式（传输完成后停止）
  hdma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH; // 高优先级，避免中断被阻塞
  hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
  {
    Error_Handler();
  }

  
    // 2. 关键补充：启用DMA传输完成中断（TCIE）+ 错误中断（TEIE）
  // 这是DMA触发中断的硬件开关，之前缺失！
  __HAL_DMA_ENABLE_IT(&hdma_spi1_tx, DMA_IT_TC);  // 传输完成中断
  __HAL_DMA_ENABLE_IT(&hdma_spi1_tx, DMA_IT_TE);  // 传输错误中断（可选，增强鲁棒性）
  /* 修复5：将DMA通道与SPI1_TX绑定 */
  __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);
  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

