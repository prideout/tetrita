// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#include "os.h"
#include "draw.h"
#include "image.h"
#include "GL/gl.h"
#include "GL/glext.h"

extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;

struct GraphicsRec
{
    GLuint backdrops[BASIL_COUNT];
    GLuint title;
    GLuint philip;
    GLuint tilemap;
    GLuint fonts[FONT_COUNT];
};

static void draw_tile(float col, float row, unsigned char type);
static void draw_pattern(const unsigned short *pattern, float row, float col);
static void draw_backboard(float mu, int level);
static GLuint create_texture(int linear);
static void blit(int x, int y, int w, int h, float scale);
static void fill(int x, int y, int w, int h);
static void outline(int x, int y, int w, int h);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Graphics *draw_create()
{
    Graphics *graphics = (Graphics *) malloc(sizeof(Graphics));
    unsigned char *philip_a;
    unsigned char *tile_la;
    unsigned char *font_la;
    unsigned char *backdrop_dxt = 0;
    unsigned char *backdrop_rgb = 0;
    int x = BOARD_LEFT * VIEW_SCALE;
    int y = BOARD_BOTTOM * VIEW_SCALE;
    int w = BOARD_WIDTH * VIEW_SCALE;
    int h = BOARD_HEIGHT * VIEW_SCALE;
    int i, size;

    glScissor(x, y, w, h);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glMatrixMode(GL_PROJECTION);
    glOrtho(0, 480, 0, 320, 0, 10);
    glMatrixMode(GL_MODELVIEW);

    // Load the backdrop textures.
    if (glCompressedTexImage2D)
    {
        size = BACKDROP_WIDTH * BACKDROP_HEIGHT / 2;
        backdrop_dxt = (unsigned char *) malloc(size + 1);
    }
    else
    {
        backdrop_rgb = (unsigned char *) malloc(BACKDROP_WIDTH * BACKDROP_HEIGHT * 3);
    }
    for (i = 0; i < BASIL_COUNT; i++)
    {
        const char **backdrop_image;
        switch (i)
        {
            case 0: backdrop_image = basil1_image; break;
            case 1: backdrop_image = basil2_image; break;
            case 2: backdrop_image = basil3_image; break;
            case 3: backdrop_image = basil4_image; break;
        }
        graphics->backdrops[i] = create_texture(0);
        if (glCompressedTexImage2D)
        {
            decode(backdrop_dxt, backdrop_image);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, npot(BACKDROP_WIDTH), npot(BACKDROP_HEIGHT), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BACKDROP_WIDTH, BACKDROP_HEIGHT, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, size, backdrop_dxt);
        }
        else
        {
            decode_dxt1(BACKDROP_WIDTH, BACKDROP_HEIGHT, backdrop_rgb, backdrop_image);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, npot(BACKDROP_WIDTH), npot(BACKDROP_HEIGHT), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BACKDROP_WIDTH, BACKDROP_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, backdrop_rgb);
        }
    }
    free(backdrop_dxt);
    free(backdrop_rgb);

    // Load the title texture.
    graphics->title = create_texture(0);
    if (VIEW_SCALE > 1)
    {
        if (glCompressedTexImage2D)
        {
            int size = TITLE_WIDTH * TITLE_HEIGHT;
            unsigned char *title_dxt = (unsigned char *) malloc(size + 1);
            decode(title_dxt, title_image);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, npot(TITLE_WIDTH), npot(TITLE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TITLE_WIDTH, TITLE_HEIGHT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, size, title_dxt);
            free(title_dxt);
        }
        else
        {
            unsigned char *title_rgba = (unsigned char *) malloc(TITLE_WIDTH * TITLE_HEIGHT * 4);
            decode_dxt5(TITLE_WIDTH, TITLE_HEIGHT, title_rgba, title_image);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, npot(TITLE_WIDTH), npot(TITLE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TITLE_WIDTH, TITLE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, title_rgba);
            free(title_rgba);
        }
    }
    else
    {
        unsigned char *title_rgba = (unsigned char *) malloc(4 * TITLE_WIDTH * TITLE_HEIGHT + 1);
        decode(title_rgba, title_image);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, npot(TITLE_WIDTH), npot(TITLE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TITLE_WIDTH, TITLE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, title_rgba);
        free(title_rgba);
    }

    // Load the author texture.
    graphics->philip = create_texture(0);
    philip_a = (unsigned char *) malloc(PHILIP_WIDTH * PHILIP_HEIGHT + 1);
    decode(philip_a, philip_image);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, npot(PHILIP_WIDTH), npot(PHILIP_HEIGHT), 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PHILIP_WIDTH, PHILIP_HEIGHT, GL_ALPHA, GL_UNSIGNED_BYTE, philip_a);
    free(philip_a);

    // Load the tile textures.
    graphics->tilemap = create_texture(0);
    tile_la = (unsigned char *) malloc(2 * TILE_SIZE * TILE_SIZE + 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, TILEBANK_WIDTH, TILEBANK_HEIGHT, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 0);
    for (y = 0, x = 0; y < TILE_COUNT; y++, x += TILE_POT_SIZE)
    {
        decode(tile_la, tile_images[y]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, TILE_SIZE, TILE_SIZE, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, tile_la);
    }
    free(tile_la);

    // Load the font atlases.
    for (x = 0; x < FONT_COUNT; x++)
    {
        graphics->fonts[x] = create_texture(0);
        font_la = (unsigned char *) malloc(fonts[x].width * fonts[x].height + 1);
        decode(font_la, fonts[x].image);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, fonts[x].width, fonts[x].height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, font_la);
        free(font_la);
    }

    return graphics;
}

