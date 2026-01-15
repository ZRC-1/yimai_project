#ifndef __FT6336_H
#define __FT6336_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "drv_ft6336.h"
#include "stm32f4xx_hal.h" // 请根据您的STM32系列修改此头文件
#include "i2c.h"

/* Exported Types ------------------------------------------------------------*/
/**
 * @brief  FT6336 Status Definition
 */
typedef enum {
    FT6336_OK = 0x00,
    FT6336_ERROR = 0x01
} FT6336_StatusTypeDef;

/**
 * @brief  FT6336 Handle Structure definition
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;      /*!< Pointer to a I2C_HandleTypeDef structure that contains the configuration information for the specified I2C. */
    uint16_t Address;             /*!< FT6336 Device I2C Address (This is the 7-bit address) */

    GPIO_TypeDef *RST_Port;       /*!< Reset GPIO Port */
    uint16_t RST_Pin;             /*!< Reset GPIO Pin */

    GPIO_TypeDef *INT_Port;       /*!< Interrupt GPIO Port */
    uint16_t INT_Pin;             /*!< Interrupt GPIO Pin */
} FT6336_HandleTypeDef;




// 用于实现状态跟踪的结构体
typedef struct {
    uint16_t press_start_x;       // 按下时的初始X坐标
    uint16_t press_start_y;       // 按下时的初始Y坐标
    uint32_t press_start_tick;    // 按下时的时刻
	uint8_t is_touching;
    uint8_t is_long_press_triggered; // 是否已触发长按
    uint8_t is_moving;            // 是否已经进入移动状态
} TouchState_t;
/**
 * @brief  Touch Event Type Definition
 */
//typedef enum {
//    TOUCH_EVENT_PRESS = 0x01,       /*!< A new touch point is detected (按下) */
//    TOUCH_EVENT_RELEASE = 0x02,     /*!< A touch point is released (抬起) */
//    TOUCH_EVENT_CONTACT = 0x04      /*!< A touch point is still touching (持续触摸/移动) */
//} TouchEvent_t;

typedef enum 
{
    TOUCH_EVENT_NOTHING =0X00,
    TOUCH_EVENT_PRESS = 0x01,       /*!< A new touch point is detected (按下) */
    TOUCH_EVENT_CONTACT = 0x02,      /*!< A touch point is still touching (持续触摸/移动) */
    TOUCH_EVENT_RELEASE = 0x03,     /*!< A touch point is released (抬起) */
} TouchEvent_t;

/**
 * @brief  Touch Point Data Structure
 */
typedef struct {
    uint16_t x;
    uint16_t y;
    TouchEvent_t event;
    uint8_t id; // Touch point ID (1 or 2)
} TouchPoint_t;

/**
 * @brief  Touch Event Callback Function Pointer
 */
typedef void (*pTouchEventCallback)(TouchPoint_t* touch_point);


/* Exported Constants --------------------------------------------------------*/
#define FT6336_I2C_ADDR_GND      0x38U<<1 /*!< I2C Address when ADDR pin is connected to GND */
#define FT6336_I2C_ADDR_VCC      0x39U /*!< I2C Address when ADDR pin is connected to VCC */

/* FT6336 Register Map */
#define FT6336_REG_VENDOR_ID_HIGH 	0xA3U
#define FT6336_REG_VENDOR_ID_MID  	0x9FU
#define FT6336_REG_VENDOR_ID_LOW 	  0xA0U
#define FT6336_REG_FIRMWARE_ID   	  0xA6U
#define FT6336_REG_DEVICE_MODE   	  0x00U
#define FT6336_REG_TD_STATUS     	  0x02U
#define FT6336_REG_TOUCH1_XH   	    0x03U
#define FT6336_REG_TOUCH1_XL   	    0x04U
#define FT6336_REG_TOUCH1_YH   	    0x05U
#define FT6336_REG_TOUCH1_YL    	  0x06U
#define FT6336_REG_TOUCH2_XH      	0x09U
#define FT6336_REG_TOUCH2_XL   	    0x0AU
#define FT6336_REG_TOUCH2_YH   	    0x0BU
#define FT6336_REG_TOUCH2_YL        0x0CU
#define FT6336_REG_TH_GROUP    	    0x80U /*!< Touch threshold */
#define FT6336_REG_PERIODACTIVE  	  0x88U /*!< Active scan period */



//config 
#define TOUCH_LONG_PRESS_MS   800U  // 长按时间阈值 (毫秒)
#define MOVE_THRESHOLD_PX     10U   // 移动距离阈值 (像素)
#define FT6336_VENDOR_ID_VALUE_HIGH    0x64U
#define FT6336_VENDOR_ID_VALUE_MID     0x26U
#define FT6336_VENDOR_ID_VALUE_LOW     0x02U
#define FT6336_DEFAULT_THRESHOLD  		 0x22U   /* Default touch threshold */ //BB为默认值，按需调整
#define FT6336_SCAN_PERIOD   					 0X0A    //100MS
/* Exported Macro ------------------------------------------------------------*/


/* Exported Functions Prototypes ---------------------------------------------*/
FT6336_StatusTypeDef FT6336_Init(FT6336_HandleTypeDef *hft6336);
FT6336_StatusTypeDef FT6336_Reset(FT6336_HandleTypeDef *hft6336);
FT6336_StatusTypeDef FT6336_ReadID(FT6336_HandleTypeDef *hft6336);
FT6336_StatusTypeDef FT6336_Config(FT6336_HandleTypeDef *hft6336);

uint8_t FT6336_Get_Touch_Count(FT6336_HandleTypeDef *hft6336);
FT6336_StatusTypeDef FT6336_Get_Touch_Point(FT6336_HandleTypeDef *hft6336, uint8_t point_num, uint16_t *x, uint16_t *y,uint8_t *id,TouchEvent_t *event);
uint8_t FT6336_Is_Touch_Detected(FT6336_HandleTypeDef *hft6336);

// Event-driven functions
void FT6336_RegisterCallback(pTouchEventCallback callback);
void FT6336_Process(void); // 必须在主循环中调用此函数
void FT6336_IRQHandler(void); // 必须在EXTI中断服务函数中调用

#ifdef __cplusplus
}
#endif

#endif /* __FT6336_H */
