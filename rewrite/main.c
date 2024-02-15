#include <windows.h>
#include <windowsx.h> // used for GET_X_LPARAM
#include <gl/gl.h>

#include "../win32.c"
#include "../opengl/glFunctions.c"
#include "../opengl/openglProgram.c"
#include "../layout.c"
#include "../font.c"
#include "../colors.c"
#include "../string.c"
#include "../format.c"
#include "../editor.c"
#include "../ui.c"

#define FILE_PATH "..\\string.c"



// I'm not happy with the UI results I have. Main issues 
// - I need to partially store state outside of the IM mode layout


// size
// input (needs draw to finish or operates on prev frame)
// draw


// size
// input (needs draw to finish or operates on prev frame)
//     update UI Model
// draw UI model

typedef struct AppState
{
    i32 isRunning;
    V2i screen;
    V2i mouse;
    f32 scale;

    StringBuffer file;

    f32 codeOffsetY;
    f32 codeLayoutHeight;
    f32 codePageHeight;
    Layout codeView;

    f32 zDeltaThisFrame;

    FontData codeFont;
    FontData lineNumbers;
    FontData selectedLineNumbers;


    GLuint vertexBuffer;
    GLuint vertexArray;
    GLuint primitivesProgram;
    GLuint textProgram;
} AppState;

AppState* state;

#define PX(val) ((val) * state->scale)
#define IsKeyPressed(key) (GetKeyState(key) & 0b10000000)
#define FLOATS_PER_VERTEX 4
float vertices[] = {
    //Position      UV coords
    1.0f, 0.0f,    1.0f, 0.0f,
    0.0f, 0.0f,    0.0f, 0.0f,
    1.0f, 1.0f,    1.0f, 1.0f,
    0.0f, 1.0f,    0.0f, 1.0f
};


void InitAppMemory()
{
    state = (AppState*) VirtualAllocateMemory(sizeof(AppState));
    state->isRunning = 1;
    state->file = ReadFileIntoDoubledSizedBuffer(FILE_PATH);
}

void ChangeProgram(GLuint program)
{
    UseProgram(program);
    SetMat4("projection", CreateViewMatrix(-1, -1, 2.0f / (f32)state->screen.x, 2.0f / (f32)state->screen.y));
}

#define DrawingShapes() ChangeProgram(state->primitivesProgram)
#define DrawingText()   ChangeProgram(state->textProgram)


void InitFonts()
{
    FontInfo info = {.size = 14, .name = "Consolas"};
    InitFont(&info, &state->codeFont, textColorHex, bgColorHex);
    InitFont(&info, &state->lineNumbers, lineColorHex, bgColorHex);
    InitFont(&info, &state->selectedLineNumbers, selectedLineColorHex, bgColorHex);
}


void PaintLayout(V4f color)
{
    SetV4f("color", color);
    SetMat4("view", CreateViewMatrix(currentLayout.x, currentLayout.y, currentLayout.width, currentLayout.height));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
}



void DrawNumberRightTop(f32 x, f32 y, u32 val)
{
    // step left by one char
    x -= currentFont->textures['_'].width;

    while(val != 0) 
    {
        u8 ch = '0' + val % 10;
        val /= 10;

        MyBitmap bitmap = currentFont->textures[ch];
        x -= bitmap.width;
        
        SetMat4("view", CreateViewMatrix(x, y, bitmap.width, bitmap.height));
        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[ch]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        ch++;
    }
}

void DrawCursor(f32 x, f32 y)
{
    DrawingShapes();

    SetV4f("color", (V4f){1,1,1,1});
    f32 width = PX(2);
    SetMat4("view", CreateViewMatrix(x - width / 2, y, width, currentFont->textMetric.tmHeight));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

    DrawingText();
}

void DrawSelectionBackground(f32 x, f32 y, f32 width)
{
    DrawingShapes();

    SetV4f("color", (V4f){65.0f / 255.0f, 155.0f / 255.0f, 255.0f / 255.0f, 0.30f});
    SetMat4("view", CreateViewMatrix(x, y, width, currentFont->textMetric.tmHeight));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

    DrawingText();
}

