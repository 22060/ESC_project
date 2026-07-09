// #include "main.h"
// #include "WS2812b.hpp"
// #include "cordic_wrapper.hpp"
// #include "ssd1306.hpp"
// #include <cstring>
// #include <cstdio>
// #include "kj_foc.hpp"

// // 数学定数の定義 (M_PIが定義されていない場合用)
// #ifndef M_PI
// #define M_PI 3.14159265358979323846f
// #endif
// #ifndef M_2PI
// #define M_2PI 6.28318530717958647692f
// #endif

// // (関数のプロトタイプ宣言やextern宣言は元のまま)
// void OLED_WriteCommand(uint8_t cmd);
// void OLED_WriteData(uint8_t data);
// void OLED_Init(void);
// void OLED_Clear(void);
// void OLED_DrawPixel(int x, int y, uint8_t color);
// void OLED_UpdateScreen(void);
// extern ADC_HandleTypeDef hadc1;
// extern ADC_HandleTypeDef hadc2;
// extern CORDIC_HandleTypeDef hcordic;
// extern FDCAN_HandleTypeDef hfdcan1;
// extern FMAC_HandleTypeDef hfmac;
// extern I2C_HandleTypeDef hi2c2;
// extern OPAMP_HandleTypeDef hopamp1;
// extern OPAMP_HandleTypeDef hopamp2;
// extern OPAMP_HandleTypeDef hopamp3;
// extern TIM_HandleTypeDef htim1;
// extern TIM_HandleTypeDef htim2;
// // extern TIM_HandleTypeDef htim3;
// extern TIM_HandleTypeDef htim4;
// extern TIM_HandleTypeDef htim6;
// extern TIM_HandleTypeDef htim7;
// extern TIM_HandleTypeDef htim8;
// extern DMA_HandleTypeDef hdma_tim4_ch2;
// extern UART_HandleTypeDef huart3;

// WS2812B WS2812(&htim4, TIM_CHANNEL_2);
// float feedback_current[3] = {0.0f};
// float offset_current[3] = {0.0f};
// uint16_t encoder_offset = 0;

// void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc){
// 	feedback_current[0] = ((ADC1->JDR1 - offset_current[0]) / 4095.0f) * 3.3f / 64.0f / 2 * 1000.0f;
// 	feedback_current[1] = ((ADC2->JDR1 - offset_current[1]) / 4095.0f) * 3.3f / 64.0f / 2 * 1000.0f;
// 	feedback_current[2] = ((ADC2->JDR2 - offset_current[2]) / 4095.0f) * 3.3f / 64.0f / 2 * 1000.0f;
// }

// int e_count = 1;
// uint32_t us_count = 0;

// // 【修正点】割り込み間で共有されるため volatile を付与
// volatile int32_t mech_count = 0;
// volatile int32_t speed_count = 0;
// volatile bool is_1s = false;

// // 【追加】Z相による補正用の変数
// volatile bool z_phase_detected_once = false;
// volatile int32_t z_phase_offset = 0;

// inline int32_t read_encoder_value(void)
// {
//     static int32_t last = 0;
//     int32_t now = TIM2->CNT;
//     int32_t diff = now - last;
//     last = now;
//     return diff;
// }

// // 【追加】Z相(外部割り込み)のコールバック処理
// // ※ CubeMX等でZ相ピンを「GPIO_EXTI」に設定してください
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
//     // GPIO_PIN_X を実際のZ相が接続されているピンに書き換えてください (例: GPIO_PIN_13)
//     if(GPIO_Pin == GPIO_PIN_15) // 例: Z相がGPIO_PIN_15に接続されている場合
//     {
//         if (!z_phase_detected_once)
//         {
//             // 初回Z相検出時: 起動時のキャリブレーション(電気角0)からZ相までのオフセットを記録
//             z_phase_offset = mech_count;
//             z_phase_detected_once = true;
//         }
//         else
//         {
//             // 2回目以降: Z相通過のタイミングで mech_count を本来のオフセット位置に強制リセット（ズレの補正）
//             mech_count = z_phase_offset;
//         }
//         speed_count++;
//     }
// }
// uint16_t count_bunsyu = 0;
// void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
// {
//     if(htim->Instance == TIM1)
//     {
//         static uint16_t count = 200;
//         static uint16_t duty[3] = {0, 0, 0};
//         static uint16_t bunkainou = 1200;
//         static uint16_t btime = 0;
//         static float electric_theta = 0;
//         static int polePairs = 7; 
//         count_bunsyu++;
//         if(count_bunsyu >= 300)
//         {
//             count_bunsyu = 0;
//             if (count <= bunkainou)
//             {
//                 count++;
//             }
//         }
//         static int32_t buf_enc = 0;

