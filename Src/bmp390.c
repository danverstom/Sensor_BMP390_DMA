//
// Created by Tom Danvers on 02/10/2024.
//

#include "bmp390.h"
#include "stdio.h"

I2C_HandleTypeDef *bmp390_i2c_handle;
HAL_StatusTypeDef i2c_status;

uint16_t NVM_PAR_T1;
uint16_t NVM_PAR_T2;
int8_t NVM_PAR_T3;
int16_t NVM_PAR_P1;
int16_t NVM_PAR_P2;
int8_t NVM_PAR_P3;
int8_t NVM_PAR_P4;
uint16_t NVM_PAR_P5;
uint16_t NVM_PAR_P6;
int8_t NVM_PAR_P7;
int8_t NVM_PAR_P8;
int16_t NVM_PAR_P9;
int8_t NVM_PAR_P10;
int8_t NVM_PAR_P11;

float par_t1;
float par_t2;
float par_t3;

float par_p1;
float par_p2;
float par_p3;
float par_p4;
float par_p5;
float par_p6;
float par_p7;
float par_p8;
float par_p9;
float par_p10;
float par_p11;

float partial_data1;
float partial_data2;
float partial_data3;
float partial_data4;
float partial_out1;
float partial_out2;

uint8_t chip_id_interrupt_test;

uint8_t data[6] = {0};

float temperature = 0;
float pressure = 0;

