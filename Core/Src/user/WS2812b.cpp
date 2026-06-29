#include "main.h"
#include "WS2812b.hpp"

static constexpr int MAX_WS2812 = 8;

static WS2812B* instances[MAX_WS2812];
static int instanceCount = 0;
WS2812B::WS2812B(TIM_HandleTypeDef* htim,
                 uint32_t channel,
                 uint8_t brightness,
                 uint8_t WS0,
                 uint8_t WS1)
{
    this->htim = htim;
    this->channel = channel;

    this->brightness = brightness;
    this->WS0 = WS0;
    this->WS1 = WS1;

    if(instanceCount < MAX_WS2812)
    {
        instances[instanceCount++] = this;
    }
}
void WS2812B::setTIM(TIM_HandleTypeDef* htim, uint32_t channel){
    this->htim = htim;
    this->channel = channel;
}

void WS2812B::setBrightness(uint8_t brightness){
    this->brightness = brightness;
}

void WS2812B::SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = ((uint32_t)g << 16) |
                     ((uint32_t)r << 8)  |
                     b;

    for(int i = 0; i < 24; i++)
    {
        pwmData[i] = (color & (1 << (23 - i))) ? WS1 : WS0;
    }

    for(int i = 24; i < 24 + RESET_SLOTS; i++)
    {
        pwmData[i] = 0;
    }

    HAL_TIM_PWM_Start_DMA(
        htim,
        channel,
        (uint32_t*)pwmData,
        BIT_NUM + RESET_SLOTS
    );
}
TIM_HandleTypeDef* WS2812B::getTIM()
{
    return htim;
}

uint32_t WS2812B::getChannel()
{
    return channel;
}
void WS2812B::dmaFinished()
{
    HAL_TIM_PWM_Stop_DMA(htim, channel);
    __HAL_TIM_SET_COMPARE(htim, channel, 0);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    for(int i = 0; i < instanceCount; i++)
    {
        if(instances[i]->getTIM()->Instance == htim->Instance)
        {
            instances[i]->dmaFinished();
        }
    }
}
void WS2812B::HSVtoRGB(uint16_t h, uint8_t* r, uint8_t* g, uint8_t* b)
{
    uint8_t region = h / 60;
    uint8_t remainder = (h % 60) * brightness / 60;

    switch(region)
    {
        case 0:
            *r = brightness;
            *g = remainder;
            *b = 0;
            break;

        case 1:
            *r = brightness - remainder;
            *g = brightness;
            *b = 0;
            break;

        case 2:
            *r = 0;
            *g = brightness;
            *b = remainder;
            break;

        case 3:
            *r = 0;
            *g = brightness - remainder;
            *b = brightness;
            break;

        case 4:
            *r = remainder;
            *g = 0;
            *b = brightness;
            break;

        case 5:
        default:
            *r = brightness;
            *g = 0;
            *b = brightness - remainder;
            break;
    }
}