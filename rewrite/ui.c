#include <windows.h>
#include <gl/gl.h>
#include "../opengl/glFunctions.c"
#include "../opengl/openglProgram.c"

#include "../core.c"
#include "../layout.c"
#include "../string.c"
#include "../font.c"
#include "../colors.c"
#include "../scrollbar.c"



typedef struct UiState
{
    V2i screen;
    Layout codeLayout;

    
    FontData codeFont;
    FontData lineNumbers;
    FontData selectedLineNumbers;


    GLuint vertexBuffer;
    GLuint vertexArray;

    f32 scale;
} UiState;

// Global state
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

void InitUi(UiState* state)
{
    FontInfo info = {.size = 14, .name = "Consolas"};
    InitFont(&info, &state->codeFont, textColorHex, bgColorHex);
    InitFont(&info, &state->lineNumbers, lineColorHex, bgColorHex);
    InitFont(&info, &state->selectedLineNumbers, selectedLineColorHex, bgColorHex);

    textProgram = CreateProgram("..\\shaders\\base.vs", "..\\shaders\\base.fs");
    primitivesProgram = CreateProgram("..\\shaders\\primitives.vs", "..\\shaders\\primitives.fs");

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
}


void RebuiltUiState(UiState* state, StringBuffer buffer)
{
    // I only need here is the pageHeight + padding
    // OffsetY based on the height changes
}



//
//
// Drawing
//
//

// Global state is assigned in Draw function, ugly, but allows me to avoid passing pointers everywhere, which makes code cleaner
const UiState* ui;

#define PX(val) ((val) * ui->scale)

void ChangeProgram(GLuint program)
{
    UseProgram(program);
    SetMat4("projection", CreateViewMatrix(-1, -1, 2.0f / (f32)ui->screen.x, 2.0f / (f32)ui->screen.y));
}

#define DrawingShapes() ChangeProgram(primitivesProgram)
#define DrawingText()   ChangeProgram(textProgram)


void PaintLayout(V4f color)
{
    SetV4f("color", color);
    SetMat4("view", CreateViewMatrix(currentLayout.x, currentLayout.y, currentLayout.width, currentLayout.height));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
}


inline f32 DrawChar(f32 x, f32 y, u8 ch)
{
    MyBitmap bitmap = currentFont->textures[ch];
    SetMat4("view", CreateViewMatrix(x, y, bitmap.width, bitmap.height));
    glBindTexture(GL_TEXTURE_2D, currentFont->cachedTextures[ch]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
    return bitmap.width;
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
        
        DrawChar(x, y, ch);

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

void PrintParagraphLeftTopMonospaced(const StringBuffer* buffer)
{
    DrawingText();
    currentFont = &ui->codeFont;
    
    f32 offsetForLineNumbers = PX(40);
    LayoutMoveRight(offsetForLineNumbers);

    f32 startX = currentLayout.x + currentLayout.runningX;
    f32 startY = currentLayout.y - currentLayout.runningY + currentLayout.offsetY + currentLayout.height - (f32)currentFont->textMetric.tmHeight;

    f32 x = startX;
    f32 y = startY;
    i32 lineNumber = 1;
    i32 hasEnteredNewLine = 1;

    i32 selectionFrom = MinI32(cursor.selectionStart, cursor.cursorIndex);
    i32 selectionTo   = MaxI32(cursor.selectionStart, cursor.cursorIndex);

    for (i32 i = 0; i < buffer->size; i++)
    {
        u8 ch = *(buffer->content + i);

        if(ch == '\n')
        {
            if (i == cursor.cursorIndex)
                DrawCursor(x, y);

            if(cursor.selectionStart != SELECTION_NONE && i >= selectionFrom && i < selectionTo)
                DrawSelectionBackground(x, y, currentFont->textures[' '].width);

            y -= currentFont->textMetric.tmHeight;
    
            currentLayout.runningY += currentFont->textMetric.tmHeight;
            x = startX;
            hasEnteredNewLine = 1;
            lineNumber++;
        }

        if(hasEnteredNewLine)
        {
            if((lineNumber - 1) == cursor.row)
                currentFont = &ui->selectedLineNumbers;
            else 
                currentFont = &ui->lineNumbers;

            DrawNumberRightTop(x, y, lineNumber);
            hasEnteredNewLine = 0;
            
            currentFont = &ui->codeFont;
        }


        if (ch != '\n')
        {
            f32 charWidth = DrawChar(x, y, ch);

            if (cursor.selectionStart != SELECTION_NONE && i >= selectionFrom && i < selectionTo)
                DrawSelectionBackground(x, y, charWidth);

            if (i == cursor.cursorIndex)
                DrawCursor(x, y);
            x += charWidth;
        }
    }


    // special case for an empty cursor at the end of the file
    if (cursor.cursorIndex == buffer->size)
        DrawCursor(x, y);
    DrawingShapes();
}




void Draw(const UiState* state, const StringBuffer* buffer)
{
    ui = state;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawingShapes();
    ResetLayout(state->screen);

    PushPersistedLayout(state->codeLayout);


    PrintParagraphLeftTopMonospaced(buffer);
    
    
    SetV4f("color", (V4f){1, 1, 1, 1});
    SetMat4("view", DrawScrollbar(&currentLayout, PX(10)));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ArrayLength(vertices) / FLOATS_PER_VERTEX);
}


void UpdateUi(UiState* state, StringBuffer* buffer)
{
    state->codeLayout.pageHeight = 0;
    for (i32 i = 0; i <= buffer->size; i++)
    {
        u8 ch = *(buffer->content + i);
        if (ch == '\n')
            state->codeLayout.pageHeight += state->codeFont.textMetric.tmHeight;
    }
    state->codeLayout.pageHeight += state->codeFont.textMetric.tmHeight;
}