//         buf_enc = read_encoder_value();
//         mech_count += buf_enc;
        
//         // mech_countを常に 0 ~ 4095 の範囲に収める
//         mech_count %= 4096;
//         if(mech_count < 0)
//         {
//             mech_count += 4096;
//         }
        
//         electric_theta = (mech_count / 4096.0f) * M_2PI * polePairs;
//         electric_theta = fmodf(electric_theta, M_2PI);
        
//         float angle[3];
//         float sin, cos;
//         CORDIC_Wrapper::sin_cos(electric_theta, &sin, &cos);
//         KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::inverseParkTransform({0, -1*count}, cos, sin);
//         KJ_FOC_Utils::Phase voltages = KJ_FOC_Utils::inverseClarkeTransform(outputAlphaBeta);
//         __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, voltages.a + 1249);
//         __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, voltages.b + 1249);
//         __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, voltages.c + 1249);
//     }else if(htim->Instance == TIM7){
//         is_1s = true;
//     }
// }

// extern "C" void setup(void)
// {
//     // ... (ADC, OPAMP, TIMの初期化処理は変更なしのため省略せずそのまま記載)
//     HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
//     HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
//     HAL_ADC_Start(&hadc1);
//     HAL_ADC_Start(&hadc2);
//     HAL_ADCEx_InjectedStart_IT(&hadc1);
    
//     HAL_OPAMP_Start(&hopamp1);
//     HAL_OPAMP_Start(&hopamp2);
//     HAL_OPAMP_Start(&hopamp3);
    
//     HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
//     HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
//     HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
//     HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
//     HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
//     HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
    
//     HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
//     HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
//     HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);
//     HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1);
//     HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_2);
//     HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_3);

//     // HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
//     HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

//     for(int i = 0; i < 10; i ++){
//         offset_current[0] += ADC1->JDR1 / 10.0f;
//         offset_current[1] += ADC2->JDR1 / 10.0f;
//         offset_current[2] += ADC2->JDR2 / 10.0f;
//         HAL_Delay(1);
//     }

//     SSD1306_Init();
//     SSD1306_WriteString(0, 0, "Hello STM32!");
//     SSD1306_WriteString(0, 8, "SSD1306 OK");
//     SSD1306_UpdateScreen();
//     HAL_UART_Transmit(&huart3, (uint8_t*)"Setup complete\r\n", 16, HAL_MAX_DELAY);
//     HAL_Delay(10);
//     HAL_UART_Transmit(&huart3, (uint8_t*)"CORDIC TEST\r\n", 11, HAL_MAX_DELAY);
//     float sin, cos;
//     CORDIC_Wrapper::sin_cos(M_PI / 4.0f, &sin, &cos);
//     char buffer[50];
//     snprintf(buffer, sizeof(buffer), "sin(45) = %f, cos(45) = %f\r\n", sin, cos);
//     HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

//     // モーターキャリブレーション
//     static KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::inverseParkTransform({1, 0}, 1, 0);
//     static KJ_FOC_Utils::Phase voltages = KJ_FOC_Utils::inverseClarkeTransform(outputAlphaBeta);
//     __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint16_t)(voltages.a * 100) + 1249);
//     __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint16_t)(voltages.b * 100) + 1249);
//     __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint16_t)(voltages.c * 100) + 1249);
//     HAL_Delay(1000);
    
//     TIM2->CNT = 0;
//     // 【追加】キャリブレーションごとにZ相のオフセットもリセットさせる
//     z_phase_detected_once = false; 
//     mech_count = 0;

