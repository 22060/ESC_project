#ifndef __SSD1306_H__
#define __SSD1306_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>
#include "main.h"
#include "cordic_wrapper.hpp"

#define SSD1306_I2C_ADDR (0x3C << 1)


void SSD1306_Init(void);
void SSD1306_SendCommand(uint8_t cmd);
void SSD1306_SendData(uint8_t* data, uint16_t size);
void SSD1306_Clear(void);
void SSD1306_UpdateScreen(void);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void SSD1306_WriteString(uint8_t x, uint8_t y, const char* str);
void SSD1306_FillScreen(uint8_t color);
void SSD1306_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
void SSD1306_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color);
void SSD1306_DrawMeter(uint8_t centerX, uint8_t centerY, float angleRad, uint8_t radius, uint8_t color);
#ifdef __cplusplus
}
#endif
#endif
