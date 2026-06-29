#include "kj_foc.hpp"
#include "main.h"
#include "cordic_wrapper.hpp"
#include <cmath>
#define OPAMP_INIT_ERROR 0x10
#define ADC_INIT_ERROR 0x20
#define CORDIC_INIT_ERROR 0x40
#define FMAC_INIT_ERROR 0x80
#define MODULE_NOT_DEFINE_ERROR 0x01

#define EPSILON 0.0001f
namespace KJ_FOC_Utils {
    bool isinitialized = false;
    bool isNeedsensor = false;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717958647692f
#endif


extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern CORDIC_HandleTypeDef hcordic;
extern FDCAN_HandleTypeDef hfdcan1;
extern FMAC_HandleTypeDef hfmac;
extern I2C_HandleTypeDef hi2c2;
extern OPAMP_HandleTypeDef hopamp1;
extern OPAMP_HandleTypeDef hopamp2;
extern OPAMP_HandleTypeDef hopamp3;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern TIM_HandleTypeDef htim8;
extern DMA_HandleTypeDef hdma_tim4_ch2;
extern UART_HandleTypeDef huart3;

int KJ_FOC_Utils::initHardware()
{
    if(isinitialized) return 0;
    //HWが初期化されてなかったらエラーを返す．
    isinitialized = true;
    if(!isNeedsensor)
    {
        return 0; //センサーが必要ないモードならこれで初期化完了
    }
    #ifndef OPAMP
    //OPAMPが有効でない場合はエラーを返す
    return OPAMP_INIT_ERROR + MODULE_NOT_DEFINE_ERROR;
    #endif
    if(HAL_OPAMP_Init(&hopamp1) != HAL_OK ||
       HAL_OPAMP_Init(&hopamp2) != HAL_OK ||
       HAL_OPAMP_Init(&hopamp3) != HAL_OK)
    {
        //エラー処理
        return OPAMP_INIT_ERROR;
    }
    #ifndef ADC
    //ADCが有効でない場合はエラーを返す
    return ADC_INIT_ERROR + MODULE_NOT_DEFINE_ERROR;
    #endif
    if(HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK ||
       HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK)
    {
        //エラー処理
        return ADC_INIT_ERROR;
    }
    #ifndef CORDIC
    //CORDICが有効でない場合はエラーを返す
    return CORDIC_INIT_ERROR + MODULE_NOT_DEFINE_ERROR;
    #endif
    #ifndef FMAC
    //FMACが有効でない場合はエラーを返す
    return FMAC_INIT_ERROR + MODULE_NOT_DEFINE_ERROR;
    #endif

    return 0;

}
KJ_FOC::KJ_FOC(uint8_t polePairs, float maxVoltageVector, float iqCurrentLimit, KJ_FOC_Utils::ControlMode controlMode)
{
    // this->polePairs = polePairs;
    setPolePairs(polePairs);
    // this->maxVoltageVector = maxVoltageVector;
    setMaxVoltageVector(maxVoltageVector);
    // this->iqCurrentLimit = iqCurrentLimit;
    setCurrentLimit(iqCurrentLimit);
    // this->controlMode = controlMode;
    setMode(controlMode);
    if(this->controlMode == KJ_FOC_Utils::ControlMode::Current)
    {
        KJ_FOC_Utils::isNeedsensor = true;
    }
    KJ_FOC_Utils::initHardware();
}

void KJ_FOC::setCurrentPIGains(float d_kp,float d_ki,float q_kp,float q_ki){
    idController.kp = d_kp;
    idController.ki = d_ki;
    iqController.kp = q_kp;
    iqController.ki = q_ki;
}

void KJ_FOC::setPolePairs(uint8_t polePairs)
{
    this->polePairs = polePairs;
}

void KJ_FOC::setMaxVoltageVector(float maxVoltageVector)
{
    this->maxVoltageVector = maxVoltageVector;
}

void KJ_FOC::setCurrentLimit(float iqCurrentLimit)
{
    this->iqCurrentLimit = iqCurrentLimit;
}

void KJ_FOC::setMode(KJ_FOC_Utils::ControlMode mode)
{
    this->controlMode = mode;
}

void KJ_FOC::setTargetVq(float targetVq)
{
    this->targetVq = targetVq;
}

KJ_FOC_Utils::Phase KJ_FOC::loop(const KJ_FOC_Utils::Phase& current,float electricalTheta,float dt){
    // 1. クラーク変換
    KJ_FOC_Utils::AlphaBeta alphaBeta = KJ_FOC_Utils::clarkeTransform(current);
    // 2. パーク変換
    float cosTheta, sinTheta;
    // electricalThetaの範囲指定(-PI ~ PI)
    electricalTheta = fmodf(electricalTheta, 2.0f * float(M_PI)) - float(M_PI);
    CORDIC_Wrapper::sin_cos(electricalTheta, &sinTheta, &cosTheta);
    KJ_FOC_Utils::DQ dq = KJ_FOC_Utils::parkTransform(alphaBeta, cosTheta, sinTheta);
    // 3. PI制御器でd軸とq軸の電圧を計算
    float error_d = targetId - dq.d;
    float error_q = targetIq - dq.q;
    if(error_d < EPSILON && error_d > -EPSILON) {
        idController.integral = 0.0f; //誤差が小さいときは積分をリセットしてオーバーシュートを防止
    }
    if(error_q < EPSILON && error_q > -EPSILON) {
        iqController.integral = 0.0f; //誤差が小さいときは積分をリセットしてオーバーシュートを防止
    }
    // 積分
    idController.integral += error_d * dt;
    iqController.integral += error_q * dt;
    // 積分風上限下限
    idController.integral = idController.integral > idController.integralLimit ? idController.integralLimit : idController.integral;
    idController.integral = idController.integral < -idController.integralLimit ? -idController.integralLimit : idController.integral;
    iqController.integral = iqController.integral > iqController.integralLimit ? iqController.integralLimit : iqController.integral;
    iqController.integral = iqController.integral < -iqController.integralLimit ? -iqController.integralLimit : iqController.integral;
    float vd = idController.kp * error_d + idController.ki * idController.integral;
    float vq = iqController.kp * error_q + iqController.ki * iqController.integral;
    // 電流制限
    if(vq > iqCurrentLimit) vq = iqCurrentLimit;
    if(vq < -iqCurrentLimit) vq = -iqCurrentLimit;
    // 4. インバースパーク変換でアルファベータ軸の電圧を計算
    KJ_FOC_Utils::AlphaBeta outputAlphaBeta = KJ_FOC_Utils::inverseParkTransform({vd, vq}, cosTheta, sinTheta);
    // 5. インバースククラーク変換でabc軸の電圧を計算して返す
    return KJ_FOC_Utils::inverseClarkeTransform(outputAlphaBeta);
}

void KJ_FOC::reset()
{
    targetId = 0.0f;
    targetIq = 0.0f;
    targetVq = 0.0f;
    resetControllers();
}

void KJ_FOC::resetControllers()
{
    idController.integral = 0.0f;
    iqController.integral = 0.0f;
}