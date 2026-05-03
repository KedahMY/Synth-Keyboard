/**
 * hub75.c
 * HUB75 64x32 RGB LED matrix driver — RGB222 2-bit BCM version.
 *
 * Pixel format change (from RGB111 to RGB222):
 *   bits [5:4] = R  (0-3)
 *   bits [3:2] = G  (0-3)
 *   bits [1:0] = B  (0-3)
 *   Use the RGB222(r,g,b) macro from hub75_color.h to build pixel values.
 *
 * BCM (Binary Code Modulation) replaces the single ISR pass per row with
 * two sub-frame passes. The MSB plane is held for 2 time units and the
 * LSB plane for 1 time unit, giving 4 perceived intensity levels per
 * channel (ratio 2:1).
 *
 * Timer maths (60 fps target):
 *   Each row now costs 3 ISR ticks (2 MSB + 1 LSB).
 *   BASE = 72 MHz / (3 ticks * 16 rows * 60 fps) = 25000
 *   MSB_ARR = 2 * 25000 - 1 = 49999    (held for 2 units)
 *   LSB_ARR = 1 * 25000 - 1 = 24999    (held for 1 unit)
 *   Effective scan rate = 60 fps. No change to animation_tick() timing.
 *
 * IMPORTANT: TIM_AUTORELOAD_PRELOAD is DISABLED so that writes to
 * TIM2->ARR inside the ISR take effect on the very next count cycle.
 * With preload enabled the new period would be delayed by one extra
 * update event, breaking the 2:1 BCM ratio.
 *
 * All GPIO pin assignments and ODR bit-packing are unchanged from the
 * bring-up session: PA3-PA8 for RGB data, PB0/PB1/PB8/PB9 for row
 * address, PB6/PB7/PB10 for CLK/LAT/OE.
 *
 * ELEC 3300 - Group 2
 */

#include "hub75.h"
#include "hub75_color.h"

/* ------------------------------------------------------------------ */
/* Framebuffer storage                                                  */
/* ------------------------------------------------------------------ */

uint8_t fb[2][HUB75_HEIGHT][HUB75_WIDTH];
volatile uint8_t fb_front = 0;
volatile uint8_t fb_back  = 1;

/* Current scan row (0..15), advanced each time the LSB plane finishes */
static volatile uint8_t scan_row = 0;

/*
 * BCM sub-frame selector.
 *   1 = MSB plane (bits 5,3,1 of each pixel channel pair)
 *   0 = LSB plane (bits 4,2,0 of each pixel channel pair)
 */
static volatile uint8_t bcm_bit = 1;

/* ------------------------------------------------------------------ */
/* BCM timer periods                                                    */
/* ------------------------------------------------------------------ */

/*
 * BASE = 72 MHz / (3 ticks/row * 16 rows * 60 fps) = 25000
 * Subtract 1 because ARR is 0-indexed (timer counts 0..ARR inclusive).
 */
#define BCM_MSB_ARR  49999u   /* MSB plane: 2 units = 2 * 25000 - 1 */
#define BCM_LSB_ARR  24999u   /* LSB plane: 1 unit  = 1 * 25000 - 1 */

/* ------------------------------------------------------------------ */
/* Pin manipulation macros (unchanged from bring-up)                   */
/* ------------------------------------------------------------------ */

#define DATA_MASK  0x01F8u  /* PA3 and PA4..PA8: bits 3,4,5,6,7,8    */
#define ADDR_MASK  0x0303u  /* PB0,PB1 (bits 0-1) and PB8,PB9 (bits 8-9) */

#define CLK_PIN    GPIO_PIN_6    /* PB6 */
#define LAT_PIN    GPIO_PIN_7    /* PB7 */
#define OE_PIN     GPIO_PIN_10   /* PB10 */

