#ifndef AS5048A_HPP
#define AS5048A_HPP

#include "main.h"

#define COMMAND_READ_ANGLE 0x3FFF
#define COMMAND_READ_MAGNITUDE 0x3FFE
#define COMMAND_READ_AGC 0x3FFD
#define COMMAND_RW_ZERO_HI 0x0016 // 6 ~ 13 bit
#define COMMAND_RW_ZERO_LOW 0x0017 //0 ~ 5 bit

#define COMMAND_NOP 0x0000
#define COMMAND_READ_CLEAR_ERROR_FLAG 0x0001

#define AS5048A_ERROR_PARITY 0x8000
#define AS5048A_ERROR_SEND_COMMAND 0x4000
#define AS5048A_ERROR_SPI 0xE000
#define AS5048A_DATA_MASK 0x3FFF

#define SPI_READ_FLAG true
#define SPI_WRITE_FLAG false

// 【警告対策】M_PIの二重定義を防ぐ
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

enum class AS5048A_COMMANDS{
    READ_ANGLE = 0x3FFF,
    READ_MAGNITUDE = 0x3FFE,
    READ_AGC = 0x3FFD,
    RW_ZERO_HI = 0x0016, // 6 ~ 13 bit
    RW_ZERO_LOW = 0x0017, //0 ~ 5 bit
    NOP = 0x0000,
    READ_CLEAR_ERROR_FLAG = 0x0001
};

enum class AS5048A_MODE{
    SINGLE_READ_WRITE, //単体のAS5048Aを読み書きするモード
    SINGLE_READ_ONLY //MOSIを使わず，SSの制御のみでAS5048Aを読み取るモード
};

struct Pins{
    uint16_t pin;
    GPIO_TypeDef* port;
};

static inline uint16_t makePacket(uint16_t data, bool rw)
{
    uint16_t packet =
        ((rw & 1) << 14) |
        (data & 0x3FFF);

    if(__builtin_popcount((unsigned int)packet) & 1)
    {
        packet |= 0x8000;
    }

    return packet;
}


class AS5048A{
    private:
    uint16_t zeroPosition;
    AS5048A_MODE mode;
    SPI_HandleTypeDef* hspi; //SPI通信のハンドル
    Pins CSPin; //CSピンの情報
    
    inline uint16_t read(uint16_t address){

        uint16_t packet = makePacket(address, SPI_READ_FLAG); 
        
        uint8_t tx[2] = { 
            static_cast<uint8_t>((packet >> 8) & 0xFF), 
            static_cast<uint8_t>(packet & 0xFF) 
        }; 
        uint8_t rx[2];
        
        // 1. 読み取りコマンドを送信 (TransmitではなくTransmitReceiveを使う)
        SPI_CS_Select();
        if(HAL_SPI_TransmitReceive(hspi, tx, rx, 2, HAL_MAX_DELAY) != HAL_OK) {
            SPI_CS_Deselect();
            return AS5048A_ERROR_SPI;
        }
        SPI_CS_Deselect();
        
        // AS5048Aのt_L(CS高期間)は最低350ns必要。余裕を持って少し長めに待つ
        for(volatile int i = 0; i < 50; i++); 
        
        // 2. NOPを送信して、1で要求したデータを回収する
        packet = makePacket(0, SPI_READ_FLAG); // NOP(0x0000) または 別のコマンド
        tx[0] = static_cast<uint8_t>((packet >> 8) & 0xFF);
        tx[1] = static_cast<uint8_t>(packet & 0xFF);
        
        SPI_CS_Select();
        if(HAL_SPI_TransmitReceive(hspi, tx, rx, 2, HAL_MAX_DELAY) != HAL_OK) {
            SPI_CS_Deselect();
            return AS5048A_ERROR_SPI;
        }
        SPI_CS_Deselect();
        
        for(volatile int i = 0; i < 50; i++); 
        
        uint16_t response = (rx[0] << 8) | rx[1]; // 受信データの結合

        // パリティチェック (戻り値全体のポップカウントが偶数(0)ならOK)
        if(__builtin_popcount((unsigned int)response) & 1)
            return AS5048A_ERROR_PARITY; 
        
        // エラーフラグ(Bit14)の確認
        if ((response & AS5048A_ERROR_SEND_COMMAND) == 0) { 
            return response & AS5048A_DATA_MASK; 
        } else {
            return AS5048A_ERROR_SEND_COMMAND; 
        }
    }

