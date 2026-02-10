#ifndef __FONT_H
#define __FONT_H

#include "stdint.h"

typedef struct {
    const uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;

//Font lib.
extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;
extern FontDef Font_Chinese_Single_16x26;
// 单个汉字索引映射表（与字库一一对应）
extern const char* chinese_single_list[];
#define SINGLE_CHINESE_COUNT   38//(sizeof(chinese_single_list) / sizeof(chinese_single_list[0])) // 单个汉字总数
//16-bit(RGB565) Image lib.
/*******************************************
 *             CAUTION:
 *   If the MCU onchip flash cannot
 *  store such huge image data,please
 *           do not use it.
 * These pics are for test purpose only.
 *******************************************/

/* 128x128 pixel RGB565 image */
//extern const uint16_t saber[][128];

// 240x240 pixel RGB565 image 
//extern const uint16_t knky[][240];
//extern const uint16_t tek[][240];
//extern const uint16_t adi1[][240];
extern const unsigned char gImage_home[153608];
extern const unsigned char gImage_config[153608];
#endif
