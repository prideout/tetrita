// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#include "os.h"
#include "game.h"
#include "draw.h"

struct GameRec
{
    Piece current_piece;
    Piece next_pieces[2];
    unsigned int frame;
    float speed;
    GameState state;
    GameState saved_state;
    TileRow board[ROW_COUNT];
    int completion[4];
    int holdthru;
    int score;
    int points;
    int level;
    int moving;
    int accelerating;
    Graphics *graphics;
};

static void move_piece(Game *game, int dc, int dr);
static int collide(const Piece *piece, const TileRow *board);
static void lock_piece(Piece *piece, TileRow *board);
static void settle(Game *game, int slam);
static void create_piece(Piece *piece);
static int check_completions(Game *game);
static void process_downward_tiles(TileRow *board, int row);
static void process_upward_tiles(TileRow *board, int row);
static void nuke_lines(Game *game);
static void pop_piece(Game *game);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Game *game_create()
{
    Game *game = (Game *) malloc(sizeof(Game));
    game->graphics = draw_create();
    game->saved_state = game->state = START_STATE;
    game->moving = 0;
    game->accelerating = 0;
    game_reset(game);
    return game;
}

void game_destroy(Game *game)
{
    draw_destroy(game->graphics);
    free(game);
}

void game_draw(const Game *game)
{
    GameState state = game->state;
    float mu = (state == EIntro) ? (game->frame / 50.0f) : 1.0f;

    draw_background(game->graphics, mu, game->level);

    if (!(state & (EIntro | EPaused | EStartQuery)))
    {
        if (state == ESlamming)
            draw_blur(&game->current_piece, game->frame);

        draw_begin_tiles(game->graphics);
        if (state == ELocking)
            draw_lock(&game->current_piece, (float) game->frame / DURATION);
        draw_board(game->board);
        if (state != EEndQuery)
            draw_piece(&game->current_piece);
        if (state == ECompleting)
            draw_completions(game->board, game->completion, game->frame);
        draw_end_tiles();

        if (state & (EPlay | ESlamming | ESettle | ELocking | ECompleting))
        {
            float mu = (state & (ELocking | ECompleting)) ? (float) game->frame / DURATION : 0;
            draw_guide(&game->current_piece, game->level);
            draw_next(game->graphics, game->next_pieces, mu);
        }
    }

    if (state == EPaused)
        draw_overlay();

    if (state & (EStartQuery | EIntro))
    {
        draw_text_box(game->graphics, EVera,
            "Welcome to Tetrita 1.0\n"
            "\n"
            "Quit:\tEsc or X or Q\n"
            "Rotate:\tUp Arrow or 8 or 5\n"
            "Left:\tLeft Arrow or 4\n"
            "Right:\tRight Arrow or 6\n"
            "Faster:\tDown Arrow or 2\n"
            "Slam:\tPgDn or Spacebar\n"
            "\n"
            "Press any key to start.",
            32, 75, 175, 200);
    }
    else if (state == EEndQuery)
    {
        draw_text_box(game->graphics, EVera, "Do you want to play again?", 32, 10, 275, 100);
    }

    if (!(state & (EIntro | EPaused | EStartQuery)))
    {
        char score_text[64];
        sprintf(score_text, "score: %d\nlevel: %d", game->score, game->level);
        draw_text(game->graphics, ENumerals, score_text, 32, 215, game->level);
    }
}

void game_reset(Game *game)
{
    create_piece(&game->current_piece);
    create_piece(game->next_pieces);
    create_piece(game->next_pieces + 1);
    game->frame = 0;
    game->speed = INIT_SPEED;
    game->holdthru = 0;
    game->score = 0;
    game->level = 0;
    memset(game->board, 0, sizeof(game->board));
}