#define HUB75_CLK_HIGH()   (GPIOB->BSRR = CLK_PIN)
#define HUB75_CLK_LOW()    (GPIOB->BRR  = CLK_PIN)
#define HUB75_LAT_HIGH()   (GPIOB->BSRR = LAT_PIN)
#define HUB75_LAT_LOW()    (GPIOB->BRR  = LAT_PIN)
#define HUB75_OE_HIGH()    (GPIOB->BSRR = OE_PIN)   /* display OFF */
#define HUB75_OE_LOW()     (GPIOB->BRR  = OE_PIN)   /* display ON  */

/* ------------------------------------------------------------------ */
/* JTAG remap (no-op with current pin config)                          */
/* ------------------------------------------------------------------ */

void hub75_remap_jtag(void) {
    /* No JTAG remap needed with current pin config. */
}

/* ------------------------------------------------------------------ */
/* GPIO init (unchanged from bring-up)                                 */
/* ------------------------------------------------------------------ */

void hub75_gpio_init(void) {
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA3 (B2) and PA4..PA8 (R1 G1 B1 R2 G2) */
    gpio.Pin   = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
                 GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PB0 PB1 (row A B), PB6 PB7 (CLK LAT), PB8 PB9 (row C D), PB10 (OE) */
    gpio.Pin   = GPIO_PIN_0  | GPIO_PIN_1  |
                 GPIO_PIN_6  | GPIO_PIN_7  |
                 GPIO_PIN_8  | GPIO_PIN_9  |
                 GPIO_PIN_10;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* Start with display disabled */
    HUB75_OE_HIGH();
    HUB75_LAT_LOW();
    HUB75_CLK_LOW();
}

/* ------------------------------------------------------------------ */
/* TIM2 init — BCM version                                             */
/* ------------------------------------------------------------------ */

static TIM_HandleTypeDef htim2;

void hub75_tim2_init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 0;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = BCM_MSB_ARR;  /* start with MSB period */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    /*
     * CRITICAL: Preload MUST be disabled.
     * With preload enabled, writes to ARR inside the ISR only take effect
     * at the NEXT update event, which delays the BCM period change by one
     * full cycle and breaks the 2:1 MSB:LSB brightness ratio.
     * With preload disabled, the write takes effect immediately on the
     * current count cycle (counter just reset to 0 after the UEV that
     * triggered this ISR, so there is no risk of the counter already
     * having passed the new ARR value).
     */
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_TIM_Base_Start_IT(&htim2);
}

/* ------------------------------------------------------------------ */
/* TIM1 PWM for OE brightness (optional, unchanged)                    */
/* ------------------------------------------------------------------ */

static TIM_HandleTypeDef htim1;

void hub75_brightness_init(void) {
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_8;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 0;
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 3599;
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim1);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_OC_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_MOE_ENABLE(&htim1);
}

void hub75_set_brightness(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint32_t ccr = (uint32_t)(100 - percent) * 3599 / 100;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
}

/* ------------------------------------------------------------------ */
/* Framebuffer operations                                               */
/* ------------------------------------------------------------------ */

void hub75_swap_buffers(void) {
    __disable_irq();
    uint8_t tmp = fb_front;
    fb_front = fb_back;
    fb_back  = tmp;
    __enable_irq();
}

void hub75_clear_back(void) {
    memset(fb[fb_back], 0, sizeof(fb[0]));
}

/* ------------------------------------------------------------------ */
/* Row-scan ISR — RGB222 BCM version                                   */
/* ------------------------------------------------------------------ */

