#ifndef APP_C
#define APP_C

#include <gl/gl.h>
#include "opengl/glFunctions.c"
#include "core.c"
#include "string.c"
#include "font.c"
#include "layout.c"

typedef struct UiState
{
    i32 isModalShown;
    i32 isFullscreen;
    V2i screen;
    Layout codeLayout;

    
    FontData codeFont;
    FontData codeFontClear;
    FontData codeFontSelected;
    FontData lineNumbers;
    FontData selectedLineNumbers;


    GLuint vertexBuffer;
    GLuint vertexArray;

    f32 scale;
} UiState;


typedef struct ModalState
{
    i32 isShown;
    StringBuffer searchTerm;
    StringBuffer files[500];
    i32 filesCount;
    i32 selectedFileIndex;
    f32 modalOffset;
} ModalState;

typedef struct AppState
{
    i32 isRunning;
    V2i mouse;


    StringBuffer filePath;
    StringBuffer file;

    UiState uiState;

    ModalState modal;
} AppState;

#endif