#include "keydef.h"
#include "main.h"

uint8_t keyState[NUM_ROWS][NUM_COLS]   = {0};
uint8_t keyCounter[NUM_ROWS][NUM_COLS] = {0};

volatile uint8_t currentCol = 0;

/* SPSC ring buffer: ISR (TIM3 callback) is the sole producer,
 * the main loop is the sole consumer. Power-of-two size lets us
 * wrap with a mask, and means head==tail unambiguously means empty. */
static volatile KeyEvent kev_buf[KEY_EVENT_QUEUE_SIZE];
static volatile uint8_t  kev_head = 0;   /* next write slot */
static volatile uint8_t  kev_tail = 0;   /* next read  slot */

#define KEV_MASK  (KEY_EVENT_QUEUE_SIZE - 1)
_Static_assert((KEY_EVENT_QUEUE_SIZE & KEV_MASK) == 0,
               "KEY_EVENT_QUEUE_SIZE must be a power of two");

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

static void Keypad_PushEvent(uint8_t r, uint8_t c, uint8_t isPress)
{
    uint8_t head = kev_head;
    uint8_t next = (uint8_t)((head + 1u) & KEV_MASK);
    /* Drop on overflow rather than overwriting unread entries: losing the
     * newest event is preferable to corrupting the FIFO order of older
     * pending press/release pairs. */
    if (next == kev_tail) return;
    kev_buf[head].row     = r;
    kev_buf[head].col     = c;
    kev_buf[head].isPress = isPress;
    kev_head = next;
}

uint8_t Keypad_GetEvent(KeyEvent *out)
{
    uint8_t tail = kev_tail;
    if (tail == kev_head) return 0;
    out->row     = kev_buf[tail].row;
    out->col     = kev_buf[tail].col;
    out->isPress = kev_buf[tail].isPress;
    kev_tail = (uint8_t)((tail + 1u) & KEV_MASK);
    return 1;
}

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
                Keypad_PushEvent((uint8_t)r, currentCol, rawPressed);
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
