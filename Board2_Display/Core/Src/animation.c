/**
 * animation.c
 * Falling note animation engine — RGB222 BCM version.
 *
 * Column layout (64px wide, 2 octaves):
 *
 *   col  0        : left margin (empty)
 *   cols 1..31    : octave 3  (31px)
 *   col  32       : gap between octaves (empty)
 *   cols 33..63   : octave 4  (31px)
 *
 * Within each 31px octave block the keys sit like a real piano:
 *   natural (white) keys : 3px wide, laid end-to-end
 *   accidental (black)   : 2px wide, tucked between the naturals
 *   no padding between any adjacent keys
 *
 *   C  C# D  D# E  F  F# G  G# A  A# B
 *   0  3  5  8  10 13 16 18 21 23 26 28   <- offset within block
 *   3  2  3  2  3  3  2  3  2  3  2  3   <- width
 *   sum = 31 ✓
 *
 * Flats are treated as their enharmonic sharp (Db = C#, etc.).
 *
 * Rainbow gradient: row 0 (top) = violet, row 31 (bottom) = red.
 * Colour is row-position only; note identity does not affect colour.
 *
 * ELEC 3300 - Group 2
 */

#include "animation.h"
#include "hub75_color.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Rainbow colour lookup — 32 rows, RGB222                             */
/* ------------------------------------------------------------------ */

static const uint8_t rainbow_row[32] = {
    /* row  0 */ RGB222(3,0,3),   /* magenta/violet  */
    /* row  1 */ RGB222(3,0,3),
    /* row  2 */ RGB222(2,0,3),   /* violet          */
    /* row  3 */ RGB222(2,0,3),
    /* row  4 */ RGB222(1,0,3),   /* blue-violet     */
    /* row  5 */ RGB222(1,0,3),
    /* row  6 */ RGB222(0,0,3),   /* blue            */
    /* row  7 */ RGB222(0,0,3),
    /* row  8 */ RGB222(0,1,3),   /* blue-teal       */
    /* row  9 */ RGB222(0,1,3),
    /* row 10 */ RGB222(0,2,3),   /* cyan-blue       */
    /* row 11 */ RGB222(0,2,3),
    /* row 12 */ RGB222(0,3,3),   /* cyan            */
    /* row 13 */ RGB222(0,3,3),
    /* row 14 */ RGB222(0,3,2),   /* teal            */
    /* row 15 */ RGB222(0,3,2),
    /* row 16 */ RGB222(0,3,1),   /* green-teal      */
    /* row 17 */ RGB222(0,3,1),
    /* row 18 */ RGB222(0,3,0),   /* green           */
    /* row 19 */ RGB222(0,3,0),
    /* row 20 */ RGB222(1,3,0),   /* yellow-green    */
    /* row 21 */ RGB222(1,3,0),
    /* row 22 */ RGB222(2,3,0),   /* yellow-green    */
    /* row 23 */ RGB222(2,3,0),
    /* row 24 */ RGB222(3,3,0),   /* yellow          */
    /* row 25 */ RGB222(3,3,0),
    /* row 26 */ RGB222(3,2,0),   /* orange          */
    /* row 27 */ RGB222(3,2,0),
    /* row 28 */ RGB222(3,1,0),   /* red-orange      */
    /* row 29 */ RGB222(3,1,0),
    /* row 30 */ RGB222(3,0,0),   /* red             */
    /* row 31 */ RGB222(3,0,0),   /* red (keyboard)  */
};

/* ------------------------------------------------------------------ */
/* Note visual descriptor                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t  active;
    uint8_t  col_start;
    uint8_t  col_width;
    int16_t  head_y;
    uint8_t  length;
    uint8_t  growing;
    uint8_t  note_name;
    uint8_t  accidental;
    uint8_t  octave;     /* stored to compute visual block for stop_growing match */
} ActiveNote;

static ActiveNote notes[MAX_ACTIVE_NOTES];

/* ------------------------------------------------------------------ */
/* Piano column layout                                                  */
/* ------------------------------------------------------------------ */

/*
 * Offset of each natural note from the start of its 31px octave block.
 * Indexed by note_name (0=C, 1=D, 2=E, 3=F, 4=G, 5=A, 6=B).
 */
static const uint8_t natural_col[7] = {
     0,   /* C  */
     5,   /* D  */
    10,   /* E  */
    13,   /* F  */
    18,   /* G  */
    23,   /* A  */
    28,   /* B  */
};

/*
 * Offset of each accidental (sharp / flat) from the start of its block.
 * Indexed by note_name of the LOWER natural (e.g. C# → index 0 = C).
 * E (index 2) and B (index 6) have no sharp — entries unused.
 */
