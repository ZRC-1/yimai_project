#include "st7789.h"
#include "stdlib.h"
#include "stdio.h"
#include "dma.h"
#ifdef USE_DMA
#include <string.h>
uint16_t DMA_MIN_SIZE = 16;
/* If you're using DMA, then u need a "framebuffer" to store datas to be displayed.
 * If your MCU don't have enough RAM, please avoid using DMA(or set 5 to 1).
 * And if your MCU have enough RAM(even larger than full-frame size),
 * Then you can specify the framebuffer size to the full resolution below.
 */
#endif

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t cmd)
{
	ST7789_Select();
	ST7789_DC_Clr();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
	ST7789_UnSelect();
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
	ST7789_Select();
	ST7789_DC_Set();

	// split data in small chunks because HAL can't send more than 64K at once

	while (buff_size > 0) {
		uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
		#ifdef USE_DMA
			if (DMA_MIN_SIZE <= buff_size)
			{
             
//                printf("DMA Start. Buff Addr: 0x%08X, Size: %d\r\n", (unsigned int)buff, chunk_size);
//                printf("First 4 bytes: %02X %02X %02X %02X\r\n", buff[0], buff[1], buff[2], buff[3]);
				HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, chunk_size);
                //tiaoshi
//                    printf("DMA CR: 0x%X\r\n", hdma_spi1_tx.Instance->CR); // 应包含EN位（第0位为1）
//                    printf("DMA NDTR: 0x%X\r\n", hdma_spi1_tx.Instance->NDTR); // 应等于chunk_size
//                    printf("SPI CR2: 0x%X\r\n", ST7789_SPI_PORT.Instance->CR2); // 应包含TXDMAEN位（第1位为1）
//                    printf("DMA2 LISR: 0x%X\r\n", DMA2->LISR); // 低优先级中断状态（Stream0-3）
//                    printf("DMA2 HISR: 0x%X\r\n", DMA2->HISR); // 高优先级中断状态（Stream4-7）
//                    printf("SPI1 SR: 0x%X\r\n", hspi1.Instance->SR); // hspi1是SPI1的句柄
                /////////
				while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY)
				{
                  //  printf("%d\n",ST7789_SPI_PORT.hdmatx->State);
                }
			}
			else
				HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
		#else
			HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
		#endif
		buff += chunk_size;
		buff_size -= chunk_size;
	}

	ST7789_UnSelect();
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
	ST7789_Select();
	ST7789_DC_Set();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
	ST7789_UnSelect();
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
	ST7789_WriteCommand(ST7789_MADCTL);	// MADCTL
	switch (m) {
	case 0:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	ST7789_Select();
	uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
	uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;
	
	/* Column Address set */
	ST7789_WriteCommand(ST7789_CASET); 
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}

	/* Row Address set */
	ST7789_WriteCommand(ST7789_RASET);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}
	/* Write to RAM */
	ST7789_WriteCommand(ST7789_RAMWR);
	ST7789_UnSelect();
}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void ST7789_Init(void)
{
	#ifdef USE_DMA

	#endif
	HAL_Delay(25);
    ST7789_RST_Clr();
    HAL_Delay(25);
    ST7789_RST_Set();
    HAL_Delay(50);
		
    ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
  	ST7789_WriteCommand(0xB2);				//	Porch control
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_SetRotation(ST7789_ROTATION);	//	MADCTL (Display Rotation)
	
	/* Internal LCD Voltage generator settings */
    ST7789_WriteCommand(0XB7);				//	Gate Control
    ST7789_WriteSmallData(0x35);			//	Default value
    ST7789_WriteCommand(0xBB);				//	VCOM setting
    ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
    ST7789_WriteCommand(0xC0);				//	LCMCTRL	
    ST7789_WriteSmallData (0x2C);			//	Default value
    ST7789_WriteCommand (0xC2);				//	VDV and VRH command Enable
    ST7789_WriteSmallData (0x01);			//	Default value
    ST7789_WriteCommand (0xC3);				//	VRH set
    ST7789_WriteSmallData (0x12);			//	+-4.45v (defalut +-4.1v for 0x0B)
    ST7789_WriteCommand (0xC4);				//	VDV set
    ST7789_WriteSmallData (0x20);			//	Default value
    ST7789_WriteCommand (0xC6);				//	Frame rate control in normal mode
    ST7789_WriteSmallData (0x0F);			//	Default value (60HZ)
    ST7789_WriteCommand (0xD0);				//	Power control
    ST7789_WriteSmallData (0xA4);			//	Default value
    ST7789_WriteSmallData (0xA1);			//	Default value
	/**************** Division line ****************/

	ST7789_WriteCommand(0xE0);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}

    ST7789_WriteCommand(0xE1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}
    ST7789_WriteCommand (ST7789_INVOFF);		//	Inversion ON
	ST7789_WriteCommand (ST7789_SLPOUT);	//	Out of sleep mode
  	ST7789_WriteCommand (ST7789_NORON);		//	Normal Display on
  	ST7789_WriteCommand (ST7789_DISPON);	//	Main screen turned on	

	HAL_Delay(50);
	ST7789_Fill_Color(BLUE);				//	Fill with Black.
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color)
{
	uint16_t i;
	ST7789_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
	ST7789_Select();

	#ifdef USE_DMA
    uint16_t pix;
    uint16_t size=0;
    uint8_t *temp_buff=malloc(ST7789_WIDTH * BLOCK_ROWS*2);
    for (i = 0; i < ST7789_HEIGHT / BLOCK_ROWS; i++)
    {
        //memset(disp_buf, color, sizeof(disp_buf));
        for (pix = 0; pix < ST7789_WIDTH * BLOCK_ROWS; pix++)
        {
            temp_buff[pix*2] = (color>>8)&0xff; // 直接赋值16位颜色，无需拆分
            temp_buff[pix*2+1] = color&0xff;
        }
        size=ST7789_WIDTH * BLOCK_ROWS*2;
        ST7789_WriteData((uint8_t *)temp_buff, size);
    }
    free(temp_buff);
	#else
		uint16_t j;
		for (i = 0; i < ST7789_WIDTH; i++)
				for (j = 0; j < ST7789_HEIGHT; j++) {
					uint8_t data[] = {color >> 8, color & 0xFF};
					ST7789_WriteData(data, sizeof(data));
				}
	#endif
	ST7789_UnSelect();
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x < 0) || (x >= ST7789_WIDTH) ||
		 (y < 0) || (y >= ST7789_HEIGHT))	return;
	
	ST7789_SetAddressWindow(x, y, x, y);
	uint8_t data[] = {color >> 8, color & 0xFF};
	ST7789_Select();
	ST7789_WriteData(data, sizeof(data));
	ST7789_UnSelect();
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
	if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) ||
		 (yEnd < 0) || (yEnd >= ST7789_HEIGHT))	return;
	ST7789_Select();
	uint16_t i, j;
	ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
	for (i = ySta; i <= yEnd; i++)
		for (j = xSta; j <= xEnd; j++) {
			uint8_t data[] = {color >> 8, color & 0xFF};
			ST7789_WriteData(data, sizeof(data));
		}
	ST7789_UnSelect();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x <= 0) || (x > ST7789_WIDTH) ||
		 (y <= 0) || (y > ST7789_HEIGHT))	return;
	ST7789_Select();
	ST7789_Fill(x - 1, y - 1, x + 1, y + 1, color);
	ST7789_UnSelect();
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        uint16_t color) {
	uint16_t swap;
    uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
		swap = x0;
		x0 = y0;
		y0 = swap;

		swap = x1;
		x1 = y1;
		y1 = swap;
        //_swap_int16_t(x0, y0);
        //_swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
		swap = x0;
		x0 = x1;
		x1 = swap;

		swap = y0;
		y0 = y1;
		y1 = swap;
        //_swap_int16_t(x0, x1);
        //_swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = ABS(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            ST7789_DrawPixel(y0, x0, color);
        } else {
            ST7789_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	ST7789_Select();
	ST7789_DrawLine(x1, y1, x2, y1, color);
	ST7789_DrawLine(x1, y1, x1, y2, color);
	ST7789_DrawLine(x1, y2, x2, y2, color);
	ST7789_DrawLine(x2, y1, x2, y2, color);
	ST7789_UnSelect();
}

