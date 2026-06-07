#ifndef SDCARD_FUNCTIONS_H
#define SDCARD_FUNCTIONS_H

#include "main.h"
#include "fatfs.h"
#include "sai.h"

/* Defines from main.c */
#define AUDIO_BUFFER_SAMPLES   2048   // per buffer (stereo: 1024 frames)
#define WAV_HEADER_SIZE        44     // skip standard PCM WAV header
#define MAX_FILENAME_LEN 64
/* Exported Variables (so main.c can read/write them in the while(1) loop) */
extern int16_t AudioBuffer[2][AUDIO_BUFFER_SAMPLES];
extern volatile uint8_t BufferReady[2];
extern FIL WavFile;
extern volatile uint8_t PlaybackActive;
extern uint8_t FileIsOpen;

/* Function Prototypes */
HAL_StatusTypeDef Audio_OpenFile(const char *path);
void Audio_FillBuffer(uint8_t bufIdx);
void Audio_Stop(void);
void Audio_Pause(void);
void Audio_Resume(void);
HAL_StatusTypeDef Audio_Play(const char *path);
void SD_ListWavFiles(const char *path);
void SD_ListAllFiles(const char *path);
int SD_GetWavFiles(const char *path, char file_list[][MAX_FILENAME_LEN], int max_files);

#endif /* SDCARD_FUNCTIONS_H */