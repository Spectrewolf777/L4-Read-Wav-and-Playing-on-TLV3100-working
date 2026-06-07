/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "sai.h"
#include "sdmmc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
#include "tlv3100.h"
#include "sdcard_functions.h"  

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// Double buffer — each half is one DMA "half" transfer
// 1024 samples * 2 channels = 2048 int16_t per buffer
#define AUDIO_BUFFER_SAMPLES   2048   // per buffer (stereo: 1024 frames)
#define WAV_HEADER_SIZE        44     // skip standard PCM WAV header
#define MAX_WAV_FILES 20
int current_song_index = 0;




/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */


// Forward declarations


int _write(int file, char *ptr, int len) {
    // 1: stdout, 2: stderr
    if (file == 1 || file == 2) {
        // Send data through USART2
        // We use a blocking transmit here
        HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
        
        if (status == HAL_OK) {
            return len; // Return number of bytes written
        }
    }
    return -1; // Return -1 if write failed or file descriptor is invalid
}
#define SINE_SAMPLES 48  // 48 samples per cycle


const int16_t SineWave_Buffer[SINE_SAMPLES * 2] = {
    0, 0, 3977, 3977, 7887, 7887, 11663, 11663, 15239, 15239, 18643, 18643,
    21811, 21811, 24680, 24680, 27192, 27192, 29298, 29298, 30952, 30952, 32120, 32120,
    32767, 32767, 32120, 32120, 30952, 30952, 29298, 29298, 27192, 27192, 24680, 24680,
    21811, 21811, 18643, 18643, 15239, 15239, 11663, 11663, 7887, 7887, 3977, 3977,
    0, 0, -3977, -3977, -7887, -7887, -11663, -11663, -15239, -15239, -18643, -18643,
    -21811, -21811, -24680, -24680, -27192, -27192, -29298, -29298, -30952, -30952, -32120, -32120,
    -32767, -32767, -32120, -32120, -30952, -30952, -29298, -29298, -27192, -27192, -24680, -24680,
    -21811, -21811, -18643, -18643, -15239, -15239, -11663, -11663, -7887, -7887, -3977, -3977
};


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SAI1_Init();
  MX_SDMMC1_SD_Init();
  MX_USART2_UART_Init();
  MX_FATFS_Init();
  MX_I2C3_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_RESET);
  HAL_Delay(10); // Hold reset for 10ms
  
  // Pull PC5 HIGH to bring the DAC out of reset
  HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_SET);
  HAL_Delay(10); // Give the DAC 10ms to boot up before sending I2C commands
  
  DAC3100_Init(&hi2c3);
  //HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t*)SineWave_Buffer, SINE_SAMPLES * 2);
  HAL_Delay(10);
 // Mount SD card
  if (f_mount(&SDFatFS, SDPath, 1) != FR_OK) {
      printf("SD mount failed\r\n");
      Error_Handler();
  }
  char my_wav_files[MAX_WAV_FILES][MAX_FILENAME_LEN];
  int total_files = SD_GetWavFiles("/", my_wav_files, MAX_WAV_FILES);
  printf("\r\n--- Playing Files ---\r\n");
  Audio_Play(my_wav_files[0]);

  


  

   /*  Open WAV and skip header
  if (Audio_Play("AUDIO441.WAV") != HAL_OK) {
      Error_Handler();
  }    */

 tlv3100_unmute(&hi2c3);



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
     // Refill whichever half the ISR flagged
    if (BufferReady[0]) {
        BufferReady[0] = 0;
        if (PlaybackActive) Audio_FillBuffer(0);
    }
    if (BufferReady[1]) {
        BufferReady[1] = 0;
        if (PlaybackActive) Audio_FillBuffer(1);
    }

    // Optional: stop SAI cleanly when file ends
    if (!PlaybackActive) {
        
        // 1. Stop the current DMA transfer and close the file
        HAL_SAI_DMAStop(&hsai_BlockA1);
        if (FileIsOpen) {
            f_close(&WavFile);
            FileIsOpen = 0;
        }

        // 2. Advance to the next song
        current_song_index++;
        
        // Wrap around to the start of the playlist if we hit the end
        if (current_song_index >= total_files) {
            current_song_index = 0; 
        }

        printf("\r\n--- Track Ended. Auto-Playing Next ---\r\n");
        
        // 3. Play the next file in the array
        if (total_files > 0) {
            Audio_Play(my_wav_files[current_song_index]);
        }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV17;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance == hsai_BlockA1.Instance)
        BufferReady[0] = 1;
}

// Called by HAL when the second half of the DMA buffer is done — refill buffer 1
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance == hsai_BlockA1.Instance)
        BufferReady[1] = 1;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