static const uint8_t sharp_col[7] = {
     3,   /* C# */
     8,   /* D# */
     0,   /* E# (unused) */
    16,   /* F# */
    21,   /* G# */
    26,   /* A# */
     0,   /* B# (unused) */
};

#define NATURAL_WIDTH   3u   /* white keys    */
#define ACCIDENTAL_WIDTH 2u  /* black keys    */
#define BASE_OCTAVE     3u   /* lowest octave */

/*
 * Column at which each octave block begins.
 *   Octave 3: col 1  (1px left margin)
 *   Octave 4: col 33 (col 1 + 31px block + 1px gap)
 * Formula: block_start = 1 + (oct - BASE_OCTAVE) * 32
 */
#define OCTAVE_STRIDE   32u  /* 31px block + 1px gap */
#define FIRST_OCT_COL    1u  /* 1px left margin      */

/* ------------------------------------------------------------------ */
/* Column resolver                                                      */
/* ------------------------------------------------------------------ */

static void resolve_note_visual(const NoteEvent *evt,
                                uint8_t *out_col,
                                uint8_t *out_width)
{
    /*
     * Map any octave to one of the two display blocks using signed modulo.
     * Notes 2 octaves apart share the same physical key on the keyboard and
     * must land in the same visual column. Wrapping preserves that invariant;
     * clamping broke it (e.g. C2 clamped to octave 3 when it should share
     * octave 4's block with C4 after a -2 octave shift).
     *
     *   block_idx = ((oct - BASE_OCTAVE) % 2 + 2) % 2
     *
     *   oct=3 (BASE)  : (0%2+2)%2 = 0  -> left block  (col  1)
     *   oct=4         : (1%2+2)%2 = 1  -> right block (col 33)
     *   oct=2 (C4-2)  : (-1%2+2)%2 = 1 -> right block (col 33) same as C4
     *   oct=5 (C3+2)  : (2%2+2)%2 = 0  -> left block  (col  1) same as C3
     *   oct=1         : (-2%2+2)%2 = 0  -> left block
     *   oct=6         : (3%2+2)%2 = 1  -> right block
     */
    int8_t  oct_diff  = (int8_t)evt->octave - (int8_t)BASE_OCTAVE;
    uint8_t block_idx = (uint8_t)(((int8_t)(oct_diff % 2) + 2) % 2);
    uint8_t block     = (uint8_t)(FIRST_OCT_COL + block_idx * OCTAVE_STRIDE);

    if (evt->accidental == ACC_SHARP) {
        if (evt->note_name == 2u || evt->note_name == 6u) {
            /* E# = F, B# = C — treat as the natural above */
            *out_col   = block + natural_col[(evt->note_name + 1u) % 7u];
            *out_width = NATURAL_WIDTH;
        } else {
            *out_col   = block + sharp_col[evt->note_name];
            *out_width = ACCIDENTAL_WIDTH;
        }
    } else if (evt->accidental == ACC_FLAT) {
        /*
         * Flat of note N is enharmonic to sharp of (N-1).
         * Db = C#, Eb = D#, Gb = F#, Ab = G#, Bb = A#.
         * Cb = B, Fb = E — treat as the natural below.
         */
        uint8_t prev = (evt->note_name == 0u) ? 6u : evt->note_name - 1u;
        if (prev == 2u || prev == 6u) {
            /* Cb = B, Fb = E */
            *out_col   = block + natural_col[prev];
            *out_width = NATURAL_WIDTH;
        } else {
            *out_col   = block + sharp_col[prev];
            *out_width = ACCIDENTAL_WIDTH;
        }
    } else {
        /* Natural */
        *out_col   = block + natural_col[evt->note_name];
        *out_width = NATURAL_WIDTH;
    }

    if (*out_col >= HUB75_WIDTH) *out_col = HUB75_WIDTH - *out_width;
}

/* ------------------------------------------------------------------ */
/* Event queue                                                          */
/* ------------------------------------------------------------------ */

static NoteEvent event_queue[EVENT_QUEUE_SIZE];
static volatile uint8_t eq_head = 0;
static volatile uint8_t eq_tail = 0;

uint8_t animation_queue_event(const NoteEvent *evt) {
    uint8_t next = (eq_head + 1u) % EVENT_QUEUE_SIZE;
    if (next == eq_tail) {
        /* Queue full — drop oldest entry to make room so that release
         * events (VEL_KEY_UP) are never lost. A dropped press is less
         * harmful than a dropped release (which leaves a note growing
         * permanently). */
        eq_tail = (eq_tail + 1u) % EVENT_QUEUE_SIZE;
    }
    event_queue[eq_head] = *evt;
    eq_head = next;
    return 0;
}