void BMP390_Init(I2C_HandleTypeDef *hi2c)
{
    bmp390_i2c_handle = hi2c;

    // check if device is ready for I2C communications
    printf("BMP390_Init: Checking if device is ready for I2C communications\n\r");
    i2c_status = HAL_I2C_IsDeviceReady(hi2c, BMP390_I2C_ADDRESS, 10, HAL_MAX_DELAY);

    if (i2c_status != HAL_OK)
    {
        printf("BMP390_Init: Device is not ready\n\r");
        return;
    }

    printf("BMP390_Init: Device is ready for I2C communications\n\r");

    uint8_t chip_id;

    // read chip ID from sensor to verify good I2C connection
    i2c_status = BMP390_Read(BMP390_REG_CHIP_ID, &chip_id, 1);
    if (chip_id != BMP390_CHIP_ID)
    {
        printf("BMP390_Init: chip ID is not correct\n\r");
        return;
    }
    printf("BMP390_Init: chip ID is OK\n\r");

    // perform a soft reset of the device
    uint8_t soft_reset_command = 0xB6;
    i2c_status = BMP390_Write(BMP390_REG_CMD, &soft_reset_command, 1);

    if (i2c_status == HAL_OK)
    {
        printf("BMP390_Init: Soft reset command issued\n\r");
    }
    else
    {
        printf("BMP390_Init: Soft reset command failed\n\r");
        return;
    }

    // small delay to allow the device to complete the soft reset
    HAL_Delay(100);

    // verify device reset by checking the EVENT register
    uint8_t event_register;
    i2c_status = BMP390_Read(BMP390_REG_EVENT, &event_register, 1);
    // bit 0 should read "1" to indicate a power on reset
    if (i2c_status == HAL_OK && (event_register & 0b01) == 0b01)
    {
        printf("BMP390_Init: EVENT register indicates good soft reset\n\r");
    }
    else
    {
        printf("BMP390_Init: EVENT register shows that softreset failed\n\r");
        return;
    }

    // also check that the PWR_CTRL register value is 0x00, indicating a good reset
    uint8_t pwr_ctrl_status;
    i2c_status = BMP390_Read(BMP390_REG_PWR_CTRL, &pwr_ctrl_status, 1);

    if (pwr_ctrl_status != 0x00 && i2c_status == HAL_OK)
    {
        printf("BMP390_Init: Failed to reset device (PWR_CTRL not 0x00)\n\r");
        return;
    }

    // read the trimming coefficients from the device
    const uint8_t buffer[21] = {0};

    i2c_status = BMP390_Read(0x31, buffer, 21);

    NVM_PAR_T1 = (uint16_t)(buffer[1] << 8 | buffer[0]);
    NVM_PAR_T2 = (uint16_t)(buffer[3] << 8 | buffer[2]);
    NVM_PAR_T3 = (int8_t)buffer[4];
    NVM_PAR_P1 = (int16_t)(buffer[6] << 8 | buffer[5]);
    NVM_PAR_P2 = (int16_t)(buffer[8] << 8 | buffer[7]);
    NVM_PAR_P3 = (int8_t)buffer[9];
    NVM_PAR_P4 = (int8_t)buffer[10];
    NVM_PAR_P5 = (uint16_t)(buffer[12] << 8 | buffer[11]);
    NVM_PAR_P6 = (uint16_t)(buffer[14] << 8 | buffer[13]);
    NVM_PAR_P7 = (int8_t)buffer[15];
    NVM_PAR_P8 = (int8_t)buffer[16];
    NVM_PAR_P9 = (int16_t)(buffer[18] << 8 | buffer[17]);
    NVM_PAR_P10 = (int8_t)buffer[19];
    NVM_PAR_P11 = (int8_t)buffer[20];

    // Now calculate the actual parameters based on the formulas
    par_t1 = (float)NVM_PAR_T1 * (float)(1ULL << 8);
    par_t2 = (float)NVM_PAR_T2 / (float)(1ULL << 30);
    par_t3 = (float)NVM_PAR_T3 / (float)(1ULL << 48);

    // Pressure parameters
    par_p1 = ((float)NVM_PAR_P1 - (float)(1ULL << 14)) / (float)(1ULL << 20);
    par_p2 = ((float)NVM_PAR_P2 - (float)(1ULL << 14)) / (float)(1ULL << 29);
    par_p3 = (float)NVM_PAR_P3 / (float)(1ULL << 32);
    par_p4 = (float)NVM_PAR_P4 / (float)(1ULL << 37);
    par_p5 = (float)NVM_PAR_P5 * (float)(1ULL << 3);
    par_p6 = (float)NVM_PAR_P6 / (float)(1ULL << 6);
    par_p7 = (float)NVM_PAR_P7 / (float)(1ULL << 8);
    par_p8 = (float)NVM_PAR_P8 / (float)(1ULL << 15);
    par_p9 = (float)NVM_PAR_P9 / (float)(1ULL << 48);
    par_p10 = (float)NVM_PAR_P10 / (float)(1ULL << 48);
    par_p11 = (float)NVM_PAR_P11 / (float)36893488147419103232.0;

    // perform other config
    // we will select "indoor navigation" and use the recommended configuration in the
    // datasheet
    // osrs_p = x16, osrs_t = x2, IIR filter coeff = 4, ODR = 25 Hz

    // start by setting the oversampling settings
    // osr_p = 100, osr_t = 001
    uint8_t osr_setting = 0b00001100; // 0b00_001_100 = 0b00_(osr_t)_(osr_p)
    i2c_status = BMP390_Write(BMP390_REG_OSR, &osr_setting, 1);
    if (i2c_status != HAL_OK)
    {
        printf("BMP390_Init: Failed to set OSR setting\n\r");
        return;
    }
    printf("BMP390_Init: OSR settings updated\n\r");

    // set the IIR filter coefficient
    // we will actually set it to 3 (010) instead of 4
    uint8_t config_iir_setting = 0b000001000; // 0b0000_010_00
    i2c_status = BMP390_Write(BMP390_REG_CONFIG, &config_iir_setting, 1);
    if (i2c_status != HAL_OK)
    {
        printf("BMP390_Init: Failed to set IIR setting\n\r");
        return;
    }
    printf("BMP390_Init: IIR settings updated\n\r");

    // configure the ODR (output data rate)
    // odr_sel = 0x03, prescaler = 8
    uint8_t odr_sel = 0x03;
    i2c_status = BMP390_Write(BMP390_REG_ODR, &odr_sel, 1);
    if (i2c_status != HAL_OK)
    {
        printf("BMP390_Init: Failed to set ODR setting\n\r");
        return;
    }
    printf("BMP390_Init: ODR settings updated (%x)\n\r", odr_sel);

    // TODO: Learn how to use DMA to do the read/write in an optimised way

    uint8_t pwr_ctrl, osr, config, odr;
    BMP390_Read(BMP390_REG_PWR_CTRL, &pwr_ctrl, 1);
    BMP390_Read(BMP390_REG_OSR, &osr, 1);
    BMP390_Read(BMP390_REG_CONFIG, &config, 1);
    BMP390_Read(BMP390_REG_ODR, &odr, 1);
    printf("PWR_CTRL: 0x%02X, OSR: 0x%02X, CONFIG: 0x%02X, ODR: 0x%02X\n\r", pwr_ctrl, osr, config, odr);

    // set the desired power mode (normal mode with pressure and temperature measurement enabled)
    uint8_t desired_power_mode = 0b00110011;
    i2c_status = BMP390_Write(BMP390_REG_PWR_CTRL, &desired_power_mode, 1);

    if (i2c_status != HAL_OK)
    {
        printf("BMP390_Init: Failed to set desired power mode\n\r");
        return;
    }

    // verify that the value inside PWR_CTRL has changed
    i2c_status = BMP390_Read(BMP390_REG_PWR_CTRL, &pwr_ctrl_status, 1);

    if (pwr_ctrl_status != desired_power_mode || i2c_status != HAL_OK)
    {
        printf("BMP390_Init: Failed to update Power Mode to Normal Mode\n\r");
        return;
    }
    printf("BMP390_Init: Power mode updated to Normal Mode\n\r");

    printf("BMP390_Init: Finished\n\r");
}

