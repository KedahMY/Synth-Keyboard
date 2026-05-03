/**
 * animation.h
 * Falling note animation engine for Board 2.
 *
 * Notes spawn at the bottom of the display (row 31, near the physical
 * keyboard) and scroll upward toward row 0. Each note is color-coded
 * and mapped to a specific column range per the design document.
 *
 * The animation tick runs every ~16 ms from the main loop (60 fps).
 *
 * ELEC 3300 - Group 2
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include "hub75.h"
#include "note_event.h"
#include <stdint.h>

/* Maximum number of notes visible on screen simultaneously */
#define MAX_ACTIVE_NOTES  32

/* Event queue for incoming NoteEvents (from UART or simulator) */
#define EVENT_QUEUE_SIZE  16

/**
 * Initialize the animation engine. Clears all active notes.
 */
void animation_init(void);

/**
 * Push a NoteEvent into the event queue.
 * Safe to call from ISR context (UART RX callback).
 * If the queue is full the oldest entry is dropped to make room,
 * ensuring release events (VEL_KEY_UP) are never lost — a dropped
 * press is less harmful than a dropped release.
 * Always returns 0.
 */
uint8_t animation_queue_event(const NoteEvent *evt);

/**
 * Main animation tick. Call this every ~16 ms from the main loop.
 * Drains the event queue, spawns new note visuals, advances positions,
 * renders into the back framebuffer, and swaps.
 */
void animation_tick(void);

#endif /* ANIMATION_H */
