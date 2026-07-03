#include "AS5048A.hpp"
#include <cstring>
#include <cstdio>
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
    }
}
void AS5048A::init() {
    // CSピンを初期化
    // HAL_GPIO_WritePin(CSPin.port, CSPin.pin, GPIO_PIN_SET); // CSをHIGHに設定
    // GPIO_InitTypeDef GPIO_InitStruct = {0};
    // GPIO_InitStruct.Pin = CSPin.pin;
    // GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // GPIO_InitStruct.Pull = GPIO_NOPULL;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // HAL_GPIO_Init(CSPin.port, &GPIO_InitStruct);
    
    // // ゼロ位置の設定
    // // setZeroPosition(zeroPosition);
    // while (HAL_SPI_GetState(this->hspi) != HAL_SPI_STATE_READY) {
    //     // タイムアウト対策を入れるとより安全です
    // }
    // if(interruptHandler != nullptr && (mode == AS5048A_MODE::SINGLE_READ_ONLY)) {
    //     static uint16_t frame_buffer = 0x0000;
    //     static HAL_StatusTypeDef spi_status;
    //     extern UART_HandleTypeDef huart3;
    //     SPI_CS_Select();
    //     spi_status = HAL_SPI_TransmitReceive_DMA(this->hspi, (uint8_t*)&frame_buffer, (uint8_t*)&frame_buffer, 2);
    //     HAL_UART_Transmit(&huart3, (uint8_t*)"AS5048A interrupt setting fin. init status: ", 44, HAL_MAX_DELAY);
    //     if(spi_status == HAL_OK) {
    //         HAL_UART_Transmit(&huart3, (uint8_t*)"OK\r\n", 4, HAL_MAX_DELAY);
    //     } else{
    //         char msg[32];
    //         sprintf(msg, "NG: State=%d\r\n", HAL_SPI_GetState(this->hspi));
    //         HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), 100);
    //     }
    // }
}
// void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
// {
//     for(int i = 0; i < instanceCount_AS5048A; i++)
//     {
//         if(instance_AS5048A[i]->getSPIHandle() == hspi)
//         {
//             static uint8_t rx[2];
//             instance_AS5048A[i]->SPI_CS_Deselect();
//             instance_AS5048A[i]->lastAngle = (rx[0] << 8) | rx[1]; // 受信したフレームから角度データを抽出して保存
            
//             __NOP();
//             __NOP();
//             __NOP();
//             __NOP();
//             __NOP();
//             __NOP();
//             __NOP();
//             instance_AS5048A[i]->SPI_CS_Select();
//             static uint16_t frame_buffer = 0x0000; // 送信するフレームの初期値
//             HAL_SPI_TransmitReceive_DMA(instance_AS5048A[i]->getSPIHandle(), (uint8_t*)&frame_buffer, rx, 2); // DMAを使用してフレームを送信
//             if(instance_AS5048A[i]->EnableInterrupt)
//             {
//                 instance_AS5048A[i]->interruptHandler(); // 割り込みハンドラを呼び出す
//             }
//         }
//     }
// }

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
