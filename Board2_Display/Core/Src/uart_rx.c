/**
 * uart_rx.c
 * USART1 RX via DMA1 Channel 5 in CIRCULAR mode on Board 2.
 *
 * Uses a 256-byte circular buffer so the DMA is ALWAYS receiving —
 * no bytes can be lost between back-to-back NoteEvent packets.
 * The Transfer Complete callback compares the DMA counter with the
 * previous value to determine how many new bytes arrived, then parses
 * complete 9-byte NoteEvents from the stream.
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

/* Circular buffer large enough that the DMA never wraps faster than
 * the callback can detect it. At 115200 baud, 256 bytes = ~178 ms.
 * The callback fires every full wrap and the animation loop polls
 * at ~60 Hz, so we never lose data. */
#define UART_RX_BUF_SIZE  256u
static uint8_t uart_rx_buf[UART_RX_BUF_SIZE];
static uint8_t prev_dma_counter = UART_RX_BUF_SIZE;

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

    /* DMA1 Channel 5 for USART1 RX — CIRCULAR mode so the DMA never
     * stops. Bytes are always written into the ring buffer even while
     * the previous callback is still running. */
    hdma_usart1_rx.Instance                 = DMA1_Channel5;
    hdma_usart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_usart1_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_usart1_rx);
    __HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);

    /* USART1: 115200 8N1, RX only */
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    /* NVIC for DMA1 Channel 5 */
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    /* Start continuous circular DMA reception. The DMA counter starts
     * at 256 and decrements as bytes arrive. On transfer complete (counter
     * hits 0 and wraps to 256) the HAL_UART_RxCpltCallback fires. */
    HAL_UART_Receive_DMA(&huart1, uart_rx_buf, UART_RX_BUF_SIZE);

    /* Disable the half-transfer interrupt (circular buffer, not needed) */
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

/* ------------------------------------------------------------------ */
/* Parse new bytes in the circular buffer for complete NoteEvents       */
/* ------------------------------------------------------------------ */

void uart_rx_buffer_process(void) {
    /* Read the DMA counter — it decreases as bytes are written into
     * the circular buffer. The position of the next write is:
     *   write_pos = UART_RX_BUF_SIZE - counter
     * When the DMA wraps (counter jumps from 0 back to 256), the
     * formula still holds because (256 - 256) = 0 = start of buffer. */
    uint32_t remaining = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
    if (remaining > UART_RX_BUF_SIZE) remaining = UART_RX_BUF_SIZE;
    uint8_t dma_counter = (uint8_t)remaining;

    uint8_t prev = prev_dma_counter;
    prev_dma_counter = dma_counter;

    uint16_t new_bytes;
    if (dma_counter < prev) {
        new_bytes = (uint16_t)(prev - dma_counter);
    } else {
        /* DMA wrapped — counter reset from 0 to 256 */
        new_bytes = (uint16_t)(UART_RX_BUF_SIZE - dma_counter + prev);
    }
    if (new_bytes == 0) return;
    if (new_bytes > UART_RX_BUF_SIZE) return;

    /* Where to start reading new data */
    uint16_t read_start = (uint16_t)(UART_RX_BUF_SIZE - prev);
    if (read_start >= UART_RX_BUF_SIZE) read_start = 0;

    /* Parse complete 9-byte NoteEvents from the stream. The buffer
     * may wrap, so we read byte-by-byte with modulo indexing. */
    uint16_t pos = read_start;
    while (new_bytes > 0) {
        /* Find start byte */
        uint16_t avail = new_bytes;
        while (avail > 0 && uart_rx_buf[pos % UART_RX_BUF_SIZE] != NOTE_EVENT_START_BYTE) {
            pos++;
            avail--;
        }
        if (avail < NOTE_EVENT_SIZE) break;

        /* We have a potential start byte. Read 9 bytes into a temp buffer. */
        uint8_t pkt[NOTE_EVENT_SIZE];
        for (uint8_t i = 0; i < NOTE_EVENT_SIZE; i++) {
            pkt[i] = uart_rx_buf[pos % UART_RX_BUF_SIZE];
            pos++;
        }
        avail -= NOTE_EVENT_SIZE;
        new_bytes -= NOTE_EVENT_SIZE;

        /* Validate packet fields before queueing */
        NoteEvent *evt = (NoteEvent *)pkt;
        if (evt->start_byte == NOTE_EVENT_START_BYTE &&
            evt->note_name <= NOTE_B &&
            evt->accidental <= ACC_FLAT &&
            evt->track_id <= TRACK_LOOP3 &&
            (evt->active == 0 || evt->active == 1)) {
            animation_queue_event(evt);
        }
        /* Else: skip past the bad start byte, keep scanning (avail already consumed) */
    }
}

/* ------------------------------------------------------------------ */
/* Callbacks (called from ISR context)                                  */
/* ------------------------------------------------------------------ */

/**
 * Fires on DMA Transfer Complete (counter wrapped from 0 to 256).
 * Parse any new bytes that arrived since the last callback.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *h) {
    if (h->Instance != USART1) return;
    uart_rx_buffer_process();
}

/* Also handle the idle-line event callback in case idle detection
 * fires between packets. Process the same way. */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *h, uint16_t sz) {
    if (h->Instance != USART1) return;
    (void)sz;
    uart_rx_buffer_process();
}

/* ------------------------------------------------------------------ */
/* IRQ handlers                                                         */
/* ------------------------------------------------------------------ */
void DMA1_Channel5_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

#else
/* Stub when simulator is active */
void uart_rx_init(void) { /* no-op */ }
void uart_rx_buffer_process(void) { /* no-op */ }
#endif /* USE_UART_RX */
