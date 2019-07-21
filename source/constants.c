// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#include "constants.h"
#include "images/numerals.c"
#include "images/philip.c"
#include "images/vera.c"

#if VIEW_SCALE > 1
#include "images/tile.big.c"
#include "images/title.dxt5.big.c"
#include "images/basil1.dxt1.big.c"
#include "images/basil2.dxt1.big.c"
#include "images/basil3.dxt1.big.c"
#include "images/basil4.dxt1.big.c"
#else
#include "images/tile.small.c"
#include "images/title.dxt5.small.c"
#include "images/basil1.dxt1.small.c"
#include "images/basil2.dxt1.small.c"
#include "images/basil3.dxt1.small.c"
#include "images/basil4.dxt1.small.c"
#endif

#undef RGB
#undef RGBA

#define RGB(r, g, b) r / 255.0f, g / 255.0f, b / 255.0f
#define RGBA(r, g, b, a) r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f

#define GRADIENT_BOTTOM PIECE_COUNT + 0
#define GRADIENT_TOP    PIECE_COUNT + 1

const float colors[PIECE_COUNT + 2][4] =
{
    RGBA(140, 90,  180, 255),
    RGBA(0,   115, 211, 255),
    RGBA(210, 165, 34,  255),
    RGBA(148, 173, 222, 255),
    RGBA(211, 115, 50,  255),
    RGBA(157, 27,  48,  255),
    RGBA(137, 158, 131, 255),
    RGBA(30,  64,  119, 255),
    RGBA(49,  106, 197, 229),
};

const float hi_colors[PIECE_COUNT][3] =
{
    RGB(160, 100, 200),
    RGB(50,  115, 255),
    RGB(220, 170, 40 ),
    RGB(185, 216, 278),
    RGB(220, 120, 50 ),
    RGB(196, 34,  60 ),
    RGB(171, 198, 164),
};

#undef RGB
#undef RGBA

const unsigned short patterns[PIECE_COUNT * 4][4] =
{
    { 0x0000, 0x0000, 0xb9c0, 0x0e00 }, { 0x0000, 0x0d00, 0xb800, 0x0e00 }, { 0x0000, 0x0000, 0x0d00, 0xbac0 }, { 0x0000, 0x0d00, 0x07c0, 0x0e00 },
    { 0x0000, 0x03c0, 0x0200, 0x0e00 }, { 0x0000, 0x0000, 0xb140, 0x00e0 }, { 0x0000, 0x0d00, 0x0200, 0xb600 }, { 0x0000, 0x0000, 0xd000, 0x51c0 },
    { 0x0000, 0xb11c, 0x0000, 0x0000 }, { 0x00d0, 0x0020, 0x0020, 0x00e0 }, { 0x0000, 0xb11c, 0x0000, 0x0000 }, { 0x00d0, 0x0020, 0x0020, 0x00e0 },
    { 0x0000, 0x0000, 0x0340, 0x0560 }, { 0x0000, 0x0000, 0x0340, 0x0560 }, { 0x0000, 0x0000, 0x0340, 0x0560 }, { 0x0000, 0x0000, 0x0340, 0x0560 },
    { 0x0000, 0x0000, 0x31c0, 0xe000 }, { 0x0000, 0xb400, 0x0200, 0x0e00 }, { 0x0000, 0x0000, 0x00d0, 0xb160 }, { 0x0000, 0x0d00, 0x0200, 0x05c0 },
    { 0x0000, 0x0000, 0x03c0, 0xb600 }, { 0x0000, 0x0d00, 0x0540, 0x00e0 }, { 0x0000, 0x0000, 0x03c0, 0xb600 }, { 0x0000, 0x0d00, 0x0540, 0x00e0 },
    { 0x0000, 0x0000, 0xb400, 0x05c0 }, { 0x0000, 0x00d0, 0x0360, 0x0e00 }, { 0x0000, 0x0000, 0xb400, 0x05c0 }, { 0x0000, 0x00d0, 0x0360, 0x0e00 },
};

#define TL(s, t) ((float) s * TILE_POT_SIZE / TILEBANK_WIDTH), ((float) t * TILE_SIZE / TILEBANK_HEIGHT)
#define TR(s, t) ((float) s * TILE_POT_SIZE / TILEBANK_WIDTH + (float) TILE_SIZE / TILEBANK_WIDTH), ((float) t * TILE_SIZE / TILEBANK_HEIGHT)

const float tile_coords[16][4][2] =
{
    TL(0, 0), TR(0, 0), TR(0, 0), TL(0, 0), // 0
    TL(0, 0), TR(0, 0), TR(0, 1), TL(0, 1), // 1
    TR(0, 0), TR(0, 1), TL(0, 1), TL(0, 0), // 2
    TL(1, 0), TR(1, 0), TR(1, 1), TL(1, 1), // 3
    TR(1, 0), TR(1, 1), TL(1, 1), TL(1, 0), // 4
    TL(1, 1), TL(1, 0), TR(1, 0), TR(1, 1), // 5
    TR(1, 1), TL(1, 1), TL(1, 0), TR(1, 0), // 6
    TR(2, 0), TR(2, 1), TL(2, 1), TL(2, 0), // 7
    TL(2, 1), TL(2, 0), TR(2, 0), TR(2, 1), // 8
    TR(2, 1), TL(2, 1), TL(2, 0), TR(2, 0), // 9
    TL(2, 0), TR(2, 0), TR(2, 1), TL(2, 1), // a
    TL(3, 0), TR(3, 0), TR(3, 1), TL(3, 1), // b
    TR(3, 1), TL(3, 1), TL(3, 0), TR(3, 0), // c
    TR(3, 0), TR(3, 1), TL(3, 1), TL(3, 0), // d
    TL(3, 1), TL(3, 0), TR(3, 0), TR(3, 1), // e
    TL(4, 0), TR(4, 0), TR(4, 1), TL(4, 1), // f
};

#undef TL
#undef TR

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const char *vera_image[];
extern const Glyph vera_glyphs[];
extern const Kerning vera_pairs[];

#define VERA_WIDTH 128
#define VERA_HEIGHT 128
#define VERA_BASE 10
#define VERA_LINE_HEIGHT 12

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const char *numerals_image[];
extern const Glyph numerals_glyphs[];
extern const Kerning numerals_pairs[];

#define NUMERALS_WIDTH 128
#define NUMERALS_HEIGHT 128
#define NUMERALS_BASE 25
#define NUMERALS_LINE_HEIGHT 32

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FontInfo fonts[FONT_COUNT] =
{
    {
        vera_image,
        vera_glyphs,
        vera_pairs,
        VERA_WIDTH,
        VERA_HEIGHT,
        VERA_BASE,
        VERA_LINE_HEIGHT,
    },
    {
        numerals_image,
        numerals_glyphs,
        0,
        NUMERALS_WIDTH,
        NUMERALS_HEIGHT,
        NUMERALS_BASE,
        NUMERALS_LINE_HEIGHT,
    }
};
