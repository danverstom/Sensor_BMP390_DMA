//
// Created by Tom Danvers on 02/10/2024.

#ifndef INCLUDE_GUARD_INC_BMP390
#define INCLUDE_GUARD_INC_BMP390
//

#ifndef BMP390_H
#define BMP390_H
#include "stm32f1xx_hal.h"

// constants
#define BMP390_I2C_ADDRESS      0x77 << 1
#define BMP390_CHIP_ID          0x60

// registers
#define BMP390_REG_CHIP_ID      0x00
#define BMP390_REG_CMD          0x7E
#define BMP390_REG_PWR_CTRL     0x1B
#define BMP390_REG_EVENT        0x10
#define BMP390_REG_OSR          0x1C
#define BMP390_REG_ODR          0x1D
#define BMP390_REG_CONFIG       0x1F
#define BMP390_REG_INT_CTRL     0x19
#define BMP390_REG_INT_STATUS   0x11

void BMP390_Init(I2C_HandleTypeDef *i2c_handle);

HAL_StatusTypeDef BMP390_Read(uint16_t MemAddress, uint8_t *pData, uint16_t Size);

HAL_StatusTypeDef BMP390_Write(uint16_t MemAddress, uint8_t *pData, uint16_t Size);

float BMP390_CompensateTemperature(uint32_t uncomp_temp);

float BMP390_CompensatePressure(uint32_t uncomp_press, float t_lin);

HAL_StatusTypeDef BMP390_ReadPressureAndTemperature(float *pressure, float *temperature);

HAL_StatusTypeDef BMP390_ReadPressureAndTemperature_IT();

#endif //BMP390_H


#endif /* INCLUDE_GUARD_INC_BMP390 */
