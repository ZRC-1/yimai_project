#include "brain_app.h"

extern void ST7789_WriteNumber_Center_Area(LCD_Area *area, int32_t num, FontDef font, uint16_t color, uint16_t bgcolor);
uint8_t end_restart_flag=0;
acupoint_t acupoint_list[6]={0};
acupoint_t *p_acupoint_list=acupoint_list;
APP_runmode_E runmode=APP_STATE_CONFIG_HOME;
LCD_Area *selct_area=NULL;
uint16_t selct_area_number=0;
int32_t start_time_ms=0;
int32_t total_time_ms=0;
uint8_t time_is_update=0;
void start_timing(acupoint_t *p_acupoint)
{
    start_time_ms=HAL_GetTick();
    total_time_ms=p_acupoint->remian_time.minutes*60*1000 + p_acupoint->remian_time.seconds*1000;
}
void update_remaining_time(acupoint_t *p_acupoint)
{
    if(runmode!=APP_STATE_RUNNING)
        return;
    uint32_t elapsed_ms=HAL_GetTick()-start_time_ms;
    uint32_t total_duration_ms=total_time_ms;

    if(elapsed_ms>=total_duration_ms)
    {
        p_acupoint->remian_time.minutes=0;
        p_acupoint->remian_time.seconds=0;
        runmode=APP_STATE_PAUSE;
        end_restart_flag=1;
    }
    else
    {
        uint32_t remaining_ms=total_duration_ms-elapsed_ms;
        if((remaining_ms/1000)/60==p_acupoint->remian_time.minutes &&
           (remaining_ms/1000)%60==p_acupoint->remian_time.seconds)
        {
            return; // 时间未变化，无需更新显示
        }
        else
        {
            time_is_update=1;
            p_acupoint->remian_time.minutes=(remaining_ms/1000)/60;
            p_acupoint->remian_time.seconds=(remaining_ms/1000)%60;
        }
    }
    // uint32_t total_remaining_ms=p_acupoint->remian_time.minutes*60*1000 + p_acupoint->remian_time.seconds*1000;
    // if(elapsed_ms>=total_remaining_ms)
    // {
    //     p_acupoint->remian_time.minutes=0;
    //     p_acupoint->remian_time.seconds=0;
    //     runmode=APP_STATE_PAUSE;
    //     end_restart_flag=1;
    // }
    // else
    // {
    //     uint32_t remaining_ms=total_remaining_ms-elapsed_ms;
    //     if((remaining_ms/1000)/60==p_acupoint->remian_time.minutes &&
    //        (remaining_ms/1000)%60==p_acupoint->remian_time.seconds)
    //     {
    //         return; // 时间未变化，无需更新显示
    //     }
    //     else
    //     {
    //         time_is_update=1;
    //         p_acupoint->remian_time.minutes=(remaining_ms/1000)/60;
    //         p_acupoint->remian_time.seconds=(remaining_ms/1000)%60;
    //     }
    // }
}
// 初始化大结构体（赋值所有区域坐标）
static All_LCD_Areas lcd_all_areas = {
    // 0-5 穴位名称区域
    {20, 110, 20, 50},          // 0: 太阳穴名称（结束于x=110）
    {160, 250, 20, 50},         // 1: 印堂穴名称（结束于x=250）
    {20, 110, 70, 100},         // 2: 神庭穴名称（结束于x=110）
    {160, 250, 70, 100},        // 3: 百会穴名称（结束于x=250）
    {20, 110, 120, 150},        // 4: 风池穴名称（结束于x=110）
    {160, 250, 120, 150},       // 5: 四神聪穴名称（结束于x=250）

    // 6-11 穴位状态区域（从名称结束位置开始，间隙0）
    {110, 150, 20, 50},         // 6: 太阳穴-禁用（开始于x=110，与名称无缝贴合）
    {250, 290, 20, 50},         // 7: 印堂穴-禁用（开始于x=250，与名称无缝贴合）
    {110, 150, 70, 100},        // 8: 神庭穴-开启（开始于x=110）
    {250, 290, 70, 100},        // 9: 百会穴-禁用（开始于x=250）
    {110, 150, 120, 150},       // 10: 风池穴-禁用（开始于x=110）
    {250, 290, 120, 150},       // 11: 四神聪穴-禁用（开始于x=250）

    // 12-14 底部按钮区域
    {10, 50, 180, 220},        // 12: 开始按钮
    {60, 100, 180, 220},       // 13: 结束按钮
    {110, 150, 180, 220},        // 14: 暂停按钮
    //添加剩余时间显示窗口
    {160,300, 165, 190},        // 15: 剩余时间显示窗口
    {160,200, 195, 220},        // 16: 剩余分钟显示
    {200,230, 195, 220},       // 17: 剩余时间数值显示
    {230,270, 195, 220},        // 18: 剩余秒数显示
    {270,300, 195, 220},        // 19: 剩余时间数值显示
};

static All_LCD_Areas_Page lcd_all_areas_page = {
    // 0-3 顶部核心功能显示区域
    {30, 150, 10, 40},           // 0: 太阳穴标题 (顶部灰色框内文字)
    {73, 122, 51, 78},         // 1: "单/组合"模式显示 (绿色"单一"区域)
    {122, 174, 51, 78},         // 2: "多频"模式显示 (单/组合右侧)
    {73, 122, 82, 104},        // 2: 高频值显示 ("高频"右侧的"100")
    {182, 233, 82, 104},        // 3: 低频值显示 ("低频"右侧的"2")

    // 4-7 交替与强度数值显示区域
    {73, 123, 108, 132},        // 4: 交替数值显示 ("交替"右侧的"10")
    {132, 183, 140, 162},        // 5: "单左"文字 (交替数值右侧的"单左")
    {183, 234, 140, 162},        // 6: "单右"文字 (单左右侧的"单右")
    {73, 123, 140, 162},        // 7: 强度数值显示 ("强度"右侧的"12")

    // 8-11 时长与操作按钮区域
    {73, 123, 166, 189},        // 8: 时长数值显示 ("时长"右侧的"5")
    {80, 140, 200, 240},        // 9: "-"减少按钮 (下方蓝色减号按钮)
    {20, 78, 200, 340},        // 10: "+"增加按钮 (下方蓝色加号按钮)
    {219, 293, 188, 223},         // 11: 主菜单按钮 (右下角蓝色"主菜单"按钮)
    {23,55,84,104},             // 12:高频
    {122,174,84,104},             // 13:低频
    {182, 232, 108, 132},        // 14: 交替数值2显示 ("交替"右侧的"10")
};

