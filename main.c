#include <windows.h>
#include <windowsx.h> // used for GET_X_LPARAM
#include <gl/gl.h>

#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"
#include "string.c"
#include "editor.c"
#include "app.c"
#include "ui.c"
#include "file.c"

#define DEFAULT_FILE "progress.txt"
// #define WORKING_FOLDER "C:\\Users\\ila_i\\OneDrive\\Pictures\\Screenshots"
#define WORKING_FOLDER "C:\\projects\\yafe"

// I'm not happy with the UI results I have. Main issues 
// - I need to partially store state outside of the IM mode layout


// Events
//     Cursor movement: left, right, up, down, word\line\page jump, file\line end\start
//     Editing: remove right\left chars
//     File save
//     Mouse: move, wheel
//     Resize

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

    state->filePath = EmptyStringBuffer();
    state->modal.searchTerm = EmptyStringBuffer();

    AppendStr(&state->filePath, WORKING_FOLDER);
    AppendStr(&state->filePath, "\\");
    AppendStr(&state->filePath, DEFAULT_FILE);
    state->file = ReadFileIntoDoubledSizedBuffer(state->filePath.content);

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
        Draw(state);
        SwapBuffers(paintDc);
        EndPaint(window, &paint);
        break;

    case WM_KEYDOWN: 
        if(state->modal.isShown)
        {
            ModalState* modal = &state->modal;
            if(wParam == VK_DOWN)
                modal->selectedFileIndex = ClampI32(modal->selectedFileIndex + 1, 0, modal->filesCount - 1);
            if(wParam == VK_UP)
                modal->selectedFileIndex = ClampI32(modal->selectedFileIndex - 1, 0, modal->filesCount - 1);


            if (wParam == VK_BACK && modal->searchTerm.size > 0)
            {
                RemoveCharAt(&modal->searchTerm, modal->searchTerm.size);
                FindAllFiles(WORKING_FOLDER, state);
            }

            else if (wParam == VK_ESCAPE)
            {
                state->modal.isShown = 0;
                BufferClear(&state->modal.searchTerm);
            }
        }
        else
        {
            f32 pageHeightInRows = state->uiState.codeLayout.height / state->uiState.codeFont.textMetric.tmHeight;

            if (wParam == VK_DOWN)
                MoveCursor(&state->file, Down, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_UP)
                MoveCursor(&state->file, Up, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_LEFT && IsKeyPressed(VK_CONTROL))
                MoveCursor(&state->file, WordJumpLeft, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_LEFT)
                MoveCursor(&state->file, Left, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_RIGHT && IsKeyPressed(VK_CONTROL))
                MoveCursor(&state->file, WordJumpRight, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_RIGHT)
                MoveCursor(&state->file, Right, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_END && IsKeyPressed(VK_CONTROL))
                MoveCursor(&state->file, FileEnd, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_HOME && IsKeyPressed(VK_CONTROL))
                MoveCursor(&state->file, FileStart, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_END)
                MoveCursor(&state->file, LineEnd, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_HOME)
                MoveCursor(&state->file, LineStart, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_NEXT)
                MoveCursor(&state->file, PageDown, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_PRIOR)
                MoveCursor(&state->file, PageUp, IsKeyPressed(VK_SHIFT), pageHeightInRows);
            else if (wParam == VK_BACK)
                RemoveCharFromLeft(&state->file);
            else if (wParam == VK_DELETE)
                RemoveCurrentChar(&state->file);
            else if (wParam == VK_F11)
            {
                state->uiState.isFullscreen = !state->uiState.isFullscreen;
                SetFullscreen(window, state->uiState.isFullscreen);
            }
            else if (wParam == 'P' && IsKeyPressed(VK_CONTROL))
            {
                FindAllFiles(WORKING_FOLDER, state);
                state->modal.isShown = 1;
            }

            else if (wParam == 'S' && IsKeyPressed(VK_CONTROL))
                WriteMyFile(state->filePath.content, state->file.content, state->file.size);

            UpdateUi(state);

            f32 rowHeight = state->uiState.codeFont.textMetric.tmHeight;
            i32 lookAhead = 5 * rowHeight; // look 5 rows ahead
            f32 cursorPos = cursor.row * rowHeight;
            f32 layoutHeight = state->uiState.codeLayout.height;
            f32 pageHeight = state->uiState.codeLayout.pageHeight;
            f32 offsetY = state->uiState.codeLayout.offsetY;

            if (cursorPos - offsetY + lookAhead > layoutHeight)
                SetOffset(cursorPos - layoutHeight + lookAhead);
            else if (cursorPos - lookAhead < offsetY)
                SetOffset(cursorPos - lookAhead);
            else // clamp existing offset because it might change dues to pageHeight change in UpdateUi
                SetOffset(offsetY);
        }
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
        ModalState* modal = &state->modal;
        if(modal->isShown)
        {
            StringBuffer* term = &modal->searchTerm;

            if(wParam == '\r')
            {
                modal->isShown = 0;
                // is it ok not to release results yet (they should be released on next search)
                BufferClear(term);

                // TODO: this is ugly, but I need to prevernt WM_CHAR event from hapenning when pressing enter
                if(modal->filesCount > 0)
                {
                    BufferClear(&state->filePath);
                    AppendStr(&state->filePath, WORKING_FOLDER);
                    AppendStr(&state->filePath, "\\");
                    AppendStr(&state->filePath, modal->files[modal->selectedFileIndex].content);


                    VirtualFreeMemory(state->file.content);
                    state->file = ReadFileIntoDoubledSizedBuffer(state->filePath.content);
                    cursor.col = cursor.row = cursor.cursorIndex = 0;
                    state->uiState.codeLayout.offsetY = 0;
                    UpdateUi(state);
                }
            }
            else if(wParam >= ' ')
            {
                InsertCharAt(term, term->size, wParam);
                FindAllFiles(WORKING_FOLDER, state);
            }
        }
        else 
        {
            if(wParam >= ' ' || wParam == '\r')
                InsertChartUnderCursor(&state->file, wParam);
        }
        break;
    }

    if (state->isRunning == 1)
    {
        Draw(state);
        SwapBuffers(GetDC(window));
    }
    return DefWindowProc(window, message, wParam, lParam);
}


void WinMainCRTStartup()
{
    PreventWindowsDPIScaling();
    InitAppMemory();

    HINSTANCE instance = GetModuleHandle(0);
    HWND window = OpenWindow(instance, OnEvent);

    // FindAllFiles(WORKING_FOLDER, state);
  
    // state->modal.isShown = 1;

    HDC dc = GetDC(window);
    Win32InitOpenGL(window);

    InitFunctions();
    

    timeBeginPeriod(1);
    wglSwapIntervalEXT(0);

    InitUi(&state->uiState);


    UpdateUi(state);

    state->isRunning = 1;
    while (state->isRunning)
    {
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Draw(state);
        // SwapBuffers(dc);
        Sleep(15);
    }

    ExitProcess(0);
}