void game_update(Game *game)
{
    game->frame++;

    if (game->state == EIntro)
    {
        if (game->frame > 50)
            game->state = EStartQuery;
        return;
    }

    if (game->state == EPlay)
    {
        float speed, previous;
        speed = game->accelerating ? 0.5f : game->speed;
        previous = game->current_piece.row;
        game->current_piece.row += speed;
        if (collide(&game->current_piece, game->board))
            settle(game, 0);
        return;
    }

    if (game->state == ESlamming)
    {
        int i;
        for (i = 0; i < SLAM_SPEED; i++)
        {
            float previous = game->current_piece.row;
            game->current_piece.row++;
            if (collide(&game->current_piece, game->board))
            {
                settle(game, 1);
                game_update(game);
                return;
            }
        }
        return;
    }

    if (game->state == ESettle)
    {
        // If the user slides the piece off a cliff, then return to play mode.
        float prevRow = game->current_piece.row;
        game->current_piece.row += game->speed;
        if (!collide(&game->current_piece, game->board))
        {
            game->state = EPlay;
            game->frame = 30;
            return;
        }
        game->current_piece.row = prevRow;

        if (game->frame > ceilf(1.0 / game->speed) || game->accelerating)
        {
            int completions;
            lock_piece(&game->current_piece, game->board);
            completions = check_completions(game);
            game->score += game->points;
            game->level = game->score / 100;
            game->speed = 0.01f * (game->level + 3);
            if (completions)
                return;
            game->state = ELocking;
            game->frame = 0;
        }
        return;
    }

    if (game->state == ECompleting)
    {
        if (game->frame > DURATION)
        {
            nuke_lines(game);
            pop_piece(game);
        }
        return;
    }

    if (game->state == ELocking)
    {
        if (game->frame > DURATION)
            pop_piece(game);
        return;
    }
}

GameState game_state(const Game *game)
{
    return game->state;
}

void game_press(Game *game, Button button)
{
    switch (button)
    {
        case EAny:
            break;
        case ESlam:
            if (game->state == EPlay)
            {
                game->state = ESlamming;
                game->frame = 0;
            }
            break;
        case ELeft:
            game->moving = 1;
            if (!game->holdthru)
                move_piece(game, -1, 0);
            break;
        case ERight:
            game->moving = 1;
            if (!game->holdthru)
                move_piece(game, 1, 0);
            break;
        case ERotate:
            game->moving = 1;
            if (!game->holdthru)
                move_piece(game, 0, 3);
            break;
        case EAccelerate:
            if (!game->holdthru)
                game->accelerating = 1;
            break;
        case EYes:
        case ENo:
        case EQuit:
            game->state = EDone;
            break;
        case EPause:
            game->state = game->saved_state;
            break;
    }
}