void draw_destroy(Graphics *graphics)
{
    int x;
    for (x = 0; x < BASIL_COUNT; x++)
        glDeleteTextures(1, &graphics->backdrops[x]);
    glDeleteTextures(1, &graphics->title);
    glDeleteTextures(1, &graphics->philip);
    glDeleteTextures(1, &graphics->tilemap);
    for (x = 0; x < FONT_COUNT; x++)
        glDeleteTextures(1, &graphics->fonts[x]);
    free(graphics);
}

void draw_background(const Graphics *graphics, float frame, int level)
{
    float mu;
    float scale = 1.0f / VIEW_SCALE;
    int index = BASIL_INDEX(level);

    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1, 1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, graphics->backdrops[index]);
    blit(0, 0, VIEW_WIDTH, VIEW_HEIGHT, 1.0f / VIEW_SCALE);

    mu = clamp(frame / 0.25f);
    glColor4f(1, 1, 1, mu);
    glBindTexture(GL_TEXTURE_2D, graphics->title);
    blit(30, 250, TITLE_WIDTH, TITLE_HEIGHT, scale);

    mu = clamp((frame - 0.25f) / 0.25f);
    if (index == 3)
        glColor4f(1, 1, 1, mu);
    else
        glColor4f(0, 0, 0, mu);
    glBindTexture(GL_TEXTURE_2D, graphics->philip);
    blit(35, 220, PHILIP_WIDTH, PHILIP_HEIGHT, scale);

    mu = clamp((frame - 0.5f) / 0.25f);
    draw_backboard(mu, level);
}

void draw_begin_tiles(const Graphics *graphics)
{
    glEnable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, graphics->tilemap);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
}

void draw_end_tiles()
{
    glEnd();
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_SCISSOR_TEST);
}

void draw_blur(const Piece *piece, int frame)
{
    const unsigned short *pattern = patterns[piece->index * 4 + piece->rotation];
    float r = hi_colors[piece->index][0];
    float g = hi_colors[piece->index][1];
    float b = hi_colors[piece->index][2];
    float w = TILE_WIDTH;
    float h = TILE_HEIGHT;
    unsigned char occupied[4][4] = {0};
    float tops[4] = {-1, -1, -1, -1};
    int x, y;

    for (y = 0; y < 4; y++)
    {
        unsigned short row = *pattern++;
        for (x = 0; x < 4; x++, row <<= 4)
        {
            if (row & 0xf000)
                occupied[x][y] = 1;
        }
    }

    for (x = 0; x < 4; x++)
    {
        for (y = 3; y >= 0; y--)
        {
            if (occupied[x][y])
                tops[x] = y + 0.5f;
        }
    }

    glEnable(GL_SCISSOR_TEST);
    for (x = 0; x < 4; x++)
    {
        if (tops[x] != -1)
        {
            float xx = BOARD_LEFT + w * (piece->col + x);
            float yy = TILE_START - h * (piece->row + tops[x] - 1);

            glBegin(GL_QUAD_STRIP);
            glColor4f(r, g, b, 1);
            glVertex2f(xx, yy);
            glVertex2f(xx + w, yy);

            glColor4f(r, g, b, 0);
            glVertex2f(xx, yy + frame * h);
            glVertex2f(xx + w, yy + frame * h);
            glEnd();
        }
    }
    glDisable(GL_SCISSOR_TEST);
    glColor4f(1, 1, 1, 1);
}

