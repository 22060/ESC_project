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
uint16_t AS5048A_ReadAngle(void);
int32_t buf_enc = 0;
float electric_theta = 0;
int polePairs = 7; 
inline int32_t read_encoder_value(void)
{
    static int32_t last = 0;
    int32_t now = TIM3->CNT;
    int32_t diff = now - last;
    last = now;
    return diff;
}
AS5048A encoder(AS5048A_MODE::SINGLE_READ_WRITE, {GPIO_PIN_15, GPIOC}, &hspi1);

// 数学定数の定義 (M_PIが定義されていない場合用)
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717958647692f
#endif


extern "C" void setup(void)
{
    // Setup code here
    HAL_UART_Transmit(&huart3, (uint8_t*)"CORDIC TEST\r\n", 11, HAL_MAX_DELAY);
    float sin, cos;
    CORDIC_Wrapper::sin_cos(M_PI / 4.0f, &sin, &cos);
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "sin(45) = %f, cos(45) = %f\r\n", sin, cos);
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

        // モーターキャリブレーション
    static KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::inverseParkTransform({1, 0}, 1, 0);
    static KJ_FOC_Utils::Phase voltages = KJ_FOC_Utils::inverseClarkeTransform(outputAlphaBeta);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint16_t)(voltages.a * 100) + 1249);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint16_t)(voltages.b * 100) + 1249);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint16_t)(voltages.c * 100) + 1249);
    HAL_Delay(1000);
    
    TIM3->CNT = 0;
    // 【追加】キャリブレーションごとにZ相のオフセットもリセットさせる
    z_phase_detected_once = false; 
    mech_count = 0;

    HAL_TIM_Base_Start_IT(&htim1);
    HAL_TIM_Base_Start_IT(&htim6);
    HAL_TIM_Base_Start_IT(&htim7);

    SSD1306_Init();
    SSD1306_WriteString(0, 0, "Hello STM32!");
    SSD1306_WriteString(0, 8, "SSD1306 OK");
    SSD1306_WriteString(0, 16, "WS2812 OK");
    SSD1306_WriteString(0, 24, "Setup complete");
    SSD1306_UpdateScreen();
}
int time = 0;
int hue = 0;
uint8_t r, g, b;
extern "C" void loop(void)
{
    if(is_100ms)
    {
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
        float rad = encoder.getAngleRadian();
        
        // SSD1306表示
        snprintf(buff, sizeof(buff), "rad: %.2f", rad);
        SSD1306_WriteString(0, 0, buff);
        
        // ----------------------------------------------------
        // 3. 磁力強度 (Magnitude) の取得・表示
        // ----------------------------------------------------
        float mag = encoder.getMagnitude();
        
        
        // SSD1306表示
        snprintf(buff, sizeof(buff), "Mag: %.2f", mag); // 画面幅に収まるよう"Mag:"に微調整
        SSD1306_WriteString(0, 10, buff); // 行間が詰まりすぎないようY座標を8→10に調整
        
        // ----------------------------------------------------
        // 4. エラー & AGCの取得・表示
        // ----------------------------------------------------
        uint16_t agc_and_err = encoder.getErrorAndAGC();
        
        // 【修正】エラーフラグのみを抽出 (上位ビットのエラーマスクをチェック)
        // パリティエラー、コマンド送信エラー、SPIエラーのいずれかがあるか確認
        uint16_t error_flag = agc_and_err & 0x0F00; // 上位3ビットを抽出
        
        // 抽出したAGC値（下位のデータ部分など、必要に応じて画面表示用として残す場合）
        // ※もし不要なら消しても大丈夫です
        uint16_t agc_val = agc_and_err & 0x00FF; // 下位8ビットを抽出


        // 【修正】UART送信：エラーフラグだけを送信
        // 16進数(0x%04X)などで出力すると、どのエラーが立っているか解析しやすくなります
        snprintf(buff, sizeof(buff), "ERR: 0x%02X", error_flag>>16);
        SSD1306_WriteString(64, 0, buff);
        
        // SSD1306表示 (画面側はこれまでの通りAGCの値を表示)
        snprintf(buff, sizeof(buff), "AGC: %d", agc_val);
        SSD1306_WriteString(64, 10, buff);
        
        // ----------------------------------------------------
        // 【追加】5. 右下32x32ピクセルでの角度可視化 (X:112, Y:48 を中心とする)
        // ----------------------------------------------------
        // 32x32の領域: X軸[96~128], Y軸[32~64]
        // 中心座標 (実質半径15の円が収まる中心)
        uint8_t centerX = 15;
        uint8_t centerY = 48;
        uint8_t radius = 14; // 外枠の円の半径

        snprintf(buff, sizeof(buff), "rad: %.2f", rad);
        SSD1306_WriteString(0, 20, buff);
        SSD1306_DrawMeter(centerX, centerY, rad, radius, 1);
        rad = mech_count * (M_2PI / 2048.0f); // 1回転で2048カウントのエンコーダを想定
        snprintf(buff, sizeof(buff), "enc: %ld", mech_count);
        SSD1306_WriteString(64, 20, buff);
        SSD1306_DrawMeter(centerX + 32, centerY, rad, radius, 1);
        rad = electric_theta; // 電気角度を表示
        SSD1306_DrawMeter(centerX + 64, centerY, rad, radius, 1);

        // ----------------------------------------------------
        // 6. 画面の物理更新 (バッファを一括転送)
        // ----------------------------------------------------
        SSD1306_UpdateScreen();
        if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6))
        {
            r = 40;
        }
        else
        {
            r = 0;
        }
        if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14)){
            
            g = 40;
        }
        else
        {
            g = 0;
        }
        WS2812.SetColor(r,g,b);
    }
    //  HAL_Delay(10);
}
#define AS5048A_CS_GPIO_PORT  GPIOC
#define AS5048A_CS_PIN        GPIO_PIN_15
// AS5048A レジスタ・コマンド定義
#define AS5048A_CMD_READ      0x4000
#define AS5048A_REG_AGC       0x3FFD
#define AS5048A_REG_MAG       0x3FFE
#define AS5048A_REG_ANGLE     0x3FFF
#define AS5048A_REG_CLRERR    0x0001
/**
 * @brief 16ビットデータの偶数パリティを計算し、最上位ビット(Bit 15)にセットする
 */
