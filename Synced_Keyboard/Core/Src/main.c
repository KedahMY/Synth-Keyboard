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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "song.h"
#include "song_player.h"
#include "sd_player.h"
#include "note_event.h"
#include "uart_tx.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define OCTAVE_STEP  2


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
 DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac_ch1;

SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_sdio;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;

UART_HandleTypeDef huart1;

SRAM_HandleTypeDef hsram1;

/* USER CODE BEGIN PV */

static const float baseNoteFreq[3][8] = {
    { 261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f },
    { 415.30f, 440.00f, 466.16f, 493.88f, 523.25f, 554.37f, 587.33f, 622.25f },
    { 659.26f, 698.46f, 739.99f, 783.99f, 830.61f, 880.00f, 932.33f, 987.77f }
};

static const char *keyNoteNames[3][8] = {
    { "C",  "C#", "D",  "D#", "E",  "F",  "F#", "G"  },
    { "G#", "A",  "A#", "B",  "C",  "C#", "D",  "D#" },
    { "E",  "F",  "F#", "G",  "G#", "A",  "A#", "B"  }
};

static int8_t   octaveOffset   = 0;
static WaveType currentWaveType = WAVE_SQUARE;
static uint8_t  lastPressedRow = 0xFF;
static uint8_t  lastPressedCol = 0xFF;

/* Number of chord (note) keys currently held. The song player must stay
 * muted while ANY chord key is down and resume only when the last one
 * is released. */
static uint8_t  activeChordKeys = 0;

/* Press timestamp per playable note key (rows 0..2) for duration_ms. */
static uint32_t pressTickPerKey[3][NUM_COLS] = {{0}};

/* ── 4-voice rolling window (voice stealing) ──
 * Holds at most MAX_CHORD_VOICES active chord keys. When a 5th key is
 * pressed the oldest entry is evicted (Audio_NoteOff + UART note-off).
 * A release for an already-evicted key is silently ignored. */
#define MAX_CHORD_VOICES 4
typedef struct { uint8_t row; uint8_t col; uint8_t octave; } VoiceSlot;
static VoiceSlot voiceWindow[MAX_CHORD_VOICES];
static uint8_t   voiceHead = 0;   /* oldest entry (eviction point) */
static uint8_t   voiceTail = 0;   /* next insert slot              */
static uint8_t   voiceCount = 0;

static void VoiceWindow_Push(uint8_t r, uint8_t c, uint8_t oct)
{
    voiceWindow[voiceTail].row    = r;
    voiceWindow[voiceTail].col    = c;
    voiceWindow[voiceTail].octave = oct;
    voiceTail = (voiceTail + 1u) % MAX_CHORD_VOICES;
    voiceCount++;
}

static uint8_t VoiceWindow_Pop(uint8_t *r, uint8_t *c, uint8_t *oct)
{
    if (voiceCount == 0u) return 0;
    *r   = voiceWindow[voiceHead].row;
    *c   = voiceWindow[voiceHead].col;
    *oct = voiceWindow[voiceHead].octave;
    voiceHead = (voiceHead + 1u) % MAX_CHORD_VOICES;
    voiceCount--;
    return 1;
}

static uint8_t VoiceWindow_FindAndRemove(uint8_t r, uint8_t c)
{
    uint8_t idx = voiceHead;
    for (uint8_t i = 0; i < voiceCount; i++) {
        if (voiceWindow[idx].row == r && voiceWindow[idx].col == c) {
            /* Shift remaining entries left to close the gap */
            uint8_t src = (idx + 1u) % MAX_CHORD_VOICES;
            for (uint8_t j = i + 1u; j < voiceCount; j++) {
                voiceWindow[idx] = voiceWindow[src];
                idx  = src;
                src  = (src + 1u) % MAX_CHORD_VOICES;
            }
            voiceCount--;
            /* Re-adjust head/tail so the logical ring stays consistent */
            voiceTail = (voiceHead + voiceCount) % MAX_CHORD_VOICES;
            return 1;  /* found and removed */
        }
        idx = (idx + 1u) % MAX_CHORD_VOICES;
    }
    return 0;  /* not in window — was already evicted */
}

/* Song player state for LCD */
static const char *currentRiffName = "";

/* NoteEvent encoding lookup for (row, col) */
typedef struct { uint8_t name; uint8_t acc; } NoteCode;

