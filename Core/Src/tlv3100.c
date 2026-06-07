
#include "tlv3100.h"


void DAC3100_SetPage(I2C_HandleTypeDef *hi2c, uint8_t page)

{

if (page != current_page) {

uint8_t buffer[2] = {0x00, page}; // Register 0 is always Page Select

HAL_I2C_Master_Transmit(hi2c, DAC3100_I2C_ADDR, buffer, 2, 100);

current_page = page;

}

}



void DAC3100_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data)

{

uint8_t buffer[2] = {reg, data};

HAL_I2C_Master_Transmit(hi2c, DAC3100_I2C_ADDR, buffer, 2, 100);

}





void DAC3100_Init(I2C_HandleTypeDef *hi2c)
{
    // =========================================================================
    // 1. Define starting point
    // =========================================================================
    
    // (b) Set register page to 0
    DAC3100_SetPage(hi2c, 0); 
    
    // (c) Initiate SW reset (PLL is powered off as part of reset)
    DAC3100_WriteRegister(hi2c, 0x01, 0x01); 
    HAL_Delay(10); // Brief pause to allow reset to complete

    // =========================================================================
    // 2. Program clock settings
    // =========================================================================
    
    // (a) Program PLL clock dividers P, J, D, R
    DAC3100_WriteRegister(hi2c, 0x04, 0x03); // PLL_clkin = MCLK, codec_clkin = PLL_CLK
    DAC3100_WriteRegister(hi2c, 0x06, 0x08); // J = 8
    
    // D = 0000 (Sequential write in script: w 30 07 00 00)
    DAC3100_WriteRegister(hi2c, 0x07, 0x00); // D(13:8) = 0
    DAC3100_WriteRegister(hi2c, 0x08, 0x00); // D(7:0) = 0

    // (b) Power up PLL
    DAC3100_WriteRegister(hi2c, 0x05, 0x91); // PLL Power up, P = 1, R = 1

    // (c) Program and power up NDAC
    DAC3100_WriteRegister(hi2c, 0x0B, 0x88); // NDAC is powered up and set to 8

    // (d) Program and power up MDAC
    DAC3100_WriteRegister(hi2c, 0x0C, 0x82); // MDAC is powered up and set to 2

    // (e) Program OSR value (Sequential write in script: w 30 0D 00 80)
    DAC3100_WriteRegister(hi2c, 0x0D, 0x00); // DOSR(9:8) = 0
    DAC3100_WriteRegister(hi2c, 0x0E, 0x80); // DOSR(7:0) = 128

    // (f) Program I2S word length if required
    DAC3100_WriteRegister(hi2c, 0x1B, 0x00); // mode is i2s, wordlength is 16, slave mode

    // (g) Program the processing block to be used
    DAC3100_WriteRegister(hi2c, 0x3C, 0x0B); // Select Processing Block PRB_P11
    
    DAC3100_SetPage(hi2c, 8);                // Switch to page 8 (w 30 00 08)
    DAC3100_WriteRegister(hi2c, 0x01, 0x04); // Program specific block settings
    DAC3100_SetPage(hi2c, 0);                // Back to page 0 (w 30 00 00)

    // (h) Miscellaneous page 0 controls
    DAC3100_WriteRegister(hi2c, 0x74, 0x00); // DAC => volume control thru pin disable

    // =========================================================================
    // 3. Program analog blocks
    // =========================================================================
    
    // (a) Set register page to 1
    DAC3100_SetPage(hi2c, 1); 

    // (b) Program common-mode voltage
    DAC3100_WriteRegister(hi2c, 0x1F, 0x04); // default = 1.35 V

    // (c) Program headphone-specific depop settings
    DAC3100_WriteRegister(hi2c, 0x21, 0x4E); // De-pop, Power on = 800 ms, Step time = 4 ms

    // (d) Program routing of DAC output to the output amplifier
    DAC3100_WriteRegister(hi2c, 0x23, 0x44); // LDAC routed to HPL out, RDAC routed to HPR out

    // (e) Unmute and set gain of output driver
    DAC3100_WriteRegister(hi2c, 0x28, 0x06); // Unmute HPL, set gain = 0 dB
    DAC3100_WriteRegister(hi2c, 0x29, 0x06); // Unmute HPR, set gain = 0 dB
    DAC3100_WriteRegister(hi2c, 0x2A, 0x1C); // Unmute Class-D, set gain = 18 dB

    // (f) Power up output drivers
    DAC3100_WriteRegister(hi2c, 0x1F, 0xC2); // HPL and HPR powered up (Overrides earlier common-mode write)
    DAC3100_WriteRegister(hi2c, 0x20, 0x86); // Power-up Class-D driver
    
    DAC3100_WriteRegister(hi2c, 0x24, 0x92); // Enable HPL output analog volume, set = -9 dB
    DAC3100_WriteRegister(hi2c, 0x25, 0x92); // Enable HPR output analog volume, set = -9 dB
    DAC3100_WriteRegister(hi2c, 0x26, 0x92); // Enable Class-D output analog volume, set = -9 dB

    // =========================================================================
    // 4. Apply waiting time
    // =========================================================================
    
    // Waiting time determined by the de-pop settings (800ms)
    HAL_Delay(800); 

    // =========================================================================
    // 5. Power up DAC
    // =========================================================================
    
    // (a) Set register page to 0
    DAC3100_SetPage(hi2c, 0);

    // (b) Power up DAC channels and set digital gain
    DAC3100_WriteRegister(hi2c, 0x3F, 0xD4); // Powerup DAC left/right channels (soft step enabled)
    
    DAC3100_WriteRegister(hi2c, 0x41, 0xD4); // DAC Left gain = -22 dB
    DAC3100_WriteRegister(hi2c, 0x42, 0xD4); // DAC Right gain = -22 dB

    // (c) Unmute digital volume control
    //DAC3100_WriteRegister(hi2c, 0x40, 0x00); // Unmute DAC left and right channels
}




void tlv3100_mute(I2C_HandleTypeDef *hi2c) {
    // 1. Ensure you are on Page 0 
    DAC3100_SetPage(hi2c, 0);

    // 2. Mute both left and right DAC channels
    // 0x0C binary is 0000 1100 (Bit 3 = Left Mute, Bit 2 = Right Mute)
    DAC3100_WriteRegister(hi2c, 0x40, 0x0C); 
}

void tlv3100_unmute(I2C_HandleTypeDef *hi2c) {
    // 1. Ensure you are on Page 0
    DAC3100_SetPage(hi2c, 0);

    // 2. Unmute both channels
    DAC3100_WriteRegister(hi2c, 0x40, 0x00); 
}