/** 
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_Select();
	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawPixel(x0 + x, y0 + y, color);
		ST7789_DrawPixel(x0 - x, y0 + y, color);
		ST7789_DrawPixel(x0 + x, y0 - y, color);
		ST7789_DrawPixel(x0 - x, y0 - y, color);

		ST7789_DrawPixel(x0 + y, y0 + x, color);
		ST7789_DrawPixel(x0 - y, y0 + x, color);
		ST7789_DrawPixel(x0 + y, y0 - x, color);
		ST7789_DrawPixel(x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */

void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    // --------------- 1. 前置边界检查 ---------------
    if (data == NULL) {
        //LOG_ERROR("Image data is NULL");
        return;
    }
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT || w == 0 || h == 0) {
       // LOG_ERROR("Invalid coords: x=%d,y=%d,w=%d,h=%d", x, y, w, h);
        return;
    }

    // 修正超出屏幕的绘制区域
    uint16_t draw_w = (x + w > ST7789_WIDTH) ? (ST7789_WIDTH - x) : w;
    uint16_t draw_h = (y + h > ST7789_HEIGHT) ? (ST7789_HEIGHT - y) : h;
    if (draw_w == 0 || draw_h == 0) return;

    
    #ifdef USE_DMA
    // --------------- 2. 计算8行分块的核心参数 ---------------
    
    uint32_t total_pixels = (uint32_t)draw_w * draw_h;       // 总像素数
    uint32_t block_pixels = (uint32_t)draw_w * BLOCK_ROWS;   // 每块像素数（240*8=1920）
    uint32_t block_bytes = block_pixels * 2;                 // 每块字节数（1920*2=3840）
    uint32_t total_blocks = (draw_h + BLOCK_ROWS - 1) / BLOCK_ROWS; // 总块数（向上取整）

    // --------------- 3. 分配8行所需的小块内存（仅3840字节）---------------
    uint8_t *block_buf = (uint8_t *)malloc(block_bytes);
    if (block_buf == NULL) {
        return;
    }
    // --------------- 4. 统一配置显示窗口+拉低CS ---------------
    ST7789_SetAddressWindow(x, y, x + draw_w - 1, y + draw_h - 1);
    ST7789_Select(); // 全程保持CS拉低，仅最后拉高

    // --------------- 5. 逐块传输8行数据 ---------------
    uint32_t pixel_offset = 0; // 全局像素偏移量（指向当前块起始像素）
    for (uint32_t block = 0; block < total_blocks; block++)
    {
        // 计算当前块的实际行数（最后一块可能不足8行）
        uint16_t current_rows = (block == total_blocks - 1) ? (draw_h - block * BLOCK_ROWS) : BLOCK_ROWS;
        uint32_t current_pixels = (uint32_t)draw_w * current_rows;
        uint32_t current_bytes = current_pixels * 2;

        // --------------- 5.1 填充当前8行块的数据 ---------------
        for (uint32_t i = 0; i < current_pixels; i++)
        {
            if (pixel_offset + i >= total_pixels) break; // 防止像素越界
            // RGB565转字节流（MSB在前，适配ST7789）
            block_buf[i*2+1]   = (data[pixel_offset + i] >> 8) & 0xFF; // 高字节
            block_buf[i*2] = data[pixel_offset + i] & 0xFF;        // 低字节
        }

        // --------------- 5.2 DMA分片传输当前8行块（防溢出）---------------
        uint32_t sent_bytes = 0;
        while (sent_bytes < current_bytes)
        {
            uint16_t chunk = (current_bytes - sent_bytes) > DMA_MAX_SIZE ? 
                              DMA_MAX_SIZE : (uint16_t)(current_bytes - sent_bytes);
            ST7789_WriteData(block_buf + sent_bytes, chunk);
            sent_bytes += chunk;
        }

        // --------------- 5.3 更新像素偏移量 ---------------
        pixel_offset += current_pixels;
    }

    // --------------- 6. 释放内存+恢复硬件状态 ---------------
    free(block_buf);
    block_buf = NULL; // 置空防野指针
    ST7789_UnSelect(); // 拉高CS
    #else
	ST7789_Select();
	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	ST7789_WriteData((uint8_t *)data, sizeof(uint16_t) * w * h);
	ST7789_UnSelect();
    #endif
}




