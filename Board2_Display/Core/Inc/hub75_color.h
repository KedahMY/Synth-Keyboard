/**
 * hub75_color.h
 * Pixel format and colour constants for RGB222 Binary Code Modulation mode.
 *
 * Pixel byte layout (stored in fb[2][32][64]):
 *
 *   bit 7  bit 6  bit 5  bit 4  bit 3  bit 2  bit 1  bit 0
 *   unused unused   R1     R0     G1     G0     B1     B0
 *
 *   R = bits [5:4]  (0-3)
 *   G = bits [3:2]  (0-3)
 *   B = bits [1:0]  (0-3)
 *
 *   Pack macro:  RGB222(r,g,b)  with r,g,b each in range 0..3
 *
 * With 4 levels per channel the display can show 4^3 = 64 distinct colours.
 * The rainbow table in animation.c uses 16 of these for the gradient.
 */

#ifndef HUB75_COLOR_H
#define HUB75_COLOR_H

#include <stdint.h>

/* Convenience macro for building pixel values */
#define RGB222(r,g,b)   (uint8_t)(((r)<<4)|((g)<<2)|(b))

/*
 * Named colour constants (drop-in replacements for the old 3-bit COLOR_xxx).
 * Full intensity on a channel = 3, half = 2, quarter = 1, off = 0.
 */
#define COLOR_BLACK    RGB222(0,0,0)
#define COLOR_RED      RGB222(3,0,0)
#define COLOR_GREEN    RGB222(0,3,0)
#define COLOR_BLUE     RGB222(0,0,3)
#define COLOR_YELLOW   RGB222(3,3,0)
#define COLOR_CYAN     RGB222(0,3,3)
#define COLOR_MAGENTA  RGB222(3,0,3)
#define COLOR_WHITE    RGB222(3,3,3)
#define COLOR_ORANGE   RGB222(3,2,0)  /* not representable in 3-bit mode */
#define COLOR_VIOLET   RGB222(2,0,3)  /* not representable in 3-bit mode */

#endif /* HUB75_COLOR_H */
