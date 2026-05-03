#ifndef SONG_PLAYER_H
#define SONG_PLAYER_H

#include "song.h"

void    SongPlayer_Start(const SongNote *sequence);
void    SongPlayer_Toggle(const SongNote *sequence);
void    SongPlayer_Stop(void);
void    SongPlayer_MuteForKey(void);
void    SongPlayer_UnmuteAfterKey(void);
void    SongPlayer_Tick(void);
uint8_t SongPlayer_IsPlaying(void);
uint8_t SongPlayer_IsPaused(void);

#endif /* SONG_PLAYER_H */