// 函数功能：在ST7789显示屏指定位置绘制图像，输入数据为uint8_t数组（前8字节为配置字节，含图像分辨率）
// 配置字节解析规则（前8字节）：第3-4位（数组下标2-3）为图像宽度（16位大端），第5-6位（数组下标4-5）为图像高度（16位大端）

// 参数说明：

//   x,y - 图像左上角坐标

//   data - 输入的uint8_t数组（前8字节为配置字节，含宽高；后续为RGB565格式字节流，每像素2字节，MSB在前）

// 配置字节解析规则（前8字节）：第3-4位（数组下标2-3）为图像宽度（16位大端），第5-6位（数组下标4-5）为图像高度（16位大端）

// 配置字节解析规则（前8字节）：假设格式为 [宽高相关配置字节1-4] + [宽高相关配置字节5-8]，具体根据实际协议调整

// 此处默认解析规则：字节2-3为图像宽度（16位，大端），字节6-7为图像高度（16位，大端），可根据实际配置调整解析逻辑

// 参数说明：

//   x,y - 图像左上角坐标

//   data - 输入的uint8_t数组（前8字节为配置字节，含宽高；后续为RGB565格式字节流，每像素2字节，MSB在前）

//   x,y - 图像左上角坐标

//   x,y - 图像左上角坐标

