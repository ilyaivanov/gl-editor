#include <windows.h>
#include <windowsx.h> // used for GET_X_LPARAM
// #include <uxtheme.h> 
#include <gl/gl.h>

#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"
#include "font.c"
#include "layout.c"
#include "ui.c"
#include "string.c"
// #include "sincos.c"






float SYSTEM_SCALE = 1;
#define PX(val) ((val) * SYSTEM_SCALE)

#define HexColor(hex) {(float)((hex >> 16) & 0xff) / 255.0f, \
                       (float)((hex >>  8) & 0xff) / 255.0f, \
                       (float)((hex >>  0) & 0xff) / 255.0f}

#define Grey(val) ((V3f){val, val, val})
#define IsKeyPressed(key) (GetKeyState(key) & 0b10000000)


f32 appTime = 0;
i32 isRunning = 1;
V2i clientAreaSize = {0};
V2i mouse = {0};

f32 zDeltaThisFrame;

#define PATH "..\\string.c"

i32 isEditMode = 0;
StringBuffer file;

f32 padding;
f32 spaceForLineNumbers;
f32 lineNumberToCode;
f32 footerHeight;
f32 footerPadding;


V3f bgColor           = HexColor(0x030303);
V3f footerColor       = HexColor(0x181818);


V3f white             = HexColor(0xffffff);
V3f back              = HexColor(0x000000);

FontInfo uiFontInfo   = {.size = 20, .name = "Segoe UI"};
FontData* uiFont;

FontInfo codeFontInfo = {.size = 20, .name = "Consolas"};
FontData* codeFont;


void InitConstants()
{
    padding = PX(15.0f);
    spaceForLineNumbers = PX(40);
    lineNumberToCode = PX(10);
    footerPadding = PX(5);
    footerHeight = uiFont->textMetric.tmHeight + footerPadding;
}

void InitFonts()
{
    u8* mem = VirtualAllocateMemory(sizeof(FontData) * 2);
    uiFont   = (FontData*) mem;
    codeFont = (FontData*) (mem + sizeof(FontData));
    InitFont(&uiFontInfo, uiFont);
    InitFont(&codeFontInfo, codeFont);
}

void OnResize()
{
    glViewport(0, 0, clientAreaSize.x, clientAreaSize.y);
}

void FormatNumber(i32 val, char *buff)
{
    char* start = buff;
    if(val < 0)
    {
        *buff = '-';
        val = -val;
        buff++;
    }

    while(val != 0) 
    {
        *buff = '0' + val % 10;
        val /= 10;
        buff++;
    }
    *buff = '\0';
    ReverseString(start);
}

GLuint primitivesProgram;
GLuint textProgram;
void DrawingText()
{
    UseProgram(textProgram);
    SetMat4("projection", CreateViewMatrix(-1, -1, 2.0f / (f32)clientAreaSize.x, 2.0f / (f32)clientAreaSize.y));
}

void DrawingShapes()
{        
    UseProgram(primitivesProgram);
    SetMat4("projection", CreateViewMatrix(-1, -1, 2.0f / (f32)clientAreaSize.x, 2.0f / (f32)clientAreaSize.y));
}

void Draw();

i32 isMouseOverHeader = 0;
i32 isDraggingWindow = 0;
i32 isMouseOverSquare = 0;
i32 isDraggingSquare = 0;
V2f squarePos = {50, 50};

// ok, this is shitty as hell
i32 ignoreNextMouseMove = 0;

POINT lastLocation;

RECT border_thickness;

