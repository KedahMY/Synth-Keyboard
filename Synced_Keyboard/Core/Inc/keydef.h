#pragma once
#ifndef KEYDEF_H
#define KEYDEF_H

#include "stm32f1xx_hal.h"

#define NUM_ROWS              4
#define NUM_COLS              8
#define DEBOUNCE_TICKS        3

/* Power-of-two so wrap is a mask. Must comfortably hold simultaneous
 * press/release events for an N-key chord plus a few in-flight ticks.
 * 32 is more than enough for the 4x8 keypad. */
#define KEY_EVENT_QUEUE_SIZE  32

typedef struct {
    GPIO_TypeDef *Port;
    uint16_t      Pin;
} GPIO_Map;

typedef struct {
    uint8_t row;
    uint8_t col;
    uint8_t isPress;   /* 1 = key down, 0 = key up */
} KeyEvent;

extern uint8_t   keyState[NUM_ROWS][NUM_COLS];
extern uint8_t   keyCounter[NUM_ROWS][NUM_COLS];
extern volatile uint8_t currentCol;

extern const char    *keyMap[NUM_ROWS][NUM_COLS];
extern const uint16_t colPins[NUM_COLS];
extern GPIO_Map       rows[NUM_ROWS];

void    Keypad_AllColumnsOff(void);
void    Keypad_ScanOneColumn(void);

/* Pop one queued event. Returns 1 if an event was retrieved, 0 otherwise.
 * Safe to call from main loop while ISR pushes (single-producer/single-consumer). */
uint8_t Keypad_GetEvent(KeyEvent *out);

#endif /* KEYDEF_H */
