#ifndef WS2812B_H
#define WS2812B_H

#define BIT_NUM 24
#define RESET_SLOTS 100
#define WS_0 36
#define WS_1 72
#define LED_BRIGHTNESS 20
class WS2812B
{
    private:
    uint16_t pwmData[BIT_NUM + RESET_SLOTS];
    uint8_t brightness;
    uint8_t WS0;
    uint8_t WS1;
    uint32_t channel;
    public:
    TIM_HandleTypeDef* htim;
    WS2812B();
    WS2812B(TIM_HandleTypeDef* htim, uint32_t channel, uint8_t brightness = LED_BRIGHTNESS,uint8_t WS0 = WS_0, uint8_t WS1 = WS_1);
    void setTIM(TIM_HandleTypeDef* htim, uint32_t channel);
    void setBrightness(uint8_t brightness);
    void SetColor(uint8_t r, uint8_t g, uint8_t b);
    void HSVtoRGB(uint16_t h, uint8_t* r, uint8_t* g, uint8_t* b);
    void dmaFinished();
    TIM_HandleTypeDef* getTIM();
    uint32_t getChannel();
};
#endif