void draw_guide(const Piece *piece, int level)
{
    const float w = TILE_WIDTH;
    const float h = TILE_HEIGHT;
    const float yy = BOARD_BOTTOM;
    const unsigned short *pattern = patterns[piece->index * 4 + piece->rotation];
    int x;
    int y;

    if (BASIL_INDEX(level) == 2)
        glColor4f(1, 1, 1, 1);
    else
        glColor4f(1, 1, 1, 0.7f);

    glBegin(GL_QUADS);
    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            unsigned short row = pattern[y];
            row <<= 4 * x;
            if (row & 0xf000)
            {
                float xx = BOARD_LEFT + w * (piece->col + x);
                glVertex2f(xx, yy - 0.75f);
                glVertex2f(xx + w, yy - 0.75f);
                glVertex2f(xx + w, yy - GUIDE_WIDTH);
                glVertex2f(xx, yy - GUIDE_WIDTH);
                break;
            }
        }
    }
    glEnd();
    glColor4f(1, 1, 1, 1);
}

void draw_lock(const Piece *piece, float mu)
{
    const float *color = hi_colors[piece->index];
    const unsigned short *pattern = patterns[piece->index * 4 + piece->rotation];
    glColor4f(color[0], color[1], color[2], 1 - mu);
    draw_pattern(pattern, piece->row, piece->col - 2 * mu);
    draw_pattern(pattern, piece->row, piece->col + 2 * mu);
}

void draw_board(const TileRow *board)
{
    int row, col;
    for (row = 0; row < ROW_COUNT; row++)
    {
        for (col = 0; col < COL_COUNT; col++)
        {
            unsigned char c = board[row][col];
            if (c)
            {
                glColor3fv(colors[c >> 4]);
                draw_tile((float) col, (float) row, c & 0xf);
            }
        }
    }
}

void draw_completions(const TileRow *board, const int *completion, int frame)
{
    int i;

    if ((frame >> 2) % 2)
        return;

    glColor4f(1, 1, 1, 1);
    for (i = 0; i < 4; i++)
    {
        int row = completion[i];
        if (row != -1)
        {
            int col;
            for (col = 0; col < COL_COUNT; col++)
                draw_tile((float) col, (float) row, board[row][col] & 0xf);
        }
    }
}

void draw_piece(const Piece *piece)
{
    const unsigned short *pattern = patterns[piece->index * 4 + piece->rotation];
    glColor3fv(hi_colors[piece->index]);
    draw_pattern(pattern, piece->row, (float) piece->col);
}

