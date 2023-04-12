/*
 * lps25hb.c
 *
 *  Created on: Apr 8, 2023
 *      Author: tomek
 */


#include "lps25hb.h"
#include "i2c.h"

#define LPS25HB_ADDR        0xBA

#define LPS25HB_WHO_AM_I 		0x0F
#define LPS25HB_CTRL_REG1 		0x20
#define LPS25HB_CTRL_REG2 		0x21
#define LPS25HB_CTRL_REG3 		0x22
#define LPS25HB_CTRL_REG4 		0x23
#define LPS25HB_PRESS_OUT_XL 	0x28
#define LPS25HB_PRESS_OUT_L 	0x29
#define LPS25HB_PRESS_OUT_H 	0x2A
#define LPS25HB_TEMP_OUT_L 		0x2B
#define LPS25HB_TEMP_OUT_H 		0x2C

#define TIMEOUT                 100

static uint8_t lps_read_reg(uint8_t reg)
{
    uint8_t value = 0;
    HAL_I2C_Mem_Read(&hi2c1, LPS25HB_ADDR, reg, 1, &value, sizeof(value), TIMEOUT);

    return value;
}

static void lps_write_reg(uint8_t reg, uint8_t value)
{
    HAL_I2C_Mem_Write(&hi2c1, LPS25HB_ADDR, reg, 1, &value, sizeof(value), TIMEOUT);
}

HAL_StatusTypeDef lps25hb_init(void)
{
    if (lps_read_reg(LPS25HB_WHO_AM_I) != 0xBD)
         return HAL_ERROR;

    lps_write_reg(LPS25HB_CTRL_REG1,  0x80); //wake up sensor
    return HAL_OK;
}

HAL_StatusTypeDef lps25hb_start_measure(void)
{
    lps_write_reg(LPS25HB_CTRL_REG1,  0xC0);
        return HAL_OK;
}

HAL_StatusTypeDef lps25hb_one_shot(void)
{
    lps_write_reg(LPS25HB_CTRL_REG2, 0x01);
    return HAL_OK;
}

HAL_StatusTypeDef lps25hb_go_sleep(void)
{
	  lps_write_reg(LPS25HB_CTRL_REG1, 0);
    return HAL_OK;
}

float lps25hb_read_temp(void)
{
     int16_t temp;

     if (HAL_I2C_Mem_Read(&hi2c1, LPS25HB_ADDR, LPS25HB_TEMP_OUT_L | 0x80, 1, (uint8_t*)&temp, sizeof(temp), TIMEOUT) != HAL_OK)
         Error_Handler();

     return 42.5f + temp / 480.0f;
}

float lps25hb_read_pressure(void)
{
     int32_t pressure = 0;

     if (HAL_I2C_Mem_Read(&hi2c1, LPS25HB_ADDR, LPS25HB_PRESS_OUT_XL | 0x80, 1, (uint8_t*)&pressure, 3, TIMEOUT) != HAL_OK)
         Error_Handler();

     return (pressure / 4096.0f)+9;
}

