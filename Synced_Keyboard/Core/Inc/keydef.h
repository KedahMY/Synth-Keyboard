#pragma once
#ifndef KEYDEF_H
#define KEYDEF_H

#include "stm32f1xx_hal.h"

#define NUM_ROWS       4
#define NUM_COLS       8
#define DEBOUNCE_TICKS 3

typedef struct {
    GPIO_TypeDef *Port;
    uint16_t      Pin;
} GPIO_Map;

extern uint8_t   keyState[NUM_ROWS][NUM_COLS];
extern uint8_t   keyCounter[NUM_ROWS][NUM_COLS];
extern volatile uint8_t currentCol;

/* Press event */
extern volatile uint8_t keyEventPending;
extern volatile uint8_t keyEventRow;
extern volatile uint8_t keyEventCol;

/* Release event */
extern volatile uint8_t keyReleaseEventPending;
extern volatile uint8_t keyReleaseRow;
extern volatile uint8_t keyReleaseCol;

extern const char    *keyMap[NUM_ROWS][NUM_COLS];
extern const uint16_t colPins[NUM_COLS];
extern GPIO_Map       rows[NUM_ROWS];

void Keypad_AllColumnsOff(void);
void Keypad_ScanOneColumn(void);

#endif /* KEYDEF_H */
