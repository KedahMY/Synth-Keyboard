#ifndef AUDIO_H
#define AUDIO_H

#include "main.h"

/* ── Buffer geometry ── */
#define AUDIO_HALF_SIZE  512
#define AUDIO_FULL_SIZE  (AUDIO_HALF_SIZE * 2)

/* ── Polyphony ── */
/*
 * Voice 0 is reserved for the song player (single-voice melody / SD).
 * Voices 1..NUM_VOICES-1 are addressable per chord key, indexed by
 *   voice = 1 + r * NUM_COLS + c
 * for r in [0..2], c in [0..7]. r=3 is reserved for control keys
 * (octave, wave, sample-rate, song) and never claims a voice slot.
 */
#define NUM_VOICES       25u

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
void             Audio_SetWave(WaveType type);
void             Audio_FillHalf(uint8_t half);

/* Single-voice convenience API — operates on voice 0 (song / SD path). */
void             Audio_SetNote(float frequency);
void             Audio_Stop(void);

/* Polyphonic per-voice API. `voice` must be < NUM_VOICES. */
void             Audio_NoteOn(uint8_t voice, float frequency);
void             Audio_NoteOff(uint8_t voice);
void             Audio_StopAll(void);

/* ── Sample rate ── */
void             Audio_SetSampleRate(SampleRateOption sr);
SampleRateOption Audio_GetSampleRate(void);
const char      *Audio_GetSampleRateName(void);

#endif /* AUDIO_H */