//   data - 输入的uint8_t数组（前8字节为配置字节，含图像分辨率；后续为RGB565格式字节流，每像素2字节，MSB在前）

//   data - 输入的uint8_t数组（前8字节为配置字节，后续为RGB565格式字节流，每像素2字节，MSB在前）

void ST7789_DrawImage_Uint8(uint16_t x, uint16_t y, const uint8_t *data)

    // --------------- 1. 前置检查与配置字节解析 ---------------

{
 if (data == NULL) {

        //LOG_ERROR("Image data is NULL");

        return;

    }

    // 解析图像宽高（34位=宽度，56位=高度，16位大端格式）

    uint16_t img_w = (data[2] << 8) | data[3]; // 第3-4位（下标2-3）→ 宽度

    uint16_t img_h = (data[4] << 8) | data[5]; // 第5-6位（下标4-5）→ 高度



    // 分辨率有效性检查

    if (img_w == 0 || img_h == 0) {

        //LOG_ERROR("Invalid image resolution: w=%d, h=%d", img_w, img_h);

        return;

    }



    // 坐标有效性检查

    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {

        // LOG_ERROR("Invalid coords: x=%d,y=%d", x, y);

        return;

    }

    // 跳过前8个配置字节，指向有效图像数据

    const uint8_t *image_data = data + 8;


    // 修正超出屏幕的绘制区域

    uint16_t draw_w = ((x + img_w) > ST7789_WIDTH) ? (ST7789_WIDTH - x) : img_w;

    uint16_t draw_h = ((y + img_h) > ST7789_HEIGHT) ? (ST7789_HEIGHT - y) : img_h;

 if (draw_w == 0 || draw_h == 0) return;



    // 计算实际需要传输的总字节数（每像素2字节）

    uint32_t total_bytes = (uint32_t)draw_w * draw_h * 2;



#ifdef USE_DMA

    // --------------- 2. 计算8行分块的核心参数 ---------------

    // 每块行数（保持原BLOCK_ROWS定义，默认8行）

    uint32_t block_pixels = (uint32_t)draw_w * BLOCK_ROWS;   // 每块像素数（draw_w*8）

    uint32_t block_bytes = block_pixels * 2;                 // 每块字节数（每像素2字节）

 uint32_t total_blocks = (draw_h + BLOCK_ROWS - 1) / BLOCK_ROWS; // 总块数（向上取整）



    // --------------- 3. 分配8行所需的小块内存（仅block_bytes字节）---------------

    uint8_t *block_buf = (uint8_t *)malloc(block_bytes);

    if (block_buf == NULL) {

        return;

    }

    // --------------- 4. 统一配置显示窗口+拉低CS ---------------

    ST7789_SetAddressWindow(x, y, x + draw_w - 1, y + draw_h - 1);

    ST7789_Select(); // 全程保持CS拉低，仅最后拉高



    // --------------- 5. 逐块传输8行数据 ---------------

    uint32_t byte_offset = 0; // 全局字节偏移量（指向当前块起始字节）

    for (uint32_t block = 0; block < total_blocks; block++)

    {

        // 计算当前块的实际行数（最后一块可能不足8行）

        uint16_t current_rows = (block == total_blocks - 1) ? (draw_h - block * BLOCK_ROWS) : BLOCK_ROWS;

        uint32_t current_pixels = (uint32_t)draw_w * current_rows;

        uint32_t current_bytes = current_pixels * 2; // 当前块实际字节数



        // --------------- 5.1 填充当前块的数据（直接拷贝有效图像字节流）---------------

        for (uint32_t i = 0; i < current_bytes; i++)

        {

            if (byte_offset + i >= total_bytes) break; // 防止字节越界

            block_buf[i] = image_data[byte_offset + i]; // 拷贝跳过配置字节后的有效数据

        }



        // --------------- 5.2 DMA分片传输当前块（防溢出）---------------

        uint32_t sent_bytes = 0;

        while (sent_bytes < current_bytes)

        {

            uint16_t chunk = (current_bytes - sent_bytes) > DMA_MAX_SIZE ? 

                              DMA_MAX_SIZE : (uint16_t)(current_bytes - sent_bytes);

            ST7789_WriteData(block_buf + sent_bytes, chunk);

            sent_bytes += chunk;

        }



        // --------------- 5.3 更新字节偏移量 ---------------

        byte_offset += current_bytes;

    }



    // --------------- 6. 释放内存+恢复硬件状态 ---------------

    free(block_buf);

    block_buf = NULL; // 置空防野指针

    ST7789_UnSelect(); // 拉高CS

#else

    // 非DMA模式：直接传输uint8_t字节流

    ST7789_Select();

    // 配置显示窗口（使用实际绘制尺寸）

    ST7789_SetAddressWindow(x, y, x + draw_w - 1, y + draw_h - 1);

    // 传输实际需要的字节数（避免传输超出屏幕区域的数据）

    ST7789_WriteData((uint8_t *)image_data, total_bytes);

    ST7789_UnSelect();

#endif

}