//     HAL_TIM_Base_Start_IT(&htim1);
//     HAL_TIM_Base_Start_IT(&htim6);
//     HAL_TIM_Base_Start_IT(&htim7);
//     SSD1306_FillScreen(1);
//     SSD1306_UpdateScreen();
// }

// int times[2] = {0, 0};
// uint16_t hue = 0;
// uint8_t r, g, b;

// extern "C" void loop(void)
// {
//     // ... (loop内の処理は元のまま変更不要です)
//     if(is_1s)
//     {
//         is_1s = false;
//         static char buff[64];
//         float rps = speed_count;
//         snprintf(buff, sizeof(buff), "Speed Count: %d", speed_count);
//         HAL_UART_Transmit(&huart3, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);
//         speed_count = 0;

//         snprintf(buff, sizeof(buff), "rps=%.2f,v=%.2f", rps, rps * 60 / 270);
//         SSD1306_WriteString(0, 0, buff);
//         snprintf(buff, sizeof(buff), "a=%.2f,b=%.2f,c=%.2f", feedback_current[0], feedback_current[1], feedback_current[2]);
//         SSD1306_WriteString(0, 8, buff);
//         KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::clarkeTransform({feedback_current[0], feedback_current[1], feedback_current[2]});
//         KJ_FOC_Utils::DQ voltages = KJ_FOC_Utils::parkTransform(outputAlphaBeta, 1, 0);
//         snprintf(buff, sizeof(buff), "Id=%.3f, Iq=%.3f", voltages.d, voltages.q);
//         SSD1306_WriteString(0, 16, buff);
//         snprintf(buff, sizeof(buff), "theta=%.2f", (mech_count / 4096.0f) * 360.0f);
//         SSD1306_WriteString(0, 24, buff);
//         SSD1306_UpdateScreen();
//     }

//     if(HAL_GetTick() - times[0] >= 14)
//     {
//         times[0] = HAL_GetTick();
//         WS2812.HSVtoRGB(hue, &r, &g, &b);
//         WS2812.SetColor(r, g, b);

//         hue++;
//         if(hue >= 360)
//             hue = 0;
//     }
//     if(HAL_GetTick() - times[1] >= 1000)
//     {
//         times[1] = HAL_GetTick();
//     }
// }

#include "main.h"
#include "WS2812b.hpp"
#include "ssd1306.hpp"
#include "cordic_wrapper.hpp"
#include <cstring>
#include <cstdio>
#include "kj_foc.hpp"
#include "AS5048A.hpp"
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern CORDIC_HandleTypeDef hcordic;
extern DAC_HandleTypeDef hdac1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FMAC_HandleTypeDef hfmac;
extern I2C_HandleTypeDef hi2c2;
extern OPAMP_HandleTypeDef hopamp1;
extern OPAMP_HandleTypeDef hopamp2;
extern OPAMP_HandleTypeDef hopamp3;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim17;
extern DMA_HandleTypeDef hdma_tim4_ch2;
extern UART_HandleTypeDef huart3;
WS2812B WS2812(&htim4, TIM_CHANNEL_2);
int e_count = 1;
uint32_t us_count = 0;

// 【修正点】割り込み間で共有されるため volatile を付与
volatile int32_t mech_count = 0;
volatile int32_t speed_count = 0;
volatile bool is_100ms = false;

