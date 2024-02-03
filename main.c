#include <windows.h>
#include <gl/gl.h>

#include "win32.c"
#include "opengl/glFunctions.c"
#include "opengl/openglProgram.c"
#include "font.c"
#include "ui.c"
#include "string.c"


// this is used by editor for PageDown\PageUp to determine how many rows there are in the page
// interesting to think about how to solve this
Layout mainLayout = {0};

FontInfo uiFontInfo   = {.size = 12, .name = "Segoe UI"};
FontData* uiFont;

FontInfo codeFontInfo = {.size = 14, .name = "Consolas"};
FontData* codeFont;

#include "editor.c"

float SYSTEM_SCALE = 1;
#define PX(val) ((val) * SYSTEM_SCALE)

#define HexColor(hex) {(float)((hex >> 16) & 0xff) / 255.0f, \
                       (float)((hex >>  8) & 0xff) / 255.0f, \
                       (float)((hex >>  0) & 0xff) / 255.0f}
#define IsKeyPressed(key) (GetKeyState(key) & 0b10000000)

i32 isRunning = 1;
V2i clientAreaSize = {0};

Layout footer = {0};

f32 zDeltaThisFrame;

#define PATH "..\\string.c"

i32 isEditMode = 0;
StringBuffer file;

f32 padding;
f32 spaceForLineNumbers;
f32 lineNumberToCode;
f32 footerHeight;
f32 footerPadding;



void Draw();


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

    mainLayout.height = clientAreaSize.y - footerHeight;
    mainLayout.width = clientAreaSize.x;
    mainLayout.y = footerHeight;

    footer.x = 0;
    footer.y = 0;
    footer.height = footerHeight;
    footer.width = clientAreaSize.x;
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
            MoveCursor(&file, Down, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_UP)
            MoveCursor(&file, Up, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_LEFT && IsKeyPressed(VK_CONTROL))
            MoveCursor(&file, WordJumpLeft, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_LEFT)
            MoveCursor(&file, Left, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_RIGHT && IsKeyPressed(VK_CONTROL))
            MoveCursor(&file, WordJumpRight, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_RIGHT)
            MoveCursor(&file, Right, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_END && IsKeyPressed(VK_CONTROL))
            MoveCursor(&file, FileEnd, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_HOME && IsKeyPressed(VK_CONTROL))
            MoveCursor(&file, FileStart, IsKeyPressed(VK_SHIFT)); 
        else if(wParam == VK_END)
            MoveCursor(&file, LineEnd, IsKeyPressed(VK_SHIFT));
        else if(wParam == VK_HOME)
            MoveCursor(&file, LineStart, IsKeyPressed(VK_SHIFT));       
        else if(wParam == VK_NEXT)
            MoveCursor(&file, PageDown, IsKeyPressed(VK_SHIFT));            
        else if(wParam == VK_PRIOR)
            MoveCursor(&file, PageUp, IsKeyPressed(VK_SHIFT));            
        else if (wParam == VK_BACK)
            RemoveCharFromLeft(&file);
        else if (wParam == VK_DELETE)
            RemoveCurrentChar(&file);
        else if (wParam == 'S' && IsKeyPressed(VK_CONTROL))
            WriteMyFile(PATH, file.content, file.size);

        f32 rowHeight = codeFont->textMetric.tmHeight;
        f32 cursorPos = cursor.row * rowHeight + padding;

        if(cursorPos + mainLayout.offsetY + rowHeight * 5 > mainLayout.height)
            mainLayout.offsetY = -(cursorPos - mainLayout.height + rowHeight * 5);

        if(cursorPos - rowHeight * 5 < -mainLayout.offsetY)
            mainLayout.offsetY = -(cursorPos - rowHeight * 5);

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
            InsertChartUnderCursor(&file, wParam);
    }

    return DefWindowProc(window, message, wParam, lParam);
}


// https://github.com/ayu-theme/ayu-vim/blob/master/colors/ayu.vim
// V3f bgColor      = HexColor(0x0F1419);
V3f bgColor           = HexColor(0x030303);
V3f footerColor       = HexColor(0x181818);

