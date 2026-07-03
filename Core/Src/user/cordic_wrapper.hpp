#ifndef CORDIC_WRAPPER_HPP
#define CORDIC_WRAPPER_HPP

#include "main.h"
#include <cmath>
extern CORDIC_HandleTypeDef hcordic;
// 数学定数の定義 (M_PIが定義されていない場合用)
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

class CORDIC_Wrapper {
    private:
        CORDIC_Wrapper() = default; // インスタンス化を防ぐためにコンストラクタをprivateにする
        enum class CORDIC_Function {
            SINE = 1,
            PHASE = 2
        };
        inline static CORDIC_Function lastFunction; // 最後に使用したCORDICの関数を記録
public:
    /**
     * @brief SinとCosを同時に計算
     * @param angle_rad 入力角度 [rad] (-π ～ π)
     * @param pSin 正弦の出力先
     * @param pCos 余弦の出力先
     */
    static inline void sin_cos(float angle_rad, float* pSin, float* pCos){
        if(lastFunction != CORDIC_Function::SINE && 1){
            // 1. CORDICの設定 (Sin/Cosモード)
            CORDIC_ConfigTypeDef sConfig = {0};
            sConfig.Function = CORDIC_FUNCTION_SINE;      // Sineを指定するとCosも計算される
            sConfig.Precision = CORDIC_PRECISION_6CYCLES; // 精度(反復回数)
            sConfig.Scale = CORDIC_SCALE_0;               // 出力レンジ1倍
            sConfig.NbWrite = CORDIC_NBWRITE_1;           // 入力1つ(角度のみ)
            sConfig.NbRead = CORDIC_NBREAD_2;             // 出力2つ(SinとCos)
            sConfig.InSize = CORDIC_INSIZE_32BITS;
            sConfig.OutSize = CORDIC_OUTSIZE_32BITS;
            
            HAL_CORDIC_Configure(&hcordic, &sConfig);
            lastFunction = CORDIC_Function::SINE;
        }

        // 2. 入力をQ31形式に変換 (-π～π を -1～1 に正規化)
        float input_norm = angle_rad / float(M_PI);
        if (input_norm > 1.0f) input_norm = 1.0f;
        if (input_norm < -1.0f) input_norm = -1.0f;
        
        int32_t q31_input = (int32_t)(input_norm * 2147483647.0f);
        int32_t q31_output[2]; // [0]=Sin, [1]=Cos

        // 3. 計算実行 (ブロッキングモード)
        HAL_CORDIC_Calculate(&hcordic, &q31_input, q31_output, 1, HAL_MAX_DELAY);

        // 4. Q31形式をfloatに戻す
        *pSin = (float)q31_output[0] / 2147483647.0f;
        *pCos = (float)q31_output[1] / 2147483647.0f;
    }

    /**
     * @brief Atan2を計算
     * @param y y成分
     * @param x x成分
     * @param pAngle_rad 出力角度 [rad] (-π ～ π)
     */
    static inline void atan2(float y, float x, float* pAngle_rad){
        if(lastFunction != CORDIC_Function::PHASE){
            CORDIC_ConfigTypeDef sConfig = {0};
            sConfig.Function = CORDIC_FUNCTION_PHASE;     // Phase = Atan2
            sConfig.Precision = CORDIC_PRECISION_6CYCLES;
            sConfig.Scale = CORDIC_SCALE_0;
            sConfig.NbWrite = CORDIC_NBWRITE_2;           // 入力2つ(x, y)
            sConfig.NbRead = CORDIC_NBREAD_1;             // 出力1つ(Angle)
            sConfig.InSize = CORDIC_INSIZE_32BITS;
            sConfig.OutSize = CORDIC_OUTSIZE_32BITS;

            HAL_CORDIC_Configure(&hcordic, &sConfig);
            lastFunction = CORDIC_Function::PHASE;
        }

        // 最大絶対値で正規化してQ31へ
        float max_val = std::abs(x) > std::abs(y) ? std::abs(x) : std::abs(y);
        if (max_val < 1e-9f) { *pAngle_rad = 0.0f; return; }

        int32_t q31_input[2] = {
            (int32_t)((x / max_val) * 2147483647.0f),
            (int32_t)((y / max_val) * 2147483647.0f)
        };
        int32_t q31_output;

        HAL_CORDIC_Calculate(&hcordic, q31_input, &q31_output, 1, HAL_MAX_DELAY);

        // 出力は -1～1 (-π～πに対応) なのでπを掛けてradに戻す
        *pAngle_rad = ((float)q31_output / 2147483647.0f) * float(M_PI);
    }
};

#endif