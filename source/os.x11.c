// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include "os.h"

static Display *g_display;
static Window g_window;
static int g_screen;
static GLXContext g_context;

static OS_Event *g_eventHead = 0;
static OS_Event **g_eventTail = &g_eventHead;

PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D = 0;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D = 0;

static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;

void osInit(const char *name, int width, int height, unsigned int flags, int *attribs)
{
    int attrib[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE, 0,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None,
    };

    XSetWindowAttributes attr;
    unsigned long mask;
    Window root;
    XVisualInfo *visinfo;

    atexit(osQuit);
    g_display = XOpenDisplay(NULL);
    g_screen = DefaultScreen(g_display);
    root = RootWindow(g_display, g_screen);
    visinfo = glXChooseVisual(g_display, g_screen, attrib);

    if (!visinfo)
    {
        printf("Error: couldn't create OpenGL window\n");
        exit(1);
    }

    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(g_display, root, visinfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    g_window = XCreateWindow(
        g_display,
        root,
        0, 0,
        width, height, 0,
        visinfo->depth,
        InputOutput,
        visinfo->visual,
        mask,
        &attr
    );

    XStoreName(g_display, g_window, name);

    g_context = glXCreateContext(g_display, visinfo, NULL, True);
    glXMakeCurrent(g_display, g_window, g_context);
    XMapWindow(g_display, g_window);

    glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC) glXGetProcAddress((const GLubyte*) "glXSwapIntervalSGI");

    if (strstr((const char *) glGetString(GL_EXTENSIONS), "GL_EXT_texture_compression_s3tc"))
    {
        glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) glXGetProcAddress((const GLubyte*) "glCompressedTexSubImage2D");
        glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) glXGetProcAddress((const GLubyte*) "glCompressedTexImage2D");
    }
}

void osQuit(void)
{
    glXDestroyContext(g_display, g_context);
    XDestroyWindow(g_display, g_window);

    while (g_eventHead)
    {
        OS_Event *pending = g_eventHead;
        g_eventHead = pending->next;
        if (!g_eventHead)
            g_eventTail = &g_eventHead;
        free(pending);
    }

}

int osGetScreenWidth()
{
    return DisplayWidth(g_display, g_screen);
}

int osGetScreenHeight()
{
    return DisplayHeight(g_display, g_screen);
}

// http://opengl.org/registry/specs/SGI/swap_control.txt
void osWaitVsync(int interval)
{
    if (glXSwapIntervalSGI)
        glXSwapIntervalSGI(interval);
}

unsigned int osGetMilliseconds()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

int osPollEvent(struct OS_EventRec *e)
{
    if (XPending(g_display))
    {
        OS_Event e;
        XEvent event;
        e.type = OS_NOEVENT;

        XNextEvent(g_display, &event);
        switch (event.type)
        {
          case Expose:
            //redraw(g_display, event.xany.window);
            break;

          case ConfigureNotify:
            //resize(event.xconfigure.width, event.xconfigure.height);
            break;

          case KeyRelease:
          case KeyPress:
          {
            XComposeStatus composeStatus;
            char asciiCode[32];
            KeySym keySym;
            int len;

            if (event.type == KeyPress)
            {     
                e.type = OS_KEYDOWN;
                e.key.state = OSKS_DOWN;
            }
            else
            {
                e.type = OS_KEYUP;
                e.key.state = OSKS_UP;
            }

            // Check for the ASCII/KeySym codes associated with the event:
            len = XLookupString(&event.xkey, asciiCode, sizeof(asciiCode), &keySym, &composeStatus);

            // ASCII Key
            if (len > 0)
            {
                e.key.key = (unsigned char) asciiCode[0];
            }
            else // use defines from /usr/include/X11/keysymdef.h
            {
                switch (keySym)
                {
                  case XK_Left:  e.key.key = OSK_LEFT;    break;
                  case XK_Right: e.key.key = OSK_RIGHT;   break;
                  case XK_Up:    e.key.key = OSK_UP;      break;
                  case XK_Down:  e.key.key = OSK_DOWN;    break;
                  case XK_Next:  e.key.key = OSK_NEXT;    break;
                  case XK_KP_Insert:
                  case XK_KP_0:  e.key.key = OSK_NUMPAD0; break;
                  case XK_KP_End:
                  case XK_KP_1:  e.key.key = OSK_NUMPAD1; break;
                  case XK_KP_Down:
                  case XK_KP_2:  e.key.key = OSK_NUMPAD2; break;
                  case XK_KP_Page_Down:
                  case XK_KP_3:  e.key.key = OSK_NUMPAD3; break;
                  case XK_KP_Left:
                  case XK_KP_4:  e.key.key = OSK_NUMPAD4; break;
                  case XK_KP_Begin:
                  case XK_KP_5:  e.key.key = OSK_NUMPAD5; break;
                  case XK_KP_Right:
                  case XK_KP_6:  e.key.key = OSK_NUMPAD6; break;
                  case XK_KP_Home:
                  case XK_KP_7:  e.key.key = OSK_NUMPAD7; break;
                  case XK_KP_Up:
                  case XK_KP_8:  e.key.key = OSK_NUMPAD8; break;
                  case XK_KP_Page_Up:
                  case XK_KP_9:  e.key.key = OSK_NUMPAD9; break;
                }
            }
          }
          break;
        }

        if (e.type != OS_NOEVENT)
        {
            e.next = 0;
            *g_eventTail = (OS_Event*) malloc(sizeof(OS_Event));
            memcpy(*g_eventTail, &e, sizeof(OS_Event));
            g_eventTail = &((*g_eventTail)->next);
        }
    }

    if (g_eventHead)
    {
        OS_Event *pending = g_eventHead;
        memcpy(e, pending, sizeof(OS_Event));
        g_eventHead = pending->next;
        if (!g_eventHead)
            g_eventTail = &g_eventHead;
        free(pending);
        return 1;
    }
}

void osSwapBuffers()
{
    glXSwapBuffers(g_display, g_window);
}

int osShowCursor(int)
{
    return 0;
}

void osGetWindowPos(int *x, int *y)
{
    if (x) *x = 0;
    if (y) *y = 0;
}

void osMoveWindow(int x, int y)
{
}
