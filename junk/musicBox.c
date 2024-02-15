#include <windows.h>
#include <windowsx.h> // used for GET_X_LPARAM
#include <gl/gl.h>

#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"
#include "font.c"
#include "layout.c"
#include "ui.c"
// #include "string.c"
#include "colors.c"


float SYSTEM_SCALE = 1;
#define PX(val) ((val) * SYSTEM_SCALE)

i32 isRunning = 1;
V2i clientAreaSize = {0};
V2i mouse = {0};

f32 zDeltaThisFrame;

GLuint primitivesProgram;
GLuint textProgram;

FontInfo uiFontInfo   = {.size = 13, .name = "Segoe UI"};
FontData* uiFont;

V3f *currentTextColor;

void InitFonts()
{
    u8* mem = VirtualAllocateMemory(sizeof(FontData));
    uiFont   = (FontData*) mem;
    InitFont(&uiFontInfo, uiFont);
}

void DrawingShapes()
{        
    UseProgram(primitivesProgram);
    SetMat4("projection", CreateViewMatrix(-1, -1, 2.0f / (f32)clientAreaSize.x, 2.0f / (f32)clientAreaSize.y));
}

void DrawingText()
{        
    UseProgram(textProgram);
    SetMat4("projection", CreateViewMatrix(-1, -1, 2.0f / (f32)clientAreaSize.x, 2.0f / (f32)clientAreaSize.y));
}

void Draw();

LRESULT OnEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        isRunning = 0;
        break;

    case WM_MOUSEWHEEL:
        zDeltaThisFrame += (float)GET_WHEEL_DELTA_WPARAM(wParam);
        break;

    case WM_MOUSEMOVE:
        mouse = (V2i){GET_X_LPARAM(lParam), clientAreaSize.y - GET_Y_LPARAM(lParam)};
        break;
    }

    if (message == WM_SIZE)
    {
        clientAreaSize.x = LOWORD(lParam);
        clientAreaSize.y = HIWORD(lParam);

        HDC dc = GetDC(window);
        SYSTEM_SCALE = (float)GetDeviceCaps(dc, LOGPIXELSY) / (float)USER_DEFAULT_SCREEN_DPI;

        glViewport(0, 0, clientAreaSize.x, clientAreaSize.y);
        InvalidateRect(window, NULL, TRUE);
    }
    else if (message == WM_PAINT)
    {
        PAINTSTRUCT paint = {0};
        HDC dc = BeginPaint(window, &paint);
        Draw(window);
        SwapBuffers(dc);
        EndPaint(window, &paint);
    }
    return DefWindowProc(window, message, wParam, lParam);
}

GLuint vertexBuffer;
GLuint vertexArray;


#define FLOATS_PER_VERTEX 4
float vertices[] = {
    //Position      UV coords
    1.0f, 0.0f,    1.0f, 0.0f,
    0.0f, 0.0f,    0.0f, 0.0f,
    1.0f, 1.0f,    1.0f, 1.0f,
    0.0f, 1.0f,    0.0f, 1.0f
};


void PaintLayout(V3f color)
{
    SetV3f("color", color);
    SetMat4("view", CreateViewMatrix(currentLayout.x, currentLayout.y, currentLayout.width, currentLayout.height));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
}

void PrintTextLeftCentered(u8* text, f32 leftPadding)
{
    DrawingText();
    SetV3f("color", *currentTextColor);
    f32 textWidth = (f32)GetTextWidth(text);
    f32 x = currentLayout.x + leftPadding;
    f32 y = currentLayout.y + currentLayout.height / 2 - (f32)currentFont->textMetric.tmHeight / 2;
    while(*text)
    {
        MyBitmap bitmap2 = currentFont->textures[*text];

        SetMat4("view", CreateViewMatrix(x, y, bitmap2.width, bitmap2.height));
        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[*text]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        //TODO: add kerning
        x += bitmap2.width + GetKerningValue(*text, *(text + 1));
        text++;
    }
    DrawingShapes();
}