void game_release(Game *game, Button button)
{
    switch (button)
    {
        case EAny:
            if (game->state == EStartQuery)
            {
                game->state = EPlay;
                game->frame = 0;
            }
            break;
        case ESlam:
            break;
        case ELeft:
        case ERight:
        case ERotate:
            game->holdthru = 0;
            game->moving = 0;
            break;
        case EAccelerate:
            game->holdthru = 0;
            game->accelerating = 0;
            break;
        case EYes:
            if (game->state == EEndQuery)
            {
                game_reset(game);
                game->state = EStartQuery;
            }
            break;
        case ENo:
            break;
        case EQuit:
            break;
        case EPause:
            game->saved_state = game->state;
            game->state = EPaused;
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void move_piece(Game *game, int dc, int dr)
{
    int previousCol = game->current_piece.col;
    int previousRot = game->current_piece.rotation;

    if (game->state != EPlay && game->state != ESettle)
        return;

    game->current_piece.col += dc;
    game->current_piece.rotation += dr;
    game->current_piece.rotation %= 4;

    if (collide(&game->current_piece, game->board))
    {
        game->current_piece.col = previousCol;
        game->current_piece.rotation = previousRot;
    }
}

static int collide(const Piece *piece, const TileRow *board)
{
    const unsigned short *pattern = patterns[piece->index * 4 + piece->rotation];
    int x;
    float y;
    for (y = 0; y < 4; y++)
    {
        int irow = (int) floorf(piece->row + y);
        unsigned short row = *pattern++;
        for (x = 0; x < 4; x++, row <<= 4)
        {
            if (row & 0xf000)
            {
                if (piece->col + x < 0 || piece->col + x > COL_COUNT - 1)
                    return 1;

                if (piece->row + y > ROW_COUNT - 1)
                    return 1;

                if (irow >= 0 && board[irow][piece->col + x])
                    return 1;

                if (floorf(piece->row) != piece->row)
                {
                    if (1 + irow >= 0 && board[1 + irow][piece->col + x])
                        return 1;
                }
            }
        }
    }

    return 0;
}

static void lock_piece(Piece *piece, TileRow *board)
{
    const unsigned short *pattern = patterns[piece->index * 4 + piece->rotation];
    int x;
    float y;

    for (y = 0; y < 4; y++)
    {
        unsigned short row = *pattern++;
        for (x = 0; x < 4; x++, row <<= 4)
        {
            if (row & 0xf000)
            {
                int r = (int) floorf(piece->row + y);
                int c = piece->col + x;
                if (r < 0 || r >= ROW_COUNT || c < 0 || c >= COL_COUNT)
                    continue;
                board[r][c] = row >> 12;
                board[r][c] |= piece->index << 4;
            }
        }
    }
}

static void settle(Game *game, int slam)
{
    Piece *piece = &game->current_piece;

    // Scoot it up until it doesn't collide with anything.
    piece->row = floorf(piece->row);
    while (collide(piece, game->board))
        piece->row--;

    game->points = (game->frame < 10) ? (10 - game->frame) : 0;
    game->frame = slam ? 1000 : 0;
    game->state = ESettle;

    if (slam)
        game->points += 2;
}

static void create_piece(Piece *piece)
{
    piece->index = rand() % PIECE_COUNT;
    piece->rotation = 0;
    piece->col = 3;
    piece->row = (piece->index == 2) ? -1.0f : -3.0f;
}

static int check_completions(Game *game)
{
    int row, col;
    int count = 0;
    memset(game->completion, 0xff, sizeof(game->completion));
    for (row = 0; row < ROW_COUNT; row++)
    {
        int complete = 1;
        for (col = 0; col < COL_COUNT && complete; col++)
            complete = game->board[row][col];
        if (complete)
            game->completion[count++] = row;
    }
    if (count)
    {
        game->frame = 0;
        game->state = ECompleting;
        switch (count)
        {
            case 1: game->points += 10; break;
            case 2: game->points += 25; break;
            case 3: game->points += 75; break;
            case 4: game->points += 300; break;
        }
    }
    return count;
}

static void process_downward_tiles(TileRow *board, int row)
{
    int col;
    for (col = 0; col < COL_COUNT; col++)
    {
        unsigned char type = board[row][col] & 0xf;
        switch (type)
        {
            case 0x2: type = 0xe; break;
            case 0x3: type = 0xb; break;
            case 0x4: type = 0xc; break;
            case 0x7: type = 0x5; break;
            case 0x8: type = 0x6; break;
            case 0x9: type = 0x1; break;
            case 0xd: type = 0xf; break;
        }
        board[row][col] = (board[row][col] & 0xf0) | type;
    }
}

static void process_upward_tiles(TileRow *board, int row)
{
    int col;
    for (col = 0; col < COL_COUNT; col++)
    {
        unsigned char type = board[row][col] & 0xf;
        switch (type)
        {
            case 0x2: type = 0xd; break;
            case 0x5: type = 0xb; break;
            case 0x6: type = 0xc; break;
            case 0x7: type = 0x3; break;
            case 0x8: type = 0x4; break;
            case 0xa: type = 0x1; break;
            case 0xe: type = 0xf; break;
        }
        board[row][col] = (board[row][col] & 0xf0) | type;
    }
}

static void nuke_lines(Game *game)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        int completion = game->completion[i];
        if (completion != -1)
        {
            int col, row;
            for (row = completion; row > 0; row--)
            {
                for (col = 0; col < COL_COUNT; col++)
                    game->board[row][col] = game->board[row - 1][col];
            }
            for (col = 0; col < COL_COUNT; col++)
                game->board[0][col] = 0;
            process_downward_tiles(game->board, completion);
            if (completion + 1 < ROW_COUNT)
                process_upward_tiles(game->board, completion + 1);
        }
    }
}

static void pop_piece(Game *game)
{
    game->current_piece = game->next_pieces[0];
    game->next_pieces[0] = game->next_pieces[1];
    if (collide(&game->current_piece, game->board))
    {
        game->state = EEndQuery;
        return;
    }
    create_piece(game->next_pieces + 1);
    game->state = EPlay;
    game->frame = 0;
    if (game->moving || game->accelerating)
    {
        game->accelerating = 0;
        game->holdthru = 1;
    }
}
