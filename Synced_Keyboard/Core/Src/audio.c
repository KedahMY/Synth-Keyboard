#include "audio.h"
#include "wavetable.h"
#include "sd_player.h"

/* ── Shared DMA buffer ── */
uint16_t audioBuf[AUDIO_FULL_SIZE];

/* ── HAL handles ── */
static DAC_HandleTypeDef *_hdac  = NULL;
static TIM_HandleTypeDef *_htim8 = NULL;

/* ── Playback state ── */
static const uint16_t  *currentWave = NULL;
static volatile float   phaseAcc    = 0.0f;
static volatile float   phaseInc    = 0.0f;

#define TABLE_SIZE  256.0f

static const uint32_t srPeriods[SAMPLE_RATE_COUNT] = {
    (72000000UL / 22050) - 1,   /* 22050 Hz → 3265 */
    (72000000UL / 32000) - 1,   /* 32000 Hz → 2249 */
    (72000000UL / 44100) - 1,   /* 44100 Hz → 1632 */
    (72000000UL / 48000) - 1,   /* 48000 Hz → 1499 */
    (72000000UL / 64000) - 1,   /* 64000 Hz → 1124 */
    (72000000UL / 88200) - 1,   /* 88200 Hz →  815 */
    (72000000UL / 96000) - 1,   /* 96000 Hz →  749 */
};

static const char *srNames[SAMPLE_RATE_COUNT] = {
    "22050", "32000", "44100", "48000", "64000", "88200", "96000"
};

static SampleRateOption _currentSR = SAMPLE_RATE_88200;

/* ────────────────────────────────────────────────────────────── */

void Audio_Init(DAC_HandleTypeDef *hdac, TIM_HandleTypeDef *htim8)
{
    _hdac  = hdac;
    _htim8 = htim8;

    currentWave = sine_table;

    for (uint32_t i = 0; i < AUDIO_FULL_SIZE; i++)
        audioBuf[i] = 2048;

    HAL_TIM_Base_Start(_htim8);

    HAL_DAC_Start_DMA(
        _hdac,
        DAC_CHANNEL_1,
        (uint32_t *)audioBuf,
        AUDIO_FULL_SIZE,
        DAC_ALIGN_12B_R
    );
}

/* ── Wave selection ── */
void Audio_SetWave(WaveType type)
{
    switch (type) {
        case WAVE_SQUARE:   currentWave = square_table;   break;
        case WAVE_TRIANGLE: currentWave = triangle_table; break;
        default:            currentWave = sine_table;     break;
    }
}

/* ── Note control ── */
void Audio_SetNote(float frequency)
{
    float sr = (float)(72000000UL / (srPeriods[_currentSR] + 1));
    phaseInc = (frequency * TABLE_SIZE) / sr;
}

void Audio_Stop(void)
{
    phaseInc = 0.0f;
}

/* ── Sample rate control ── */
void Audio_SetSampleRate(SampleRateOption sr)
{
    if (sr >= SAMPLE_RATE_COUNT) return;
    _currentSR = sr;
    __HAL_TIM_SET_AUTORELOAD(_htim8, srPeriods[sr]);

    /* Recalculate phase increment if a note is currently playing */
    if (phaseInc > 0.0f) {
        float freq = (phaseInc * (float)(72000000UL / (srPeriods[sr] + 1))) / TABLE_SIZE;
        Audio_SetNote(freq);
    }
}

SampleRateOption Audio_GetSampleRate(void)     { return _currentSR;          }
const char      *Audio_GetSampleRateName(void) { return srNames[_currentSR]; }

/* ── Buffer fill (called from DAC DMA ISR) ── */
void Audio_FillHalf(uint8_t half)
{
    /* SD player owns the buffer while a song is playing */
    if (SdPlayer_GetState() == SD_PLAYER_PLAYING) {
        SdPlayer_RequestFill(half);
        return;
    }

    uint16_t *dst = &audioBuf[half * AUDIO_HALF_SIZE];

    if (phaseInc == 0.0f) {
        for (uint32_t i = 0; i < AUDIO_HALF_SIZE; i++)
            dst[i] = 2048;
        phaseAcc = 0.0f;
        return;
    }

    for (uint32_t i = 0; i < AUDIO_HALF_SIZE; i++) {
        uint32_t idx = (uint32_t)phaseAcc & 0xFF;
        dst[i]   = currentWave[idx];
        phaseAcc += phaseInc;
        if (phaseAcc >= TABLE_SIZE) phaseAcc -= TABLE_SIZE;
    }
}

/* ── DAC DMA callbacks ── */
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    Audio_FillHalf(0);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    Audio_FillHalf(1);
}
