#include "audio.h"
#include "wavetable.h"
#include "sd_player.h"

/* ── Shared DMA buffer ── */
uint16_t audioBuf[AUDIO_FULL_SIZE];

/* ── HAL handles ── */
static DAC_HandleTypeDef *_hdac  = NULL;
static TIM_HandleTypeDef *_htim8 = NULL;

/* ── Wavetable ── */
static const uint16_t *currentWave = NULL;
#define TABLE_SIZE  256.0f

/* ── Voice bank ──
 * phaseInc == 0 means the voice is silent. freq is retained so we can
 * recompute phaseInc when the sample rate changes.
 *
 * volatile: written by main-thread (Audio_NoteOn/Off) and read inside the
 * DAC DMA ISR (Audio_FillHalf). On Cortex-M3 a 32-bit aligned float load /
 * store is a single LDR/STR — atomic, so no tearing across the boundary.
 * The volatile qualifier just stops the compiler from caching the values
 * across the ISR boundary.
 */
typedef struct {
    float phaseAcc;
    float phaseInc;
    float freq;
} Voice;

static volatile Voice voices[NUM_VOICES];

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

static inline float current_sr_hz(void)
{
    return (float)(72000000UL / (srPeriods[_currentSR] + 1));
}

/* ────────────────────────────────────────────────────────────── */

void Audio_Init(DAC_HandleTypeDef *hdac, TIM_HandleTypeDef *htim8)
{
    _hdac  = hdac;
    _htim8 = htim8;

    currentWave = sine_table;

    for (uint8_t v = 0; v < NUM_VOICES; v++) {
        voices[v].phaseAcc = 0.0f;
        voices[v].phaseInc = 0.0f;
        voices[v].freq     = 0.0f;
    }

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

/* ── Per-voice control ── */
void Audio_NoteOn(uint8_t voice, float frequency)
{
    if (voice >= NUM_VOICES) return;
    float inc = (frequency * TABLE_SIZE) / current_sr_hz();
    voices[voice].freq     = frequency;
    voices[voice].phaseAcc = 0.0f;     /* reset phase so retrigger is clean */
    voices[voice].phaseInc = inc;
}

void Audio_NoteOff(uint8_t voice)
{
    if (voice >= NUM_VOICES) return;
    voices[voice].phaseInc = 0.0f;
    voices[voice].freq     = 0.0f;
}

void Audio_StopAll(void)
{
    for (uint8_t v = 0; v < NUM_VOICES; v++) {
        voices[v].phaseInc = 0.0f;
        voices[v].freq     = 0.0f;
    }
}

/* ── Single-voice convenience (voice 0 = song / melody) ── */
void Audio_SetNote(float frequency) { Audio_NoteOn(0, frequency); }
void Audio_Stop(void)               { Audio_NoteOff(0);           }

/* ── Sample rate control ── */
void Audio_SetSampleRate(SampleRateOption sr)
{
    if (sr >= SAMPLE_RATE_COUNT) return;
    _currentSR = sr;
    __HAL_TIM_SET_AUTORELOAD(_htim8, srPeriods[sr]);

    /* Recompute every active voice's phase increment for the new SR.
     * The previous implementation derived the new freq from the OLD
     * phaseInc using the NEW sample rate, which inverted the math
     * and produced the wrong pitch on every SR change. */
    float sr_hz = current_sr_hz();
    for (uint8_t v = 0; v < NUM_VOICES; v++) {
        if (voices[v].freq > 0.0f) {
            voices[v].phaseInc = (voices[v].freq * TABLE_SIZE) / sr_hz;
        }
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

    /* Snapshot which voices are active for this fill. Voice activations
     * from the main loop during the fill are picked up on the next half. */
    uint8_t  active_idx[NUM_VOICES];
    uint8_t  active_count = 0;
    for (uint8_t v = 0; v < NUM_VOICES; v++) {
        if (voices[v].phaseInc > 0.0f) {
            active_idx[active_count++] = v;
        }
    }

    if (active_count == 0) {
        for (uint32_t i = 0; i < AUDIO_HALF_SIZE; i++)
            dst[i] = 2048;
        return;
    }

    for (uint32_t i = 0; i < AUDIO_HALF_SIZE; i++) {
        int32_t sum = 0;
        for (uint8_t a = 0; a < active_count; a++) {
            uint8_t v = active_idx[a];
            uint32_t idx = (uint32_t)voices[v].phaseAcc & 0xFFu;
            sum += (int32_t)currentWave[idx] - 2048;   /* AC-coupled */
            voices[v].phaseAcc += voices[v].phaseInc;
            if (voices[v].phaseAcc >= TABLE_SIZE)
                voices[v].phaseAcc -= TABLE_SIZE;
        }
        /* Average across active voices: keeps the mix headroom-bounded
         * (no clipping for any chord size) at the cost of single-note
         * loudness scaling down with chord size. */
        int32_t out = 2048 + (sum / active_count);
        if (out < 0)        out = 0;
        else if (out > 4095) out = 4095;
        dst[i] = (uint16_t)out;
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
