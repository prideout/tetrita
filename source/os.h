// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

void osInit(const char *name, int width, int height, unsigned int flags, int *attribs);
void osQuit();
int osGetScreenWidth();
int osGetScreenHeight();
void osWaitVsync(int);
unsigned int osGetMilliseconds();
int osPollEvent(struct OS_EventRec *e);
void osSwapBuffers();
int osShowCursor(int);
void osGetWindowPos(int *, int *);
void osMoveWindow(int, int);

#ifdef WIN32
#include <windows.h>
void fatalf(const char *format, ...);
void debugf(const char *format, ...);
#else
#define fatalf(...) { fprintf(stderr, __VA_ARGS__); exit(1); }
#define debugf(...) { fprintf(stdout, __VA_ARGS__); }
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#define OSK_ESCAPE  0x1B
#define OSK_LEFT    0x25
#define OSK_RIGHT   0x27
#define OSK_SHIFT   0xA0
#define OSK_UP      0x26
#define OSK_DOWN    0x28
#define OSK_CLEAR   0x0C
#define OSK_NEXT    0x22
#define OSK_NUMPAD0 0x60
#define OSK_NUMPAD1 0x61
#define OSK_NUMPAD2 0x62
#define OSK_NUMPAD3 0x63
#define OSK_NUMPAD4 0x64
#define OSK_NUMPAD5 0x65
#define OSK_NUMPAD6 0x66
#define OSK_NUMPAD7 0x67
#define OSK_NUMPAD8 0x68
#define OSK_NUMPAD9 0x69

#define OSKS_UP 0
#define OSKS_DOWN 1

#define OS_BUTTON_LEFT 1
#define OS_BUTTON_MIDDLE 2
#define OS_BUTTON_RIGHT 4

#define OS_QUERY -1
#define OS_IGNORE 0
#define OS_DISABLE 0
#define OS_ENABLE 1
#define OS_FULLSCREEN  0x80000000
#define OS_RESIZABLE   0x00000010
#define OS_FSAA        0x00000020
#define OS_OVERLAY     0x00000040
#define OS_DEPTH       0x00000080
#define OS_ALPHA       0x00000100

typedef enum
{
   OS_NOEVENT,
   OS_KEYDOWN,
   OS_KEYUP,
   OS_MOUSEMOTION,
   OS_MOUSEBUTTONDOWN,
   OS_MOUSEBUTTONUP,
   OS_QUIT,
   OS_RESIZE,
   OS_ACTIVATE,
   OS_DEACTIVATE,
   OS_PAINT,
} OS_EventType;

typedef struct OS_KeyboardEventRec
{
    unsigned char state;
    unsigned char key;
} OS_KeyboardEvent;

typedef struct OS_MouseEventRec
{
    unsigned char button;
    unsigned short x, y;
} OS_MouseEvent;

typedef struct OS_ResizeEventRec
{
    int width;
    int height;
} OS_ResizeEvent;

typedef struct OS_EventRec
{
    OS_EventType type;
    struct OS_EventRec *next;
    union
    {
        OS_KeyboardEvent key;
        OS_MouseEvent mouse;
        OS_ResizeEvent resize;
    };
} OS_Event;

#ifdef __cplusplus
}
#endif
