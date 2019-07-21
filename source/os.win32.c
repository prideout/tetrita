// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#include "os.h"
#include "GL/gl.h"
#include "GL/glext.h"
#include "GL/wglext.h"

PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D = 0;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D = 0;

static PFNWGLSWAPINTERVALEXTPROC wglSwapInterval = 0;

static HINSTANCE g_hInstance;
static HDC g_hDC;
static OS_Event *g_eventHead = 0;
static OS_Event **g_eventTail = &g_eventHead;
static HWND g_hWnd;
static int g_cursorVisible = 1;

int main(int argc, char **argv);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    char** argv = (char**) CommandLineToArgvW((LPCWSTR) lpCmdLine, &argc);
    g_hInstance = hInstance;
    return main(argc, argv);
}

void osWaitVsync(int interval)
{
    if (wglSwapInterval)
        wglSwapInterval(interval);
}

int osGetScreenWidth() { return GetSystemMetrics(SM_CXSCREEN); }
int osGetScreenHeight() { return GetSystemMetrics(SM_CYSCREEN); }

#pragma warning(push)
#pragma warning(disable:4996)
void fatalf(const char *format, ...)
{
    va_list args;
    int length;
    char *buffer;

    va_start(args, format);
    length = _vscprintf(format, args);
    buffer = (char *) malloc(length + 1);
    vsprintf(buffer, format, args);
    OutputDebugString("\n\n");
    OutputDebugString(buffer);
    OutputDebugString("\n\n");
    free(buffer);
    exit(1);
}
void debugf(const char *format, ...)
{
    va_list args;
    int length;
    char *buffer;

    va_start(args, format);
    length = _vscprintf(format, args);
    buffer = (char *) malloc(length + 1);
    vsprintf(buffer, format, args);
    OutputDebugString(buffer);
    free(buffer);
}
#pragma warning(pop)

void osQuit(void)
{
    timeEndPeriod(1);

    if (g_hWnd)
    {
        DestroyWindow(g_hWnd);
        g_hWnd = 0;
    }

    while (g_eventHead)
    {
        OS_Event *pending = g_eventHead;
        g_eventHead = pending->next;
        if (!g_eventHead)
            g_eventTail = &g_eventHead;
        free(pending);
    }
}

