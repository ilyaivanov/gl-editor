#ifndef WIN32_C
#define WIN32_C

#include <windows.h>
#include <dwmapi.h>

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
    // windowClass.hbrBackground = CreateSolidBrush(0x111111);
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

#endif