#ifndef WAVETABLE_H
#define WAVETABLE_H

#include <stdint.h>

#define WAVE_TABLE_SIZE 256

/* 12-bit sine wave, centred at 2048, amplitude ±2000 */
extern const uint16_t sine_table[WAVE_TABLE_SIZE];
extern const uint16_t square_table[WAVE_TABLE_SIZE];
extern const uint16_t triangle_table[WAVE_TABLE_SIZE];

#endif /* WAVETABLE_H */
