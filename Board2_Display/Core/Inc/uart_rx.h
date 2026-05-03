/**
 * uart_rx.h
 * UART receive handler for Board 2.
 *
 * Receives 8-byte NoteEvent packets from Board 1 over USART1 RX (PA10)
 * at 115200 baud using DMA with idle line detection.
 *
 * Currently scaffolded. To activate for real Board 1 integration:
 *   1. Add -DUSE_UART_RX to your compiler defines in STM32CubeIDE.
 *   2. This will compile in the UART init and callbacks below.
 *   3. The simulator (sim_events.c) will automatically compile out.
 *
 * ELEC 3300 - Group 2
 */

#ifndef UART_RX_H
#define UART_RX_H

#include "stm32f1xx_hal.h"

/**
 * Initialize USART1 for RX in DMA mode with idle line detection.
 * PA10 = USART1_RX (confirmed datasheet Table 5).
 */
void uart_rx_init(void);

#endif /* UART_RX_H */