void PrintParagraphLeftTopMonospaced(StringBuffer* buffer)
{
    DrawingText();
    currentFont = &state->codeFont;

    f32 startX = currentLayout.x + currentLayout.runningX;
    f32 startY = currentLayout.y - currentLayout.runningY - currentLayout.offsetY + currentLayout.height - (f32)currentFont->textMetric.tmHeight;

    f32 x = startX;
    f32 y = startY;
    i32 lineNumber = 1;
    i32 hasEnteredNewLine = 1;

    i32 selectionFrom = MinI32(cursor.selectionStart, cursor.cursorIndex);
    i32 selectionTo   = MaxI32(cursor.selectionStart, cursor.cursorIndex);

    for (i32 i = 0; i <= buffer->size; i++)
    {
        u8 ch = *(buffer->content + i);

        if(hasEnteredNewLine)
        {
            if((lineNumber - 1) == cursor.row)
                currentFont = &state->selectedLineNumbers;
            else 
                currentFont = &state->lineNumbers;

            DrawNumberRightTop(x, y, lineNumber);
            hasEnteredNewLine = 0;
            
            currentFont = &state->codeFont;
        }

        if(ch == '\n')
        {
            if(i == cursor.cursorIndex)
                DrawCursor(x, y);

            if(cursor.selectionStart != SELECTION_NONE && i >= selectionFrom && i < selectionTo)
                DrawSelectionBackground(x, y, currentFont->textures[' '].width);

            y -= currentFont->textMetric.tmHeight;
            currentLayout.runningY += currentFont->textMetric.tmHeight;
            x = startX;
            hasEnteredNewLine = 1;
            lineNumber++;
            continue;
        }

        MyBitmap bitmap = currentFont->textures[ch];

        SetMat4("view", CreateViewMatrix(x, y, bitmap.width, bitmap.height));
        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[ch]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        if(i == cursor.cursorIndex)
            DrawCursor(x, y);
        
        if(cursor.selectionStart != SELECTION_NONE && i >= selectionFrom && i < selectionTo)
            DrawSelectionBackground(x, y, bitmap.width);

        x += bitmap.width;

    }

    state->codePageHeight = currentLayout.runningY;
    state->codeLayoutHeight = currentLayout.height;

    DrawingShapes();
}




void Draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawingShapes();
    ResetLayout(state->screen);

    PushPersistedLayout(state->codeView);

   
    if(IsMouseOverLayout(state->mouse))
        state->codeOffsetY += state->zDeltaThisFrame;
    
    if(state->codePageHeight > state->codeLayoutHeight)
        currentLayout.offsetY = Clampf32(state->codeOffsetY, -(state->codePageHeight - state->codeLayoutHeight), 0);


    f32 offsetForLineNumbers = PX(40);
    LayoutMoveRight(offsetForLineNumbers);

    PrintParagraphLeftTopMonospaced(&state->file);
    
    
    SetV4f("color", (V4f){1, 1, 1, 1});
    SetMat4("view", DrawScrollbar(&currentLayout, PX(10)));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
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
        state->screen.x = LOWORD(lParam);
        state->screen.y = HIWORD(lParam);

        HDC dc = GetDC(window);
        state->scale = (float)GetDeviceCaps(dc, LOGPIXELSY) / (float)USER_DEFAULT_SCREEN_DPI;
        
        
        state->codeView.height = state->screen.y;
        state->codeView.width = state->screen.x;
        state->codeView.x = 0;
        state->codeView.y = 0;

        glViewport(0, 0, state->screen.x, state->screen.y);
        InvalidateRect(window, NULL, TRUE);
        break;

    case WM_PAINT:
        PAINTSTRUCT paint = {0};
        HDC paintDc = BeginPaint(window, &paint);
        Draw(window);
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

        f32 rowHeight = state->codeFont.textMetric.tmHeight;
        f32 cursorPos = cursor.row * rowHeight;
        f32 layoutHeight = state->codeLayoutHeight;
        f32 pageHeight = state->codePageHeight;
        f32 offsetY = state->codeOffsetY;

        i32 rowShowAhead = 1;
        
        if(cursorPos + offsetY + rowHeight * rowShowAhead > layoutHeight)
            state->codeOffsetY = -(cursorPos - layoutHeight + rowHeight * rowShowAhead);

        if(cursorPos - rowHeight * rowShowAhead < -offsetY)
            state->codeOffsetY = -(cursorPos - rowHeight * rowShowAhead);
        break;

    case WM_KEYUP: 
        break;
        
    case WM_MOUSEWHEEL: 
        state->zDeltaThisFrame += (f32)GET_WHEEL_DELTA_WPARAM(wParam);
        break;

    case WM_MOUSEMOVE: 
        state->mouse = (V2i){GET_X_LPARAM(lParam), state->screen.y - GET_Y_LPARAM(lParam)};
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
    InitFonts();


    // timeBeginPeriod(1);
    // wglSwapIntervalEXT(0);

    state->textProgram = CreateProgram("..\\shaders\\base.vs", "..\\shaders\\base.fs");
    state->primitivesProgram = CreateProgram("..\\shaders\\primitives.vs", "..\\shaders\\primitives.fs");

    glGenBuffers(1, &state->vertexBuffer);
    glGenVertexArrays(1, &state->vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, state->vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(state->vertexArray);
    size_t stride = FLOATS_PER_VERTEX * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);  

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);  
    
    V3f bg = bgColor;
    glClearColor(bg.r, bg.g, bg.b, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    
    while (state->isRunning)
    {

        state->zDeltaThisFrame = 0;
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Draw(window);
        SwapBuffers(dc);
        // Sleep(16);

    }

    ExitProcess(0);
}

