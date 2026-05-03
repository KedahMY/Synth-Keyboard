#ifndef UART_TX_H
#define UART_TX_H

#include "stm32f1xx_hal.h"
#include "note_event.h"

void UART_TX_Init(void);
void UART_TX_SendNoteEvent(const NoteEvent *evt);

#endif
