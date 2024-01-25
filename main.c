#include <windows.h>
#include <gl/gl.h>
#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"
#include "font.c"
#include "ui.c"

float SYSTEM_SCALE = 1;
#define PX(val) ((val) * SYSTEM_SCALE)

i32 isRunning = 1;
V2i clientAreaSize = {0};

Layout mainLayout = {0};

f32 zDeltaThisFrame;

void Draw();

void OnResize()
{
    glViewport(0, 0, clientAreaSize.x, clientAreaSize.y);

    mainLayout.height = clientAreaSize.y;
    mainLayout.width = clientAreaSize.x;
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

        HDC dc = GetDC(window);
        SYSTEM_SCALE = (float)GetDeviceCaps(dc, LOGPIXELSY) / (float)USER_DEFAULT_SCREEN_DPI;

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
    else if (message == WM_KEYDOWN)
    {
    }
    else if (message == WM_KEYUP)
    {
    }
    else if (message == WM_MOUSEWHEEL)
    {
        zDeltaThisFrame += (float)GET_WHEEL_DELTA_WPARAM(wParam);
    }

    return DefWindowProc(window, message, wParam, lParam);
}


GLuint vertexBuffer;
GLuint vertexArray;
GLuint primitivesProgram;
GLuint textProgram;

#define FLOATS_PER_VERTEX 4
float vertices[] = {
    //Position     UV coords
    1.0f, 0.0f,    1.0f, 0.0f,
    0.0f, 0.0f,    0.0f, 0.0f,
    1.0f, 1.0f,    1.0f, 1.0f,
    0.0f, 1.0f,    0.0f, 1.0f
};
FileContent file;

void Draw()
{
    mainLayout.offsetY = Clampf32(mainLayout.offsetY + zDeltaThisFrame, -(mainLayout.pageHeight - mainLayout.height), 0);

    zDeltaThisFrame = 0;

    UseProgram(textProgram);

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

    currentFont = &codeFont;

    f32 padding = 20.0f;
    f32 startY = clientAreaSize.y - currentFont->textMetric.tmHeight - padding;

    f32 runningX = padding;
    f32 runningY = startY - mainLayout.offsetY;

    u8 *ch = file.content;
    while(*ch)
    {
        u8 code = *ch;
        if(code == '\n')
        {
            runningX = padding;
            runningY -= currentFont->textMetric.tmHeight;
        }
        else if(code >= ' ') {

            MyBitmap bitmap = currentFont->textures[code];
            
            Mat4 view = 
            {
                bitmap.width, 0, 0, runningX,
                0, bitmap.height, 0, runningY,
                0, 0, 1, 0,
                0, 0, 0, 1,
            };
            SetMat4("view", view);
            glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[code]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

            //TODO monospaced fonts have no kerning, I can figure if font is monospaced and skip kerning lookup
            runningX += bitmap.width + GetKerningValue(code, *(ch+1));
        }
        ch++;
    }
    runningY -= (currentFont->textMetric.tmHeight + padding);

    mainLayout.pageHeight = startY - runningY - mainLayout.offsetY;

    UseProgram(primitivesProgram);
    Mat4 scrollView = DrawScrollbar(&mainLayout, PX(10));
    SetMat4("projection", projection);
    SetV3f("color", (V3f){0.3f, 0.3f, 0.3f});
    SetMat4("view", scrollView);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
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

    file = ReadMyFileImp("..\\main.c");
    textProgram = CreateProgram("..\\shaders\\base.vs", "..\\shaders\\base.fs");
    primitivesProgram = CreateProgram("..\\shaders\\primitives.vs", "..\\shaders\\primitives.fs");

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