/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_InvertColors(uint8_t invert)
{
	ST7789_Select();
	ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
	ST7789_UnSelect();
}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
	uint32_t i, b, j;
	ST7789_Select();
	ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

	for (i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				uint8_t data[] = {color >> 8, color & 0xFF};
				ST7789_WriteData(data, sizeof(data));
			}
			else {
				uint8_t data[] = {bgcolor >> 8, bgcolor & 0xFF};
				ST7789_WriteData(data, sizeof(data));
			}
		}
	}
	ST7789_UnSelect();
}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
	ST7789_Select();
	while (*str) {
		if (x + font.width >= ST7789_WIDTH) {
			x = 0;
			y += font.height;
			if (y + font.height >= ST7789_HEIGHT) {
				break;
			}

			if (*str == ' ') {
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}
		ST7789_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
	ST7789_UnSelect();
}

//////////////////////
/**
 * @brief  在指定区域写入数字（仿照ST7789_WriteString风格）
 * @param  x: 数字显示起始X坐标
 * @param  y: 数字显示起始Y坐标
 * @param  num: 要显示的整数（支持正负）
 * @param  font: 字体样式（FontDef结构体，与原函数一致）
 * @param  color: 数字显示颜色（RGB565格式）
 * @param  bgcolor: 数字背景颜色（RGB565格式）
 * @param  max_x: 显示区域最大X边界（超出则换行）
 * @param  max_y: 显示区域最大Y边界（超出则停止显示）
 * @retval 无
 * @note   1. 兼容正负整数显示，自动处理负号
 *         2. 严格限制在指定显示区域内，超出则换行/停止
 *         3. 硬件操作风格与ST7789_WriteString完全一致（全程CS拉低）
 */
void ST7789_WriteNumber(uint16_t x, uint16_t y, int32_t num, FontDef font,uint16_t color, uint16_t bgcolor, uint16_t max_x, uint16_t max_y)
{
    // 1. 统一拉低CS（与原函数一致，全程保持选中）
    ST7789_Select();

    // 2. 边界校验：确保显示区域有效
    if (max_x > ST7789_WIDTH) max_x = ST7789_WIDTH;
    if (max_y > ST7789_HEIGHT) max_y = ST7789_HEIGHT;
    if (x >= max_x || y >= max_y || font.width == 0 || font.height == 0) 
        {
        ST7789_UnSelect();
        return;
    }

    // 3. 将数字转换为字符串（处理正负号）
    char num_str[16] = {0}; // 足够存储int32_t范围（-2147483648 ~ 2147483647）
    snprintf(num_str, sizeof(num_str), "%d", num);

    // 4. 逐字符显示数字（逻辑与原字符串函数对齐）
    char *digit = num_str;
    while (*digit) {
        // 检查X边界：超出则换行
        if (x + font.width >= max_x) {
            x = 0; // 换行后X归0
            y += font.height; // Y偏移一行高度
            // 检查Y边界：超出则停止显示
            if (y + font.height >= max_y) {
                break;
            }
            // 跳过换行后的无效字符（此处数字无空格，仅做兼容）
            if (*digit == ' ') {
                digit++;
                continue;
            }
        }

        // 调用原字符显示函数绘制单个数字/负号
        ST7789_WriteChar(x, y, *digit, font, color, bgcolor);
        
        // 更新X坐标（右移一个字体宽度）
        x += font.width;
        // 处理下一个字符
        digit++;
    }

    // 5. 释放CS（与原函数一致）
    ST7789_UnSelect();
}

/**
 * @brief  简化版数字显示函数（默认使用全屏区域，无需指定max_x/max_y）
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  num: 要显示的整数
 * @param  font: 字体样式
 * @param  color: 数字颜色
 * @param  bgcolor: 背景颜色
 * @retval 无
 * @note   适配原字符串函数的极简调用方式，无需关心边界
 */
void ST7789_WriteNumber_Simple(uint16_t x, uint16_t y, int32_t num, FontDef font,uint16_t color, uint16_t bgcolor)
{
    ST7789_WriteNumber(x, y, num, font, color, bgcolor, ST7789_WIDTH, ST7789_HEIGHT);
}
/////////////////////
/** 
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	ST7789_Select();
	uint8_t i;

	/* Check input parameters */
	if (x >= ST7789_WIDTH ||
		y >= ST7789_HEIGHT) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= ST7789_WIDTH) {
		w = ST7789_WIDTH - x;
	}
	if ((y + h) >= ST7789_HEIGHT) {
		h = ST7789_HEIGHT - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		ST7789_DrawLine(x, y + i, x + w, y + i, color);
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	/* Draw lines */
	ST7789_DrawLine(x1, y1, x2, y2, color);
	ST7789_DrawLine(x2, y2, x3, y3, color);
	ST7789_DrawLine(x3, y3, x1, y1, color);
	ST7789_UnSelect();
}

