/**
 * note_event.h
 * Shared header between Board 1 (Brain) and Board 2 (Display Engine).
 * Defines the 8-byte NoteEvent packet used for UART communication,
 * SD card storage, and internal event queuing.
 *
 * ELEC 3300 - Group 2 - Compact Synth Keyboard
 */

#ifndef NOTE_EVENT_H
#define NOTE_EVENT_H

#include <stdint.h>

#define NOTE_EVENT_START_BYTE  0xAA
#define NOTE_EVENT_SIZE        9

/* Note name encoding */
#define NOTE_C   0
#define NOTE_D   1
#define NOTE_E   2
#define NOTE_F   3
#define NOTE_G   4
#define NOTE_A   5
#define NOTE_B   6

/* Accidental encoding */
#define ACC_NATURAL  0
#define ACC_SHARP    1
#define ACC_FLAT     2

/* Velocity conventions */
#define VEL_KEY_UP     0
#define VEL_KEY_DOWN   100   /* default on velocity */

/* Track IDs */
#define TRACK_LIVE     0
#define TRACK_LOOP1    1
#define TRACK_LOOP2    2
#define TRACK_LOOP3    3

#pragma pack(push, 1)
typedef struct {
    uint8_t  active;        /* 1 = key down (note on), 0 = key up (note off) */
    uint8_t  start_byte;    /* 0xAA fixed delimiter                        */
    uint8_t  note_name;     /* NOTE_C=0 ... NOTE_B=6                       */
    uint8_t  accidental;    /* ACC_NATURAL=0, ACC_SHARP=1, ACC_FLAT=2      */
    uint8_t  octave;        /* 2 to 7 (MIDI octave numbering)              */
    uint8_t  velocity;      /* 0 = key up, 64..127 = key down              */
    uint16_t duration_ms;   /* ms the note was held (0 if still pressed)   */
    uint8_t  track_id;      /* 0=live, 1..3=looper tracks                  */
} NoteEvent;
#pragma pack(pop)

/* Sanity check at compile time */
_Static_assert(sizeof(NoteEvent) == NOTE_EVENT_SIZE,
               "NoteEvent struct must be exactly 9 bytes");

#endif /* NOTE_EVENT_H */
