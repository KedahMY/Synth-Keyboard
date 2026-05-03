#include "keydef.h"
#include "main.h"

uint8_t keyState[NUM_ROWS][NUM_COLS]   = {0};
uint8_t keyCounter[NUM_ROWS][NUM_COLS] = {0};

volatile uint8_t currentCol = 0;

/* Press event */
volatile uint8_t keyEventPending = 0;
volatile uint8_t keyEventRow     = 0;
volatile uint8_t keyEventCol     = 0;

/* Release event */
volatile uint8_t keyReleaseEventPending = 0;
volatile uint8_t keyReleaseRow          = 0;
volatile uint8_t keyReleaseCol          = 0;

const char *keyMap[NUM_ROWS][NUM_COLS] = {
    {"R0C0","R0C1","R0C2","R0C3","R0C4","R0C5","R0C6","R0C7"},
    {"R1C0","R1C1","R1C2","R1C3","R1C4","R1C5","R1C6","R1C7"},
    {"R2C0","R2C1","R2C2","R2C3","R2C4","R2C5","R2C6","R2C7"},
    {""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    }
};

const uint16_t colPins[NUM_COLS] = {
    Col0_Pin, Col1_Pin, Col2_Pin, Col3_Pin,
    Col4_Pin, Col5_Pin, Col6_Pin, Col7_Pin
};

GPIO_Map rows[NUM_ROWS] = {
    {GPIOC, Row0_Pin},
    {GPIOC, Row1_Pin},
    {GPIOA, Row2_Pin},
    {GPIOA, Row3_Pin}
};

void Keypad_AllColumnsOff(void)
{
    for (int c = 0; c < NUM_COLS; c++) {
        HAL_GPIO_WritePin(GPIOB, colPins[c], GPIO_PIN_RESET);
    }
}

void Keypad_ScanOneColumn(void)
{
    Keypad_AllColumnsOff();
    HAL_GPIO_WritePin(GPIOB, colPins[currentCol], GPIO_PIN_SET);

    for (int r = 0; r < NUM_ROWS; r++) {

        uint8_t rawPressed =
            (HAL_GPIO_ReadPin(rows[r].Port, rows[r].Pin) == GPIO_PIN_SET) ? 1 : 0;

        if (rawPressed != keyState[r][currentCol]) {
            keyCounter[r][currentCol]++;

            if (keyCounter[r][currentCol] >= DEBOUNCE_TICKS) {
                keyState[r][currentCol]   = rawPressed;
                keyCounter[r][currentCol] = 0;

                if (rawPressed) {
                    /* KEY PRESSED */
                    keyEventPending = 1;
                    keyEventRow     = r;
                    keyEventCol     = currentCol;
                } else {
                    /* KEY RELEASED */
                    keyReleaseEventPending = 1;
                    keyReleaseRow          = r;
                    keyReleaseCol          = currentCol;
                }
            }
        } else {
            keyCounter[r][currentCol] = 0;
        }
    }

    HAL_GPIO_WritePin(GPIOB, colPins[currentCol], GPIO_PIN_RESET);

    currentCol++;
    if (currentCol >= NUM_COLS) {
        currentCol = 0;
    }
}
