#ifndef AUDIO_H
#define AUDIO_H

#include "main.h"

/* ── Buffer geometry ── */
#define AUDIO_HALF_SIZE  512
#define AUDIO_FULL_SIZE  (AUDIO_HALF_SIZE * 2)

/* ── Wave types ── */
typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
} WaveType;

/* ── Sample rates ── */
typedef enum {
    SAMPLE_RATE_22050 = 0,
    SAMPLE_RATE_32000,
    SAMPLE_RATE_44100,
    SAMPLE_RATE_48000,
    SAMPLE_RATE_64000,
    SAMPLE_RATE_88200,
    SAMPLE_RATE_96000,
    SAMPLE_RATE_COUNT
} SampleRateOption;

/* ── Shared DMA buffer ── */
extern uint16_t audioBuf[AUDIO_FULL_SIZE];

/* ── Init & control ── */
void             Audio_Init(DAC_HandleTypeDef *hdac, TIM_HandleTypeDef *htim8);
void             Audio_SetNote(float frequency);
void             Audio_Stop(void);
void             Audio_SetWave(WaveType type);
void             Audio_FillHalf(uint8_t half);

/* ── Sample rate ── */
void             Audio_SetSampleRate(SampleRateOption sr);
SampleRateOption Audio_GetSampleRate(void);
const char      *Audio_GetSampleRateName(void);

#endif /* AUDIO_H */
