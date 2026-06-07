/**
  ******************************************************************************
  * @file           : dac3100.h
  * @brief          : Header for dac3100.c file.
  ******************************************************************************
  */
#ifndef __DAC3100_H
#define __DAC3100_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported Defines ----------------------------------------------------------*/
#define DAC3100_I2C_ADDR (0x18 << 1)
static uint8_t current_page = 0xFF;

/* Exported Functions Prototypes ---------------------------------------------*/
void DAC3100_Init(I2C_HandleTypeDef *hi2c);
void DAC3100_SetPage(I2C_HandleTypeDef *hi2c, uint8_t page);
void DAC3100_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data);
void tlv3100_mute(I2C_HandleTypeDef *hi2c);
void tlv3100_unmute(I2C_HandleTypeDef *hi2c);
#ifdef __cplusplus
}
#endif

#endif /* __DAC3100_H */