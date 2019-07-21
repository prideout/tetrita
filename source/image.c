// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#include "os.h"
#include "image.h"

float clamp(float val)
{
    if (val < 0) return 0;
    if (val > 1) return 1;
    return val;
}

int npot(int val)
{
   int pot = 1;
   while (pot < val)
      pot <<= 1;
   return pot;
}

int align(int size)
{
    return (size + 3) & ~3;
}

static void dump(FILE *fp, const char i6)
{
    if (!fp)
        return;
    if (i6 == '\\' - '0' || i6 == '?' - '0')
        fputc('\\', fp);
    fputc(i6 + '0', fp);
}

int encode(unsigned char *data, int size, FILE *fp)
{
    int byte;
    int shift = 2;
    int cols = 0;
    int rows = 0;
    unsigned char prev = 0;

    for (byte = 0; byte < size; byte++)
    {
        unsigned char i8 = *data++;
        unsigned char i6 = (i8 >> shift) | prev;

        if (cols == 0 && fp)
            fprintf(fp, "\"");

        dump(fp, i6);
        cols++;

        if (shift == 6)
        {
            i6 = i8 & 0x3f;
            dump(fp, i6);
            prev = 0;
            shift = 2;
            if (++cols > 126)
            {
                if (fp)
                    fprintf(fp, "\",\n");
                rows++;
                cols = 0;
            }
        }
        else
        {
            prev = (i8 << (6 - shift)) & 0x3f;
            shift += 2;
        }
    }

    if (shift != 2)
        dump(fp, prev);

    if (cols != 0)
    {
        if (fp)
            fprintf(fp, "\",\n");
        rows++;
    }

    return rows;
}

int decode(unsigned char *dest, const char **src)
{
    unsigned char prev = 0;
    int shift = 0;
    int written = 0;
    const char *bytes;

    while ((bytes = *src++) && *bytes)
    {
        while (bytes && *bytes)
        {
            unsigned char i6 = *bytes - '0';

            if (shift == 1)
                *dest++ = (prev << 2) | (i6 >> 4);
            else if (shift == 2)
                *dest++ = (prev << 4) | (i6 >> 2);
            else if (shift == 3)
                *dest++ = (prev << 6) | i6;

            if (shift)
                written++;

            shift = (shift + 1) % 4;
            prev = i6;
            bytes++;
        }
    }

    // note that this requires the caller to allocate an extra byte
    // in dest memory for cases where the size is not a multiple of 12
    if (shift == 1)
        *dest++ = (prev << 2);
    else if (shift == 2)
        *dest++ = (prev << 4);
    else if (shift == 3)
        *dest++ = (prev << 6);

    if (shift)
        written++;

    return written;
}

static void decode_dxt(unsigned char *dst, unsigned char code,
                       unsigned short rgb0, unsigned short rgb1,
                       int hasAlpha, int encoding)
{
    unsigned char red0 = (rgb0 >> 11) << 3;
    unsigned char grn0 = ((rgb0 >> 5) & 0x3f) << 2;
    unsigned char blu0 = (rgb0 & 0x1f) << 3;
    unsigned char red1 = (rgb1 >> 11) << 3;
    unsigned char grn1 = ((rgb1 >> 5) & 0x3f) << 2;
    unsigned char blu1 = (rgb1 & 0x1f) << 3;
    unsigned char red, grn, blu, alp;

    if (encoding)
    {
        switch (code)
        {
            case 0:
                red = red0;
                grn = grn0;
                blu = blu0;
                alp = 0xff;
                break;

            case 1:
                red = red1;
                grn = grn1;
                blu = blu1;
                alp = 0xff;
                break;

            case 2:
                red = (2 * red0 + red1) / 3;
                grn = (2 * grn0 + grn1) / 3;
                blu = (2 * blu0 + blu1) / 3;
                alp = 0xff;
                break;

            case 3:
                red = (2 * red1 + red0) / 3;
                grn = (2 * grn1 + grn0) / 3;
                blu = (2 * blu1 + blu0) / 3;
                alp = 0xff;
                break;
        }
    }
    else
    {
        switch (code)
        {
            case 0:
                red = red0;
                grn = grn0;
                blu = blu0;
                alp = 0xff;
                break;

            case 1:
                red = red1;
                grn = grn1;
                blu = blu1;
                alp = 0xff;
                break;

            case 2:
                red = (red0 + red1) / 2;
                grn = (grn0 + grn1) / 2;
                blu = (blu0 + blu1) / 2;
                alp = 0xff;
                break;

            case 3:
                red = 0;
                grn = 0;
                blu = 0;
                alp = 0;
                break;
        }
    }

    *(dst++) = red;
    *(dst++) = grn;
    *(dst++) = blu;
    if (hasAlpha)
        *(dst++) = alp;
}