char* left[] = {
    "Carbon Based Lifeforms",
    "Circular",
    "I Awake",
    "James Murray",
    "Miktek",
    "Koan",
    "Zero Cult",
    "Androcell",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Carbon Based Lifeforms",
    "Circular",
    "I Awake",
    "James Murray",
    "Miktek",
    "Koan",
    "Zero Cult",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Carbon Based Lifeforms",
    "Circular",
    "I Awake",
    "James Murray",
    "Miktek",
    "Koan",
    "Zero Cult",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Cell",
    "Biosphere",
    "Aes Dana",
    "Side Liner	",
    "Fahrenheit Project",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Carbon Based Lifeforms",
    "Circular",
    "I Awake",
    "James Murray",
    "Miktek",
    "Koan",
    "Zero Cult",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "H.U.V.A Network",
    "Solar Fields",
    "Sync24",
    "Ascendant",
};

char* right[] = {
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Carbon Based Lifeforms",
    "Circular",
    "I Awake",
    "James Murray",
    "Miktek",
    "Koan",
    "Zero Cult",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Carbon Based Lifeforms",
    "Circular",
    "I Awake",
    "James Murray",
    "Miktek",
    "Koan",
    "Zero Cult",
    "Androcell",
    "Scann-Tec",
    "Hol Baumann",
    "Asura",
    "Cell",
    "Biosphere",
    "Aes Dana",
    "Side Liner	",
    "Fahrenheit Project",
    "H.U.V.A Network",
    "Solar Fields",
    "Sync24",
    "Ascendant",
};
i32 IsMouseOverLayout()
{
    return mouse.x >= currentLayout.x && 
           mouse.y >= currentLayout.y && 
           mouse.x <= (currentLayout.x + currentLayout.width) &&
           mouse.y <= (currentLayout.y + currentLayout.height);
}

// x,y is a top left coordinate
void DrawItem(char* item)
{
    f32 padding = PX(3);
    PushRow(uiFont->textMetric.tmHeight + padding * 2);



    currentTextColor = &white;
    currentFont = uiFont;


    PrintTextLeftCentered(item, PX(10));
    PopLayout();    
}

f32 leftOffset = -20;
f32 rightOffset = 0;

void Draw(HWND window)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
    ResetLayout(clientAreaSize);
    DrawingShapes();

    float borderWidth = PX(2);
    PushColumnWithOffset(currentLayout.width / 2 - borderWidth / 2, leftOffset);    
        for(int i = 0; i < ArrayLength(left); i++)
        {
            DrawItem(left[i]);
        }

        if(IsMouseOverLayout())
            leftOffset += zDeltaThisFrame;

        SetV3f("color", (V3f){0.3f, 0.3f, 0.3f});
        SetMat4("view", DrawScrollbar(&currentLayout, PX(10)));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
    PopLayout();

    PushColumn(borderWidth);    
        PaintLayout((V3f){0.4f, 0.4f, 0.4f});
    PopLayout();

    PushColumnWithOffset(currentLayout.width / 2 - borderWidth / 2, rightOffset);
        for(int i = 0; i < ArrayLength(right); i++)
        {
            DrawItem(right[i]);
        }

        if(IsMouseOverLayout())
            rightOffset += zDeltaThisFrame;


        SetV3f("color", (V3f){0.3f, 0.3f, 0.3f});
        SetMat4("view", DrawScrollbar(&currentLayout, PX(10)));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
    PopLayout();


    zDeltaThisFrame = 0;
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
    // InitConstants();

    // layout sizes depends on constants, constants depends on fonts, fonts on window initialization
    // thus I can't rely on WM_SIZE events during window creation, because fonts aren't there yet
    // OnResize();

    // wglSwapIntervalEXT(0);


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
    
    glClearColor(bgColor.r, bgColor.g, bgColor.b, 1);

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


        Draw(window);
        SwapBuffers(dc);

    }

    ExitProcess(0);
}

