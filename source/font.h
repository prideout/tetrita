// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#pragma once

typedef struct
{
    unsigned short id;
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
    short xoffset;
    short yoffset;
    short xadvance;
} Glyph;

typedef struct
{
    unsigned short first;
    unsigned short second;
    short amount;
} Kerning;

typedef struct
{
    const char **image;
    const Glyph *glyphs;
    const Kerning *pairs;
    int width;
    int height;
    int base;
    int line_height;
    unsigned int texture;
} FontInfo;