static uint16_t AS5048A_CalculateParity(uint16_t data) {
    uint16_t count = 0;
    uint16_t temp = data & 0x7FFF; // 15ビット目クリア

    while (temp) {
        count += (temp & 1);
        temp >>= 1;
    }
    
    // ビットの総数が奇数なら、15ビット目を1にして偶数（Even）にする
    if (count % 2 != 0) {
        data |= 0x8000;
    } else {
        data &= 0x7FFF;
    }
    return data;
}
/**
 * @brief AS5048Aと16ビットの送受信を行う低層関数
 */
uint16_t AS5048A_SPI_Transfer(uint16_t command) {
    uint16_t tx_data = AS5048A_CalculateParity(command);
    uint16_t rx_data = 0;

    // CSをLOW（アクティブ）にする
    HAL_GPIO_WritePin(AS5048A_CS_GPIO_PORT, AS5048A_CS_PIN, GPIO_PIN_RESET);
    
    // 16ビット送受信 (タイムアウト10ms)
    HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&tx_data, (uint8_t*)&rx_data, 1, 10);
    
    // CSをHIGH（非アクティブ）にする
    HAL_GPIO_WritePin(AS5048A_CS_GPIO_PORT, AS5048A_CS_PIN, GPIO_PIN_SET);

    return rx_data;
}
/**
 * @brief AS5048Aから現在の生の角度データ(0〜16383)を取得する
 * @return uint16_t 14ビットの角度データ。エラー時は0xFFFFを返す
 */
uint16_t AS5048A_ReadAngle(void) {
    uint16_t response;
    
    // 1. 角度レジスタの読み出しコマンドを送る（戻り値は前回のコマンド結果なので破棄）
    AS5048A_SPI_Transfer(AS5048A_CMD_READ | AS5048A_REG_ANGLE);
    
    // 2. もう一度ダミーで送ることで、上のコマンドに対する角度データが返ってくる
    response = AS5048A_SPI_Transfer(AS5048A_CMD_READ | AS5048A_REG_ANGLE);

    // エラーフラグ(Bit 14)を確認
    if (response & 0x4000) {
        // エラーが発生している場合はエラーレジスタをクリア
        AS5048A_SPI_Transfer(AS5048A_CMD_READ | AS5048A_REG_CLRERR);
        return 0xFFFF; // エラーを示す値を返す
    }

    // 下位14ビットが角度データ
    return (response & 0x3FFF);
}

/**
 * @brief 生の角度データを「度（degree, 0.0〜360.0）」に変換する関数
 */
float AS5048A_GetAngleDegrees(void) {
    uint16_t raw_angle = AS5048A_ReadAngle();
    
    if (raw_angle == 0xFFFF) {
        return -1.0f; // エラー時
    }
    
    // 14ビット(16384)で360度
    return ((float)raw_angle * 360.0f) / 16384.0f;
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // GPIO_PIN_X を実際のZ相が接続されているピンに書き換えてください (例: GPIO_PIN_13)
    if(GPIO_Pin == GPIO_PIN_13) // 例: Z相がGPIO_PIN_15に接続されている場合
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
            mech_count = z_phase_offset;
        }
        speed_count++;
    }
}

uint16_t count_bunsyu = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM1)
    {
        static uint16_t count = 200;
        static uint16_t duty[3] = {0, 0, 0};
        static uint16_t bunkainou = 1200;
        static uint16_t btime = 0;
        count_bunsyu++;
        if(count_bunsyu >= 300)
        {
            count_bunsyu = 0;
            if (count <= bunkainou)
            {
                count++;
            }
        }

        buf_enc = read_encoder_value();
        mech_count += buf_enc;
        
        // mech_countを常に 0 ~ 4095 の範囲に収める
        mech_count %= 2048; // 変更: 4096から2048に変更
        if(mech_count < 0)
        {
            mech_count += 2048;
        }
        
        electric_theta = (mech_count / 2048.0f) * M_2PI * polePairs;
        electric_theta = fmodf(electric_theta, M_2PI);
        
        float angle[3];
        float sin, cos;
        CORDIC_Wrapper::sin_cos(electric_theta, &sin, &cos);
        KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::inverseParkTransform({0, -1*count}, cos, sin);
        KJ_FOC_Utils::Phase voltages = KJ_FOC_Utils::inverseClarkeTransform(outputAlphaBeta);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, voltages.a + 1249);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, voltages.b + 1249);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, voltages.c + 1249);
    }else if(htim->Instance == TIM7){
        is_100ms = true;
    }
}