// 【追加】Z相による補正用の変数
volatile bool z_phase_detected_once = false;
volatile int32_t z_phase_offset = 0;
int32_t buf_enc = 0;
float electric_theta = 0;
int polePairs = 7;
int rotate_per_sec = 0;
uint16_t AS5048A_ReadAngle(void);
inline int32_t read_encoder_value(void)
{
    static int32_t last = 0;
    int32_t now = TIM3->CNT;
    int32_t diff = now - last;
    last = now;
    return diff;
}
int callbackcout = 0;
int make1s = 0;
int callback_flag = 0;
void test_callback(void)
{
    callbackcout++;
    // callback_flag = 1;
    // HAL_UART_Transmit(&huart3, (uint8_t*)"Interrupt triggered!\r\n", 22, HAL_MAX_DELAY);
}
AS5048A encoder(AS5048A_MODE::SINGLE_READ_WRITE, {GPIO_PIN_15, GPIOC}, &hspi1,0, nullptr);
bool isStopped = true;
void stopmotor(void)
{
    if(!isStopped)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1249);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 1249);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 1249);
        isStopped = true;
    }
}
void startmotor(void)
{
    if(isStopped)
    {
        isStopped = false;
    }
}
float feedback_current[3] = {0.0f};
float offset_current[3] = {0.0f};

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc){
    if (hadc->Instance == ADC1) {
        feedback_current[0] = ((ADC1->JDR1 - offset_current[0]) / 4095.0f) * 3.3f / 64.0f / 2 * 1000.0f;
    }
    else if (hadc->Instance == ADC2) {
        feedback_current[1] = ((ADC2->JDR1 - offset_current[1]) / 4095.0f) * 3.3f / 64.0f / 2 * 1000.0f;
        feedback_current[2] = ((ADC2->JDR2 - offset_current[2]) / 4095.0f) * 3.3f / 64.0f / 2 * 1000.0f;
    }
}
// 数学定数の定義 (M_PIが定義されていない場合用)
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717958647692f
#endif


