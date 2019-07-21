// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#pragma once
#include "constants.h"

typedef struct GameRec Game;

typedef enum
{
    EAny,
    EAccelerate,
    ESlam,
    ELeft,
    ERight,
    ERotate,
    EYes,
    ENo,
    EQuit,
    EPause,
} Button;

typedef enum
{
    EIntro      = 1,
    EStartQuery = 2,
    EPlay       = 4,
    EPaused     = 8,
    ECompleting = 16,
    ESlamming   = 32,
    ESettle     = 64,
    ELocking    = 128,
    EEndQuery   = 256,
    EDone       = 512,
} GameState;

Game     *game_create();
void      game_destroy(Game *);
void      game_draw(const Game *);
void      game_reset(Game *);
void      game_update(Game *);
GameState game_state(const Game *);
void      game_press(Game *, Button);
void      game_release(Game *, Button);

typedef struct PieceRec
{
    int index;
    int rotation;
    int col;
    float row;
} Piece;

typedef unsigned char TileRow[COL_COUNT];
