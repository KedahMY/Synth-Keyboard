#include "sd_player.h"
#include "audio.h"
#include "fatfs.h"
#include <string.h>
#include <stdio.h>

extern char SDPath[4];

#define WAV_HEADER_SIZE 44

static FATFS         _fs;
static FIL           _file;
static DIR           _dir;
static FILINFO       _fno;

static SdPlayerState _state    = SD_PLAYER_IDLE;
static uint8_t       _fileOpen = 0;
static uint8_t       _dirOpen  = 0;
static char          _filename[64] = "---";

static volatile uint8_t _fillPending = 0;
static volatile uint8_t _fillHalf    = 0;

void SdPlayer_Init(void)
{
    if (f_mount(&_fs, SDPath, 1) == FR_OK) {
        if (f_opendir(&_dir, "/") == FR_OK)
            _dirOpen = 1;
    }
}

static void CloseFile(void)
{
    if (_fileOpen) { f_close(&_file); _fileOpen = 0; }
}

uint8_t SdPlayer_LoadNext(void)
{
    CloseFile();
    if (!_dirOpen) return 0;

    while (f_readdir(&_dir, &_fno) == FR_OK && _fno.fname[0] != 0) {
        if (_fno.fattrib & AM_DIR) continue;
        char *ext = strrchr(_fno.fname, '.');
        if (!ext) continue;
        if (strcmp(ext, ".WAV") != 0 && strcmp(ext, ".wav") != 0) continue;

        char path[72];
        snprintf(path, sizeof(path), "%s%s", SDPath, _fno.fname);
        if (f_open(&_file, path, FA_READ) == FR_OK) {
            f_lseek(&_file, WAV_HEADER_SIZE);
            strncpy(_filename, _fno.fname, sizeof(_filename) - 1);
            _filename[sizeof(_filename) - 1] = '\0';
            _fileOpen = 1;
            return 1;
        }
    }

    f_rewinddir(&_dir);
    strncpy(_filename, "---", sizeof(_filename));
    return 0;
}

void SdPlayer_Play(void)
{
    if (!_fileOpen && !SdPlayer_LoadNext()) return;
    _state = SD_PLAYER_PLAYING;
}

void SdPlayer_Stop(void)
{
    _state = SD_PLAYER_IDLE;
    _fillPending = 0;
    CloseFile();
    strncpy(_filename, "---", sizeof(_filename));
}

void SdPlayer_Toggle(void)
{
    if      (_state == SD_PLAYER_PLAYING) _state = SD_PLAYER_PAUSED;
    else if (_state == SD_PLAYER_PAUSED)  _state = SD_PLAYER_PLAYING;
    else                                  SdPlayer_Play();
}

/* Called from DAC DMA ISR — just sets a flag, never blocks */
void SdPlayer_RequestFill(uint8_t half)
{
    _fillHalf    = half;
    _fillPending = 1;
}

/* Called from main() loop — f_read() polling is safe here */
void SdPlayer_Tick(void)
{
    if (!_fillPending) return;
    if (_state != SD_PLAYER_PLAYING || !_fileOpen) {
        _fillPending = 0;
        return;
    }

    uint8_t  half = _fillHalf;
    _fillPending  = 0;

    uint16_t *dst = &audioBuf[half * AUDIO_HALF_SIZE];

    /* 4-byte aligned for SDIO internal DMA (FatFS handles this) */
    static int16_t readBuf[AUDIO_HALF_SIZE] __attribute__((aligned(4)));

    UINT    bytesRead = 0;
    FRESULT res = f_read(&_file, readBuf,
                         AUDIO_HALF_SIZE * sizeof(int16_t), &bytesRead);

    uint32_t samples = bytesRead / sizeof(int16_t);

    /* Signed 16-bit PCM → unsigned 12-bit DAC */
    for (uint32_t i = 0; i < samples; i++) {
        dst[i] = (uint16_t)(((int32_t)readBuf[i] + 32768) >> 4);
    }
    /* Pad with silence */
    for (uint32_t i = samples; i < AUDIO_HALF_SIZE; i++) {
        dst[i] = 2048;
    }

    /* End of file → auto-advance */
    if (res != FR_OK || bytesRead < AUDIO_HALF_SIZE * sizeof(int16_t)) {
        CloseFile();
        _state = SdPlayer_LoadNext() ? SD_PLAYER_PLAYING : SD_PLAYER_IDLE;
    }
}

SdPlayerState SdPlayer_GetState(void)    { return _state;    }
const char   *SdPlayer_GetFilename(void) { return _filename; }