HAL_StatusTypeDef BMP390_Read(uint16_t MemAddress, uint8_t *pData, uint16_t Size)
{
    return HAL_I2C_Mem_Read(
        bmp390_i2c_handle,
        BMP390_I2C_ADDRESS,
        MemAddress,
        I2C_MEMADD_SIZE_8BIT,
        pData,
        Size,
        HAL_MAX_DELAY);
}

HAL_StatusTypeDef BMP390_Write(uint16_t MemAddress, uint8_t *pData, uint16_t Size)
{
    return HAL_I2C_Mem_Write(
        bmp390_i2c_handle,
        BMP390_I2C_ADDRESS,
        MemAddress,
        I2C_MEMADD_SIZE_8BIT,
        pData,
        Size,
        HAL_MAX_DELAY);
}

float BMP390_CompensateTemperature(uint32_t uncomp_temp)
{
    // refer to datasheet page 55
    // https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp390-ds002.pdf
    const float partial_data1 = (float)uncomp_temp - par_t1;
    const float partial_data2 = partial_data1 * par_t2;

    return partial_data2 + partial_data1 * partial_data1 * par_t3;
}

float BMP390_CompensatePressure(uint32_t uncomp_press, float t_lin)
{
    float partial_data1 = par_p6 * t_lin;
    float partial_data2 = par_p7 * (t_lin * t_lin);
    float partial_data3 = par_p8 * (t_lin * t_lin * t_lin);
    float partial_out1 = par_p5 + partial_data1 + partial_data2 + partial_data3;

    partial_data1 = par_p2 * t_lin;
    partial_data2 = par_p3 * (t_lin * t_lin);
    partial_data3 = par_p4 * (t_lin * t_lin * t_lin);
    float partial_out2 = (float)uncomp_press * (par_p1 + partial_data1 + partial_data2 + partial_data3);

    partial_data1 = (float)uncomp_press * (float)uncomp_press;
    partial_data2 = par_p9 + par_p10 * t_lin;
    partial_data3 = partial_data1 * partial_data2;
    float partial_data4 = partial_data3 + (float)uncomp_press * (float)uncomp_press * (float)uncomp_press *
                                              par_p11;
    float comp_press = partial_out1 + partial_out2 + partial_data4;
    return comp_press;
}

HAL_StatusTypeDef BMP390_ReadPressureAndTemperature(float *pressure, float *temperature)
{

    uint8_t device_status = 0;
    i2c_status = BMP390_Read(0x03, &device_status, 1);

    uint8_t device_power_mode = 0;
    i2c_status = BMP390_Read(0x1B, &device_power_mode, 1);

    i2c_status = BMP390_Read(0x04, data, 6);

    // if we failed, do not update the external variables pressure and temperature
    if (i2c_status != HAL_OK)
    {
        return i2c_status;
    }

    i2c_status = BMP390_Read(0x03, &device_status, 1);

    const uint32_t uncomp_press = data[2] << 16 | data[1] << 8 | data[0];
    const uint32_t uncomp_temp = data[5] << 16 | data[4] << 8 | data[3];

    *temperature = BMP390_CompensateTemperature(uncomp_temp);
    *pressure = BMP390_CompensatePressure(uncomp_press, *temperature);

    return i2c_status;
}

HAL_StatusTypeDef BMP390_ReadPressureAndTemperature_DMA()
{
    printf("Starting DMA request\n\r");

    uint32_t start = HAL_GetTick();
    i2c_status = HAL_I2C_Mem_Read_DMA(bmp390_i2c_handle, BMP390_I2C_ADDRESS, 0x04, I2C_MEMADD_SIZE_8BIT, data, 6);

    printf("DMA request sent\n\r");

    printf("Pressure: %d, Temperature: %d\n\r", (int)temperature, (int)pressure);

    return i2c_status;
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == bmp390_i2c_handle)
    {
        const uint32_t uncomp_press = data[2] << 16 | data[1] << 8 | data[0];
        const uint32_t uncomp_temp = data[5] << 16 | data[4] << 8 | data[3];

        temperature = BMP390_CompensateTemperature(uncomp_temp);
        pressure = BMP390_CompensatePressure(uncomp_press, temperature);
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    printf("I2C DMA error!");
}