static const NoteCode noteCode[3][8] = {
    /* Row 0: C C# D D# E F F# G */
    { {NOTE_C,ACC_NATURAL},{NOTE_C,ACC_SHARP},{NOTE_D,ACC_NATURAL},{NOTE_D,ACC_SHARP},
      {NOTE_E,ACC_NATURAL},{NOTE_F,ACC_NATURAL},{NOTE_F,ACC_SHARP},{NOTE_G,ACC_NATURAL} },
    /* Row 1: G# A A# B C C# D D# */
    { {NOTE_G,ACC_SHARP},{NOTE_A,ACC_NATURAL},{NOTE_A,ACC_SHARP},{NOTE_B,ACC_NATURAL},
      {NOTE_C,ACC_NATURAL},{NOTE_C,ACC_SHARP},{NOTE_D,ACC_NATURAL},{NOTE_D,ACC_SHARP} },
    /* Row 2: E F F# G G# A A# B */
    { {NOTE_E,ACC_NATURAL},{NOTE_F,ACC_NATURAL},{NOTE_F,ACC_SHARP},{NOTE_G,ACC_NATURAL},
      {NOTE_G,ACC_SHARP},{NOTE_A,ACC_NATURAL},{NOTE_A,ACC_SHARP},{NOTE_B,ACC_NATURAL} },
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FSMC_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM8_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE BEGIN 0 */

static float GetNoteFreq(uint8_t r, uint8_t c)
{
    float base = baseNoteFreq[r][c];
    if (octaveOffset > 0) {
        for (int8_t i = 0; i < octaveOffset; i++) base *= 2.0f;
    } else if (octaveOffset < 0) {
        for (int8_t i = 0; i > octaveOffset; i--) base /= 2.0f;
    }
    return base;
}

static void LCD_ShowOctave(void)
{
    char buf[8] = "OCT:   ";
    if (octaveOffset == 0) {
        buf[5] = ' '; buf[6] = '0';
    } else if (octaveOffset > 0) {
        buf[5] = '+';
        buf[6] = '0' + (char)octaveOffset;
    } else {
        buf[5] = '-';
        buf[6] = '0' + (char)(-octaveOffset);
    }
    LCD_DrawString(70, 120, "       ");
    LCD_DrawString(70, 120, buf);
}

static void LCD_ShowWave(void)
{
    LCD_DrawString(70, 150, "        ");
    switch (currentWaveType) {
        case WAVE_SINE:     LCD_DrawString(70, 150, "SINE");     break;
        case WAVE_SQUARE:   LCD_DrawString(70, 150, "SQUARE");   break;
        case WAVE_TRIANGLE: LCD_DrawString(70, 150, "TRIANGLE"); break;
        default:            LCD_DrawString(70, 150, "???");      break;
    }
}

/* Retune every chord voice that is currently in the voice window.
 * We iterate voiceWindow instead of keyState[][] because an evicted key
 * may still read HIGH on GPIO (physically held) but must not be retuned —
 * its audio was already stolen. */
static void Chord_RetuneAll(void)
{
    uint8_t idx = voiceHead;
    for (uint8_t i = 0; i < voiceCount; i++) {
        uint8_t r = voiceWindow[idx].row;
        uint8_t c = voiceWindow[idx].col;
        uint8_t voice = (uint8_t)(1u + r * NUM_COLS + c);
        Audio_NoteOn(voice, GetNoteFreq(r, c));
        idx = (idx + 1u) % MAX_CHORD_VOICES;
    }
}

static void CycleWave(void)
{
    switch (currentWaveType) {
        case WAVE_SINE:     currentWaveType = WAVE_SQUARE;   break;
        case WAVE_SQUARE:   currentWaveType = WAVE_TRIANGLE; break;
        case WAVE_TRIANGLE: currentWaveType = WAVE_SINE;     break;
        default:            currentWaveType = WAVE_SINE;     break;
    }
    Audio_SetWave(currentWaveType);
    LCD_ShowWave();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) {
        Keypad_ScanOneColumn();
    }
}

static void LCD_ShowSong(const char *name)
{
    LCD_DrawString(70, 180, "          ");
    LCD_DrawString(70, 180, (char *)name);
}

