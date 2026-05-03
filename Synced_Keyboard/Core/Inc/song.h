#ifndef SONG_H
#define SONG_H

#include <stdint.h>

/* Renamed to avoid conflict with UART NoteEvent packet */
typedef struct {
    float    frequency;
    uint16_t duration_ms;
} SongNote;

/* Sentinel to mark end of a sequence */
#define NOTE_END  { 0.0f, 0 }

extern const SongNote riffA[];
extern const SongNote riffB[];

#endif /* SONG_H */
