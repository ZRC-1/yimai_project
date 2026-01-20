#include "drv_ft6336.h"
#include <string.h> // For memset
#include "st7789.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
/* Private Variables ---------------------------------------------------------*/
static FT6336_HandleTypeDef *g_hft6336 = NULL;
static pTouchEventCallback g_touch_callback = NULL;

// 触摸点状态管理
static TouchPoint_t g_touch_points[2] = {0};
static volatile uint8_t g_touch_interrupt_flag = 0;
static uint8_t g_last_touch_count = 0;

/* Private Function Prototypes -----------------------------------------------*/
// 注意：这两个函数是私有的，只在.c文件内部使用，所以声明在这里
static FT6336_StatusTypeDef FT6336_ReadReg(FT6336_HandleTypeDef *hft6336, uint8_t reg, uint8_t *data);
static FT6336_StatusTypeDef FT6336_WriteReg(FT6336_HandleTypeDef *hft6336, uint8_t reg, uint8_t data);
static void FT6336_ClearInterrupt(void);

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


/* Exported Functions --------------------------------------------------------*/

/**
 * @brief  Initializes the FT6336 touch controller.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure that contains the configuration information.
 * @retval FT6336_StatusTypeDef status
 */
FT6336_StatusTypeDef FT6336_Init(FT6336_HandleTypeDef *hft6336)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hft6336 == NULL) return FT6336_ERROR;

    g_hft6336 = hft6336;

    /* 1. Configure GPIOs */
    // RST Pin
    GPIO_InitStruct.Pin = g_hft6336->RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(g_hft6336->RST_Port, &GPIO_InitStruct);
    
    // INT Pin
    GPIO_InitStruct.Pin = g_hft6336->INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Interrupt on falling edge
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // Internal pull-up
    HAL_GPIO_Init(g_hft6336->INT_Port, &GPIO_InitStruct);

    /* Note: NVIC configuration for EXTI interrupt should be done in the main application
       based on the specific INT_Pin. Example for PB1:
       HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
       HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    */
    HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    /* 2. Perform Hardware Reset */
    if (FT6336_Reset(g_hft6336) != FT6336_OK) {
        return FT6336_ERROR;
    }

    /* 3. Verify Device ID */
    if (FT6336_ReadID(g_hft6336) != FT6336_OK) {
        return FT6336_ERROR;
    }

    /* 4. Configure Touch Parameters */
    if (FT6336_Config(g_hft6336) != FT6336_OK) {
        return FT6336_ERROR;
    }
    
    return FT6336_OK;
}

/**
 * @brief  Performs a hardware reset on the FT6336.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @retval FT6336_StatusTypeDef status
 */
FT6336_StatusTypeDef FT6336_Reset(FT6336_HandleTypeDef *hft6336)
{
    HAL_GPIO_WritePin(hft6336->RST_Port, hft6336->RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10); // Keep low for at least 5us, 10ms is safe
    HAL_GPIO_WritePin(hft6336->RST_Port, hft6336->RST_Pin, GPIO_PIN_SET);
    HAL_Delay(200); // Wait for firmware to boot up, requires >100ms

    return FT6336_OK;
}

/**
 * @brief  Reads the vendor ID to verify communication.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @retval FT6336_StatusTypeDef status
 */
FT6336_StatusTypeDef FT6336_ReadID(FT6336_HandleTypeDef *hft6336)
{
    uint8_t vendor_id[3]= {0};
    if (FT6336_ReadReg(hft6336, FT6336_REG_VENDOR_ID_LOW, &vendor_id[0]) != FT6336_OK) {
        return FT6336_ERROR;
    }
		if (FT6336_ReadReg(hft6336, FT6336_REG_VENDOR_ID_MID, &vendor_id[1]) != FT6336_OK) {
        return FT6336_ERROR;
    }
		if (FT6336_ReadReg(hft6336, FT6336_REG_VENDOR_ID_HIGH, &vendor_id[2]) != FT6336_OK) {
        return FT6336_ERROR;
    }
    if ((vendor_id[0]!= FT6336_VENDOR_ID_VALUE_LOW)||(vendor_id[1]!= FT6336_VENDOR_ID_VALUE_MID)||(vendor_id[2]!= FT6336_VENDOR_ID_VALUE_HIGH)) {
        return FT6336_ERROR;
    }
    return FT6336_OK;
}

