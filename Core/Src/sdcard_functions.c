#include "sdcard_functions.h"
#include <stdio.h>
#include <string.h>

/* Define the variables here (memory allocated here) */
int16_t AudioBuffer[2][AUDIO_BUFFER_SAMPLES];
volatile uint8_t BufferReady[2] = {0, 0};
FIL WavFile;
volatile uint8_t PlaybackActive = 0;
uint8_t FileIsOpen = 0;

extern SAI_HandleTypeDef hsai_BlockA1; // Allows access to the SAI handle defined in sai.c

HAL_StatusTypeDef Audio_OpenFile(const char *path)
{
    FRESULT fr;
    UINT bytesRead;
    uint8_t header[WAV_HEADER_SIZE];

    fr = f_open(&WavFile, path, FA_READ);
    if (fr != FR_OK) {
        printf("f_open failed: %d\r\n", fr);
        return HAL_ERROR;
    }

    // Skip the 44-byte WAV header — you said format is guaranteed correct
    fr = f_read(&WavFile, header, WAV_HEADER_SIZE, &bytesRead);
    if (fr != FR_OK || bytesRead != WAV_HEADER_SIZE) {
        printf("Header read failed\r\n");
        f_close(&WavFile);
        return HAL_ERROR;
    }

    return HAL_OK;
}

// Read one buffer-worth of PCM from the file into AudioBuffer[bufIdx]
// Zeros the remainder if we hit EOF (clean end of track)
void Audio_FillBuffer(uint8_t bufIdx)
{
    UINT bytesRead = 0;
    uint32_t bytesToRead = AUDIO_BUFFER_SAMPLES * sizeof(int16_t);

    FRESULT fr = f_read(&WavFile, AudioBuffer[bufIdx], bytesToRead, &bytesRead);

    if (fr != FR_OK || bytesRead == 0) {
        // EOF or error — silence the buffer and stop
        memset(AudioBuffer[bufIdx], 0, bytesToRead);
        PlaybackActive = 0;
        return;
    }

    // Partial read at EOF — zero-pad the rest
    if (bytesRead < bytesToRead) {
        memset((uint8_t *)AudioBuffer[bufIdx] + bytesRead, 0, bytesToRead - bytesRead);
        PlaybackActive = 0;  // will stop after this buffer drains
    }
}

void Audio_Stop(void)
{
    PlaybackActive = 0;
    HAL_SAI_DMAStop(&hsai_BlockA1);
    f_close(&WavFile);
    BufferReady[0] = 0;
    BufferReady[1] = 0;
    printf("Audio stopped\r\n");
}

void Audio_Pause(void)
{
    if (!PlaybackActive) return;
    PlaybackActive = 0;
    HAL_SAI_DMAStop(&hsai_BlockA1);
    // file stays open, f_tell() holds our position
    printf("Audio paused at byte %lu\r\n", f_tell(&WavFile));
}

void Audio_Resume(void)
{
    if (PlaybackActive) return;

    // Refill both buffers from current file position and restart DMA
    Audio_FillBuffer(0);
    Audio_FillBuffer(1);
    BufferReady[0] = 0;
    BufferReady[1] = 0;
    PlaybackActive = 1;
    HAL_SAI_Transmit_DMA(&hsai_BlockA1,
                          (uint8_t *)AudioBuffer,
                          AUDIO_BUFFER_SAMPLES * 2);
    printf("Audio resumed\r\n");
}

HAL_StatusTypeDef Audio_Play(const char *path)
{
    if (PlaybackActive) Audio_Stop();
    if (FileIsOpen) {          // <-- use flag instead of f_is_open
        f_close(&WavFile);
        FileIsOpen = 0;
    }

    if (Audio_OpenFile(path) != HAL_OK) return HAL_ERROR;

    Audio_FillBuffer(0);
    Audio_FillBuffer(1);
    BufferReady[0] = 0;
    BufferReady[1] = 0;
    PlaybackActive = 1;

    HAL_SAI_Transmit_DMA(&hsai_BlockA1,
                          (uint8_t *)AudioBuffer,
                          AUDIO_BUFFER_SAMPLES * 2);
    printf("Playing: %s\r\n", path);
    return HAL_OK;
}