static LRESULT CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    OS_Event e;
    e.type = 0;

    switch (uMsg)
    {
        case WM_KEYDOWN:
            e.type = OS_KEYDOWN;
            e.key.key = (unsigned char) wParam;
            e.key.state = OSKS_DOWN;
            break;

        case WM_KEYUP:
            e.type = OS_KEYUP;
            e.key.key = (unsigned char) wParam;
            e.key.state = OSKS_UP;
            break;

        case WM_SIZE:
            e.type = OS_RESIZE;
            e.resize.width = LOWORD(lParam);
            e.resize.height = HIWORD(lParam);
            break;

        case WM_ACTIVATE:
            e.type = (wParam == WA_INACTIVE) ? OS_DEACTIVATE : OS_ACTIVATE;
            break;

        case WM_PAINT:
            e.type = OS_PAINT;
            break;

        case WM_ERASEBKGND:
            break;

        case WM_MOUSEMOVE:
            e.type = OS_MOUSEMOTION;
            e.mouse.button = 0;
            if (wParam & MK_LBUTTON) e.mouse.button |= OS_BUTTON_LEFT;
            if (wParam & MK_MBUTTON) e.mouse.button |= OS_BUTTON_MIDDLE;
            if (wParam & MK_RBUTTON) e.mouse.button |= OS_BUTTON_RIGHT;
            e.mouse.x = LOWORD(lParam);
            e.mouse.y = HIWORD(lParam);
            break;

        case WM_LBUTTONDOWN:
            e.type = OS_MOUSEBUTTONDOWN;
            e.mouse.button = 0;
            if (wParam & MK_LBUTTON) e.mouse.button |= OS_BUTTON_LEFT;
            if (wParam & MK_MBUTTON) e.mouse.button |= OS_BUTTON_MIDDLE;
            if (wParam & MK_RBUTTON) e.mouse.button |= OS_BUTTON_RIGHT;
            e.mouse.x = LOWORD(lParam);
            e.mouse.y = HIWORD(lParam);
            break;

        case WM_LBUTTONUP:
            e.type = OS_MOUSEBUTTONUP;
            e.mouse.button = OS_BUTTON_LEFT;
            e.mouse.x = LOWORD(lParam);
            e.mouse.y = HIWORD(lParam);
            break;

        case WM_CLOSE:
        case WM_QUIT:
            e.type = OS_QUIT;
            break;
    }

    if (e.type)
    {
        e.next = 0;
        *g_eventTail = (OS_Event *) malloc(sizeof(OS_Event));
        memcpy(*g_eventTail, &e, sizeof(OS_Event));
        g_eventTail = &((*g_eventTail)->next);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void osInit(const char *name, int width, int height, unsigned int flags, int *attribs)
{
    PIXELFORMATDESCRIPTOR pfd;
    int pixelFormat;
    HGLRC hRC;
    RECT rect;
    DWORD dwStyle, dwExStyle;
    int x, y;

    WNDCLASS wndClass =
    {
        CS_OWNDC, WinProc, 0, 0,
        g_hInstance, LoadIcon(g_hInstance, "IDI_APP"), LoadCursor(0, IDC_ARROW), 0, 0, name
    };

    atexit(osQuit);
    RegisterClass(&wndClass);

    if (flags & OS_FULLSCREEN)
    {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
        dwStyle = WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE | WS_EX_TOPMOST;
        x = y = 0;
    }
    else if (flags & OS_OVERLAY)
    {
        dwStyle = WS_SYSMENU | WS_VISIBLE | WS_POPUP | WS_BORDER | WS_MINIMIZEBOX;
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        x = GetSystemMetrics(SM_CXSCREEN) / 2 - width / 2;
        y = GetSystemMetrics(SM_CYSCREEN) / 2 - height / 2;
    }
    else
    {
        dwStyle = WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX;
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        x = y = CW_USEDEFAULT;
        if (flags & OS_RESIZABLE)
            dwStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;
    }

    // Create the window.
    SetRect(&rect, 0, 0, width, height);
    AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    g_hWnd = CreateWindowEx(0, name, name, dwStyle, x, y, width, height, 0, 0, g_hInstance, 0);

    // Create the GL context.
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = (flags & OS_ALPHA) ? 32 : 24;
    pfd.cAlphaBits = (flags & OS_ALPHA) ? 8  : 0;
    pfd.cDepthBits = (flags & OS_DEPTH) ? 16 : 0;
    pfd.cStencilBits = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    g_hDC = GetDC(g_hWnd);
    pixelFormat = ChoosePixelFormat(g_hDC, &pfd);

    SetPixelFormat(g_hDC, pixelFormat, &pfd);
    hRC = wglCreateContext(g_hDC);
    wglMakeCurrent(g_hDC, hRC);

    if (flags & OS_FSAA)
    {
        PROC proc = wglGetProcAddress("wglChoosePixelFormatARB");
        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) proc;

        unsigned int numFormats;
        int iAttributes[] =
        {
            WGL_SAMPLES_ARB, 16,
            WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_RED_BITS_ARB, 8,
            WGL_GREEN_BITS_ARB, 8,
            WGL_BLUE_BITS_ARB, 8,
            WGL_ALPHA_BITS_ARB, (flags & OS_ALPHA) ? 8  : 0,
            WGL_DEPTH_BITS_ARB, (flags & OS_DEPTH) ? 16 : 0,
            WGL_STENCIL_BITS_ARB, 0,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            0, 0
        };

        int *sampleCount = iAttributes + 1;
        int *useSampleBuffer = iAttributes + 3;
        int pixelFormat = -1;

        if (!wglChoosePixelFormatARB)
            fatalf("Could not load function pointer for 'wglChoosePixelFormatARB'.  Is your driver properly installed?");

        // Try fewer and fewer samples per pixel till we find one that is supported.
        while (pixelFormat <= 0 && *sampleCount >= 0)
        {
            wglChoosePixelFormatARB(g_hDC, iAttributes, 0, 1, &pixelFormat, &numFormats);
            (*sampleCount)--;
            if (*sampleCount <= 1)
                *useSampleBuffer = GL_FALSE;
        }

        // Win32 allows the pixel format to be set only once per window, so destroy and re-create the window.
        if (pixelFormat != -1)
        {
            DestroyWindow(g_hWnd);
            g_hWnd = CreateWindowEx(0, name, name, dwStyle, x, y, width, height, 0, 0, g_hInstance, 0);
            g_hDC = GetDC(g_hWnd);
            SetPixelFormat(g_hDC, pixelFormat, &pfd);
            hRC = wglCreateContext(g_hDC);
            wglMakeCurrent(g_hDC, hRC);
        }
    }

    wglSwapInterval = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");

    if (strstr(glGetString(GL_EXTENSIONS), "GL_EXT_texture_compression_s3tc"))
    {
        glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) wglGetProcAddress("glCompressedTexSubImage2D");
        glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) wglGetProcAddress("glCompressedTexImage2D");
    }

    timeBeginPeriod(1);
}

unsigned int osGetMilliseconds()
{
    return timeGetTime();
}

int osPollEvent(OS_Event *e)
{
    MSG msg;
    msg.message = WM_NULL;

    while (PeekMessage(&msg, g_hWnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
        {
            e->type = OS_QUIT;
            return 1;
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

    e->type = 0;
    return 0;
}

void osSwapBuffers()
{
    SwapBuffers(g_hDC);
}

int osShowCursor(int toggle)
{
    switch (toggle)
    {
        case OS_ENABLE:
            if (!g_cursorVisible)
            {
                ShowCursor(TRUE);
                g_cursorVisible = 1;
            }
            break;

        case OS_DISABLE:
            if (g_cursorVisible)
            {
                ShowCursor(FALSE);
                g_cursorVisible = 0;
            }
            break;

        case OS_QUERY:
            return g_cursorVisible;
    }

    return g_cursorVisible;
}

void osGetWindowPos(int *x, int *y)
{
    RECT rect;
    GetWindowRect(g_hWnd, &rect);
    *x = rect.left;
    *y = rect.top;
}

void osMoveWindow(int x, int y)
{
    RECT rect;
    GetWindowRect(g_hWnd, &rect);
    MoveWindow(g_hWnd, x, y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}