void hub75_row_isr(void) {
    /*
     * Flow (same as before, with BCM additions):
     * 1. OE HIGH  — blank display during shift.
     * 2. Shift 64 pixel pairs, extracting only the current bit plane.
     * 3. Latch.
     * 4. Set row address.
     * 5. OE LOW   — display the latched row.
     * 6. Set TIM2->ARR for the display duration of this plane.
     * 7. Advance bcm_bit; advance scan_row only when LSB plane finishes.
     *
     * The NOP brightness loop from the old version is removed.
     * Display time is now controlled entirely by TIM2->ARR (BCM ratio).
     */

    HUB75_OE_HIGH();

    const uint8_t *top_row = fb[fb_front][scan_row];
    const uint8_t *bot_row = fb[fb_front][scan_row + 16];

    for (int col = 0; col < HUB75_WIDTH; col++) {
        uint8_t top_pix = top_row[col];
        uint8_t bot_pix = bot_row[col];

        /*
         * RGB222 pixel format: bits[5:4]=R, bits[3:2]=G, bits[1:0]=B.
         *
         * Extract one bit per channel for the current BCM plane:
         *   bcm_bit=1 (MSB): shift by 5,3,1 to get the upper bit of each pair
         *   bcm_bit=0 (LSB): shift by 4,2,0 to get the lower bit of each pair
         *
         * The shift amounts 4+bcm_bit, 2+bcm_bit, 0+bcm_bit cover both cases.
         */
        uint8_t r_top = (top_pix >> (4u + bcm_bit)) & 1u;
        uint8_t g_top = (top_pix >> (2u + bcm_bit)) & 1u;
        uint8_t b_top = (top_pix >> (     bcm_bit)) & 1u;
        uint8_t r_bot = (bot_pix >> (4u + bcm_bit)) & 1u;
        uint8_t g_bot = (bot_pix >> (2u + bcm_bit)) & 1u;
        uint8_t b_bot = (bot_pix >> (     bcm_bit)) & 1u;

        /*
         * GPIO assignment (unchanged from bring-up):
         *   Top half:    R1=PA4  G1=PA5  B1=PA6
         *   Bottom half: R2=PA7  G2=PA8  B2=PA3
         */
        uint32_t pa_out =
            (r_top << 4u) |   /* R1 -> PA4 */
            (g_top << 5u) |   /* G1 -> PA5 */
            (b_top << 6u) |   /* B1 -> PA6 */
            (r_bot << 7u) |   /* R2 -> PA7 */
            (g_bot << 8u) |   /* G2 -> PA8 */
            (b_bot << 3u);    /* B2 -> PA3 */

        GPIOA->ODR = (GPIOA->ODR & ~DATA_MASK) | pa_out;

        HUB75_CLK_HIGH();
        __NOP(); __NOP(); __NOP();
        HUB75_CLK_LOW();
    }

    HUB75_LAT_HIGH();
    __NOP(); __NOP();
    HUB75_LAT_LOW();

    /* Row address (unchanged from bring-up: A=PB0, B=PB1, C=PB8, D=PB9) */
    uint32_t addr =
        ( (scan_row & 1u)         << 0u) |
        (((scan_row >> 1u) & 1u)  << 1u) |
        (((scan_row >> 2u) & 1u)  << 8u) |
        (((scan_row >> 3u) & 1u)  << 9u);
    GPIOB->ODR = (GPIOB->ODR & ~ADDR_MASK) | addr;

    /* Re-enable display — this row+plane stays lit until the next ISR */
    HUB75_OE_LOW();

    /*
     * BCM state machine.
     *
     * Set TIM2->ARR for how long THIS plane is displayed before the next
     * ISR fires. Since ARPE=0, the write takes effect on the current count
     * cycle (counter just reset to 0 from the UEV that fired this ISR).
     *
     * MSB plane (bcm_bit=1):
     *   Set ARR = BCM_MSB_ARR so the MSB is held for 2 time units.
     *   Next ISR will process the LSB plane for this same row.
     *
     * LSB plane (bcm_bit=0):
     *   Set ARR = BCM_LSB_ARR so the LSB is held for 1 time unit.
     *   Both planes done for this row — advance scan_row.
     *   Next ISR will process the MSB plane of the next row.
     */
    if (bcm_bit == 1u) {
        TIM2->ARR = BCM_MSB_ARR;
        bcm_bit   = 0u;
        /* scan_row does NOT advance here */
    } else {
        TIM2->ARR = BCM_LSB_ARR;
        bcm_bit   = 1u;
        scan_row  = (scan_row + 1u) & 0x0Fu;
    }
}

/* ------------------------------------------------------------------ */
/* TIM2 IRQ handler (unchanged)                                        */
/* ------------------------------------------------------------------ */

void TIM2_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
        hub75_row_isr();
    }
}
