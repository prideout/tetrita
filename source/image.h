// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#pragma once

float clamp(float val);
int npot(int val);
int align(int size);
int decode(unsigned char *dest, const char **src);
void decode_dxt1(int width, int height, unsigned char *dest, const char **src);
void decode_dxt5(int width, int height, unsigned char *dest, const char **src);
int encode(unsigned char *data, int size, FILE *fp);