static uint8_t queue_pop(NoteEvent *out) {
    if (eq_tail == eq_head) return 0;
    *out = event_queue[eq_tail];
    eq_tail = (eq_tail + 1u) % EVENT_QUEUE_SIZE;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Spawn / stop helpers                                                 */
/* ------------------------------------------------------------------ */

static void spawn_note(const NoteEvent *evt) {
    uint8_t col, width;
    resolve_note_visual(evt, &col, &width);
    for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
        if (!notes[i].active) {
            notes[i].active     = 1;
            notes[i].col_start  = col;
            notes[i].col_width  = width;
            /* Spawn visible at the bottom of the trail region (row 30,
             * just above the row-31 keyboard indicator). Trail extends
             * upward each tick while the key is held, so the LED lights
             * up immediately on press instead of being hidden until
             * release. */
            notes[i].head_y     = 30;
            notes[i].length     = 1;
            notes[i].growing    = 1;
            notes[i].note_name  = evt->note_name;
            notes[i].accidental = evt->accidental;
            notes[i].octave     = evt->octave;
            return;
        }
    }
}

/*
 * Resolve an octave value to its visual block index (0 or 1) on the
 * 64px display. Notes that are an octave apart wrap to the same block
 * so that e.g. C4 with octaveOffset=0 and C4 with octaveOffset=+2
 * land on the same physical column. This is the same wrapping used
 * by resolve_note_visual() during spawn.
 */
static uint8_t octave_to_visual_block(uint8_t oct)
{
    int8_t diff = (int8_t)oct - (int8_t)BASE_OCTAVE;
    return (uint8_t)(((int8_t)(diff % 2) + 2) % 2);
}

static void stop_growing(const NoteEvent *evt) {
    uint8_t evt_block = octave_to_visual_block(evt->octave);
    for (int i = MAX_ACTIVE_NOTES - 1; i >= 0; i--) {
        if (notes[i].active && notes[i].growing &&
            notes[i].note_name  == evt->note_name &&
            notes[i].accidental == evt->accidental &&
            octave_to_visual_block(notes[i].octave) == evt_block) {
            notes[i].growing = 0;
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Keyboard indicator row (row 31)                                      */
/* ------------------------------------------------------------------ */

static void draw_keyboard_row(void) {
    for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
        if (!notes[i].active || !notes[i].growing) continue;
        for (int dx = 0; dx < notes[i].col_width; dx++) {
            uint8_t x = notes[i].col_start + (uint8_t)dx;
            if (x < HUB75_WIDTH)
                fb[fb_back][HUB75_HEIGHT - 1][x] = rainbow_row[31];
        }
    }
}

/* ------------------------------------------------------------------ */
/* Event processing                                                     */
/* ------------------------------------------------------------------ */

static void process_events(void) {
    NoteEvent evt;
    while (queue_pop(&evt)) {
        if (evt.start_byte != NOTE_EVENT_START_BYTE) continue;
        if (evt.active)
            spawn_note(&evt);
        else
            stop_growing(&evt);
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void animation_init(void) {
    memset(notes, 0, sizeof(notes));
    eq_head = 0;
    eq_tail = 0;
}

void animation_tick(void) {
    process_events();
    hub75_clear_back();

    for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
        if (!notes[i].active) continue;

        if (notes[i].growing) {
            /* Grow the trail upward while the key is held: bottom edge
             * stays anchored at row 30, top edge advances toward row 0. */
            if (notes[i].length < HUB75_HEIGHT) {
                notes[i].length++;
                notes[i].head_y--;
            } else {
                /* Failsafe: note has filled the display column. Stop growing
                 * so it scrolls away; prevents a permanently lit column if
                 * the release packet was lost over UART. */
                notes[i].growing = 0;
            }
        } else {
            notes[i].head_y--;
        }

        int16_t top_edge    = notes[i].head_y;
        int16_t bottom_edge = notes[i].head_y + (int16_t)notes[i].length - 1;

        if (bottom_edge < 0) {
            notes[i].active = 0;
            continue;
        }

        for (int16_t r = top_edge; r <= bottom_edge; r++) {
            if (r < 0 || r > 30) continue;
            uint8_t colour = rainbow_row[(uint8_t)r];
            for (int dx = 0; dx < notes[i].col_width; dx++) {
                uint8_t x = notes[i].col_start + (uint8_t)dx;
                if (x < HUB75_WIDTH)
                    fb[fb_back][r][x] = colour;
            }
        }
    }

    draw_keyboard_row();
    hub75_swap_buffers();
}