static void LCD_ShowSampleRate(void)
{
    LCD_DrawString(70, 240, "      ");
    LCD_DrawString(70, 240, Audio_GetSampleRateName());
}
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
  MX_FSMC_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_DAC_Init();
  MX_TIM2_Init();
  MX_TIM8_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  LCD_INIT();
  HAL_TIM_Base_Start_IT(&htim3);

  LCD_DrawString(0,   0, "KeyBoard Audio Test");
  LCD_DrawString(0,  30, "Note: ");
  LCD_DrawString(0,  60, "Row:  ");
  LCD_DrawString(0,  90, "Col:  ");
  LCD_DrawString(0, 120, "Oct:  ");
  LCD_DrawString(0, 150, "Wave: ");

  Audio_Init(&hdac, &htim8);
  Audio_SetWave(WAVE_SQUARE);
  LCD_ShowOctave();
  LCD_ShowWave();
  LCD_DrawString(0, 180, "Song: ");
  LCD_ShowSong("---");

  SdPlayer_Init();
  LCD_DrawString(0, 240, "SR:   ");
  LCD_ShowSampleRate();

  UART_TX_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  /* ── Song player tick ── */
	  SongPlayer_Tick();
	  SdPlayer_Tick();

	  /* ── Drain ALL queued key events ──
	   * Each scan-tick can enqueue up to 4 events (one per row); a chord
	   * crossing the debounce threshold within a single column scan
	   * produces multiple events back-to-back. The previous single-slot
	   * design lost every event after the first, which dropped chord
	   * notes both on the speaker AND on Board 2's display.
	   */
	  KeyEvent ev;
	  while (Keypad_GetEvent(&ev)) {
	      uint8_t r = ev.row;
	      uint8_t c = ev.col;

	      /* Always update row/col display */
	      char rowStr[2] = {'0' + r, '\0'};
	      LCD_DrawString(70, 60, "  ");
	      LCD_DrawString(70, 60, rowStr);

	      char colStr[2] = {'0' + c, '\0'};
	      LCD_DrawString(70, 90, "  ");
	      LCD_DrawString(70, 90, colStr);

	      if (ev.isPress) {
	          if (r < 3) {
	              /* ── Note key PRESS (with 4-voice cap) ── */
	              if (voiceCount == MAX_CHORD_VOICES) {
	                  uint8_t evR, evC, evOct;
	                  if (VoiceWindow_Pop(&evR, &evC, &evOct)) {
	                      uint8_t evVoice = (uint8_t)(1u + evR * NUM_COLS + evC);
	                      Audio_NoteOff(evVoice);

	                      uint32_t evHeld = HAL_GetTick() - pressTickPerKey[evR][evC];
	                      NoteEvent evEvt = {
	                          .start_byte  = NOTE_EVENT_START_BYTE,
	                          .note_name   = noteCode[evR][evC].name,
	                          .accidental  = noteCode[evR][evC].acc,
	                          .octave      = evOct,
	                          .velocity    = VEL_KEY_UP,
	                          .duration_ms = (evHeld > 0xFFFFu) ? 0xFFFFu : (uint16_t)evHeld,
	                          .track_id    = TRACK_LIVE,
	                          .active      = 0,
	                      };
	                      UART_TX_SendNoteEvent(&evEvt);
	                      if (activeChordKeys > 0) activeChordKeys--;
	                  }
	              }

	              if (activeChordKeys == 0) {
	                  SongPlayer_MuteForKey();
	              }
	              activeChordKeys++;

	              int baseOct   = (r == 0 || (r == 1 && c < 4)) ? 4 : 5;
	              int actualOct = baseOct + octaveOffset;
	              VoiceWindow_Push(r, c, (uint8_t)actualOct);

	              float freq = GetNoteFreq(r, c);


	              char noteBuf[5] = {0};
	              const char *name = keyNoteNames[r][c];
	              if (name[1] == '#') {
	                  noteBuf[0] = name[0];
	                  noteBuf[1] = '#';
	                  noteBuf[2] = '0' + (char)actualOct;
	                  noteBuf[3] = '\0';
	              } else {
	                  noteBuf[0] = name[0];
	                  noteBuf[1] = '0' + (char)actualOct;
	                  noteBuf[2] = '\0';
	              }

	              LCD_DrawString(70, 30, "     ");
	              LCD_DrawString(70, 30, noteBuf);

	              uint8_t voice = (uint8_t)(1u + r * NUM_COLS + c);
	              Audio_NoteOn(voice, freq);

	              lastPressedRow = r;
	              lastPressedCol = c;
	              pressTickPerKey[r][c] = HAL_GetTick();

	              NoteEvent evt = {
	                  .start_byte  = NOTE_EVENT_START_BYTE,
	                  .note_name   = noteCode[r][c].name,
	                  .accidental  = noteCode[r][c].acc,
	                  .octave      = (uint8_t)actualOct,
	                  .velocity    = VEL_KEY_DOWN,
	                  .duration_ms = 0,
	                  .track_id    = TRACK_LIVE,
	                  .active      = 1,
	              };
	              UART_TX_SendNoteEvent(&evt);

	          } else {
	              /* ── Row 3 special keys ── */
	              switch (c) {
	              case 1:
	                  if (octaveOffset >= -2 + OCTAVE_STEP) {
	                      octaveOffset -= OCTAVE_STEP;
	                      LCD_ShowOctave();
	                      Chord_RetuneAll();
	                  }
	                  break;
	              case 2:
	                  if (octaveOffset <= 3 - OCTAVE_STEP) {
	                      octaveOffset += OCTAVE_STEP;
	                      LCD_ShowOctave();
	                      Chord_RetuneAll();
	                  }
	                  break;
	              case 3:
	                  CycleWave();
	                  break;
	              case 4:
	                  SdPlayer_Toggle();
	                  LCD_ShowSong(SdPlayer_GetFilename());
	                  break;
	              case 5:
	                  SdPlayer_Stop();
	                  if (SdPlayer_LoadNext()) {
	                      SdPlayer_Play();
	                  }
	                  LCD_ShowSong(SdPlayer_GetFilename());
	                  break;
	              case 6:
	                  Audio_SetSampleRate((Audio_GetSampleRate() + 1) % SAMPLE_RATE_COUNT);
	                  LCD_ShowSampleRate();
	                  break;
	              default:
	                  LCD_DrawString(70, 30, "     ");
	                  LCD_DrawString(70, 30, "---");
	                  break;
	              }
	          }
	      } else {
	          /* ── Key RELEASE ── */
	          if (r < 3) {
	              /* Remove from voice window first. If not found, the key
	               * was already evicted (stolen) — skip audio/UART to avoid
	               * corrupting voiceCount or activeChordKeys. */
	              if (!VoiceWindow_FindAndRemove(r, c)) {
	                  /* Key was evicted before its release arrived; already
	                   * sent NoteOff. Just keep state consistent. */
	                  if (activeChordKeys > 0) activeChordKeys--;
	                  if (activeChordKeys == 0) {
	                      LCD_DrawString(70, 30, "     ");
	                      lastPressedRow = 0xFF;
	                      lastPressedCol = 0xFF;
	                      SongPlayer_UnmuteAfterKey();
	                      LCD_ShowSong(currentRiffName);
	                  }
	                  continue;
	              }

	              uint8_t voice = (uint8_t)(1u + r * NUM_COLS + c);
	              Audio_NoteOff(voice);

	              uint32_t held_ms = HAL_GetTick() - pressTickPerKey[r][c];

	              int baseOct   = (r == 0 || (r == 1 && c < 4)) ? 4 : 5;
	              int actualOct = baseOct + octaveOffset;

	              NoteEvent evt = {
	                  .start_byte  = NOTE_EVENT_START_BYTE,
	                  .note_name   = noteCode[r][c].name,
	                  .accidental  = noteCode[r][c].acc,
	                  .octave      = (uint8_t)actualOct,
	                  .velocity    = VEL_KEY_UP,
	                  .duration_ms = (held_ms > 0xFFFFu) ? 0xFFFFu : (uint16_t)held_ms,
	                  .track_id    = TRACK_LIVE,
	                  .active      = 0,
	              };
	              UART_TX_SendNoteEvent(&evt);

	              if (activeChordKeys > 0) activeChordKeys--;
	              if (activeChordKeys == 0) {
	                  LCD_DrawString(70, 30, "     ");
	                  lastPressedRow = 0xFF;
	                  lastPressedCol = 0xFF;
	                  SongPlayer_UnmuteAfterKey();
	                  LCD_ShowSong(currentRiffName);
	              }
	          }
	          /* Row 3 release: nothing to do (special keys are press-only). */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_T8_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
  hsd.Init.ClockDiv = 4;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7200-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 32500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 1630;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
  /* DMA2_Channel4_5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel4_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Col2_Pin|Col3_Pin|Col4_Pin|Col5_Pin
                          |Col6_Pin|Col7_Pin|Col0_Pin|Col1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pins : Row0_Pin Row1_Pin */
  GPIO_InitStruct.Pin = Row0_Pin|Row1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : Row2_Pin Row3_Pin */
  GPIO_InitStruct.Pin = Row2_Pin|Row3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Col2_Pin Col3_Pin Col4_Pin Col5_Pin
                           Col6_Pin Col7_Pin Col0_Pin Col1_Pin */
  GPIO_InitStruct.Pin = Col2_Pin|Col3_Pin|Col4_Pin|Col5_Pin
                          |Col6_Pin|Col7_Pin|Col0_Pin|Col1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PD12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

}

/* FSMC initialization function */
static void MX_FSMC_Init(void)
{

  /* USER CODE BEGIN FSMC_Init 0 */

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  /* USER CODE BEGIN FSMC_Init 1 */

  /* USER CODE END FSMC_Init 1 */

  /** Perform the SRAM1 memory initialization sequence
  */
  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram1.Init */
  hsram1.Init.NSBank = FSMC_NORSRAM_BANK1;
  hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
  hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  /* Timing */
  Timing.AddressSetupTime = 15;
  Timing.AddressHoldTime = 15;
  Timing.DataSetupTime = 255;
  Timing.BusTurnAroundDuration = 15;
  Timing.CLKDivision = 16;
  Timing.DataLatency = 17;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;
  /* ExtTiming */

  if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
  {
    Error_Handler( );
  }

  /** Disconnect NADV
  */

  __HAL_AFIO_FSMCNADV_DISCONNECTED();

  /* USER CODE BEGIN FSMC_Init 2 */

  /* USER CODE END FSMC_Init 2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