void brain_driver_init(void)
{
    ST7789_Init();
    // ST7789_Test();
    drv_FT6336_init();
}
void display_home_screen(acupoint_t *acupoint_list)
{
    //ST7789_DrawImage_Uint8(0,0,gImage_home);
    ST7789_Fill_Color(BGCOLOR);
    All_LCD_Areas *areas = &lcd_all_areas;
    //绘制穴位名称区域
    ST7789_WriteChineseStrCenter(areas->temple_name.x_min, areas->temple_name.y_min, areas->temple_name.x_max, areas->temple_name.y_max,"太阳穴", BLACK, WHITE);
    ST7789_WriteChineseStrCenter(areas->yintang_name.x_min, areas->yintang_name.y_min, areas->yintang_name.x_max, areas->yintang_name.y_max, "印堂穴", BLACK, WHITE);
    ST7789_WriteChineseStrCenter(areas->shenting_name.x_min, areas->shenting_name.y_min, areas->shenting_name.x_max, areas->shenting_name.y_max, "神庭穴", BLACK, WHITE);
    ST7789_WriteChineseStrCenter(areas->baihui_name.x_min, areas->baihui_name.y_min, areas->baihui_name.x_max, areas->baihui_name.y_max, "百会穴", BLACK, WHITE);
    ST7789_WriteChineseStrCenter(areas->fengchi_name.x_min, areas->fengchi_name.y_min, areas->fengchi_name.x_max, areas->fengchi_name.y_max, "风池穴", BLACK, WHITE);
    ST7789_WriteChineseStrCenter(areas->sishencong_name.x_min, areas->sishencong_name.y_min, areas->sishencong_name.x_max, areas->sishencong_name.y_max, "四神聪穴", BLACK, WHITE);
    LCD_Area *area=&lcd_all_areas.temple_status;
    for (int i = 0; i < ACUPOINT_count_MAX; i++)
    {
        // 根据穴位状态绘制状态区域
        if (acupoint_list[i].state == STATE_OFF)
        {
            ST7789_WriteChineseStrCenter(area->x_min, area->y_min, area->x_max, area->y_max, "禁用", BLACK, RED);
        }
        else
        {
            ST7789_WriteChineseStrCenter(area->x_min, area->y_min, area->x_max, area->y_max, "开启", BLACK, GREEN);
        }
        area++;
    }
    // 绘制底部按钮区域
    ST7789_WriteChineseStrCenter_nointerval(areas->btn_start.x_min, areas->btn_start.y_min, areas->btn_start.x_max, areas->btn_start.y_max, "开始", BLACK, BLUE);
    ST7789_WriteChineseStrCenter_nointerval(areas->btn_end.x_min, areas->btn_end.y_min, areas->btn_end.x_max, areas->btn_end.y_max, "结束", BLACK, BLUE);
    ST7789_WriteChineseStrCenter_nointerval(areas->btn_pause.x_min, areas->btn_pause.y_min, areas->btn_pause.x_max, areas->btn_pause.y_max, "暂停", BLACK, BLUE);
    ST7789_WriteChineseStrCenter_nointerval(areas->remaining_time_name.x_min, areas->remaining_time_name.y_min, areas->remaining_time_name.x_max, areas->remaining_time_name.y_max, "剩余时长", BLACK, BLUE);
    ST7789_WriteNumber_Center_Area(&areas->remaining_time_min, acupoint_list[0].remian_time.minutes, Font_11x18, BLACK, WHITE);
    ST7789_WriteChineseStrCenter_nointerval(areas->remaining_time_minname.x_min, areas->remaining_time_minname.y_min, areas->remaining_time_minname.x_max, areas->remaining_time_minname.y_max, "分", BLACK, WHITE);
    ST7789_WriteNumber_Center_Area(&areas->remaining_time_sec, acupoint_list[0].remian_time.seconds, Font_11x18, BLACK, WHITE);
    ST7789_WriteChineseStrCenter_nointerval(areas->remaining_time_secnane.x_min, areas->remaining_time_secnane.y_min, areas->remaining_time_secnane.x_max, areas->remaining_time_secnane.y_max, "秒", BLACK, WHITE);
}
void display_config_page(acupoint_t *acupoint)
{
    ST7789_DrawImage_Uint8(0,0,gImage_config);
    All_LCD_Areas_Page *areas_page = &lcd_all_areas_page;
    //显示穴位名称
    switch (acupoint->name)
    {
        case 0: // 太阳穴
        {
            ST7789_WriteChineseStrCenter(areas_page->acupoint_name.x_min, areas_page->acupoint_name.y_min, areas_page->acupoint_name.x_max, areas_page->acupoint_name.y_max, "太阳穴", BLACK, WHITE);
            break;
        }
        case 1: // 印堂穴
        {
            ST7789_WriteChineseStrCenter(areas_page->acupoint_name.x_min, areas_page->acupoint_name.y_min, areas_page->acupoint_name.x_max, areas_page->acupoint_name.y_max, "印堂穴", BLACK, WHITE);
            break;
        }
        case 2: // 神庭穴
        {
            ST7789_WriteChineseStrCenter(areas_page->acupoint_name.x_min, areas_page->acupoint_name.y_min, areas_page->acupoint_name.x_max, areas_page->acupoint_name.y_max, "神庭穴", BLACK, WHITE);
            break;
        }
        case 3: // 百会穴
        {
            ST7789_WriteChineseStrCenter(areas_page->acupoint_name.x_min, areas_page->acupoint_name.y_min, areas_page->acupoint_name.x_max, areas_page->acupoint_name.y_max, "百会穴", BLACK, WHITE);
            break;
        }
        case 4: // 风池穴
        {
            ST7789_WriteChineseStrCenter(areas_page->acupoint_name.x_min, areas_page->acupoint_name.y_min, areas_page->acupoint_name.x_max, areas_page->acupoint_name.y_max, "风池穴", BLACK, WHITE);
            break;
        }
        case 5: // 四神聪穴
        {
            ST7789_WriteChineseStrCenter(areas_page->acupoint_name.x_min, areas_page->acupoint_name.y_min, areas_page->acupoint_name.x_max, areas_page->acupoint_name.y_max, "四神聪穴", BLACK, WHITE);
            break;
        }
        default:
            break;
    }
    
    if(acupoint->mode==MODE_single)
    {
        ST7789_WriteChineseStrCenter_nointerval(areas_page->high_freq_name.x_min, areas_page->high_freq_name.y_min, areas_page->high_freq_name.x_max, areas_page->high_freq_name.y_max, "频率", BLACK, BGCOLOR);
        ST7789_WriteChineseStrCenter(areas_page->single_freg.x_min, areas_page->single_freg.y_min, areas_page->single_freg.x_max, areas_page->single_freg.y_max, "单频", BLACK, GREEN);
        ST7789_WriteChineseStrCenter(areas_page->multi_freq.x_min, areas_page->multi_freq.y_min, areas_page->multi_freq.x_max, areas_page->multi_freq.y_max, "组合", BLACK, GRAY);
        ST7789_WriteNumber_Center_Area(&areas_page->freq_high_value,acupoint_frequence_buff[acupoint->frequency], Font_11x18, BLACK, WHITE);
        
    }
    else if(acupoint->mode==MODE_conbination)
    {
        ST7789_WriteChineseStrCenter_nointerval(areas_page->high_freq_name.x_min, areas_page->high_freq_name.y_min, areas_page->high_freq_name.x_max, areas_page->high_freq_name.y_max, "高频", BLACK, BGCOLOR);
        ST7789_WriteChineseStrCenter_nointerval(areas_page->low_freq_name.x_min, areas_page->low_freq_name.y_min, areas_page->low_freq_name.x_max, areas_page->low_freq_name.y_max, "低频", BLACK, BGCOLOR);
        ST7789_WriteChineseStrCenter_nointerval(areas_page->single_freg.x_min, areas_page->single_freg.y_min, areas_page->single_freg.x_max, areas_page->single_freg.y_max, "单频", BLACK, GRAY);
        ST7789_WriteChineseStrCenter_nointerval(areas_page->multi_freq.x_min, areas_page->multi_freq.y_min, areas_page->multi_freq.x_max, areas_page->multi_freq.y_max, "组合", BLACK, GREEN);
        ST7789_WriteNumber_Center_Area(&areas_page->freq_high_value,acupoint_high_frequence_buff[acupoint->high_frequency], Font_11x18, BLACK, WHITE);
        ST7789_WriteNumber_Center_Area(&areas_page->freq_low_value, acupoint_low_frequence_buff[acupoint->low_frequency], Font_11x18, BLACK, WHITE);
    }
    // ST7789_WriteNumber_Center_Area(&areas_page->freq_high_value,acupoint_high_frequence_buff[acupoint->high_frequency], Font_11x18, BLACK, WHITE);
    // ST7789_WriteNumber_Center_Area(&areas_page->freq_low_value, acupoint_low_frequence_buff[acupoint->low_frequency], Font_11x18, BLACK, WHITE);
    if(acupoint->mode==MODE_single)
    {
        // ST7789_WriteNumber_Center_Area(&areas_page->alternate_value, acupoint_alternate_time_buff[acupoint->alternating_time], Font_11x18, BLACK, GRAY);
        // ST7789_WriteNumber_Center_Area(&areas_page->alternate_value_2, acupoint_alternate_time_buff[acupoint->alternating_time_2], Font_11x18, BLACK, GRAY);
    }
    else
    {
        ST7789_WriteNumber_Center_Area(&areas_page->alternate_value, acupoint_alternate_time_buff[acupoint->alternating_time], Font_11x18, BLACK, WHITE);
        ST7789_WriteNumber_Center_Area(&areas_page->alternate_value_2, acupoint_alternate_time_buff[acupoint->alternating_time_2], Font_11x18, BLACK, WHITE);
    }
    ST7789_WriteNumber_Center_Area(&areas_page->strength_value, acupoint->strength, Font_11x18, BLACK, WHITE);
    ST7789_WriteNumber_Center_Area(&areas_page->time_value, acupoint->duration_time, Font_11x18, BLACK, WHITE);
    if(acupoint->ch_choice==single_left)
    {
        ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GREEN);
        ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GRAY);
    }
    else if(acupoint->ch_choice==single_right)
    {
        ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GRAY);
        ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GREEN);
    }
    else if(acupoint->ch_choice==all)
    {
        ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GREEN);
        ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GREEN);
    }
}
void brain_app_init(acupoint_t *acupoint_list)
{
    // 在此处添加应用初始化代码
    for (int i = 0; i < ACUPOINT_count_MAX; i++)
    {
        acupoint_list[i].name = (acupoint_id_e)i;
        acupoint_list[i].state = STATE_OFF;
        acupoint_list[i].mode = MODE_single;
        acupoint_list[i].high_frequency = FREQUENCY_100HZ;
        acupoint_list[i].low_frequency = FREQUENCY_2HZ;
        acupoint_list[i].frequency = FREQUENCY_10HZ_;
        acupoint_list[i].ch_choice = all;
        acupoint_list[i].alternating_time = ALTERNATE_3min;
        acupoint_list[i].alternating_time_2 = ALTERNATE_3min;
        acupoint_list[i].strength = 1;
        acupoint_list[i].duration_time = 15;
        acupoint_list[i].remian_time.minutes = 15;
        acupoint_list[i].remian_time.seconds = 0;
    }
    display_home_screen(acupoint_list);
}
void app_conifg_home(acupoint_t *acupoint_list)
{
    if(touch_event[0].event!=TOUCH_EVENT_STATE_NONE||touch_event[1].event!=TOUCH_EVENT_STATE_NONE)
    {
        for(uint8_t i=0;i<2;i++)
        {
            if(touch_event[i].event==TOUCH_EVENT_STATE_SHORT_CLICK)
            {
                All_LCD_Areas *areas=&lcd_all_areas;
                // 检测触摸点是否在某个区域内
                LCD_Area *name_area = &lcd_all_areas.temple_name; // 获取穴位名称区域
                // 检测穴位名称区域
                for (int j = 0; j < 6; j++)
                {
                    if (touch_event[i].x >= name_area->x_min && touch_event[i].x <= name_area->x_max &&
                        touch_event[i].y >= name_area->y_min && touch_event[i].y <= name_area->y_max)
                    {
                        // 触摸点在该穴位名称区域内，进入该穴位配置页面
                        runmode = APP_STATE_CONFIG_PAGE;
                        p_acupoint_list=&acupoint_list[j];
                        display_config_page(p_acupoint_list);
                        return;
                    }
                    name_area++;
                }
                name_area=&lcd_all_areas.btn_start;
                // 检测底部按钮区域
                for (int j = 0; j < 3; j++)
                {
                    if (touch_event[i].x >= name_area->x_min && touch_event[i].x <= name_area->x_max &&
                        touch_event[i].y >= name_area->y_min && touch_event[i].y <= name_area->y_max)
                    {
                        // 触摸点在该按钮区域内，执行相应操作
                        if(j==0) // 开始按钮
                        {
                            ST7789_WriteChineseStrCenter_nointerval(areas->btn_start.x_min, areas->btn_start.y_min, areas->btn_start.x_max, areas->btn_start.y_max, "开始", BLACK, GREEN);
                            runmode=APP_STATE_RUNNING;
                            start_timing(&acupoint_list[0]);
                        }
                        
                        else if(j==1) // 结束按钮
                        {
                            // end_restart_flag=1;
                            // runmode=APP_STATE_PAUSE;
                        }
                        else if(j==2) // 暂停按钮
                        {
                            // 暂停操作（根据需求实现）
                            //runmode=APP_STATE_PAUSE;
                        }
                    }
                    name_area++;
                }
            }
            else if(touch_event[i].event==TOUCH_EVENT_STATE_LONG_PRESS)
            {
                LCD_Area *name_area=&lcd_all_areas.temple_status;
                // 检测穴位名称区域
                for (int j = 0; j < 6; j++)
                {
                    if (touch_event[i].x >= name_area->x_min && touch_event[i].x <= name_area->x_max &&
                        touch_event[i].y >= name_area->y_min && touch_event[i].y <= name_area->y_max)
                    {
                        // 触摸点在该穴位名称区域内，切换穴位状态
                        acupoint_list[j].state = (acupoint_list[j].state == STATE_OFF) ? STATE_ON : STATE_OFF;
                        // 更新显示
                        if (acupoint_list[j].state == STATE_OFF)
                        {
                            ST7789_WriteChineseStrCenter(name_area->x_min, name_area->y_min, name_area->x_max, name_area->y_max, "禁用", BLACK, RED);
                        }
                        else
                        {
                            ST7789_WriteChineseStrCenter(name_area->x_min, name_area->y_min, name_area->x_max, name_area->y_max, "开启", BLACK, GREEN);
                        }
                    }
                    name_area++;
                }
            }
            else if(touch_event[i].event==TOUCH_EVENT_STATE_MOVE)
            {
                // 暂不需要处理移动事件
            }
            // 清除该点事件
            touch_event[i].event=TOUCH_EVENT_STATE_NONE;
        }
    }

}
static int jugement_pioint_in_area(touch_event_state_t touch_event, LCD_Area *area)
{
    if (touch_event.x >= area->x_min && touch_event.x <= area->x_max &&
        touch_event.y >= area->y_min && touch_event.y <= area->y_max)
    {
        return 1; // 点在区域内
    }
    return 0; // 点不在区域内
}
void app_conifg_page(acupoint_t *acupoint)
{
    if(touch_event[0].event!=TOUCH_EVENT_STATE_NONE||touch_event[1].event!=TOUCH_EVENT_STATE_NONE)
    {
        All_LCD_Areas_Page *areas_page=&lcd_all_areas_page;
        LCD_Area *area=&lcd_all_areas_page.acupoint_name;
        for(uint8_t i=0;i<2;i++)
        {
            if(touch_event[i].event==TOUCH_EVENT_STATE_SHORT_CLICK)
            {
                if((selct_area!=NULL)&&(jugement_pioint_in_area(touch_event[i], selct_area)==0)
                    &&(jugement_pioint_in_area(touch_event[i], &lcd_all_areas_page.btn_minus)==0)
                    &&(jugement_pioint_in_area(touch_event[i], &lcd_all_areas_page.btn_plus)==0))
                {
                    // 取消选中区域的高亮显示
                    ST7789_WriteNumber_Center_Area(selct_area,selct_area_number, Font_11x18, BLACK,WHITE);
                    selct_area = NULL;
                    selct_area_number = 0;
                }
                
                // 检测触摸点是否在某个区域内
                for (int j = 0; j < 16; j++)
                {
                    if (touch_event[i].x >= area->x_min && touch_event[i].x <= area->x_max &&
                        touch_event[i].y >= area->y_min && touch_event[i].y <= area->y_max)
                    {
                        // 触摸点在该按钮区域内，执行相应操作
                        switch (j)
                        {
                            case 1: // 单频按钮
                            {
                                acupoint->mode=MODE_single;
                                ST7789_WriteChineseStrCenter_nointerval(areas_page->single_freg.x_min, areas_page->single_freg.y_min, areas_page->single_freg.x_max, areas_page->single_freg.y_max, "单频", BLACK, GREEN);
                                ST7789_WriteChineseStrCenter_nointerval(areas_page->multi_freq.x_min, areas_page->multi_freq.y_min, areas_page->multi_freq.x_max, areas_page->multi_freq.y_max, "组合", BLACK, GRAY);
                                ST7789_WriteChineseStrCenter_nointerval(areas_page->high_freq_name.x_min, areas_page->high_freq_name.y_min, areas_page->high_freq_name.x_max, areas_page->high_freq_name.y_max, "频率", BLACK, BGCOLOR);
                                ST7789_WriteNumber_Center_Area(&areas_page->freq_high_value, acupoint_frequence_buff[acupoint->frequency], Font_11x18, BLACK, WHITE);
                                ST7789_Fill(areas_page->freq_low_value.x_min, areas_page->freq_low_value.y_min, areas_page->freq_low_value.x_max, areas_page->freq_low_value.y_max, BGCOLOR);
                                ST7789_Fill(areas_page->low_freq_name.x_min, areas_page->low_freq_name.y_min, areas_page->low_freq_name.x_max, areas_page->low_freq_name.y_max, BGCOLOR);
                                ST7789_Fill(areas_page->alternate_value.x_min, areas_page->alternate_value.y_min, areas_page->alternate_value.x_max, areas_page->alternate_value.y_max, BGCOLOR);
                                ST7789_Fill(areas_page->alternate_value_2.x_min, areas_page->alternate_value_2.y_min, areas_page->alternate_value_2.x_max, areas_page->alternate_value_2.y_max, BGCOLOR);
                                break;
                            }
                            case 2: // 组合按钮
                            {
                                acupoint->mode=MODE_conbination;
                                ST7789_WriteChineseStrCenter_nointerval(areas_page->single_freg.x_min, areas_page->single_freg.y_min, areas_page->single_freg.x_max, areas_page->single_freg.y_max, "单频", BLACK, GRAY);
                                ST7789_WriteChineseStrCenter_nointerval(areas_page->multi_freq.x_min, areas_page->multi_freq.y_min, areas_page->multi_freq.x_max, areas_page->multi_freq.y_max, "组合", BLACK, GREEN);
                                ST7789_WriteNumber_Center_Area(&areas_page->alternate_value, acupoint_alternate_time_buff[acupoint->alternating_time], Font_11x18, BLACK, WHITE);
                                ST7789_WriteNumber_Center_Area(&areas_page->alternate_value_2, acupoint_alternate_time_buff[acupoint->alternating_time_2], Font_11x18, BLACK, WHITE);
                                ST7789_WriteChineseStrCenter_nointerval(areas_page->high_freq_name.x_min, areas_page->high_freq_name.y_min, areas_page->high_freq_name.x_max, areas_page->high_freq_name.y_max, "高频", BLACK, BGCOLOR);
                                ST7789_WriteChineseStrCenter_nointerval(areas_page->low_freq_name.x_min, areas_page->low_freq_name.y_min, areas_page->low_freq_name.x_max, areas_page->low_freq_name.y_max, "低频", BLACK, BGCOLOR);
                                ST7789_WriteNumber_Center_Area(&areas_page->freq_high_value, acupoint_high_frequence_buff[acupoint->high_frequency], Font_11x18, BLACK, WHITE);
                                ST7789_WriteNumber_Center_Area(&areas_page->freq_low_value, acupoint_low_frequence_buff[acupoint->low_frequency], Font_11x18, BLACK, WHITE);
                                break;
                            }
                            case 3: // 高频数值
                            {
                                if(acupoint->mode==MODE_conbination)
                                {
                                    selct_area= &lcd_all_areas_page.freq_high_value;
                                    selct_area_number=acupoint_high_frequence_buff[acupoint->high_frequency];
                                }
                                else if(acupoint->mode==MODE_single)
                                {
                                    selct_area= &lcd_all_areas_page.freq_high_value;
                                    selct_area_number=acupoint_frequence_buff[acupoint->frequency];
                                }
                                break;
                            }
                            case 4: // 低频数值
                            {
                                if (acupoint->mode==MODE_conbination)
                                {
                                    selct_area= &lcd_all_areas_page.freq_low_value;
                                    selct_area_number=acupoint_low_frequence_buff[acupoint->low_frequency];
                                }
                                //ST7789_WriteNumber_Center_Area(&areas_page->freq_low_value, (acupoint->low_frequency+1)*2, Font_11x18, BLACK, RED);
                                break;
                            }
                            case 5: // 交替数值
                            {
                                if(acupoint->mode==MODE_conbination)
                                {
                                    selct_area= &lcd_all_areas_page.alternate_value;
                                    selct_area_number=acupoint_alternate_time_buff[acupoint->alternating_time];
                                }
                                break;
                            }
                            case 6: // 单左按钮
                            {
                                acupoint->ch_choice=single_left;
                                if(acupoint->ch_choice==single_left)
                                {
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GREEN);
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GRAY);
                                }
                                else if(acupoint->ch_choice==single_right)
                                {
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GRAY);
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GREEN);
                                }
                                break;
                            }
                            case 7: // 单右按钮
                            {
                                acupoint->ch_choice=single_right;
                                if(acupoint->ch_choice==single_left)
                                {
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GREEN);
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GRAY);
                                }
                                else if(acupoint->ch_choice==single_right)
                                {
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GRAY);
                                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GREEN);
                                }
                                break;
                            }
                            case 8: // 强度数值
                            {
                                selct_area= &lcd_all_areas_page.strength_value;
                                selct_area_number=acupoint->strength;
                                break;
                            }
                            case 9: // 时长数值
                            {
                                selct_area= &lcd_all_areas_page.time_value;
                                selct_area_number=acupoint->duration_time;
                                break;
                            }
                            case 10: // 减少按钮
                            {
                                if(selct_area!=NULL)
                                {
                                    if(selct_area==&areas_page->freq_high_value)
                                    {
                                        if(acupoint->mode==MODE_single)
                                        {
                                            if(acupoint->frequency>FREQUENCY_2HZ_) acupoint->frequency--;
                                            if(acupoint->frequency<FREQUENCY_2HZ_)
                                            {
                                                acupoint->frequency=FREQUENCY_2HZ_;
                                            }
                                            else if(acupoint->frequency>FREQUENCY_100HZ_)
                                            {
                                                acupoint->frequency=FREQUENCY_100HZ_;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_frequence_buff[acupoint->frequency], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_frequence_buff[acupoint->frequency];
                                        }
                                        else if(acupoint->mode==MODE_conbination)
                                        {   
                                            if(acupoint->high_frequency>FREQUENCY_40HZ) acupoint->high_frequency--;
                                            if(acupoint->high_frequency<FREQUENCY_40HZ)
                                            {
                                                acupoint->high_frequency=FREQUENCY_40HZ;
                                            }
                                            else if(acupoint->high_frequency>FREQUENCY_100HZ)
                                            {
                                                acupoint->high_frequency=FREQUENCY_100HZ;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_high_frequence_buff[acupoint->high_frequency], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_high_frequence_buff[acupoint->high_frequency];
                                        }
                                    }
                                    else if(selct_area==&areas_page->freq_low_value)
                                    {
                                        if(acupoint->mode==MODE_conbination)
                                        {
                                            if(acupoint->low_frequency>FREQUENCY_2HZ) acupoint->low_frequency--;
                                            if(acupoint->low_frequency<FREQUENCY_2HZ)
                                            {
                                                acupoint->low_frequency=FREQUENCY_2HZ;
                                            }
                                            else if(acupoint->low_frequency>FREQUENCY_15HZ)
                                            {
                                                acupoint->low_frequency=FREQUENCY_15HZ;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_low_frequence_buff[acupoint->low_frequency], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_low_frequence_buff[acupoint->low_frequency];
                                        }
                                    }
                                    else if (selct_area==&areas_page->alternate_value)
                                    {
                                        if(acupoint->mode==MODE_conbination)
                                        {
                                            if(acupoint->alternating_time>ALTERNATE_1min) acupoint->alternating_time--;
                                            if(acupoint->alternating_time<ALTERNATE_1min)
                                            {
                                                acupoint->alternating_time=ALTERNATE_1min;
                                            }
                                            else if(acupoint->alternating_time>ALTERNATE_10min)
                                            {
                                                acupoint->alternating_time=ALTERNATE_10min;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_alternate_time_buff[acupoint->alternating_time], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_alternate_time_buff[acupoint->alternating_time];
                                        }
                                    }
                                    else if (selct_area==&areas_page->strength_value)
                                    {
                                        if(acupoint->strength>1) acupoint->strength--;
                                        if(acupoint->strength<1)
                                        {
                                            acupoint->strength=1;
                                        }
                                        else if(acupoint->strength>15)
                                        {
                                            acupoint->strength=15;
                                        }
                                        ST7789_WriteNumber_Center_Area(selct_area,acupoint->strength, Font_11x18, BLACK,RED);
                                        selct_area_number=acupoint->strength;
                                    }
                                    else if (selct_area==&areas_page->time_value)
                                    {
                                        // if(acupoint->duration_time>5) acupoint->duration_time-=5;
                                        // if(acupoint->duration_time<5)
                                        // {
                                        //     acupoint->duration_time=5;
                                        // }
                                        // else if(acupoint->duration_time>45)
                                        // {
                                        //     acupoint->duration_time=45;
                                        // }
                                        // ST7789_WriteNumber_Center_Area(selct_area,acupoint->duration_time, Font_11x18, BLACK,RED);
                                        // selct_area_number=acupoint->duration_time;


                                        //根据要求暂时所有穴位公用相同的时长设置
                                        if(acupoint->duration_time>5) acupoint->duration_time-=5;
                                        if(acupoint->duration_time<5)       
                                        {
                                            acupoint->duration_time=5;
                                        }
                                        else if(acupoint->duration_time>45)
                                        {
                                            acupoint->duration_time=45;
                                        }
                                        // 更新所有穴位的时长设置
                                        for(int idx=0;idx<ACUPOINT_count_MAX;idx++)
                                        {
                                            p_acupoint_list[idx].duration_time=acupoint->duration_time;
                                            p_acupoint_list[idx].remian_time.minutes = acupoint->duration_time;
                                            p_acupoint_list[idx].remian_time.seconds = 0;
                                        }
                                        ST7789_WriteNumber_Center_Area(selct_area,acupoint->duration_time, Font_11x18, BLACK,RED);
                                        selct_area_number=acupoint->duration_time;
                                    }
                                    else if (selct_area==&areas_page->alternate_value_2)
                                    {
                                        if(acupoint->mode==MODE_conbination)
                                        {
                                            if(acupoint->alternating_time_2>ALTERNATE_1min) acupoint->alternating_time_2--;
                                            if(acupoint->alternating_time_2<ALTERNATE_1min)
                                            {
                                                acupoint->alternating_time_2=ALTERNATE_1min;
                                            }
                                            else if(acupoint->alternating_time_2>ALTERNATE_10min)
                                            {
                                                acupoint->alternating_time_2=ALTERNATE_10min;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_alternate_time_buff[acupoint->alternating_time_2], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_alternate_time_buff[acupoint->alternating_time_2];
                                        }
                                    }
                                    
                                }
                                break;
                            }
                            case 11: // 增加按钮
                            {
                                if(selct_area!=NULL)
                                {
                                    if(selct_area==&areas_page->freq_high_value)
                                    {
                                        if(acupoint->mode==MODE_single)
                                        {
                                            acupoint->frequency++;
                                            if(acupoint->frequency<FREQUENCY_2HZ_)
                                            {
                                                acupoint->frequency=FREQUENCY_2HZ_;
                                            }
                                            else if(acupoint->frequency>FREQUENCY_100HZ_)
                                            {
                                                acupoint->frequency=FREQUENCY_100HZ_;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_frequence_buff[acupoint->frequency], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_frequence_buff[acupoint->frequency];
                                        }
                                        else if(acupoint->mode==MODE_conbination)
                                        {   
                                            acupoint->high_frequency++;
                                            if(acupoint->high_frequency<FREQUENCY_40HZ)
                                            {
                                                acupoint->high_frequency=FREQUENCY_40HZ;
                                            }
                                            else if(acupoint->high_frequency>FREQUENCY_100HZ)
                                            {
                                                acupoint->high_frequency=FREQUENCY_100HZ;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_high_frequence_buff[acupoint->high_frequency], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_high_frequence_buff[acupoint->high_frequency];
                                        }
                                    }
                                    else if(selct_area==&areas_page->freq_low_value)
                                    {
                                        if(acupoint->mode==MODE_conbination)
                                        {
                                            acupoint->low_frequency++;
                                            if(acupoint->low_frequency<FREQUENCY_2HZ)
                                            {
                                                acupoint->low_frequency=FREQUENCY_2HZ;
                                            }
                                            else if(acupoint->low_frequency>FREQUENCY_15HZ)
                                            {
                                                acupoint->low_frequency=FREQUENCY_15HZ;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_low_frequence_buff[acupoint->low_frequency], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_low_frequence_buff[acupoint->low_frequency];
                                        }
                                    }
                                    else if (selct_area==&areas_page->alternate_value)
                                    {
                                        if(acupoint->mode==MODE_conbination)
                                        {
                                            acupoint->alternating_time++;
                                            if(acupoint->alternating_time<ALTERNATE_1min)
                                            {
                                                acupoint->alternating_time=ALTERNATE_1min;
                                            }
                                            else if(acupoint->alternating_time>ALTERNATE_10min)
                                            {
                                                acupoint->alternating_time=ALTERNATE_10min;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_alternate_time_buff[acupoint->alternating_time], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_alternate_time_buff[acupoint->alternating_time];
                                        }
                                    }
                                    else if (selct_area==&areas_page->strength_value)
                                    {
                                        acupoint->strength++;
                                        if(acupoint->strength<1)
                                        {
                                            acupoint->strength=1;
                                        }
                                        else if(acupoint->strength>15)
                                        {
                                            acupoint->strength=15;
                                        }
                                        ST7789_WriteNumber_Center_Area(selct_area,acupoint->strength, Font_11x18, BLACK,RED);
                                        selct_area_number=acupoint->strength;
                                    }
                                    else if (selct_area==&areas_page->time_value)
                                    {
                                        // if(acupoint->duration_time>5) acupoint->duration_time-=5;
                                        // if(acupoint->duration_time<5)
                                        // {
                                        //     acupoint->duration_time=5;
                                        // }
                                        // else if(acupoint->duration_time>45)
                                        // {
                                        //     acupoint->duration_time=45;
                                        // }
                                        // ST7789_WriteNumber_Center_Area(selct_area,acupoint->duration_time, Font_11x18, BLACK,RED);
                                        // selct_area_number=acupoint->duration_time;


                                        //根据要求暂时所有穴位公用相同的时长设置
                                        acupoint->duration_time+=5;
                                        if(acupoint->duration_time<5)       
                                        {
                                            acupoint->duration_time=5;
                                        }
                                        else if(acupoint->duration_time>45)
                                        {
                                            acupoint->duration_time=45;
                                        }
                                        // 更新所有穴位的时长设置
                                        for(int idx=0;idx<ACUPOINT_count_MAX;idx++)
                                        {
                                            p_acupoint_list[idx].duration_time=acupoint->duration_time;
                                            p_acupoint_list[idx].remian_time.minutes = acupoint->duration_time;
                                            p_acupoint_list[idx].remian_time.seconds = 0;
                                        }
                                        ST7789_WriteNumber_Center_Area(selct_area,acupoint->duration_time, Font_11x18, BLACK,RED);
                                        selct_area_number=acupoint->duration_time;
                                    }
                                    else if (selct_area==&areas_page->alternate_value_2)
                                    {
                                        if(acupoint->mode==MODE_conbination)
                                        {
                                            acupoint->alternating_time_2++;
                                            if(acupoint->alternating_time_2<ALTERNATE_1min)
                                            {
                                                acupoint->alternating_time_2=ALTERNATE_1min;
                                            }
                                            else if(acupoint->alternating_time_2>ALTERNATE_10min)
                                            {
                                                acupoint->alternating_time_2=ALTERNATE_10min;
                                            }
                                            ST7789_WriteNumber_Center_Area(selct_area,acupoint_alternate_time_buff[acupoint->alternating_time_2], Font_11x18, BLACK,RED);
                                            selct_area_number=acupoint_alternate_time_buff[acupoint->alternating_time_2];
                                        }
                                    }
                                }
                                break;
                            }
                            case 12: // 主菜单按钮
                            {
                                selct_area = NULL;
                                selct_area_number = 0;
                                display_home_screen(acupoint_list);
                                runmode=APP_STATE_CONFIG_HOME;
                                return;
                            }
                            case 15: // 交替数值2
                            {
                                if(acupoint->mode==MODE_conbination)
                                {
                                    selct_area= &lcd_all_areas_page.alternate_value_2;
                                    selct_area_number=acupoint_alternate_time_buff[acupoint->alternating_time_2];
                                }
                            }
                            default:
                                break;
                        }
                    }
                    area++;
                }
            }
            else if(touch_event[i].event==TOUCH_EVENT_STATE_LONG_PRESS)
            {
                // 暂不需要处理长按事件
                if (touch_event[i].x >= areas_page->alternate_left.x_min && touch_event[i].x <= areas_page->alternate_right.x_max &&
                        touch_event[i].y >= areas_page->alternate_left.y_min && touch_event[i].y <= areas_page->alternate_left.y_max)
                {
                    acupoint->ch_choice=all;
                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_left.x_min, areas_page->alternate_left.y_min, areas_page->alternate_left.x_max, areas_page->alternate_left.y_max, "单左", BLACK, GREEN);
                    ST7789_WriteChineseStrCenter_nointerval(areas_page->alternate_right.x_min, areas_page->alternate_right.y_min, areas_page->alternate_right.x_max, areas_page->alternate_right.y_max, "单右", BLACK, GREEN);
                }
            }
            else if(touch_event[i].event==TOUCH_EVENT_STATE_MOVE)
            {
                // 暂不需要处理移动事件
            }
            // 清除该点事件

            if(selct_area!=NULL)
            {
                // 选中区域的高亮显示
                ST7789_WriteNumber_Center_Area(selct_area,selct_area_number, Font_11x18, BLACK,RED);
            }

            touch_event[i].event=TOUCH_EVENT_STATE_NONE;
        }
    }
}
void drv_dac_config(acupoint_t *acupoint)
{
    // 在此处添加DAC配置代码
}

void refresh_display_remaining_time(acupoint_t *acupoint)
{
    All_LCD_Areas *areas=&lcd_all_areas;
    if(time_is_update)
    {
        ST7789_WriteNumber_Center_Area(&areas->remaining_time_min, acupoint->remian_time.minutes, Font_11x18, BLACK, WHITE);
        ST7789_WriteNumber_Center_Area(&areas->remaining_time_sec, acupoint->remian_time.seconds, Font_11x18, BLACK, WHITE);
        time_is_update=0;
    }
}
void drv_dac_circulation(acupoint_t *acupoint)
{
    All_LCD_Areas *areas=&lcd_all_areas;
    update_remaining_time(acupoint);
    refresh_display_remaining_time(acupoint);
    // ST7789_WriteNumber_Center_Area(&areas->remaining_time_min, acupoint_list[0].remian_time.minutes, Font_11x18, BLACK, WHITE);
    // ST7789_WriteNumber_Center_Area(&areas->remaining_time_sec, acupoint_list[0].remian_time.seconds, Font_11x18, BLACK, WHITE);
    // // 在此处添加DAC循环代码
    if(touch_event[0].event!=TOUCH_EVENT_STATE_NONE||touch_event[1].event!=TOUCH_EVENT_STATE_NONE)
    {
        for(uint8_t i=0;i<2;i++)
        {
            if(touch_event[i].event==TOUCH_EVENT_STATE_SHORT_CLICK)
            {
                
                LCD_Area *area=&lcd_all_areas.btn_start;
                // 检测底部按钮区域
                for (int j = 0; j < 3; j++)
                {
                    if (touch_event[i].x >= area->x_min && touch_event[i].x <= area->x_max &&
                        touch_event[i].y >= area->y_min && touch_event[i].y <= area->y_max)
                    {
                        // 触摸点在该按钮区域内，执行相应操作
                        if(j==1) // 结束按钮
                        {
                            ST7789_WriteChineseStrCenter_nointerval(areas->btn_start.x_min, areas->btn_start.y_min, areas->btn_start.x_max, areas->btn_start.y_max, "开始", BLACK, RED);
                            end_restart_flag=1;
                            runmode=APP_STATE_PAUSE;
                        }
                        else if(j==2) // 暂停按钮
                        {
                            ST7789_WriteChineseStrCenter_nointerval(areas->btn_start.x_min, areas->btn_start.y_min, areas->btn_start.x_max, areas->btn_start.y_max, "开始", BLACK, RED);
                            runmode=APP_STATE_PAUSE;
                        }
                        else if(j==0) // 开始按钮
                        {
                            //runmode=APP_STATE_RUNNING;
                        }
                    }
                    area++;
                }
            }
            // 清除该点事件
            touch_event[i].event=TOUCH_EVENT_STATE_NONE;
        }
    }
}
void drv_dac_stop(acupoint_t *acupoint)
{
    // 在此处添加DAC停止代码
    if(end_restart_flag==1)
    {
        end_restart_flag=0;
        acupoint_list[0].remian_time.minutes = acupoint_list[0].duration_time;
        acupoint_list[0].remian_time.seconds = 0;
        display_home_screen(acupoint_list);
        runmode=APP_STATE_CONFIG_HOME;
        return;
    }
    if(touch_event[0].event!=TOUCH_EVENT_STATE_NONE||touch_event[1].event!=TOUCH_EVENT_STATE_NONE)
    {
        for(uint8_t i=0;i<2;i++)
        {
            if(touch_event[i].event==TOUCH_EVENT_STATE_SHORT_CLICK)
            {
                All_LCD_Areas *areas=&lcd_all_areas;
                LCD_Area *area=&lcd_all_areas.btn_start;
                // 检测底部按钮区域
                for (int j = 0; j < 3; j++)
                {
                    if (touch_event[i].x >= area->x_min && touch_event[i].x <= area->x_max &&
                        touch_event[i].y >= area->y_min && touch_event[i].y <= area->y_max)
                    {
                        // 触摸点在该按钮区域内，执行相应操作
                        if(j==0) // 开始按钮
                        {
                            ST7789_WriteChineseStrCenter_nointerval(areas->btn_start.x_min, areas->btn_start.y_min, areas->btn_start.x_max, areas->btn_start.y_max, "开始", BLACK, GREEN);
                            runmode=APP_STATE_RUNNING;
                            start_timing(&acupoint_list[0]);
                        }
                        if (j==1)
                        {
                            /* code */
                            end_restart_flag=0;
                            acupoint_list[0].remian_time.minutes = acupoint_list[0].duration_time;
                            acupoint_list[0].remian_time.seconds = 0;
                            display_home_screen(acupoint_list);
                            runmode=APP_STATE_CONFIG_HOME;
                            return;
                        }
                        
                    }
                    area++;
                }
            }
            // 清除该点事件
            touch_event[i].event=TOUCH_EVENT_STATE_NONE;
        }
    }
}
void brain_app_circulation(void)
{
    // 在此处添加应用初始化代码
    switch (runmode)
    {
        case APP_STATE_IDLE:
        {

            break;
        }       
        case APP_STATE_CONFIG_HOME:
        {
            app_conifg_home(acupoint_list);
            break;
        } 
        case APP_STATE_CONFIG_PAGE:
        {
            app_conifg_page(p_acupoint_list);
            break;
        }
        case APP_STATE_RUNNING:
        {
            drv_dac_config(acupoint_list);
            drv_dac_circulation(acupoint_list);
            break;
        }
        case APP_STATE_PAUSE:
        {
            // 暂停状态处理代码
            drv_dac_stop(acupoint_list);
            break;
        }
        case APP_STATE_ERROR:
        {
            
            break;
        }
        default:
            break;
    }
}
void brain_driver_circulation(void)
{
    FT6336_circulation();
    // 在此处添加驱动初始化代码
}
