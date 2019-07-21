// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#pragma once
#include "game.h"

typedef struct GraphicsRec Graphics;

Graphics *draw_create();
void      draw_destroy(Graphics *);
void      draw_background(const Graphics *, float mu, int level);
void      draw_begin_tiles(const Graphics *);
void      draw_end_tiles();
void      draw_blur(const Piece *, int frame);
void      draw_guide(const Piece *, int level);
void      draw_lock(const Piece *, float mu);
void      draw_board(const TileRow *board);
void      draw_completions(const TileRow *board, const int *completion, int frame);
void      draw_piece(const Piece *);
void      draw_next(const Graphics *, const Piece *next_pieces, float mu);
void      draw_overlay();
void      draw_text(const Graphics *, Font, const char *text, int left, int top, int level);
void      draw_text_box(const Graphics *, Font, const char *text, int left, int bottom, int right, int top);
