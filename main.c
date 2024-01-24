#include <windows.h>
#include <gl/gl.h>
#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"

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



GLuint currentProgram;

void UseProgram(GLuint program)
{
    glUseProgram(program);
    currentProgram = program;
}


inline void setV3f(char *name, V3f vec)
{
    glUniform3f(glGetUniformLocation(currentProgram, name), vec.x, vec.y, vec.z);
}


GLuint vertexBuffer;
GLuint vertexArray;

#define POINTS_PER_VERTEX 2
float vertices[] = {
    0.5f, 0.0f,
    0.0f, 0.0f,
    0.5f, 0.5f,
    0.0f, 0.5f
};

void Draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setV3f("color", (V3f){1, 1, 1});
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / POINTS_PER_VERTEX);
}



void WinMainCRTStartup()
{
    HINSTANCE instance = GetModuleHandle(0);
    HWND window = OpenWindow(instance, OnEvent);

    HDC dc = GetDC(window);
    Win32InitOpenGL(window);
    InitFunctions();

    GLuint program = CreateProgram("..\\base.vs", "..\\base.fs");
    UseProgram(program);

    glGenBuffers(1, &vertexBuffer);
    glGenVertexArrays(1, &vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(vertexArray);
    size_t stride = POINTS_PER_VERTEX * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);  
    
    glClearColor(0.1, 0.1, 0.1, 1);

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
