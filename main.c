#include <windows.h>
#include <gl/gl.h>
#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"
#include "font.c"
#include "ui.c"
#include "string.c"

float SYSTEM_SCALE = 1;
#define PX(val) ((val) * SYSTEM_SCALE)

#define HexColor(hex) {(float)((hex >> 16) & 0xff) / 255.0f, \
                       (float)((hex >>  8) & 0xff) / 255.0f, \
                       (float)((hex >>  0) & 0xff) / 255.0f}


i32 isRunning = 1;
V2i clientAreaSize = {0};

Layout mainLayout = {0};

f32 zDeltaThisFrame;


i32 cursorIndex = 0;

i32 isEditMode = 0;
StringBuffer file;

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
        if(wParam == VK_DOWN)
        {
            i32 prevNewLineIndex = -1;
            for(int i = cursorIndex - 1; i >= 0; i--)
            {
                if(*(file.content + i) == '\n')
                {
                    prevNewLineIndex = i;
                    break;
                }
            }

            i32 nextNewLineIndex = 0;
            for(int i = cursorIndex; i < file.size; i++)
            {
                if(*(file.content + i) == '\n')
                {
                    nextNewLineIndex = i;
                    break;
                }
            }

            i32 currentOffset = (cursorIndex - prevNewLineIndex);
            
            cursorIndex = nextNewLineIndex + currentOffset;
        }
        else if(wParam == VK_UP)
        {
            i32 prevNewLineIndex = -1;
            for(int i = cursorIndex - 1; i >= 0; i--)
            {
                if(*(file.content + i) == '\n')
                {
                    prevNewLineIndex = i;
                    break;
                }
            }

            i32 prevPrevNewLineIndex = -1;
            for(int i = prevNewLineIndex - 1; i >= 0; i--)
            {
                if(*(file.content + i) == '\n')
                {
                    prevPrevNewLineIndex = i;
                    break;
                }
            }

            i32 currentOffset = (cursorIndex - prevNewLineIndex);
            
            cursorIndex = prevPrevNewLineIndex + currentOffset;
        }

        else if(wParam == VK_LEFT)
        {
            cursorIndex -= 1;
        }
        else if(wParam == VK_RIGHT)
        {
            cursorIndex += 1;
        }
        else if (wParam == VK_BACK)
        {
            if(cursorIndex > 0)
            {
                RemoveCharAt(&file, cursorIndex - 1);
                cursorIndex -= 1;
            }
        }
        
    }
    else if (message == WM_KEYUP)
    {
    }
    else if (message == WM_MOUSEWHEEL)
    {
        zDeltaThisFrame += (float)GET_WHEEL_DELTA_WPARAM(wParam);
    }
    else if (message == WM_CHAR)
    {
        if(wParam >= ' ' || wParam == '\r')
        {
            WPARAM code = wParam == '\r' ? '\n' : wParam;
            InsertCharAt(&file, cursorIndex, code);
            cursorIndex++;
        }
    }

    return DefWindowProc(window, message, wParam, lParam);
}


// https://github.com/ayu-theme/ayu-vim/blob/master/colors/ayu.vim


// V3f bgColor      = HexColor(0x0F1419);
V3f bgColor           = HexColor(0x030303);

V3f cursorColor       = HexColor(0xA011A0);
V3f textColor         = HexColor(0xDDDDDD);
V3f selectedTextColor = HexColor(0xEEEEEE);

V3f macroColor        = HexColor(0xC686BD);
V3f keywordColor      = HexColor(0x5E99D2);
V3f stringColor       = HexColor(0xCC937B);

GLuint vertexBuffer;
GLuint vertexArray;
GLuint primitivesProgram;
GLuint textProgram;

#define FLOATS_PER_VERTEX 4
float vertices[] = {
    //Position      UV coords
    1.0f, 0.0f,    1.0f, 0.0f,
    0.0f, 0.0f,    0.0f, 0.0f,
    1.0f, 1.0f,    1.0f, 1.0f,
    0.0f, 1.0f,    0.0f, 1.0f
};


void Draw()
{
    if(mainLayout.pageHeight > mainLayout.height)
        mainLayout.offsetY = Clampf32(mainLayout.offsetY + zDeltaThisFrame, -(mainLayout.pageHeight - mainLayout.height), 0);

    zDeltaThisFrame = 0;

    

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

    f32 padding = 20.0f;
    currentFont = &codeFont;


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // cache these for performance
    i32 cursorCol = -1;
    i32 cursorRow = 0;

    for(int i = cursorIndex - 1; i >= 0; i--)
    {
        if(*(file.content + i) == '\n')
            cursorRow++;

        if(*(file.content + i) == '\n' && cursorCol == -1)
        {
            cursorCol = cursorIndex - i - 1;
        }
    }
    cursorCol = cursorCol == -1 ? cursorIndex : cursorCol;

    UseProgram(primitivesProgram);
    Mat4 cursorView = 
    {
        currentFont->textures['W'].width, 0, 0, padding + cursorCol * currentFont->textures['W'].width,
        0, currentFont->textMetric.tmHeight, 0, mainLayout.height - padding - (cursorRow + 1) * currentFont->textMetric.tmHeight,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    SetV3f("color", cursorColor);
    SetMat4("view", cursorView);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);



    UseProgram(textProgram);
    SetMat4("projection", projection);
    SetV3f("color", textColor);


    f32 startY = clientAreaSize.y - currentFont->textMetric.tmHeight - padding;

    f32 runningX = padding;
    f32 runningY = startY - mainLayout.offsetY;
    i32 currentRow = 0;
    i32 currentCol = 0;
    for(i32 i = 0; i < file.size; i++)
    {
        u8 code = *(file.content + i);
        u8 nextCode = *(file.content + i + 1);

        if(code == '\n')
        {
            currentRow += 1;
            currentCol = 0;
            runningX = padding;
            runningY -= currentFont->textMetric.tmHeight;
        }
        else if(code >= ' ') 
        {

            MyBitmap bitmap = currentFont->textures[code];
            
            Mat4 view = 
            {
                bitmap.width, 0, 0, runningX,
                0, bitmap.height, 0, runningY,
                0, 0, 1, 0,
                0, 0, 0, 1,
            };
            SetMat4("view", view);
            if (currentRow == cursorRow && currentCol == cursorCol)
                SetV3f("color", selectedTextColor);
            else
                SetV3f("color", textColor);

            glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[code]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

            //TODO monospaced fonts have no kerning, I can figure if font is monospaced and skip kerning lookup
            runningX += bitmap.width + GetKerningValue(code, nextCode);
            currentCol += 1;
        }
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

    file = ReadFileIntoDoubledSizedBuffer("..\\sample.txt");
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

    }

    ExitProcess(0);
}
