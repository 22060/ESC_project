#include "AS5048A.hpp"
#define MAX_AS5048A 4
static AS5048A* instance_AS5048A[MAX_AS5048A];
static int instanceCount_AS5048A = 0;

AS5048A::AS5048A(AS5048A_MODE mode, Pins cspin, SPI_HandleTypeDef* hspi, uint16_t zeroPosition, void (*interruptHandler)(void)) {
    this->mode = mode;
    this->CSPin = cspin;
    this->hspi = hspi;
    this->zeroPosition = zeroPosition;
    this->EnableInterrupt = false;
    if(instanceCount_AS5048A < MAX_AS5048A)
    {
        instance_AS5048A[instanceCount_AS5048A++] = this;
    }
    if(interruptHandler != nullptr && (mode == AS5048A_MODE::SINGLE_READ_ONLY)) {
        EnableInterrupt = true;
        this->interruptHandler = interruptHandler;
        uint16_t frame_buffer = 0x0000;
        HAL_SPI_Transmit_DMA(this->hspi, (uint8_t*)&frame_buffer, 2);
    }
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for(int i = 0; i < instanceCount_AS5048A; i++)
    {
        if(instance_AS5048A[i]->getSPIHandle() == hspi)
        {
            uint8_t rx[2];
            instance_AS5048A[i]->SPI_CS_Deselect();
            instance_AS5048A[i]->lastAngle = (rx[0] << 8) | rx[1]; // 受信したフレームから角度データを抽出して保存
            
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            instance_AS5048A[i]->SPI_CS_Select();
            uint16_t frame_buffer = 0x0000; // 送信するフレームの初期値
            HAL_SPI_TransmitReceive_DMA(instance_AS5048A[i]->getSPIHandle(), (uint8_t*)&frame_buffer, rx, 2); // DMAを使用してフレームを送信
            if(instance_AS5048A[i]->EnableInterrupt)
            {
                instance_AS5048A[i]->interruptHandler(); // 割り込みハンドラを呼び出す
            }
        }
    }
}

void AS5048A::clearErrorFlag() {
    read(COMMAND_READ_AGC); // エラーフラグをクリアするためにエラー状態を読み取る
    read(COMMAND_READ_CLEAR_ERROR_FLAG); // エラーフラグをクリアするコマンドを送信
}

uint16_t AS5048A::getErrorAndAGC() {
    return read(COMMAND_READ_AGC); // AGC値とエラーフラグを含むデータを読み取る
}

void AS5048A::setZeroPosition(uint16_t zeroPosition) {
    zeroPosition = zeroPosition & AS5048A_DATA_MASK; // ゼロ位置は14ビットのデータなので、マスクをかけて上位ビットをクリア
    write(COMMAND_RW_ZERO_HI, (zeroPosition >> 6) & 0xFF); // 上位8ビットをゼロ位置のレジスタに書き込む
    write(COMMAND_RW_ZERO_LOW, (zeroPosition) & 0x3F); // 下位8ビットをゼロ位置のレジスタに書き込む
}

uint16_t AS5048A::getMagnitude() {
    return read(COMMAND_READ_AGC);
}