/** 
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	}
	else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	}
	else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		ST7789_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	ST7789_Select();
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);
	ST7789_DrawLine(x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		ST7789_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

		ST7789_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
		ST7789_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}


/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_TearEffect(uint8_t tear)
{
	ST7789_Select();
	ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
	ST7789_UnSelect();
}


/** 
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_Test(void)
{
	ST7789_Fill_Color(WHITE);
	HAL_Delay(1000);
	ST7789_WriteString(10, 20, "Speed Test", Font_11x18, RED, WHITE);
	HAL_Delay(1000);
	ST7789_Fill_Color(CYAN);
    HAL_Delay(500);
	ST7789_Fill_Color(RED);
    HAL_Delay(500);
	ST7789_Fill_Color(BLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(GREEN);
    HAL_Delay(500);
	ST7789_Fill_Color(YELLOW);
    HAL_Delay(500);
	ST7789_Fill_Color(BROWN);
    HAL_Delay(500);
	ST7789_Fill_Color(DARKBLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(MAGENTA);
    HAL_Delay(500);
	ST7789_Fill_Color(LIGHTGREEN);
    HAL_Delay(500);
	ST7789_Fill_Color(LGRAY);
    HAL_Delay(500);
	ST7789_Fill_Color(LBBLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(WHITE);
	HAL_Delay(500);

	ST7789_WriteString(10, 10, "Font test.", Font_16x26, GBLUE, WHITE);
	ST7789_WriteString(10, 50, "Hello Steve!", Font_7x10, RED, WHITE);
	ST7789_WriteString(10, 75, "Hello Steve!", Font_11x18, YELLOW, WHITE);
	ST7789_WriteString(10, 100, "Hello Steve!", Font_16x26, MAGENTA, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawRectangle(30, 30, 100, 100, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledRectangle(30, 30, 50, 50, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawCircle(60, 60, 25, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Cir.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledCircle(60, 60, 25, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Triangle", Font_11x18, YELLOW, BLACK);
	ST7789_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Tri", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
	HAL_Delay(1000);

	//	If FLASH cannot storage anymore datas, please delete codes below.
	ST7789_Fill_Color(WHITE);
//	ST7789_DrawImage(0, 0, 128, 128, (uint16_t *)saber);
	HAL_Delay(3000);
}
