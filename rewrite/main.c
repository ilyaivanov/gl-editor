#include <windows.h>
#include <windowsx.h> // used for GET_X_LPARAM
#include <gl/gl.h>

#include "../win32.c"
#include "../opengl/glFunctions.c"
#include "../opengl/openglProgram.c"
#include "../string.c"
#include "../editor.c"
#include "ui.c"

#define FILE_PATH "..\\rewrite\\progress.txt"

// I'm not happy with the UI results I have. Main issues 
// - I need to partially store state outside of the IM mode layout


// Events
//     Cursor movement: left, right, up, down, word\line\page jump, file\line end\start
//     Editing: remove right\left chars
//     File save
//     Mouse: move, wheel
//     Resize


typedef struct AppState
{
    i32 isRunning;
    V2i mouse;

    StringBuffer file;

    UiState uiState;
} AppState;

AppState* state;

#define IsKeyPressed(key) (GetKeyState(key) & 0b10000000)


void SetOffset(f32 offset)
{
    Layout* layout = &state->uiState.codeLayout;
    if(layout->pageHeight <= layout->height)
        layout->offsetY = 0;
    else 
        layout->offsetY = Clampf32(offset, 0, layout->pageHeight - layout->height);
}

void InitAppMemory()
{
    state = (AppState*) VirtualAllocateMemory(sizeof(AppState));
    state->isRunning = 1;
    state->file = ReadFileIntoDoubledSizedBuffer(FILE_PATH);
}

LRESULT OnEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        state->isRunning = 0;
        break;
    
    case WM_SIZE:
        i32 width  = LOWORD(lParam);
        i32 height = HIWORD(lParam);
        state->uiState.screen.x = width;
        state->uiState.screen.y = height;

        HDC dc = GetDC(window);
        state->uiState.scale = (float)GetDeviceCaps(dc, LOGPIXELSY) / (float)USER_DEFAULT_SCREEN_DPI;
        
        state->uiState.codeLayout.width = width;
        state->uiState.codeLayout.height = height;
        state->uiState.codeLayout.x = 0;
        state->uiState.codeLayout.y = 0;

        glViewport(0, 0, width, height);
        InvalidateRect(window, NULL, TRUE);
        break;

    case WM_PAINT:
        PAINTSTRUCT paint = {0};
        HDC paintDc = BeginPaint(window, &paint);
        Draw(&state->uiState, &state->file);
        SwapBuffers(paintDc);
        EndPaint(window, &paint);
        break;

    case WM_KEYDOWN: 
        if(wParam == VK_DOWN)
            MoveCursor(&state->file, Down, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_UP)
            MoveCursor(&state->file, Up, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_LEFT && IsKeyPressed(VK_CONTROL))
            MoveCursor(&state->file, WordJumpLeft, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_LEFT)
            MoveCursor(&state->file, Left, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_RIGHT && IsKeyPressed(VK_CONTROL))
            MoveCursor(&state->file, WordJumpRight, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_RIGHT)
            MoveCursor(&state->file, Right, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_END && IsKeyPressed(VK_CONTROL))
            MoveCursor(&state->file, FileEnd, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_HOME && IsKeyPressed(VK_CONTROL))
            MoveCursor(&state->file, FileStart, IsKeyPressed(VK_SHIFT)); 
        else if(wParam == VK_END)
            MoveCursor(&state->file, LineEnd, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_HOME)
            MoveCursor(&state->file, LineStart, IsKeyPressed(VK_SHIFT));       
        else if(wParam == VK_NEXT)
            MoveCursor(&state->file, PageDown, IsKeyPressed(VK_SHIFT));            
        else if(wParam == VK_PRIOR)
            MoveCursor(&state->file, PageUp, IsKeyPressed(VK_SHIFT));            
        else if (wParam == VK_BACK)
            RemoveCharFromLeft(&state->file);
        else if (wParam == VK_DELETE)
            RemoveCurrentChar(&state->file);
        else if (wParam == 'S' && IsKeyPressed(VK_CONTROL))
            WriteMyFile(FILE_PATH, state->file.content, state->file.size);

        UpdateUi(&state->uiState, &state->file);


        f32 rowHeight = state->uiState.codeFont.textMetric.tmHeight;
        i32 lookAhead = 5 * rowHeight; // look 5 rows ahead
        f32 cursorPos = cursor.row * rowHeight;
        f32 layoutHeight = state->uiState.codeLayout.height;
        f32 pageHeight = state->uiState.codeLayout.pageHeight;
        f32 offsetY = state->uiState.codeLayout.offsetY;

        
        if(cursorPos - offsetY + lookAhead > layoutHeight)
            SetOffset(cursorPos - layoutHeight + lookAhead); 
        else if(cursorPos - lookAhead < offsetY)
            SetOffset(cursorPos - lookAhead); 
        else //clamp existing offset because it might change dues to pageHeight change in UpdateUi 
            SetOffset(offsetY); 
        break;

    case WM_KEYUP: 
        break;
        
    case WM_MOUSEWHEEL: 
        PushPersistedLayout(state->uiState.codeLayout);
        if(IsMouseOverLayout(state->mouse))
            SetOffset(state->uiState.codeLayout.offsetY - GET_WHEEL_DELTA_WPARAM(wParam));
        PopLayout();
        break;

    case WM_MOUSEMOVE: 
         state->mouse = (V2i){GET_X_LPARAM(lParam), state->uiState.screen.y - GET_Y_LPARAM(lParam)};
        break;

    case WM_CHAR:
        if(wParam >= ' ' || wParam == '\r')
            InsertChartUnderCursor(&state->file, wParam);
        break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}


void WinMainCRTStartup()
{
    PreventWindowsDPIScaling();
    InitAppMemory();

    HINSTANCE instance = GetModuleHandle(0);
    HWND window = OpenWindow(instance, OnEvent);


    HDC dc = GetDC(window);
    Win32InitOpenGL(window);
    InitFunctions();


    // timeBeginPeriod(1);
    // wglSwapIntervalEXT(0);

    InitUi(&state->uiState);
    UpdateUi(&state->uiState, &state->file);

    while (state->isRunning)
    {
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        // this is bad - I shoudn't modify a state in a draw function
        // if(IsMouseOverLayout(state->mouse))
        //     state->uiState.codeLayout.offsetY += state->zDeltaThisFrame;

        // if(state->codePageHeight > state->codeLayoutHeight)
            // currentLayout.offsetY = Clampf32(state->codeOffsetY, -(state->codePageHeight - state->codeLayoutHeight), 0);


        Draw(&state->uiState, &state->file);
        SwapBuffers(dc);
        // Sleep(16);

    }

    ExitProcess(0);
}