LRESULT OnEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_CREATE:
    {
        //find border thickness
        SetRectEmpty(&border_thickness);
        if (GetWindowLongPtr(window, GWL_STYLE) & WS_THICKFRAME)
        {
            AdjustWindowRectEx(&border_thickness, GetWindowLongPtr(window, GWL_STYLE) & ~WS_CAPTION, FALSE, NULL);
            border_thickness.left *= -1;
            border_thickness.top *= -1;
        }
        else if (GetWindowLongPtr(window, GWL_STYLE) & WS_BORDER)
        {
            SetRect(&border_thickness, 1, 1, 1, 1);
        }

        MARGINS margins = { 0 };
        DwmExtendFrameIntoClientArea(window, &margins);
        SetWindowPos(window, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        break;
    }
    case WM_NCCALCSIZE:
        if (wParam)
        {
            NCCALCSIZE_PARAMS *sz = (NCCALCSIZE_PARAMS *)lParam;
            sz->rgrc[0].left += border_thickness.left;
            sz->rgrc[0].right -= border_thickness.right;
            sz->rgrc[0].bottom -= border_thickness.bottom;
            return 0;
        }
        break;
    }

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
    //https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest
    else if (message ==  WM_NCHITTEST)
    {

        //do default processing, but allow resizing from top-border
        LRESULT result = DefWindowProc(window, message, wParam, lParam);
        if (result == HTCLIENT)
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(window, &pt);
            if (pt.y < border_thickness.top) return HTTOP;
        }


        POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(window, &point);

        mouse.x = point.x;
        mouse.y = clientAreaSize.y - point.y;



        if (isMouseOverSquare)
        {
            return HTCAPTION;
        }

        // if(mouse.x < 4)
        //     return HTLEFT;
        return result;
    }
    else if (message == WM_KEYDOWN)
    {
        if(wParam == VK_LEFT)
        {
            RECT rect;
            GetWindowRect(window, &rect);
            SetWindowPos(
                window, 0,
                rect.left - 1,
                rect.top,
                0, 0,
                SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
            );
        }
        if(wParam == VK_RIGHT)
        {
            RECT rect;
            GetWindowRect(window, &rect);
            SetWindowPos(
                window, 0,
                rect.left + 1,
                rect.top,
                0, 0,
                SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
            );
        }
        HDC hdc;
        hdc = GetDCEx(window, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
        // Paint into this DC 
        ReleaseDC(window, hdc);
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

V3f* currentTextColor;

void PrintTextLineCentered(u8* text)
{
    DrawingText();
    SetV3f("color", *currentTextColor);
    f32 textWidth = (f32)GetTextWidth(text);
    f32 x = currentLayout.x + currentLayout.width / 2 - textWidth / 2;
    f32 y = currentLayout.y + currentLayout.height / 2 - (f32)currentFont->textMetric.tmHeight / 2;
    while(*text)
    {
        MyBitmap bitmap2 = currentFont->textures[*text];

        SetMat4("view", CreateViewMatrix(x, y, bitmap2.width, bitmap2.height));
        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[*text]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        //TODO: add kerning
        x += bitmap2.width;
        text++;
    }
    DrawingShapes();
}

i32 IsMouseOverLayout()
{
    return mouse.x >= currentLayout.x && 
           mouse.y >= currentLayout.y && 
           mouse.x <= (currentLayout.x + currentLayout.width) &&
           mouse.y <= (currentLayout.y + currentLayout.height);
}

void Draw()
{
    currentFont = uiFont;
    currentTextColor = &white;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
    ResetLayout();
    DrawingShapes();

    PushLayout(0, 0, clientAreaSize.x, clientAreaSize.y);



    PushRow(PX(20));
        if(IsMouseOverLayout())
        {
            PaintLayout((V3f){0.6f, 0.0f, 0.6f});
            isMouseOverHeader = 1;
        }
        else
        {
            PaintLayout((V3f){0.4f, 0.0f, 0.4f});
            isMouseOverHeader = 0;
        }
    PopLayout();

    currentLayout.runningY += PX(10);  // ugly

    PushRow(currentLayout.height - currentLayout.runningY - PX(10));
        f32 sidebarWidth = PX(200);
        PushColumn(currentLayout.width - sidebarWidth); // could be better
            ShrinkHorizontal(PX(10));
            PaintLayout(Grey(0.2f));

            PushRow(PX(50));
                PaintLayout(Grey(0.3f));
                
                PushColumn(PX(50));
                    PaintLayout(Grey(0.5f));
                PopLayout();

                f32 textPadding = PX(20);
                currentLayout.runningX += textPadding;  // ugly
                PushColumn(GetTextWidth("Home") + textPadding * 2);
                    if(IsMouseOverLayout())
                        PaintLayout((V3f){0.4f, 0.0f, 0.4f});
                    PrintTextLineCentered("Home");
                PopLayout();

                PushColumn(GetTextWidth("Games") + textPadding * 2);
                    if(IsMouseOverLayout())
                        PaintLayout((V3f){0.4f, 0.0f, 0.4f});
                    PrintTextLineCentered("Games");
                PopLayout();

                PushColumn(GetTextWidth("Shop") + textPadding * 2);
                    if(IsMouseOverLayout())
                        PaintLayout((V3f){0.4f, 0.0f, 0.4f});
                    PrintTextLineCentered("Shop");
                PopLayout();

                f32 lastItemWidth = PX(50);
                currentLayout.runningX = currentLayout.width - lastItemWidth;  // ugly
                PushColumn(lastItemWidth);
                    PaintLayout(Grey(0.4f));
                PopLayout();

            PopLayout();

            currentLayout.runningY += PX(10);  // ugly

            PushRow(PX(50));
                PushColumn(PX(250));
                    PaintLayout(Grey(0.5f));
                PopLayout();

                currentLayout.runningX += PX(10);  // ugly

                PushColumn(currentLayout.width - currentLayout.runningX); // could be better
                    PaintLayout(Grey(0.5f));
                PopLayout();

            PopLayout();

        PopLayout();

        PushColumn(sidebarWidth - PX(10));
            PaintLayout(white);
        PopLayout();
    PopLayout();


    PushLayout(squarePos.x, squarePos.y, 50, 50);
        if(IsMouseOverLayout())
        {
            PaintLayout(white);
            isMouseOverSquare = 1;
        }
        else
        {
            PaintLayout(Grey(0.7f));
            isMouseOverSquare = 0;
        }
    PopLayout();

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
    InitConstants();

    // layout sizes depends on constants, constants depends on fonts, fonts on window initialization
    // thus I can't rely on WM_SIZE events during window creation, because fonts aren't there yet
    OnResize();


    file = ReadFileIntoDoubledSizedBuffer(PATH);
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


        Draw();
        SwapBuffers(dc);

        appTime += 16.66666f;

    }

    ExitProcess(0);
}

