#ifndef WIN32_C
#define WIN32_C

#include <windows.h>
#include <dwmapi.h>
#include "core.c"

//https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
#define EDITOR_DEFAULT_WINDOW_STYLE (WS_OVERLAPPEDWINDOW)
// #define EDITOR_DEFAULT_WINDOW_STYLE ( WS_THICKFRAME)
// #define EDITOR_DEFAULT_WINDOW_STYLE WS_POPUP

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

    int windowWidth = 1000;
    int windowHeight = 1200;
    int padding = 20;

    // HWND window  = CreateWindowExW(
    //     WS_EX_CLIENTEDGE,
    //     windowClass.lpszClassName,
    //     L"Resizable Window without Title Bar",
    //     WS_POPUP | WS_THICKFRAME | WS_VISIBLE,
    //     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    //     NULL, NULL, instance, NULL
    // );



    HWND window = CreateWindowW(windowClass.lpszClassName, (wchar_t *)"Editor", 
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                screenWidth - windowWidth - padding, padding, windowWidth, windowHeight,
                                0, 0, instance, 0);

    BOOL USE_DARK_MODE = TRUE;
    BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
        window, DWMWA_USE_IMMERSIVE_DARK_MODE, &USE_DARK_MODE, sizeof(USE_DARK_MODE)));

    // TODO: maybe this is because window is flickering during runtime
    // DeleteObject(windowClass.hbrBackground);
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

void InitBitmapInfo(BITMAPINFO * bitmapInfo, u32 width, u32 height)
{
    bitmapInfo->bmiHeader.biSize = sizeof(bitmapInfo->bmiHeader);
    bitmapInfo->bmiHeader.biBitCount = 32;
    bitmapInfo->bmiHeader.biWidth = width;
    bitmapInfo->bmiHeader.biHeight = height; // makes rows go up, instead of going down by default
    bitmapInfo->bmiHeader.biPlanes = 1;
    bitmapInfo->bmiHeader.biCompression = BI_RGB;
}


// DPI Scaling
// user32.dll is linked statically, so dynamic linking won't load that dll again
// taken from https://github.com/cmuratori/refterm/blob/main/refterm.c#L80
// this is done because GDI font drawing is ugly and unclear when DPI scaling is enabled

typedef BOOL WINAPI set_process_dpi_aware(void);
typedef BOOL WINAPI set_process_dpi_awareness_context(DPI_AWARENESS_CONTEXT);
static void PreventWindowsDPIScaling()
{
    HMODULE WinUser = LoadLibraryW(L"user32.dll");
    set_process_dpi_awareness_context *SetProcessDPIAwarenessContext = (set_process_dpi_awareness_context *)GetProcAddress(WinUser, "SetProcessDPIAwarenessContext");
    if (SetProcessDPIAwarenessContext)
    {
        SetProcessDPIAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }
    else
    {
        set_process_dpi_aware *SetProcessDPIAware = (set_process_dpi_aware *)GetProcAddress(WinUser, "SetProcessDPIAware");
        if (SetProcessDPIAware)
        {
            SetProcessDPIAware();
        }
    }
}


// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
WINDOWPLACEMENT prevWindowDimensions = {sizeof(prevWindowDimensions)};
void SetFullscreen(HWND window, i32 isFullscreen)
{
    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (isFullscreen)
    {
        MONITORINFO monitorInfo = {sizeof(monitorInfo)};
        if (GetWindowPlacement(window, &prevWindowDimensions) &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
        {
            SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);

            SetWindowPos(window, HWND_TOP,
                         monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                         monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &prevWindowDimensions);
        SetWindowPos(window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
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

u32 GetMyFileSize(char *path)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    LARGE_INTEGER size;
    GetFileSizeEx(file, &size);

    CloseHandle(file);
    return (u32)size.QuadPart;
}

void ReadFileInto(char *path, u32 fileSize, char* buffer)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD bytesRead;
    ReadFile(file, buffer, fileSize, &bytesRead, 0);
    CloseHandle(file);
}


void WriteMyFile(char *path, char* content, int size)
{
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    DWORD bytesWritten;
    int res = WriteFile(file, content, size, &bytesWritten, 0);
    CloseHandle(file);

    Assert(bytesWritten == size);
}



#endif
