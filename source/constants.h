// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#pragma once
#include "font.h"

#define VIEW_SCALE   1
#define VIEW_WIDTH   (480 * VIEW_SCALE)
#define VIEW_HEIGHT  (320 * VIEW_SCALE)
#define PIECE_COUNT  7
#define ROW_COUNT    20
#define COL_COUNT    10
#define TILE_HEIGHT  15
#define TILE_WIDTH   15
#define BOARD_LEFT   300
#define BOARD_BOTTOM 10
#define BOARD_WIDTH  (TILE_WIDTH * COL_COUNT)
#define BOARD_HEIGHT (TILE_HEIGHT * ROW_COUNT)
#define TILE_START   (BOARD_BOTTOM + BOARD_HEIGHT - TILE_HEIGHT)
#define GUIDE_WIDTH  5
#define MAX_FPS      60.0f
#define INIT_SPEED   0.03f
#define SLAM_SPEED   2
#define DURATION     10
#define START_STATE  EIntro

#if VIEW_SCALE > 1
#define BACKDROP_WIDTH 960
#define BACKDROP_HEIGHT 640
#else
#define BACKDROP_WIDTH 480
#define BACKDROP_HEIGHT 320
#endif

#define GRADIENT_BOTTOM PIECE_COUNT + 0
#define GRADIENT_TOP    PIECE_COUNT + 1

extern const float colors[PIECE_COUNT + 2][4];
extern const float hi_colors[PIECE_COUNT][3];
extern const unsigned short patterns[PIECE_COUNT * 4][4];
extern const float tile_coords[16][4][2];

#if VIEW_SCALE > 1
#define TILE_STRINGS 20
#define TILE_SIZE 30
#define TILE_POT_SIZE 32
#define TILEBANK_WIDTH 256
#define TITLE_WIDTH 212
#define TITLE_HEIGHT 52
#else
#define TILE_STRINGS 6
#define TILE_SIZE 15
#define TILE_POT_SIZE 16
#define TILEBANK_WIDTH 128
#define TITLE_WIDTH 107
#define TITLE_HEIGHT 27
#endif

#define TILE_COUNT 5
#define TILEBANK_HEIGHT TILE_POT_SIZE
#define PHILIP_WIDTH 154
#define PHILIP_HEIGHT 19

extern const char *tile_images[TILE_COUNT][TILE_STRINGS];
extern const char *title_image[];
extern const char *philip_image[];

#define BASIL_COUNT 4
#define BASIL_INDEX(a) ((a / 2) >= BASIL_COUNT ? (BASIL_COUNT - 1) : (a / 2))

extern const char *basil1_image[];
extern const char *basil2_image[];
extern const char *basil3_image[];
extern const char *basil4_image[];

#define FONT_COUNT 2

typedef enum
{
    EVera,
    ENumerals,
} Font;

extern FontInfo fonts[FONT_COUNT];