V3f cursorColor       = HexColor(0xA011A0);
V3f selectionColor    = HexColor(0x601160);
V3f textColor         = HexColor(0xDDDDDD);
V3f uiFadedColor      = HexColor(0x888888);
V3f lineNumberColor   = HexColor(0x666666);
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

void FormatNumber(i32 val, char *buff)
{
    while(val != 0) 
    {
        *buff = '0' + val % 10;
        val /= 10;
        buff++;
    }
    *buff = '\0';
    ReverseString(buff);
}

void DrawTextBottomRight(float x, float y, char *text, V3f color)
{
    while(*text)
    {
        MyBitmap bitmap2 = currentFont->textures[*text];
        x -= bitmap2.width;

        SetMat4("view", CreateViewMatrix(x, y, bitmap2.width, bitmap2.height));
        SetV3f("color", color);
        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[*text]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        text++;
    }
}



void DrawSquareAt(i32 row, i32 col, V3f color)
{
    Mat4 cursorView = CreateViewMatrix(
        /*x*/ spaceForLineNumbers + col * currentFont->textures['W'].width,
        /*y*/ mainLayout.height + mainLayout.y - padding - (row + 1) * currentFont->textMetric.tmHeight - mainLayout.offsetY,
        /*w*/ currentFont->textures['W'].width,
        /*h*/ currentFont->textMetric.tmHeight
    );
    SetV3f("color", color);
    SetMat4("view", cursorView);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
}

void DrawCursor(StringBuffer *buffer)
{
    if(cursor.selectionStart > -1)
    {
        i32 from = MinI32(cursor.selectionStart, cursor.cursorIndex);
        i32 to   = MaxI32(cursor.selectionStart, cursor.cursorIndex);
        
        i32 row = GetRowAtPosition(buffer, from);
        i32 col = GetColAtPosition(buffer, from);

        for(int i = from; i <= to; i++)
        {
            DrawSquareAt(row, col, selectionColor);
            
            if(*(buffer->content + i) == '\n')
            {
                row++;
                col = 0;
            }
            else 
                col++;
        }
    }

    DrawSquareAt(cursor.row, cursor.col, cursorColor);
}

void DrawFooter()
{
    
    UseProgram(primitivesProgram);
    Mat4 projection = CreateViewMatrix(-1, -1, 2.0f / (f32)clientAreaSize.x, 2.0f / (f32)clientAreaSize.y);
    SetMat4("projection", projection);

    SetV3f("color", footerColor);
    SetMat4("view", CreateViewMatrix(footer.x, footer.y, footer.width, footer.height));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);



    UseProgram(textProgram);
    SetMat4("projection", projection);
    SetV3f("color", uiFadedColor);

    currentFont = uiFont;
    u8* label = PATH;
    f32 x = footerPadding;
    f32 y = footer.height / 2 - currentFont->textMetric.tmHeight / 2;

    while(*label)
    {
        u8 code = *(label);
        u8 nextCode = *(label + 1);

        MyBitmap bitmap = currentFont->textures[code];
        
        SetMat4("view", CreateViewMatrix(x, y, bitmap.width, bitmap.height));

        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[code]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        x += bitmap.width + GetKerningValue(code, nextCode);

        label++;
    }

    if(1)
    {
        u8 asterisk = '*';
        MyBitmap bitmap = currentFont->textures[asterisk];

        SetMat4("view", CreateViewMatrix(x, y, bitmap.width, bitmap.height));

        glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[asterisk]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
    }
}

void DrawModal()
{
    UseProgram(primitivesProgram);

    f32 borderSize = PX(10);
    f32 modalWidth = PX(200);
    f32 modalHeight = PX(300);
    f32 modalTopMargin = PX(20);



    f32 x = mainLayout.width / 2 - modalWidth / 2;
    SetV3f("color", (V3f){1.0f, 1.0f, 1.0f});
    SetMat4("view", CreateViewMatrix(
        x, 
        mainLayout.y + mainLayout.height - modalTopMargin - modalHeight, 
        modalWidth, modalHeight));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

    SetV3f("color", (V3f){0.1f, 0.1f, 0.1f});
    f32 modalLeft = x + borderSize;
    f32 modalBottom = mainLayout.y + mainLayout.height - modalTopMargin - modalHeight + borderSize;
    SetMat4("view", CreateViewMatrix(
        modalLeft, modalBottom,
        modalWidth - borderSize * 2, modalHeight - borderSize * 2));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);


    f32 pointSize = PX(20);
    f32 runningY = modalBottom + modalHeight - PX(10) - borderSize - pointSize;
    f32 runningX = modalLeft + PX(10);
    SetV3f("color", (V3f){0.8f, 0.8f, 0.8f});
    for (int i = 0; i < 10; i++)
    {
        SetMat4("view", CreateViewMatrix(runningX, runningY, pointSize, pointSize));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

        runningY -= PX(50);
    }
}