extern "C" void setup(void)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)"Setup start\r\n", 13, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart3, (uint8_t*)"this is a bldc MD @ 2026-06-29\r\n", 36, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart3, (uint8_t*)"datasheet: https://circuit.ryutolab.com/datasheets \r\n", 50, HAL_MAX_DELAY);

    HAL_UART_Transmit(&huart3, (uint8_t*)"Starting CORDIC\r\n", 18, HAL_MAX_DELAY);
    HAL_CORDIC_Init(&hcordic);

    HAL_UART_Transmit(&huart3, (uint8_t*)"Starting ADC\r\n", 15, HAL_MAX_DELAY);
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_Start(&hadc2);
    HAL_ADCEx_InjectedStart_IT(&hadc1);
    HAL_ADCEx_InjectedStart_IT(&hadc2);
    
    HAL_OPAMP_Start(&hopamp1);
    HAL_OPAMP_Start(&hopamp2);
    HAL_OPAMP_Start(&hopamp3);

    

    HAL_UART_Transmit(&huart3, (uint8_t*)"Timer for PWM_CH1 start\r\n", 28, HAL_MAX_DELAY);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

    HAL_UART_Transmit(&huart3, (uint8_t*)"Timer for rotary encoder start\r\n", 30, HAL_MAX_DELAY);
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

    HAL_UART_Transmit(&huart3, (uint8_t*)"Motor calibration start\r\n", 27, HAL_MAX_DELAY);
        // モーターキャリブレーション
    static KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::inverseParkTransform({1, 0}, 1, 0);
    static KJ_FOC_Utils::Phase voltages = KJ_FOC_Utils::inverseClarkeTransform(outputAlphaBeta);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint16_t)(voltages.a * 150) + 1249);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint16_t)(voltages.b * 150) + 1249);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint16_t)(voltages.c * 150) + 1249);
    HAL_Delay(1000);
    
    TIM3->CNT = 0;
    // 【追加】キャリブレーションごとにZ相のオフセットもリセットさせる
    z_phase_detected_once = false; 
    mech_count = 0;
    encoder.setZeroPosition(encoder.getAngle());
    HAL_Delay(500);
    HAL_UART_Transmit(&huart3, (uint8_t*)"Motor calibration complete\r\n", 33, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart3, (uint8_t*)"Starting timers interrupts for FOC\r\n", 36, HAL_MAX_DELAY);
    // encoder.init();
    HAL_TIM_Base_Start_IT(&htim1);
    // HAL_TIM_Base_Start_IT(&htim6);
    HAL_TIM_Base_Start_IT(&htim7);
    for(int i = 0; i < 10; i ++){
        offset_current[0] += ADC1->JDR1 / 10.0f;
        offset_current[1] += ADC2->JDR1 / 10.0f;
        offset_current[2] += ADC2->JDR2 / 10.0f;
        HAL_Delay(1);
    }
    HAL_UART_Transmit(&huart3, (uint8_t*)"Starting SSD1306 OLED\r\n", 25, HAL_MAX_DELAY);
    SSD1306_Init();
    SSD1306_WriteString(0, 0, "Hello STM32!");
    SSD1306_WriteString(0, 8, "SSD1306 OK");
    SSD1306_WriteString(0, 16, "WS2812 OK");
    SSD1306_WriteString(0, 24, "Setup complete");
    SSD1306_UpdateScreen();
    HAL_UART_Transmit(&huart3, (uint8_t*)"Setup complete\r\n", 16, HAL_MAX_DELAY);

}
int time = 0;
int hue = 0;
int _rotate_per_ = 0;
uint8_t r, g, b;
volatile int32_t  encoder_row = 0;
extern "C" void loop(void)
{
    if(is_100ms)
    {
        // make1s++;
        static char buff[64];
        is_100ms = false;
        // ログ送信
        
        // ----------------------------------------------------
        // 1. 描画バッファをクリア（重なり防止）
        // ----------------------------------------------------
        SSD1306_Clear();

        // ----------------------------------------------------
        // 2. ラジアン角度の取得・表示
        // ----------------------------------------------------
        float rad = mech_count * 2.0f * M_PI / 4096.0f; // 14ビットのエンコーダを想定
        // float rad = (encoder_row / 16384.0f) * M_2PI * polePairs;
        
        // SSD1306表示
        snprintf(buff, sizeof(buff), "rad: %.2f", rad);
        SSD1306_WriteString(0, 0, buff);

        snprintf(buff, sizeof(buff), "rps: %d", speed_count*10);
        SSD1306_WriteString(0, 8, buff);
        speed_count = 0;

        KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::clarkeTransform({feedback_current[0], feedback_current[1], feedback_current[2]});
        KJ_FOC_Utils::DQ voltages = KJ_FOC_Utils::parkTransform(outputAlphaBeta, 1, 0);
        snprintf(buff, sizeof(buff), "Id=%.3f, Iq=%.3f", voltages.d, voltages.q);
        SSD1306_WriteString(0, 16, buff);

        snprintf(buff, sizeof(buff), "Ia=%.2f, Ib=%.2f, Ic=%.2f\r\n", feedback_current[0], feedback_current[1], feedback_current[2]);
        HAL_UART_Transmit(&huart3, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);
        // ----------------------------------------------------
        // 【追加】5. 右下32x32ピクセルでの角度可視化 (X:112, Y:48 を中心とする)
        // ----------------------------------------------------
        // 32x32の領域: X軸[96~128], Y軸[32~64]
        // 中心座標 (実質半径15の円が収まる中心)
        uint8_t centerX = 15;
        uint8_t centerY = 48;
        uint8_t radius = 14; // 外枠の円の半径

        // snprintf(buff, sizeof(buff), "rad: %.2f", rad);
        // SSD1306_WriteString(0, 20, buff);
        // SSD1306_DrawMeter(centerX, centerY, rad, radius, 1);
        rad = electric_theta; // 電気角度を表示
        // SSD1306_DrawMeter(centerX + 64, centerY, rad, radius, 1);

        // ----------------------------------------------------
        // 6. 画面の物理更新 (バッファを一括転送)
        // ----------------------------------------------------
        SSD1306_UpdateScreen();
        if(!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6))
        {
            r = 10;
        }
        else
        {
            r = 0;
        }
        if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14)){
            
            g = 10;
            isStopped = true;
        }
        else
        {
            g = 0;
            isStopped = false;
        }
        WS2812.SetColor(r,g,b);
    }
    //  HAL_Delay(10);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // GPIO_PIN_X を実際のZ相が接続されているピンに書き換えてください (例: GPIO_PIN_13)
    if(GPIO_Pin == GPIO_PIN_3) // 例: Z相がGPIO_PIN_15に接続されている場合
    {
        if (!z_phase_detected_once)
        {
            // 初回Z相検出時: 起動時のキャリブレーション(電気角0)からZ相までのオフセットを記録
            z_phase_offset = mech_count;
            z_phase_detected_once = true;
        }
        else
        {
            // 2回目以降: Z相通過のタイミングで mech_count を本来のオフセット位置に強制リセット（ズレの補正）
            // mech_count = z_phase_offset;
        }
        speed_count++;
    }
}
volatile bool spiBusy;
volatile uint8_t encoderRx[2];
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == encoder.getSPIHandle())
    {
        // ブレークポイント
        encoder.SPI_CS_Deselect();
        uint16_t response = (encoderRx[0] << 8) | encoderRx[1];
        if(!(__builtin_popcount((unsigned int)response) & 1)){
            encoder_row = response & 0x3FFF; // 14ビットのデータ部分を抽出
        }
        // エラーフラグ(Bit14)の確認
        if ((response & AS5048A_ERROR_SEND_COMMAND) != 0) 
        {
            encoder_row = ((encoderRx[0] << 8) | encoderRx[1]);
            encoder_row = (16384 - encoder_row) & 0x3FFF;
        }
        spiBusy = false;
    }
}
uint16_t count_bunsyu = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM1 && isStopped == false)
    {
        static uint16_t count = 100;
        static uint16_t duty[3] = {0, 0, 0};
        static uint16_t bunkainou = 1249;
        static uint16_t btime = 0;
        count_bunsyu++;
        if(count_bunsyu >= 200)
        {
            count_bunsyu = 0;
            if (count <= bunkainou)
            {
                count++;
            }
        }

        buf_enc = read_encoder_value();
        // buf_enc = encoder.getAngleRadian();
        mech_count -= buf_enc;
        // mech_count %= 4096; // 変更: 4096から2048に変更
        if(mech_count < 0)
        {
            mech_count += 4096; // 変更: 4096から2048に変更
        }else if(mech_count >= 4096) // 変更: 4096から2048に変更
        {
            mech_count -= 4096; // 変更: 4096から2048に変更
        }
        // while(mech_count < 0)
        // {
        //     mech_count += 4096; // 変更: 4096から2048に変更
        // }
        // while(mech_count >= 4096) // 変更: 4096から2048に変更
        // {
        //     mech_count -= 4096; // 変更: 4096から2048に変更
        // }
        // electric_theta = (encoder_row / 16384.0f) * M_2PI * polePairs;
        #define MORTOR_GAIN M_2PI * polePairs / 4096.0f
        electric_theta = (mech_count *MORTOR_GAIN); // 変更: 4096から2048に変更// --------------------------------------------------------
        // 【追加】高速回転時の遅延補正（進み角の追加）
        // --------------------------------------------------------
        // _rotate_per_ (rps) に比例して電気角を進める
        // 補正係数（0.005fなど）はモータの極対数や制御周期に合わせて調整します。
        // ※回転方向に応じて符号（+ / -）が逆になる場合は調整してください。
        // float delay_compensation = (float)_rotate_per_ * 0.001f; 
        // electric_theta += delay_compensation;
        // --------------------------------------------------------
        // electric_theta = fmodf(electric_theta, M_2PI);
        while(electric_theta < -M_PI) electric_theta += M_2PI;
        while(electric_theta >= M_PI) electric_theta -= M_2PI;
        
        float angle[3];
        float sin, cos;
        CORDIC_Wrapper::sin_cos(electric_theta, &sin, &cos);
        KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::inverseParkTransform({0, -1*count}, cos, sin);
        KJ_FOC_Utils::Phase voltages = KJ_FOC_Utils::inverseClarkeTransform(outputAlphaBeta);
        
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, voltages.a + 1249);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, voltages.b + 1249);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, voltages.c + 1249);
                    
            callbackcout++;
        if(spiBusy == false)
        {
            spiBusy = true;
            encoder.SPI_CS_Select();
            const static uint16_t packet = makePacket(0x3FFF, true); // 送信するパケットを作成
        
            const static uint8_t tx[2] = { 
                static_cast<uint8_t>((packet >> 8) & 0xFF), 
                static_cast<uint8_t>(packet & 0xFF) 
            }; 
            HAL_SPI_TransmitReceive_DMA(
                encoder.getSPIHandle(),
                tx,
                (uint8_t*)&encoderRx,
                2);
        }
    }else if(htim->Instance == TIM7){
        is_100ms = true;
    }
}