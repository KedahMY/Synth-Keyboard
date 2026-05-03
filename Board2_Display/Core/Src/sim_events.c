/**
 * sim_events.c
 * Standalone note simulator — plays a full chromatic run from C3 to B4
 * (all 24 notes matching the keyboard layout) then loops.
 *
 * Called every 16 ms from the main loop. Uses HAL_GetTick() for timing
 * so the hold and gap durations are in real milliseconds regardless of
 * how long the animation_tick() takes.
 *
 * Compiled away to empty stubs when USE_UART_RX is defined (Board 1
 * connected), so no code changes are needed when switching modes.
 *
 * ELEC 3300 - Group 2
 */

#include "sim_events.h"

#ifndef USE_UART_RX

#include "note_event.h"
#include "animation.h"
#include "stm32f1xx_hal.h"

/* ------------------------------------------------------------------ */
/* Timing constants                                                     */
/* ------------------------------------------------------------------ */

/*
 * Each note is held for NOTE_HOLD_MS, then there is a silent gap of
 * NOTE_GAP_MS before the next key-down. Tune these to taste.
 *
 * At 300 ms hold + 100 ms gap = 400 ms per note:
 *   24 notes * 400 ms = 9.6 s per full cycle.
 */
#define NOTE_HOLD_MS   300u
#define NOTE_GAP_MS    100u

/* ------------------------------------------------------------------ */
/* Note table — full chromatic scale C3 to B4                          */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t note_name;   /* NOTE_C=0 .. NOTE_B=6  */
    uint8_t accidental;  /* ACC_NATURAL=0, ACC_SHARP=1 */
    uint8_t octave;
} SimNote;

static const SimNote melody[] = {
    /* --- Octave 3 --- */
    { 0, 0, 3 },   /* C3  */
    { 0, 1, 3 },   /* C#3 */
    { 1, 0, 3 },   /* D3  */
    { 1, 1, 3 },   /* D#3 */
    { 2, 0, 3 },   /* E3  */
    { 3, 0, 3 },   /* F3  */
    { 3, 1, 3 },   /* F#3 */
    { 4, 0, 3 },   /* G3  */
    { 4, 1, 3 },   /* G#3 */
    { 5, 0, 3 },   /* A3  */
    { 5, 1, 3 },   /* A#3 */
    { 6, 0, 3 },   /* B3  */
    /* --- Octave 4 --- */
    { 0, 0, 4 },   /* C4  */
    { 0, 1, 4 },   /* C#4 */
    { 1, 0, 4 },   /* D4  */
    { 1, 1, 4 },   /* D#4 */
    { 2, 0, 4 },   /* E4  */
    { 3, 0, 4 },   /* F4  */
    { 3, 1, 4 },   /* F#4 */
    { 4, 0, 4 },   /* G4  */
    { 4, 1, 4 },   /* G#4 */
    { 5, 0, 4 },   /* A4  */
    { 5, 1, 4 },   /* A#4 */
    { 6, 0, 4 },   /* B4  */
};

#define MELODY_LEN  (sizeof(melody) / sizeof(melody[0]))

/* ------------------------------------------------------------------ */
/* Simulator state                                                      */
/* ------------------------------------------------------------------ */

static uint8_t  sim_note_idx      = 0;    /* index into melody[]      */
static uint8_t  sim_key_held      = 0;    /* 0 = waiting for key-down */
static uint32_t sim_next_event_ms = 0;    /* HAL_GetTick() threshold  */

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void sim_init(void) {
    sim_note_idx      = 0;
    sim_key_held      = 0;
    /* Small startup delay so the display is ready before the first note */
    sim_next_event_ms = HAL_GetTick() + 500u;
}

void sim_tick(void) {
    uint32_t now = HAL_GetTick();
    if (now < sim_next_event_ms) return;

    const SimNote *n = &melody[sim_note_idx];
    NoteEvent evt    = { 0 };

    evt.start_byte  = NOTE_EVENT_START_BYTE;
    evt.note_name   = n->note_name;
    evt.accidental  = n->accidental;
    evt.octave      = n->octave;
    evt.track_id    = 0;

    if (!sim_key_held) {
        /* Key-down: start the note falling */
        evt.velocity    = 80;
        evt.duration_ms = 0;
        animation_queue_event(&evt);

        sim_key_held      = 1;
        sim_next_event_ms = now + NOTE_HOLD_MS;
    } else {
        /* Key-up: freeze the falling block so it scrolls off */
        evt.velocity    = 0;
        evt.duration_ms = NOTE_HOLD_MS;
        animation_queue_event(&evt);

        sim_key_held  = 0;
        sim_note_idx  = (uint8_t)((sim_note_idx + 1u) % MELODY_LEN);
        sim_next_event_ms = now + NOTE_GAP_MS;
    }
}

/* ------------------------------------------------------------------ */
/* Stubs when UART mode is active                                       */
/* ------------------------------------------------------------------ */

#else  /* USE_UART_RX defined */

void sim_init(void) {}
void sim_tick(void) {}

#endif /* USE_UART_RX */
