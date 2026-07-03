#include "ssd1306.hpp"
#include "main.h"
#include <cstdlib>
#include <cstring>
#include "cordic_wrapper.hpp"
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717958647692f
#endif

static uint8_t buffer[128 * 64 / 8];
extern I2C_HandleTypeDef hi2c2;
static const uint8_t Font6x8[][6] = {
#include "font6x8.inc"   // フォント別ファイルを後述
};

void SSD1306_SendCommand(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c2, SSD1306_I2C_ADDR, data, 2, 10);
}

void SSD1306_SendData(uint8_t* data, uint16_t size) {
    uint8_t buf[129];
    buf[0] = 0x40;
    memcpy(&buf[1], data, size);
    HAL_I2C_Master_Transmit(&hi2c2, SSD1306_I2C_ADDR, buf, size + 1, 100);
}

void SSD1306_Init(void) {
    HAL_Delay(100);

    SSD1306_SendCommand(0xAE);
    SSD1306_SendCommand(0x20); SSD1306_SendCommand(0x00);
    SSD1306_SendCommand(0xB0);
    SSD1306_SendCommand(0xC8);
    SSD1306_SendCommand(0x00);
    SSD1306_SendCommand(0x10);
    SSD1306_SendCommand(0x40);
    SSD1306_SendCommand(0x81); SSD1306_SendCommand(0x7F);
    SSD1306_SendCommand(0xA1);
    SSD1306_SendCommand(0xA6);
    SSD1306_SendCommand(0xA8); SSD1306_SendCommand(0x3F);
    SSD1306_SendCommand(0xA4);
    SSD1306_SendCommand(0xD3); SSD1306_SendCommand(0x00);
    SSD1306_SendCommand(0xD5); SSD1306_SendCommand(0x80);
    SSD1306_SendCommand(0xD9); SSD1306_SendCommand(0xF1);
    SSD1306_SendCommand(0xDA); SSD1306_SendCommand(0x12);
    SSD1306_SendCommand(0xDB); SSD1306_SendCommand(0x40);
    SSD1306_SendCommand(0x8D); SSD1306_SendCommand(0x14);
    SSD1306_SendCommand(0xAF);

    SSD1306_Clear();
    SSD1306_UpdateScreen();
}

void SSD1306_Clear(void) {
    memset(buffer, 0, sizeof(buffer));
}

void SSD1306_UpdateScreen(void) {
    for (uint8_t page = 0; page < 8; page++) {
        SSD1306_SendCommand(0xB0 + page);
        SSD1306_SendCommand(0x00);
        SSD1306_SendCommand(0x10);

        SSD1306_SendData(&buffer[128 * page], 128);
    }
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= 128 || y >= 64) return;

    uint16_t index = x + (y / 8) * 128;
    if (color) buffer[index] |=  (1 << (y % 8));
    else       buffer[index] &= ~(1 << (y % 8));
}

void SSD1306_WriteString(uint8_t x, uint8_t y, const char* str) {
    while (*str) {
        char c = *str++;
        if (c < 32 || c > 126) c = '?';

        for (uint8_t i = 0; i < 6; i++) {
            uint8_t col = Font6x8[c - 32][i];
            for (uint8_t j = 0; j < 8; j++) {
                SSD1306_DrawPixel(x + i, y + j, (col >> j) & 1);
            }
        }
        x += 6;
    }
}

void SSD1306_FillScreen(uint8_t color) {
    for (uint16_t i = 0; i < 384; i++) {
        buffer[i] = color ? 0x01 : 0x00;
    }
    SSD1306_UpdateScreen();
}

void SSD1306_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = -abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    while (true) {
        SSD1306_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void SSD1306_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    SSD1306_DrawLine(x, y, x + w - 1, y, color);
    SSD1306_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    SSD1306_DrawLine(x, y, x, y + h - 1, color);
    SSD1306_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, color);
    SSD1306_DrawPixel(x0, y0 - r, color);
    SSD1306_DrawPixel(x0 + r, y0, color);
    SSD1306_DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawPixel(x0 + x, y0 + y, color);
        SSD1306_DrawPixel(x0 - x, y0 + y, color);
        SSD1306_DrawPixel(x0 + x, y0 - y, color);
        SSD1306_DrawPixel(x0 - x, y0 - y, color);
        SSD1306_DrawPixel(x0 + y, y0 + x, color);
        SSD1306_DrawPixel(x0 - y, y0 + x, color);
        SSD1306_DrawPixel(x0 + y, y0 - x, color);
        SSD1306_DrawPixel(x0 - y, y0 - x, color);
    }
}

void SSD1306_DrawMeter(uint8_t centerX, uint8_t centerY, float angleRad, uint8_t radius, uint8_t color) {
    // 1. 外枠は毎回描画しなくて良いならここから外してください
    SSD1306_DrawCircle(centerX, centerY, radius, color);

    float sin_val = 0.0f;
    float cos_val = 0.0f;
    if(angleRad < -M_PI) angleRad += M_2PI;
    if(angleRad > M_PI) angleRad -= M_2PI;
    CORDIC_Wrapper::sin_cos(angleRad, &sin_val, &cos_val);

    // int16_t で計算を行う
    uint8_t targetX = centerX + (int16_t)(cos_val * (radius - 2));
    uint8_t targetY = centerY - (int16_t)(sin_val * (radius - 2));

    // 描画範囲外へのハミ出し防止（クリッピング）
    if (targetX < 0) targetX = 0;
    if (targetX > 127) targetX = 127;
    if (targetY < 0) targetY = 0;
    if (targetY > 63) targetY = 63;

    // Line描画関数は uint8_t を要求しているため、ここでキャスト
    SSD1306_DrawLine(centerX, centerY, (uint8_t)targetX, (uint8_t)targetY, color);
}