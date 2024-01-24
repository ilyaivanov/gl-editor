#include <windows.h>
#include <gl/gl.h>
#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"
#include "font.c"

int isRunning = 1;
V2i clientAreaSize = {0};

void Draw();

void OnResize()
{
    glViewport(0, 0, clientAreaSize.x, clientAreaSize.y);
}

LRESULT OnEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        isRunning = 0;
    }
    else if (message == WM_SIZE)
    {
        clientAreaSize.x = LOWORD(lParam);
        clientAreaSize.y = HIWORD(lParam);
        OnResize();
        InvalidateRect(window, NULL, TRUE);
    }
    else if (message == WM_PAINT)
    {
        PAINTSTRUCT paint = {0};
        HDC dc = BeginPaint(window, &paint);
        Draw();
        SwapBuffers(dc);
        EndPaint(window, &paint);
    }

    return DefWindowProc(window, message, wParam, lParam);
}


GLuint vertexBuffer;
GLuint vertexArray;

#define FLOATS_PER_VERTEX 4
float vertices[] = {
    //Position     UV coords
    1.0f, 0.0f,    1.0f, 0.0f,
    0.0f, 0.0f,    0.0f, 0.0f,
    1.0f, 1.0f,    1.0f, 1.0f,
    0.0f, 1.0f,    0.0f, 1.0f
};

void Draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // allows me to set vecrtex coords as 0..width/height, instead of -1..+1
    // 0,0 is bottom left, not top left
    // matrix in code != matrix in math notation, details at https://youtu.be/kBuaCqaCYwE?t=3084
    // in short: rows in math are columns in code
    Mat4 projection = {
        2.0f / (f32)clientAreaSize.x, 0, 0, -1,
        0, 2.0f / (f32)clientAreaSize.y, 0, -1,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    SetMat4("projection", projection);


    u8* line = "SetMat4(\"projection\", projection);";
    currentFont = &codeFont;

    i32 padding = 20;
    i32 x = padding;
    i32 y = clientAreaSize.y - currentFont->textMetric.tmHeight - padding;

    u8 *ch = line;
    while(*ch)
    {
        u8 code = *ch;
        MyBitmap bitmap = currentFont->textures[code];
        
        Mat4 view = 
        {
            bitmap.width, 0, 0, x,
            0, bitmap.height, 0, y,
            0, 0, 1, 0,
            0, 0, 0, 1,
        };
        SetMat4("view", view);
        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[code]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        x += bitmap.width + GetKerningValue(code, *(ch+1));
        ch++;
    }
}


void WinMainCRTStartup()
{
    PreventWindowsDPIScaling();

    HINSTANCE instance = GetModuleHandle(0);
    HWND window = OpenWindow(instance, OnEvent);

    HDC dc = GetDC(window);
    Win32InitOpenGL(window);
    InitFunctions();
    InitFonts();

    GLuint program = CreateProgram("..\\base.vs", "..\\base.fs");
    UseProgram(program);

    glGenBuffers(1, &vertexBuffer);
    glGenVertexArrays(1, &vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(vertexArray);
    size_t stride = FLOATS_PER_VERTEX * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);  

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);  
    
    glClearColor(0.1, 0.1, 0.1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (isRunning)
    {
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        Draw();
        SwapBuffers(dc);

    }

    ExitProcess(0);
}
