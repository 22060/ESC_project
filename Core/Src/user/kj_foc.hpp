#ifndef KJ_FOC_HPP
#define KJ_FOC_HPP
/*
    CORDIC，FMAC，FPU使用前提ライブラリ群の計算部分．センサの値取得はkj_foc_sensors.hpp，三相交流電圧からPWM生成はkj_foc_pwm.hppに分ける．
    (STM32G431CBU6の場合はCORDICとFMACをオンにし，小数はすべてfloatにすること)
    三相交流制御用ライブラリ．
    フィードバック無し電圧制御モードと，電流フィードバック電圧制御モードの二種類がある．
    複数モーターを一つのプログラムで動かすためにクラスを用意するが，クラーク変換などの共通関数はstaticにすることによりメモリ節約
*/

#define SQRT3 1.73205080757f
#define INV_SQRT3 0.57735026919f

#include "main.h"


namespace KJ_FOC_Utils
{
    extern bool isinitialized;
    extern bool isNeedsensor;
    enum class ControlMode {
        Voltage,
        Current
    };
    struct PIController {
        float kp;
        float ki;
        float integral;
        float integralLimit;
        float outputLimit;
        
    };

    struct DQ {
        float d;
        float q;
    };
    struct AlphaBeta {
        float Alpha;
        float Beta;
    };
    struct Phase {
        float a;
        float b;
        float c;
    };
    int initHardware();
    inline KJ_FOC_Utils::AlphaBeta clarkeTransform(const KJ_FOC_Utils::Phase& voltage){
        KJ_FOC_Utils::AlphaBeta result;
        result.Alpha = voltage.a;
        result.Beta = (voltage.a + 2.0f * voltage.b) *INV_SQRT3;
        return result;
    }
    inline KJ_FOC_Utils::DQ parkTransform(const KJ_FOC_Utils::AlphaBeta& input, float cosElectricalTheta, float sinElectricalTheta)
    {
        KJ_FOC_Utils::DQ result;
        result.d = input.Alpha * cosElectricalTheta + input.Beta * sinElectricalTheta;
        result.q = -input.Alpha * sinElectricalTheta + input.Beta * cosElectricalTheta;
        return result;
    }
    inline KJ_FOC_Utils::AlphaBeta inverseParkTransform(const KJ_FOC_Utils::DQ& input, float cosElectricalTheta, float sinElectricalTheta)
    {
        KJ_FOC_Utils::AlphaBeta result;
        result.Alpha = input.d * cosElectricalTheta - input.q * sinElectricalTheta;
        result.Beta = input.d * sinElectricalTheta + input.q * cosElectricalTheta;
        return result;
    }
    inline KJ_FOC_Utils::Phase inverseClarkeTransform(const KJ_FOC_Utils::AlphaBeta& input)
    {
        KJ_FOC_Utils::Phase result;
        result.a = input.Alpha;
        result.b = (-input.Alpha + SQRT3 * input.Beta) / 2.0f;
        result.c = (-input.Alpha - SQRT3 * input.Beta) / 2.0f;
        return result;
    }
}

class KJ_FOC
{
    private:
        uint8_t  polePairs;
        float maxVoltageVector;
        float iqCurrentLimit;
        KJ_FOC_Utils::ControlMode controlMode;
        KJ_FOC_Utils::PIController idController;
        KJ_FOC_Utils::PIController iqController;
        float targetVq;
        float targetId;
        float targetIq;
    public:
        KJ_FOC(uint8_t polePairs, float maxVoltageVector, float iqCurrentLimit, KJ_FOC_Utils::ControlMode controlMode);
        void setPolePairs(uint8_t polePairs);
        void setMaxVoltageVector(float maxVoltageVector);
        void setCurrentLimit(float iqCurrentLimit);
        void setMode(KJ_FOC_Utils::ControlMode mode);
        void setCurrentPIGains(float d_kp,float d_ki,float q_kp,float q_ki);
        void setTargetVq(float vq);
        void setTargetIq(float iq);
        void setTargetId(float id);
        void reset();
        void resetControllers();
        KJ_FOC_Utils::Phase loop(const KJ_FOC_Utils::Phase& current,float electricalTheta,float dt); // ループ関数。センサ値とデルタ時間を渡すと、出力電圧を計算して返す
};





#endif /* KJ_FOC_HPP */
