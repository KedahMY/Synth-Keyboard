/**
 * sim_events.h
 * Standalone note simulator for Board 2 bring-up testing.
 * Disabled at compile time when USE_UART_RX is defined.
 *
 * ELEC 3300 - Group 2
 */

#ifndef SIM_EVENTS_H
#define SIM_EVENTS_H

void sim_init(void);
void sim_tick(void);

#endif /* SIM_EVENTS_H */
