#ifndef __BRAIN_APP_H
#define __BRAIN_APP_H


#include "brain_app.h"
#include "st7789.h"
#include "fonts.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "usart.h"
#include "drv_ft6336.h"
#include "string.h"
#include "main.h"

static uint16_t acupoint_high_frequence_buff[2]={40,100};
static uint8_t acupoint_low_frequence_buff[3]={2,10,15};
static uint8_t acupoint_frequence_buff[5]={2,10,15,40,100};
static uint8_t acupoint_alternate_time_buff[5]={1,3,5,7,10};
typedef enum
{
    APP_STATE_IDLE = 0,
    APP_STATE_CONFIG_HOME,
    APP_STATE_CONFIG_PAGE,
    APP_STATE_RUNNING,
    APP_STATE_PAUSE,
    APP_STATE_ERROR
} APP_runmode_E;
typedef enum
{
    ACUPOINT_taiyang = 0,
    ACUPOINT_yintang,
    ACUPOINT_baihui,
    ACUPOINT_shenting,
    ACUPOINT_fengchi,
    ACUPOINT_sishencong,
    ACUPOINT_count_MAX,
} acupoint_id_e;
typedef enum
{
    STATE_OFF = 0,
    STATE_ON,
}acupoint_state_e;
typedef enum
{
    MODE_single = 0,
    MODE_conbination,
}acupoint_mode_e;
typedef enum
{
    single_left = 0,
    single_right,
    all,
}acupoint_ch_choice_e;
typedef enum
{
    FREQUENCY_40HZ = 0,
    FREQUENCY_100HZ = 1, 
}acupoint_high_frequence_e;
typedef enum
{
    FREQUENCY_2HZ = 0,
    FREQUENCY_10HZ = 1,
    FREQUENCY_15HZ = 2, 
}acupoint_low_frequence_e;
typedef enum
{
    FREQUENCY_2HZ_  = 0,
    FREQUENCY_10HZ_ = 1,
    FREQUENCY_15HZ_ = 2, 
    FREQUENCY_40HZ_ = 3,
    FREQUENCY_100HZ_ = 4,
}acupoint_frequence_e;
typedef enum
{
    ALTERNATE_1min = 0,
    ALTERNATE_3min = 1,
    ALTERNATE_5min = 2,
    ALTERNATE_7min = 3,
    ALTERNATE_10min = 4,
}alternate_time_e;
typedef struct 
{
    uint8_t minutes;
    uint8_t seconds;
}remaining_time_t;
typedef struct
{
    acupoint_id_e name;
    acupoint_state_e state;
    acupoint_mode_e mode;
    acupoint_high_frequence_e high_frequency;
    acupoint_low_frequence_e low_frequency;
    acupoint_frequence_e frequency;
    acupoint_ch_choice_e ch_choice;
    alternate_time_e alternating_time; // ms
    alternate_time_e alternating_time_2; // ms
    uint8_t  strength;         // 1-15
    uint16_t duration_time;    // min
    remaining_time_t remian_time;
} acupoint_t;

// 基础LCD区域坐标结构体
 typedef struct {
    int x_min;  // x轴下限（左）
    int x_max;  // x轴上限（右）
    int y_min;  // y轴下限（上）
    int y_max;  // y轴上限（下）
} LCD_Area;

// 整合所有区域的大结构体（序号从0开始）
typedef struct {
    // 区域0-5：穴位名称区域
    LCD_Area temple_name;        // 0: 太阳穴名称
    LCD_Area yintang_name;       // 1: 印堂穴名称
    LCD_Area shenting_name;      // 2: 神庭穴名称
    LCD_Area baihui_name;        // 3: 百会穴名称
    LCD_Area fengchi_name;       // 4: 风池穴名称
    LCD_Area sishencong_name;    // 5: 四神聪穴名称

    // 区域6-11：穴位状态区域
    LCD_Area temple_status;      // 6: 太阳穴-禁用
    LCD_Area yintang_status;     // 7: 印堂穴-禁用
    LCD_Area shenting_status;    // 8: 神庭穴-开启
    LCD_Area baihui_status;      // 9: 百会穴-禁用
    LCD_Area fengchi_status;     // 10: 风池穴-禁用
    LCD_Area sishencong_status;  // 11: 四神聪穴-禁用

    // 区域12-14：底部蓝色按钮区域
    LCD_Area btn_start;          // 12: 开始按钮
    LCD_Area btn_end;            // 13: 结束按钮
    LCD_Area btn_pause;          // 14: 暂停按钮
    LCD_Area remaining_time_name;     // 15: 剩余时间显示窗口
    LCD_Area remaining_time_min;        // 16: 剩余分钟显示
    LCD_Area remaining_time_minname;    // 16: 剩余时间数值显示
    LCD_Area remaining_time_sec;        // 17: 剩余秒数显示
    LCD_Area remaining_time_secnane;    // 17: 剩余时间数值显示
} All_LCD_Areas;

// 整合所有区域的大结构体（移除无需显示的文字区域，序号重新整理）
typedef struct {
    // 0-3 顶部核心功能显示区域
    LCD_Area acupoint_name;         // 0: 太阳穴标题 (顶部左侧主标题)
    LCD_Area single_freg;         // 1: "单频"模式显示 (标题右侧)
    LCD_Area multi_freq;          // 2: "多频"模式显示 (单/组合右侧)
    LCD_Area freq_high_value;     // 3: 高频值显示 (模式右侧)
    LCD_Area freq_low_value;      // 4: 低频值显示 (高频右侧)

    // 4-7 交替与强度数值显示区域
    LCD_Area alternate_value;    // 5: 交替数值显示 (第二行左侧)
    LCD_Area alternate_left;     // 6: "单左"文字 (交替数值右侧)
    LCD_Area alternate_right;    // 7: "单右"文字 (单左右侧)
    LCD_Area strength_value;     // 8: 强度数值显示 (单右右侧)

    // 8-12 时长与操作按钮区域
    LCD_Area time_value;         // 9: 时长数值显示 (第三行左侧)
    LCD_Area btn_minus;          // 10: "-"减少按钮 (时长数值右侧)
    LCD_Area btn_plus;           // 11: "+"增加按钮 (减号按钮右侧)
    LCD_Area btn_main_menu;      // 12: 主菜单按钮 (加号按钮右侧)
    LCD_Area high_freq_name;     // 13:高频名字 (最右侧)
    LCD_Area low_freq_name;      // 14:低频名字 (最右侧)
    LCD_Area alternate_value_2;  // 15:交替数值2 (最右侧)
} All_LCD_Areas_Page;


extern APP_runmode_E runmode;
extern acupoint_t acupoint_list[6];

extern void brain_app_init(acupoint_t *acupoint_list);
extern void brain_driver_init(void);    
extern void brain_app_circulation(void);
extern void brain_driver_circulation(void);


#endif
