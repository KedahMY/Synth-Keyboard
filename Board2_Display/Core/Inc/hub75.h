/**
 * hub75.h
 * HUB75 64x32 RGB LED matrix driver for Board 2.
 * GPIO bit-bang refresh inside TIM2 ISR at 1920 Hz (32 rows x 60 fps).
 *
 * Can be driven directly from 3.3V GPIO (no 74HC245 level shifter needed).
 * The panel's internal shift registers accept 3.3V as logic HIGH.
 * Extra NOPs in the CLK pulse ensure clean edges without a buffer IC.
 *
 *   R1=PA4  G1=PA5  B1=PA6  R2=PA7  G2=PA8  B2=PA3
 *   A=PB0   B=PB1   C=PB8   D=PB9   CLK=PB6  LAT=PB7  OE=PB10
 *   No JTAG remap needed. None of these pins are JTAG pins.
 *
 * ELEC 3300 - Group 2
 */

#ifndef HUB75_H
#define HUB75_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>
#include "hub75_color.h"

/* Panel geometry */
#define HUB75_WIDTH   64
#define HUB75_HEIGHT  32
#define HUB75_ROWS    16    /* 1/16 scan: upper half + lower half */

/* Double-buffered framebuffer.
 * Each pixel is 1 byte (only lower 3 bits used for RGB).
 * Total: 2 x 32 x 64 = 4096 bytes.
 */
extern uint8_t fb[2][HUB75_HEIGHT][HUB75_WIDTH];
extern volatile uint8_t fb_front;
extern volatile uint8_t fb_back;

/**
 * MUST be called before any GPIO init on Board 2.
 * Remaps JTAG to SWD-only, freeing PB3 (row D) and PB4 (CLK).
 * Bug A3 from the verification audit.
 */
void hub75_remap_jtag(void);

/**
 * Configures all 13 HUB75 GPIO pins as push-pull outputs.
 * Call AFTER hub75_remap_jtag().
 */
void hub75_gpio_init(void);

/**
 * Starts TIM2 at 1920 Hz for the row-scan ISR.
 * PSC=0, ARR=37499 => 72 MHz / 37500 = 1920 Hz.
 * Bug A8 corrected value.
 */
void hub75_tim2_init(void);

/**
 * (Optional) Starts TIM1 CH1 PWM on PA8 for OE brightness control.
 * 20 kHz PWM. Duty cycle 0..100% maps to full bright..off.
 */
void hub75_brightness_init(void);
void hub75_set_brightness(uint8_t percent);  /* 0=off, 100=full */

/**
 * Swap front and back framebuffers atomically.
 * Called from the main loop after a full frame has been drawn into fb_back.
 */
void hub75_swap_buffers(void);

/**
 * Clear the back buffer to all black.
 */
void hub75_clear_back(void);

/**
 * Set a single pixel in the back buffer.
 * x: 0..63, y: 0..31, color: 3-bit RGB (COLOR_xxx defines).
 */
static inline void hub75_set_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x < HUB75_WIDTH && y < HUB75_HEIGHT)
        fb[fb_back][y][x] = color & 0x07;
}

/**
 * TIM2 ISR handler. Call this from TIM2_IRQHandler() in stm32f1xx_it.c.
 * Shifts out one row pair (top + bottom half) per invocation.
 */
void hub75_row_isr(void);

#endif /* HUB75_H */