/**
 * @brief  Configures the FT6336 with default parameters.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @retval FT6336_StatusTypeDef status
 */
FT6336_StatusTypeDef FT6336_Config(FT6336_HandleTypeDef *hft6336)
{
    // Set touch threshold. Higher value = less sensitive.
    if (FT6336_WriteReg(hft6336, FT6336_REG_TH_GROUP, FT6336_DEFAULT_THRESHOLD) != FT6336_OK) {
        return FT6336_ERROR;
    }
    // Set active scan period to control report rate.
    if (FT6336_WriteReg(hft6336, FT6336_REG_PERIODACTIVE, FT6336_SCAN_PERIOD) != FT6336_OK) {
        return FT6336_ERROR;
    }
    
    return FT6336_OK;
}

/**
 * @brief  Registers a callback function for touch events.
 * @param  callback: Pointer to the callback function.
 * @retval None
 */
void FT6336_RegisterCallback(pTouchEventCallback callback)
{
    g_touch_callback = callback;
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
        //printf("Touch %d: LONG PRESS\r\n", i + 1);
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
touch_event_state_t  touch_event[2];
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
        {
             uint32_t press_duration = HAL_GetTick() - touch_state[point_id].press_start_tick;
            // 在抬起时，根据整个过程的标志位，最终判定事件类型
            if (touch_state[point_id].is_moving)
            {
                printf("Touch %d: MOVE (Duration: %d ms)\r\n", touch_point->id, press_duration);
                // touch_event[point_id].event=TOUCH_EVENT_STATE_MOVE;
                // touch_event[point_id].x=touch_point->x;
                // touch_event[point_id].y=touch_point->y;
                // touch_event[point_id].id=touch_point->id;
            }
            else if (touch_state[point_id].is_long_press_triggered)
            {
                // 长按已经打印过，这里可以什么都不做，或者打印一个结束事件
                printf("Touch %d: LONG PRESS (Duration: %d ms)\r\n", touch_point->id, press_duration);
                touch_event[point_id].event=TOUCH_EVENT_STATE_LONG_PRESS;
                touch_event[point_id].x=touch_point->x;
                touch_event[point_id].y=touch_point->y;
                touch_event[point_id].id=point_id;
            }
            else
            {
                printf("Touch %d: SHORT CLICK (Duration: %d ms)\r\n", touch_point->id, press_duration);
                touch_event[point_id].event=TOUCH_EVENT_STATE_SHORT_CLICK;
                touch_event[point_id].x=touch_point->x;
                touch_event[point_id].y=touch_point->y;
                touch_event[point_id].id=point_id;
            }
            
            // 清理该点的所有状态
            memset(&touch_state[point_id], 0, sizeof(TouchState_t));
            break;
        }
      
        default:
        break;
    }
}

/**
 * @brief  This function must be called in the main loop to handle touch events.
 * @param  None
 * @retval None
 */
