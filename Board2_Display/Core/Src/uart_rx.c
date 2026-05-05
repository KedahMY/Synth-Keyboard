/**
 * uart_rx.c
 * USART1 RX via DMA with idle-line detection on Board 2.
 *
 * When a full or partial packet arrives and the line goes idle,
 * the DMA callback fires. We validate the start byte and push
 * the NoteEvent into the animation queue.
 *
 * Wire connections:
 *   Board 1 PA9  (USART1 TX) -----> Board 2 PA10 (USART1 RX)
 *   Board 1 GND  -----> Board 2 GND
 *
 * ELEC 3300 - Group 2
 */

#include "uart_rx.h"
#include "animation.h"
#include "note_event.h"
#include <string.h>

/* Only compile if real UART mode is enabled */
#ifdef USE_UART_RX

static UART_HandleTypeDef huart1;
static DMA_HandleTypeDef  hdma_usart1_rx;
static uint8_t uart_rx_buf[2][NOTE_EVENT_SIZE];
static uint8_t rx_active = 0;

/* ------------------------------------------------------------------ */
/* USART1 + DMA1 Channel 5 init                                        */
/* ------------------------------------------------------------------ */
/*
 * Per RM0008 Table 78: USART1_RX is mapped to DMA1 Channel 5.
 */

void uart_rx_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* PA10 = USART1 RX: input floating (default for UART RX) */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* DMA1 Channel 5 for USART1 RX */
    hdma_usart1_rx.Instance                 = DMA1_Channel5;
    hdma_usart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode                = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_usart1_rx);
    __HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);

    /* USART1: 115200 8N1 */
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    /* NVIC for DMA1 Channel 5 and USART1 */
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    /* Start listening for packets using idle line detection.
     * This fires an interrupt when the UART line has been idle
     * for one byte period after receiving data, which neatly
     * frames our 8-byte packets. */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_buf[0], NOTE_EVENT_SIZE);

    /* Disable the DMA half-transfer interrupt (we only care about idle) */
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

/* ------------------------------------------------------------------ */
/* Callbacks (called from ISR context)                                  */
/* ------------------------------------------------------------------ */

/**
 * This HAL callback fires when the UART line goes idle after receiving
 * data, OR when the DMA transfer completes (whichever comes first).
 * sz = number of bytes actually received.
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *h, uint16_t sz) {
    if (h->Instance != USART1) return;

    uint8_t just_filled = rx_active;
    rx_active ^= 1u;

    /* Re-arm onto the other buffer before reading, so DMA cannot
     * overwrite just_filled while we copy out of it. */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_buf[rx_active], NOTE_EVENT_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

    if (sz != NOTE_EVENT_SIZE) return;

    NoteEvent *evt = (NoteEvent *)uart_rx_buf[just_filled];

    /* Structural sanity checks — reject obviously corrupted packets before
     * they enter the animation pipeline.  A corrupted 'active' byte is the
     * primary cause of permanently-stuck notes: a key-up packet with
     * active=1 spawns a ghost note that never gets stopped. */
    if (evt->start_byte != NOTE_EVENT_START_BYTE) return;
    if (evt->note_name  >  NOTE_B)                return;
    if (evt->accidental >  ACC_FLAT)              return;
    if (evt->octave < 2 || evt->octave > 7)       return;
    if (evt->active > 1)                          return;
    /* Board 1 always sets velocity=VEL_KEY_DOWN when active=1 and
     * velocity=VEL_KEY_UP when active=0.  A mismatch means a byte was
     * corrupted; drop the packet rather than misrouting it. */
    if (evt->active == 1 && evt->velocity != VEL_KEY_DOWN) return;
    if (evt->active == 0 && evt->velocity != VEL_KEY_UP)   return;

    animation_queue_event(evt);
}

/* ------------------------------------------------------------------ */
/* IRQ handlers                                                         */
/* ------------------------------------------------------------------ */
/* These should go in stm32f1xx_it.c, but are placed here for clarity.
 * If your project already has these handlers in the IT file, move
 * the bodies there and add extern declarations for huart1 and
 * hdma_usart1_rx. */

void DMA1_Channel5_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

#else
/* Stub when simulator is active */
void uart_rx_init(void) { /* no-op */ }
#endif /* USE_UART_RX */
