#ifndef SD_PLAYER_H
#define SD_PLAYER_H

#include <stdint.h>

typedef enum {
    SD_PLAYER_IDLE = 0,
    SD_PLAYER_PLAYING,
    SD_PLAYER_PAUSED,
} SdPlayerState;

void          SdPlayer_Init(void);
void          SdPlayer_Play(void);
void          SdPlayer_Stop(void);
void          SdPlayer_Toggle(void);
uint8_t       SdPlayer_LoadNext(void);
void          SdPlayer_RequestFill(uint8_t half);
void          SdPlayer_Tick(void);
SdPlayerState SdPlayer_GetState(void);
const char   *SdPlayer_GetFilename(void);

#endif /* SD_PLAYER_H */