    inline uint16_t write(uint16_t address, uint16_t data){
        uint8_t tx[2];
        uint8_t rx[2];
        uint16_t packet = makePacket(address, SPI_WRITE_FLAG); // RWビットを0にして書き込みコマンドを作成
        tx[0] = static_cast<uint8_t>((packet >> 8) & 0xFF);
        tx[1] = static_cast<uint8_t>(packet & 0xFF);
        
        //send write command
        SPI_CS_Select();
        if(HAL_SPI_Transmit(hspi, tx, 2, HAL_MAX_DELAY) != HAL_OK) {
            SPI_CS_Deselect();
            return AS5048A_ERROR_SPI; // 通信エラーが発生した場合はエラーフラグを立てた値を返す
        }
        SPI_CS_Deselect();
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
        
        //send data
        packet = makePacket(data, SPI_WRITE_FLAG); // 書き込むデータをコマンド形式に変換
        tx[0] = static_cast<uint8_t>((packet >> 8) & 0xFF); //上位8ビット
        tx[1] = static_cast<uint8_t>(packet & 0xFF); //下位8ビット
        
        SPI_CS_Select();
        if(HAL_SPI_Transmit(hspi, tx, 2, HAL_MAX_DELAY) != HAL_OK) {
            SPI_CS_Deselect();
            return AS5048A_ERROR_SPI; // 通信エラーが発生した場合はエラーフラグを立てた値を返す
        }
        SPI_CS_Deselect();
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
        
        //receive response
        packet = makePacket(0, SPI_READ_FLAG); // NOPコマンド(R)を作成
        tx[0] = static_cast<uint8_t>((packet >> 8) & 0xFF);
        tx[1] = static_cast<uint8_t>(packet & 0xFF);
        
        SPI_CS_Select();
        if(HAL_SPI_TransmitReceive(hspi, tx, rx, 2, HAL_MAX_DELAY) != HAL_OK) {
            SPI_CS_Deselect();
            return AS5048A_ERROR_SPI; // 通信エラーが発生した場合はエラーフラグを立てた値を返す
        }
        SPI_CS_Deselect();
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
        
        uint16_t response = (rx[0] << 8) | rx[1]; //受信したデータを16ビットに結合
        
        // parity check
        if(__builtin_popcount((unsigned int)response) & 1)
        {
            return AS5048A_ERROR_PARITY; // パリティエラーがある場合はエラーフラグを立てた値を返す
        }
        if ((response & AS5048A_ERROR_SEND_COMMAND) == 0) { //エラーフラグが立っていないか確認
            return response & AS5048A_DATA_MASK; //エラーがなければデータ部分を返す
        } else {
            return AS5048A_ERROR_SEND_COMMAND; // パリティエラーがある場合はエラーフラグを立てた値を返す
        }
    }
    
    public:
    inline void SPI_CS_Select(void)   { HAL_GPIO_WritePin(CSPin.port, CSPin.pin, GPIO_PIN_RESET); }
    inline void SPI_CS_Deselect(void) { HAL_GPIO_WritePin(CSPin.port, CSPin.pin, GPIO_PIN_SET); }
    bool EnableInterrupt;
    uint16_t lastAngle; //前回の角度を保存する変数
    AS5048A(AS5048A_MODE mode, Pins cspin, SPI_HandleTypeDef* hspi, uint16_t zeroPosition = 0, void (*interruptHandler)(void) = nullptr);
    void init();
    void clearErrorFlag();
    uint16_t getErrorAndAGC();
    void setZeroPosition(uint16_t zeroPosition);
    inline uint16_t getAngle(){
        if(mode == AS5048A_MODE::SINGLE_READ_WRITE)
        {
            lastAngle = read(COMMAND_READ_ANGLE);
        }
        return lastAngle; 
    }
    inline float getAngleDegree(){
        return (float)getAngle() * 360.0f / 16384.0f;
    }
    inline float getAngleRadian(){
        return (float)getAngle() * 2.0f * M_PI / 16384.0f;
    }
    void loop(); //SINGLE_READ_ONLYのとき無効
    uint16_t getMagnitude();
    inline SPI_HandleTypeDef* getSPIHandle() { return hspi; }
    void (*interruptHandler)() = nullptr;
};

#endif /* AS5048A_HPP */