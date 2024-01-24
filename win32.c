#ifndef WIN32_C
#define WIN32_C

#include <windows.h>
#include <dwmapi.h>
#include "core.c"
#include "memory.c"

#define EDITOR_DEFAULT_WINDOW_STYLE (WS_OVERLAPPEDWINDOW)

HWND OpenWindow(HINSTANCE instance, WNDPROC OnEvent)
{
    WNDCLASSW windowClass = {0};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = OnEvent;
    windowClass.lpszClassName = L"MyWindow";
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    // not using COLOR_WINDOW + 1 because it doesn't fucking work
    // this line fixes a flash of a white background for 1-2 frames during start
    windowClass.hbrBackground = CreateSolidBrush(0x111111);
    // };
    RegisterClassW(&windowClass);

    HDC dc = GetDC(0);
    int screenWidth = GetDeviceCaps(dc, HORZRES);
    int screenHeight = GetDeviceCaps(dc, VERTRES);

    HWND window = CreateWindowW(windowClass.lpszClassName, (wchar_t *)"Editor", EDITOR_DEFAULT_WINDOW_STYLE | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                0, 0, instance, 0);

    BOOL USE_DARK_MODE = TRUE;
    BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
        window, DWMWA_USE_IMMERSIVE_DARK_MODE, &USE_DARK_MODE, sizeof(USE_DARK_MODE)));

    DeleteObject(windowClass.hbrBackground);
    return window;
}


void Win32InitOpenGL(HWND Window)
{
    HDC WindowDC = GetDC(Window);

    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {0};
    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
    DesiredPixelFormat.cColorBits = 32;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
    
    HGLRC OpenGLRC = wglCreateContext(WindowDC);
    if(!wglMakeCurrent(WindowDC, OpenGLRC))        
        Fail("Failed to initialize OpenGL");

    ReleaseDC(Window, WindowDC);
}



//
// File IO
//

typedef struct FileContent
{
    char *content;
    i32 size;
} FileContent;


FileContent ReadMyFileImp(char* path)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    LARGE_INTEGER size;
    GetFileSizeEx(file, &size);

    u32 fileSize = (u32)size.QuadPart;

    void *buffer = VirtualAllocateMemory(fileSize);

    DWORD bytesRead;
    ReadFile(file, buffer, fileSize, &bytesRead, 0);
    CloseHandle(file);

    FileContent res = {0};
    res.content = (char*) buffer;
    res.size = bytesRead;
    return res;
}


#endif
