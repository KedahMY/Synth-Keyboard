#include "song_player.h"
#include "audio.h"

static const SongNote *_seq = NULL;  /* ← Changed from NoteEvent */
static uint32_t         _noteIndex = 0;
static uint32_t         _noteTimer = 0;
static uint8_t          _playing   = 0;
static uint8_t          _paused    = 0;

void SongPlayer_Start(const SongNote *sequence)  /* ← Changed parameter type */
{
    _seq       = sequence;
    _noteIndex = 0;
    _noteTimer = HAL_GetTick();
    _playing   = 1;
    _paused    = 0;

    if (_seq[0].duration_ms != 0) {
        Audio_SetNote(_seq[0].frequency);
    }
}

void SongPlayer_Toggle(const SongNote *sequence)  /* ← Changed parameter type */
{
    /* If a different sequence is requested, start fresh */
    if (_seq != sequence) {
        SongPlayer_Start(sequence);
        return;
    }

    /* Same sequence — toggle play/pause */
    if (!_playing) {
        SongPlayer_Start(sequence);
    } else if (_paused) {
        /* Resume */
        _paused    = 0;
        _noteTimer = HAL_GetTick();
        Audio_SetNote(_seq[_noteIndex].frequency);
    } else {
        /* Pause */
        _paused = 1;
        Audio_Stop();
    }
}

void SongPlayer_Stop(void)
{
    _playing   = 0;
    _paused    = 0;
    _seq       = NULL;
    _noteIndex = 0;
    Audio_Stop();
}

void SongPlayer_MuteForKey(void)
{
    if (_playing && !_paused) {
        _paused = 1;
        Audio_Stop();
    }
}

void SongPlayer_UnmuteAfterKey(void)
{
    if (_playing && _paused) {
        _paused    = 0;
        _noteTimer = HAL_GetTick();
        Audio_SetNote(_seq[_noteIndex].frequency);
    }
}

uint8_t SongPlayer_IsPlaying(void)  { return _playing; }
uint8_t SongPlayer_IsPaused(void)   { return _paused;  }

void SongPlayer_Tick(void)
{
    if (!_playing || _paused || _seq == NULL) return;

    uint32_t now = HAL_GetTick();
    uint16_t dur = _seq[_noteIndex].duration_ms;

    if (dur == 0) {
        /* NOTE_END — loop back */
        _noteIndex = 0;
        _noteTimer = now;
        Audio_SetNote(_seq[0].frequency);
        return;
    }

    if ((now - _noteTimer) >= dur) {
        _noteIndex++;
        _noteTimer = now;

        if (_seq[_noteIndex].duration_ms == 0) {
            _noteIndex = 0;   /* Loop */
        }

        Audio_SetNote(_seq[_noteIndex].frequency);
    }
}