void FT6336_Process()
{
    if (g_touch_interrupt_flag && g_hft6336)
    {
        g_touch_interrupt_flag = 0; // 清除标志

        uint8_t touch_count = FT6336_Get_Touch_Count(g_hft6336);
        //printf("touch_count=%d\n",touch_count);
        #if 0
        if(touch_count)
        {
            for (int i = 0; i < 2; i++)
            {
                FT6336_Get_Touch_Point(g_hft6336, i + 1, &g_touch_points[i].x, &g_touch_points[i].y, &g_touch_points[i].id,&g_touch_points[i].event);
                if((g_touch_points[i].x>ST7789_WIDTH)||(g_touch_points[i].y>ST7789_HEIGHT)||(g_touch_points[i].event==TOUCH_EVENT_NOTHING))
                {
                    continue;
                }
                printf("x:%d,y:%d,id:%d,event:%d\n",g_touch_points[i].x,g_touch_points[i].y,g_touch_points[i].id,g_touch_points[i].event);
                g_touch_points[i].id = g_touch_points[i].id + 1;
                if(g_touch_points[i].event==TOUCH_EVENT_PRESS)
                {
                    if (g_touch_callback) g_touch_callback(&g_touch_points[i]);
                }
                else if(g_touch_points[i].event==TOUCH_EVENT_CONTACT)
                {
                    if (g_touch_callback) g_touch_callback(&g_touch_points[i]);
                }
                else if(g_touch_points[i].event==TOUCH_EVENT_RELEASE)
                {
                    if (g_touch_callback) g_touch_callback(&g_touch_points[i]);
                    // 清空该点数据
                    memset(&g_touch_points[i], 0, sizeof(TouchPoint_t));
                }
            }
        }
        
        
        #else
        for (int i = 0; i < 2; i++)
        {
            uint8_t current_point_exists = (i < touch_count);
            uint8_t last_point_exists = (i < g_last_touch_count);

            if (current_point_exists && !last_point_exists)
            {
                // 事件：按下
                FT6336_Get_Touch_Point(g_hft6336, i + 1, &g_touch_points[i].x, &g_touch_points[i].y, &g_touch_points[i].id,&g_touch_points[i].event);
                g_touch_points[i].event = TOUCH_EVENT_PRESS;
                //printf("x:%d,y:%d,id:%d,event:%d\n",g_touch_points[i].x,g_touch_points[i].y,g_touch_points[i].id,g_touch_points[i].event);
                g_touch_points[i].id = g_touch_points[i].id + 1;
                //if (g_touch_callback) g_touch_callback(&g_touch_points[i]);
                if (g_touch_callback && g_touch_points[i].event == TOUCH_EVENT_PRESS) g_touch_callback(&g_touch_points[i]);
                

            }
            else if (current_point_exists && last_point_exists)
            {
                // 事件：持续触摸/移动
                uint16_t new_x, new_y;
                FT6336_Get_Touch_Point(g_hft6336, i + 1, &new_x, &new_y,&g_touch_points[i].id,&g_touch_points[i].event);
                g_touch_points[i].id = g_touch_points[i].id + 1;
                //printf("x:%d,y:%d,id:%d,event:%d\n",g_touch_points[i].x,g_touch_points[i].y,g_touch_points[i].id,g_touch_points[i].event);
                // if (new_x != g_touch_points[i].x || new_y != g_touch_points[i].y)
                if ((new_x != g_touch_points[i].x || new_y != g_touch_points[i].y)&&(g_touch_points[i].event == TOUCH_EVENT_CONTACT))
                {
                    g_touch_points[i].x = new_x;
                    g_touch_points[i].y = new_y;
                     g_touch_points[i].id = g_touch_points[i].id + 1;
                   // g_touch_points[i].event = TOUCH_EVENT_CONTACT;
                   // if (g_touch_callback) g_touch_callback(&g_touch_points[i]);
                    if (g_touch_callback && g_touch_points[i].event == TOUCH_EVENT_CONTACT) g_touch_callback(&g_touch_points[i]);
                }
            }
            else if (!current_point_exists && last_point_exists)
            {
                // 事件：抬起
                g_touch_points[i].event = TOUCH_EVENT_RELEASE;
                if (g_touch_callback) g_touch_callback(&g_touch_points[i]);
                // 清空该点数据
                memset(&g_touch_points[i], 0, sizeof(TouchPoint_t));
            }
        }
        g_last_touch_count = touch_count;
        #endif
        // 清除FT6336的中断标志，否则INT引脚会一直为低
        FT6336_ClearInterrupt();
    }
}

/**
 * @brief  The actual IRQ handler, should be called from EXTIx_IRQHandler.
 * @param  None
 * @retval None
 */
void FT6336_IRQHandler(void)
{
    // 只设置标志，具体处理留给主循环
    g_touch_interrupt_flag = 1;
    // 清除EXTI中断 pending bit
    __HAL_GPIO_EXTI_CLEAR_IT(g_hft6336->INT_Pin);
}

/**
 * @brief  Gets the number of current touch points.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @retval Number of touch points (0, 1, or 2).
 */