void draw_next(const Graphics *graphics, const Piece *next_pieces, float mu)
{
    int i, index;

    struct
    {
        int rotation;
        float row;
        int col;
        int ox;
        float oy;
    }
    pieces[PIECE_COUNT] = 
    {
        0, 15, -15, 1, 1,
        0, 12, -18, 1, 1.5,
        0, 11, -16, 2, 2.5,
        0, 12, -16, 2, 1,
        1, 12, -15, 1, 1.5,
        1, 15, -17, 2, 1.5,
        1, 16, -15, 2, 1.5,
    }, *piece;

    glBindTexture(GL_TEXTURE_2D, graphics->tilemap);
    glEnable(GL_TEXTURE_2D);

    // Index 0 is the outgoing piece;
    // Index 1 is the incoming piece.
    for (i = 0; i < 2; i++)
    {
        index = next_pieces[i].index;
        piece = pieces + index;
        glPushMatrix();
        glTranslatef
        (
            (float) BOARD_LEFT + piece->col * TILE_WIDTH,
            TILE_START - piece->row * TILE_HEIGHT,
            0
        );
        glRotatef(mu * 360, 0, 0, 1);
        glScalef(i ? mu : (1 - mu), i ? mu : (1 - mu), 1);
        glTranslatef
        (
            (float) -BOARD_LEFT - piece->col * TILE_WIDTH,
            -TILE_START + piece->row * TILE_HEIGHT,
            0
        );
        glBegin(GL_QUADS);
        glColor4f(hi_colors[index][0], hi_colors[index][1], hi_colors[index][2], 1);
        draw_pattern(patterns[index * 4 + piece->rotation],
            piece->row + piece->oy - 3,
            (float) piece->col - piece->ox
        );
        glEnd();
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
}

void draw_overlay()
{
    glColor4f(1, 1, 1, 0.25f);
    fill(0, 0, BACKDROP_WIDTH, BACKDROP_HEIGHT);
    glColor4f(1, 1, 1, 1);
}

void draw_text(const Graphics *graphics, Font font, const char *text, int start, int top, int level)
{
    int left = start;
    int c = 0;
    const char *pText = text;
    const FontInfo *info = fonts + font;

    glPushMatrix();
    glScalef(1.0f / VIEW_SCALE, 1.0f / VIEW_SCALE, 1.0f / VIEW_SCALE);
    glTranslatef((float) left * (VIEW_SCALE - 1), (float) top * (VIEW_SCALE - 1), 0);

    if (BASIL_INDEX(level) == 3)
        glColor4f(0.75f, 0.75f, 0.25f, 1);
    else
        glColor4f(0, 0.125f, 0.25f, 1);

    glBindTexture(GL_TEXTURE_2D, graphics->fonts[font]);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    while (pText && *pText)
    {
        const Glyph *glyph = info->glyphs;
        float s, t, w, h;

        if (*pText == '\n')
        {
            left = start;
            top -= info->line_height;
            pText++;
            c++;
            continue;
        }

        while (glyph->id)
        {
            if (glyph->id == *pText)
                break;
            glyph++;
        }

        if (!glyph->id)
        {
            pText++;
            continue;
        }

        s = (float) glyph->x / info->width;
        t = 1.0f - (float) glyph->y / info->height;
        w = (float) glyph->width / info->width;
        h = (float) glyph->height / info->height;

        if (c > 0)
        {
            unsigned short first = text[c - 1];
            unsigned short second = text[c];
            const Kerning *k;

            for (k = info->pairs; k && k->first; k++)
            {
                if (k->first == first && k->second == second)
                {
                    left += k->amount;
                    break;
                }
            }
        }

        left += glyph->xoffset;
        top -= glyph->yoffset;

        glTexCoord2f(s, t);
        glVertex2i(left, top);

        glTexCoord2f(s + w, t);
        glVertex2i(left + glyph->width, top);

        glTexCoord2f(s + w, t - h);
        glVertex2i(left + glyph->width, top - glyph->height);

        glTexCoord2f(s, t - h);
        glVertex2i(left, top - glyph->height);

        left -= glyph->xoffset;
        top += glyph->yoffset;
        left += glyph->xadvance;

        pText++;
        c++;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void draw_text_box(const Graphics *graphics, Font font, const char *text, int l, int b, int r, int t)
{
    const int wrap = 1;
    const int padding = 2;
    int c = 0;
    const char *pText = text;
    int left, top;
    const FontInfo *info = fonts + font;

    left = l + padding;
    top = t;

    glPushMatrix();
    glScalef(1.0f / VIEW_SCALE, 1.0f / VIEW_SCALE, 1.0f / VIEW_SCALE);
    glTranslatef((float) l * (VIEW_SCALE - 1), (float) t * (VIEW_SCALE - 1), 0);

    glColor4f(1, 1, 1, 0.75f);
    fill(l, b, r - l, t - b);

    glColor4f(0, 0, 0, 1);
    outline(l, b, r - l, t - b);

    glBindTexture(GL_TEXTURE_2D, graphics->fonts[font]);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    while (pText && *pText)
    {
        const Glyph *glyph = info->glyphs;
        float s, t, w, h;

        if (*pText == '\n')
        {
            left = l + padding;
            top -= info->line_height;
            pText++;
            c++;
            continue;
        }

        if (*pText == '\t')
        {
            left = left + 75;
            left -= left % 75;
            pText++;
            c++;
            continue;
        }

        while (glyph->id)
        {
            if (glyph->id == *pText)
                break;
            glyph++;
        }

        if (!glyph->id)
        {
            pText++;
            continue;
        }

        s = (float) glyph->x / info->width;
        t = 1.0f - (float) glyph->y / info->height;
        w = (float) glyph->width / info->width;
        h = (float) glyph->height / info->height;

        if (c > 0)
        {
            unsigned short first = text[c - 1];
            unsigned short second = text[c];
            const Kerning *k;

            for (k = info->pairs; k && k->first; k++)
            {
                if (k->first == first && k->second == second)
                {
                    left += k->amount;
                    break;
                }
            }
        }

        left += glyph->xoffset;
        top -= glyph->yoffset;

        // Dumb line wrapping.
        if (left + glyph->width >= r)
        {
            if (wrap)
            {
                left = l + padding;
                top -= info->line_height;
            }
            else
            {
                pText++;
                c++;
                continue;
            }
        }

        if (top - glyph->height < b)
            break;

        glTexCoord2f(s, t);
        glVertex2i(left, top);

        glTexCoord2f(s + w, t);
        glVertex2i(left + glyph->width, top);

        glTexCoord2f(s + w, t - h);
        glVertex2i(left + glyph->width, top - glyph->height);

        glTexCoord2f(s, t - h);
        glVertex2i(left, top - glyph->height);

        left -= glyph->xoffset;
        top += glyph->yoffset;
        left += glyph->xadvance;

        pText++;
        c++;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glColor4f(1, 1, 1, 1);
    glPopMatrix();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_tile(float col, float row, unsigned char type)
{
    float w = TILE_WIDTH;
    float h = TILE_HEIGHT;
    float x = BOARD_LEFT + w * col;
    float y = TILE_START - h * row;

    glTexCoord2fv(tile_coords[type][0]);
    glVertex2f(x, y);
    glTexCoord2fv(tile_coords[type][1]);
    glVertex2f(x + w, y);
    glTexCoord2fv(tile_coords[type][2]);
    glVertex2f(x + w, y + h);
    glTexCoord2fv(tile_coords[type][3]);
    glVertex2f(x, y + h);
}

static void draw_pattern(const unsigned short *pattern, float prow, float col)
{
    float x;
    float y;
    for (y = 0; y < 4; y++)
    {
        unsigned short row = *pattern++;
        for (x = 0; x < 4; x++, row <<= 4)
        {
            if (row & 0xf000)
                draw_tile(col + x, prow + y, row >> 12);
        }
    }
}

static void draw_backboard(float mu, int level)
{    
    int x = BOARD_LEFT;
    int y = BOARD_BOTTOM;
    int w = BOARD_WIDTH;
    int h = BOARD_HEIGHT;

    if (mu <= 0)
        return;

    if (mu < 1)
    {
        w = (int) (w * mu);
        h = (int) (h * mu);
    }

    glColor4f(0, 0, 0, 1);
    outline(x, y, w, h);

    glColor4f(0, 0, 0, 0.5f);
    glBegin(GL_QUADS);
    glColor4fv(colors[(GRADIENT_BOTTOM + level) % (GRADIENT_BOTTOM + 1)]);
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glColor4fv(colors[GRADIENT_TOP]);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
    glColor4f(1, 1, 1, 1);
}

static GLuint create_texture(int linear)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    if (linear)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    return id;
}

static void blit(int x, int y, int w, int h, float scale)
{
    float s = (float) w / npot(w);
    float t = (float) h / npot(h);
    float xx = (float) x;
    float yy = (float) y;
    float ww = w * scale;
    float hh = h * scale;
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(xx, yy);
    glTexCoord2f(s, 0); glVertex2f(xx + ww, yy);
    glTexCoord2f(s, t); glVertex2f(xx + ww, yy + hh);
    glTexCoord2f(0, t); glVertex2f(xx, yy + hh);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static void fill(int x, int y, int w, int h)
{
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
}

static void outline(int x, int y, int w, int h)
{
    const float oa = -1.0f / (VIEW_SCALE * 2);
    const float ob = +1.0f / (VIEW_SCALE * 2);

    // Shift from pixel boundaries to pixel centers using oa and ob.
    glBegin(GL_LINE_STRIP);
    glVertex2f(x + oa, y + oa);
    glVertex2f(x + w + ob, y + oa);
    glVertex2f(x + w + ob, y + h + ob);
    glVertex2f(x + oa, y + h + ob);
    glVertex2f(x + oa, y + oa);
    glEnd();
    glBegin(GL_POINTS);
    glVertex2f(x + oa, y + oa);
    glVertex2f(x + w + ob, y + oa);
    glVertex2f(x + w + ob, y + h + ob);
    glVertex2f(x + oa, y + h + ob);
    glEnd();
}
