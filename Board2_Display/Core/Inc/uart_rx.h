/**
 * uart_rx.h
 * UART receive handler for Board 2.
 *
 * Receives 9-byte NoteEvent packets from Board 1 over USART1 RX (PA10)
 * at 115200 baud using DMA in CIRCULAR mode with a 256-byte buffer.
 * This ensures no bytes are lost between back-to-back packets.
 *
 * ELEC 3300 - Group 2
 */

#ifndef UART_RX_H
#define UART_RX_H

#include "stm32f1xx_hal.h"

/**
 * Initialize USART1 for RX in DMA_CIRCULAR mode.
 * PA10 = USART1_RX (confirmed datasheet Table 5).
 */
void uart_rx_init(void);

/**
 * Poll the circular buffer for any new bytes received since the last call.
 * Parses complete NoteEvents and queues them for the animation engine.
 * Call this from the main loop (e.g. in animation_tick) as a safety net
 * in case the DMA transfer-complete callback hasn't fired yet.
 */
void uart_rx_buffer_process(void);

#endif /* UART_RX_H */