uint8_t FT6336_Get_Touch_Count(FT6336_HandleTypeDef *hft6336)
{
    uint8_t td_status = 0;
    if (FT6336_ReadReg(hft6336, FT6336_REG_TD_STATUS, &td_status) != FT6336_OK) {
        return 0; // Error, return 0 touches
    }
    return (td_status & 0x0F);
}

/**
 * @brief  Gets the coordinates of a specific touch point.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @param  point_num: The touch point number (1 or 2).
 * @param  x: Pointer to a variable to store the X coordinate.
 * @param  y: Pointer to a variable to store the Y coordinate.
 * @retval FT6336_StatusTypeDef status
 */
FT6336_StatusTypeDef FT6336_Get_Touch_Point(FT6336_HandleTypeDef *hft6336, uint8_t point_num, uint16_t *x, uint16_t *y,uint8_t *id,TouchEvent_t *event)
{
    uint8_t data[4];
    uint8_t base_reg;

    if (point_num == 1) {
        base_reg = FT6336_REG_TOUCH1_XH;
    } else if (point_num == 2) {
        base_reg = FT6336_REG_TOUCH2_XH;
    } else {
        return FT6336_ERROR; // Invalid point number
    }

    // Burst read XH, XL, YH, YL
    if (HAL_I2C_Mem_Read(hft6336->hi2c, hft6336->Address, base_reg, I2C_MEMADD_SIZE_8BIT, data, 4, 100) != HAL_OK) {
        return FT6336_ERROR;
    }

//    *x = ((data[0] & 0x0F) << 8) | data[1];
//    *y = ((data[2] & 0x0F) << 8) | data[3];
    
    *x = 320-(((data[2] & 0x0F) << 8) | data[3]);
    *y = ((data[0] & 0x0F) << 8) | data[1];
    
    *id = (data[2] >> 4) & 0x0F;
    *event = (TouchEvent_t)((data[0] & 0xC0) >> 6);
    return FT6336_OK;
}

/**
 * @brief  Checks if a touch event is detected by reading the interrupt pin.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @retval 1 if touch detected (INT pin is low), 0 otherwise.
 */
uint8_t FT6336_Is_Touch_Detected(FT6336_HandleTypeDef *hft6336)
{
    return (HAL_GPIO_ReadPin(hft6336->INT_Port, hft6336->INT_Pin) == GPIO_PIN_RESET);
}

/* Private Functions ---------------------------------------------------------*/

/**
 * @brief  Reads the touch status register to clear the interrupt flag on the FT6336.
 * @param  None
 * @retval None
 */
static void FT6336_ClearInterrupt(void)
{
    uint8_t temp;
    FT6336_ReadReg(g_hft6336, FT6336_REG_TD_STATUS, &temp);
}

/**
 * @brief  Reads a single register from the FT6336.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @param  reg: The register address to read from.
 * @param  data: Pointer to a variable to store the read data.
 * @retval FT6336_StatusTypeDef status
 */
static FT6336_StatusTypeDef FT6336_ReadReg(FT6336_HandleTypeDef *hft6336, uint8_t reg, uint8_t *data)
{
    if (HAL_I2C_Mem_Read(hft6336->hi2c, hft6336->Address, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 100) != HAL_OK) {
        return FT6336_ERROR;
    }
    return FT6336_OK;
}

/**
 * @brief  Writes a single register to the FT6336.
 * @param  hft6336: Pointer to a FT6336_HandleTypeDef structure.
 * @param  reg: The register address to write to.
 * @param  data: The data to write.
 * @retval FT6336_StatusTypeDef status
 */
static FT6336_StatusTypeDef FT6336_WriteReg(FT6336_HandleTypeDef *hft6336, uint8_t reg, uint8_t data)
{
    if (HAL_I2C_Mem_Write(hft6336->hi2c, hft6336->Address, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) != HAL_OK) {
        return FT6336_ERROR;
    }
    return FT6336_OK;
}




void FT6336_circulation(void)
{
    FT6336_Process();
    FT6336_LONGPRESS_DETECTION(touch_state);
}

void drv_FT6336_init(void)
{
    if (FT6336_Init(&hft6336) != FT6336_OK)
    {
        Error_Handler();
    }
    FT6336_RegisterCallback(MyTouchCallback);
    HAL_Delay(10);
}