void Draw()
{
    if(mainLayout.pageHeight > mainLayout.height)
        mainLayout.offsetY = Clampf32(mainLayout.offsetY + zDeltaThisFrame, -(mainLayout.pageHeight - mainLayout.height), 0);

    zDeltaThisFrame = 0;

    
    // allows me to set vecrtex coords as 0..width/height, instead of -1..+1
    // 0,0 is bottom left, not top left
    // matrix in code != matrix in math notation, details at https://youtu.be/kBuaCqaCYwE?t=3084
    // in short: rows in math are columns in code
    Mat4 projection = CreateViewMatrix(-1, -1, 2.0f / (f32)clientAreaSize.x, 2.0f / (f32)clientAreaSize.y);
    
    currentFont = codeFont;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   
    glEnable(GL_SCISSOR_TEST);
    glScissor(mainLayout.x, mainLayout.y, mainLayout.width, mainLayout.height);


    UseProgram(primitivesProgram);
    SetMat4("projection", projection);

        
    // SetV3f("color", (V3f){0.1f, 0.2f, 0.1f});
    // SetMat4("view", CreateViewMatrix(mainLayout.x, mainLayout.y, mainLayout.width, mainLayout.height));
    // glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

    DrawCursor(&file);
    // SetV3f("color", (V3f){0.5f, 0.3f, 0.3f});
    // SetMat4("view", CreateViewMatrix(footer.x, footer.y, footer.width, footer.height));
    // glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);


    UseProgram(textProgram);
    SetMat4("projection", projection);


    f32 startY = mainLayout.height + mainLayout.y - currentFont->textMetric.tmHeight - padding;

    f32 runningX = spaceForLineNumbers;
    f32 runningY = startY - mainLayout.offsetY;
    i32 currentRow = 0;
    i32 currentCol = 0;
    u8 lineNumberBuff[26];

    for(i32 i = 0; i < file.size; i++)
    {
        u8 code = *(file.content + i);
        u8 nextCode = *(file.content + i + 1);

        if(code == '\n')
        {
            FormatNumber(currentRow + 1, lineNumberBuff);
            DrawTextBottomRight(spaceForLineNumbers - lineNumberToCode, runningY, lineNumberBuff, lineNumberColor);

            currentRow += 1;
            currentCol = 0;
            runningX = spaceForLineNumbers;
            runningY -= currentFont->textMetric.tmHeight;
        }
        else if(code >= ' ') 
        {

            MyBitmap bitmap = currentFont->textures[code];
            
            SetMat4("view", CreateViewMatrix(runningX, runningY, bitmap.width, bitmap.height));
            if (i == cursor.cursorIndex)
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

    FormatNumber(currentRow + 1, lineNumberBuff);
    DrawTextBottomRight(spaceForLineNumbers - lineNumberToCode, runningY, lineNumberBuff, lineNumberColor);
    
    glDisable(GL_SCISSOR_TEST);
    
    runningY -= (currentFont->textMetric.tmHeight + padding);

    mainLayout.pageHeight = startY - runningY - mainLayout.offsetY + padding;

    UseProgram(primitivesProgram);
    Mat4 scrollView = DrawScrollbar(&mainLayout, PX(10));
    SetMat4("projection", projection);
    SetV3f("color", (V3f){0.3f, 0.3f, 0.3f});
    SetMat4("view", scrollView);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);

    DrawFooter();
    DrawModal();
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

    }

    ExitProcess(0);
}