static unsigned char decode_latc(unsigned char code, unsigned char lum0, unsigned char lum1, unsigned char minlum, unsigned char maxlum)
{
    if (lum0 > lum1)
    {
        switch (code)
        {
            case 0: return lum0;
            case 1: return lum1;
            case 2: return (6*lum0+  lum1)/7;
            case 3: return (5*lum0+2*lum1)/7;
            case 4: return (4*lum0+3*lum1)/7;
            case 5: return (3*lum0+4*lum1)/7;
            case 6: return (2*lum0+5*lum1)/7;
            case 7: return (  lum0+6*lum1)/7;
        }
    }
    else
    {
        switch (code)
        {
            case 0: return lum0;
            case 1: return lum1;
            case 2: return (4*lum0+  lum1)/5;
            case 3: return (3*lum0+2*lum1)/5;
            case 4: return (2*lum0+3*lum1)/5;
            case 5: return (  lum0+4*lum1)/5;
            case 6: return minlum;
            case 7: return maxlum;
        }
    }
    return 0;
}

void decode_dxt1(int width, int height, unsigned char *dest, const char **p6)
{
    int horzBlocks = (width + 3) >> 2;
    int vertBlocks = (height + 3) >> 2;
    unsigned char *src;
    unsigned char *pDest = dest;
    unsigned char *pBlock;
    int rowSize = width * 3;
    int size = rowSize * height;
    unsigned short rgb0, rgb1;
    unsigned int bits;
    unsigned char code;
    int linear0, linear1, linear2, linear3;
    int blockRow, blockCol, x, y;
    int srcSize;

    pBlock = src = (unsigned char *) malloc(1 + 8 * horzBlocks * vertBlocks);
    srcSize = decode(src, p6);
    assert(srcSize == 8 * horzBlocks * vertBlocks);

    for (blockRow = 0, linear0 = 0; blockRow < vertBlocks; ++blockRow, linear0 += rowSize << 2)
    {
        for (blockCol = 0, linear1 = linear0; blockCol < horzBlocks; ++blockCol, linear1 += 12)
        {
            rgb0 = *(pBlock++);
            rgb0 |= *(pBlock++) << 8;
            rgb1 = *(pBlock++);
            rgb1 |= *(pBlock++) << 8;
            bits = *((unsigned int *) pBlock);
            for (y = 0, linear2 = linear1; y < 4 && linear2 < size; ++y, linear2 += rowSize)
            {
                for (x = 0, linear3 = linear2; x < min(width, 4); ++x, linear3 += 3)
                {
                    code = bits & 0x3;
                    decode_dxt(pDest + linear3, code, rgb0, rgb1, 0, rgb0 > rgb1);
                    bits >>= 2;
                }
            }
            pBlock += 4;
        }
    }

    free(src);
}

void decode_dxt5(int width, int height, unsigned char *dest, const char **p6)
{
    unsigned char *src;
    unsigned char *pDest = dest;
    unsigned char *pBlock;
    int horzBlocks = (width + 3) >> 2;
    int vertBlocks = (height + 3) >> 2;
    int rowSize = width * 4;
    int size = rowSize * height;
    unsigned short rgb0, rgb1;
    unsigned int bits;
    unsigned char code, alp0, alp1, bit;
    int linear0, linear1, linear2, linear3;
    int blockRow, blockCol, x, y;
    int srcSize;

    pBlock = src = (unsigned char *) malloc(1 + 16 * horzBlocks * vertBlocks);
    srcSize = decode(src, p6);

    for (blockRow = 0, linear0 = 0; blockRow < vertBlocks; ++blockRow, linear0 += rowSize << 2)
    {
        for (blockCol = 0, linear1 = linear0; blockCol < horzBlocks; ++blockCol, linear1 += 16)
        {
            // Alpha block
            alp0 = *(pBlock++);
            alp1 = *(pBlock++);
            bit = 2;
            for (y = 0, linear2 = linear1; y < 4 && linear2 < size; ++y, linear2 += rowSize)
            {
                for (x = 0, linear3 = linear2 + 3; x < min(width, 4); ++x, linear3 += 4)
                {
                    code  = (pBlock[bit >> 3] >> (bit & 0x7)) & 1; bit--; code <<= 1;
                    code |= (pBlock[bit >> 3] >> (bit & 0x7)) & 1; bit--; code <<= 1;
                    code |= (pBlock[bit >> 3] >> (bit & 0x7)) & 1;
                    pDest[linear3] = decode_latc(code, alp0, alp1, 0, 255);
                    bit += 5;
                }
            }
            pBlock += 6;

            // RGB block
            rgb0 = *(pBlock++);
            rgb0 |= *(pBlock++) << 8;
            rgb1 = *(pBlock++);
            rgb1 |= *(pBlock++) << 8;
            bits = *((unsigned int *) pBlock);
            for (y = 0, linear2 = linear1; y < 4 && linear2 < size; ++y, linear2 += rowSize)
            {
                for (x = 0, linear3 = linear2; x < min(width, 4); ++x, linear3 += 4)
                {
                    code = bits & 0x3;
                    decode_dxt(pDest + linear3, code, rgb0, rgb1, 0, 1);
                    bits >>= 2;
                }
            }
            pBlock += 4;
        }
    }
}
