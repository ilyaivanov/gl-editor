#include "string.c"
#include "core.c"

typedef struct CursorState
{ 
    i32 cursorIndex;
    i32 col;
    i32 row;
    i32 desiredCol;

    i32 selectionStart;
} CursorState;

#define SELECTION_NONE -1
CursorState cursor = {.selectionStart = SELECTION_NONE};


i32 GetRowAtPosition(StringBuffer* buffer, i32 pos)
{
    i32 res = 0;
    for (i32 i = pos - 1; i >= 0; i--)
    {
        if (*(buffer->content + i) == '\n')
            res++;
    }
    return res;
}
i32 GetColAtPosition(StringBuffer* buffer, i32 pos)
{
    for (i32 i = pos - 1; i >= 0; i--)
    {
        if (*(buffer->content + i) == '\n')
            return pos - i - 1; 
    }
    return pos;
}

void UpdateCursorPosition(StringBuffer* buffer, i32 newIndex)
{
    cursor.cursorIndex = newIndex;

    cursor.col = -1; //GetColAtPosition(buffer, newIndex);
    cursor.row = 0;

    // There is definitelly a better way, compared to traversing the whole file
    // maybe remove col and row completelly and use cursor index for now
    for (int i = cursor.cursorIndex - 1; i >= 0; i--)
    {
        if (*(buffer->content + i) == '\n')
            cursor.row++;

        if (*(buffer->content + i) == '\n' && cursor.col == -1)
            cursor.col = cursor.cursorIndex - i - 1;
    }
    cursor.col = cursor.col == -1 ? cursor.cursorIndex : cursor.col;
}

void TrackSelection(i32 isSelecting)
{
    if(isSelecting && cursor.selectionStart == SELECTION_NONE)
        cursor.selectionStart = cursor.cursorIndex;
    else if (!isSelecting && cursor.selectionStart != SELECTION_NONE)
        cursor.selectionStart = SELECTION_NONE;
}

i32 GetSymbolIndexBeforeExclusive(StringBuffer* buffer, i32 pos, u8 symbol)
{
    for (int i = pos - 1; i >= 0; i--)
    {
        if (*(buffer->content + i) == symbol)
            return i;
    }
    return -1;
}
i32 GetSymbolIndexAfterInclusive(StringBuffer* buffer, i32 pos, u8 symbol)
{
    for (int i = pos; i < buffer->size; i++)
    {
        if (*(buffer->content + i) == symbol)
            return i;
    }
    return -1;
}

inline i32 GetNewLineBefore(StringBuffer* buffer, i32 pos)
{
    return GetSymbolIndexBeforeExclusive(buffer, pos, '\n');
}

i32 GetNewLineAfter(StringBuffer* buffer, i32 pos)
{
    i32 res = GetSymbolIndexAfterInclusive(buffer, pos, '\n');
    if(res == -1)
        return buffer->size;
    return res;
}

typedef enum CursorMovement
{
    Left, Right, Down, Up,
    LineEnd, LineStart,
    FileStart, FileEnd,
    PageDown, PageUp,
    WordJumpRight, WordJumpLeft,

} CursorMovement;


i32 MoveByLines(StringBuffer* buffer, i32 linesCountToMove)
{
    i32 direction = linesCountToMove < 0 ? -1 : 1;

    //increasing by one because I don't want to count current line offset of going backwards
    if(linesCountToMove < 0)
    {
        linesCountToMove -= 1;

        //when cursor is looking at new line, that char visually is the next line
        if(*(buffer->content + cursor.cursorIndex) == '\n')
            linesCountToMove -= 1;
    }

    linesCountToMove = AbsI32(linesCountToMove);

    i32 targetLineIndex = cursor.cursorIndex;
    while(targetLineIndex >= 0 && targetLineIndex <= buffer->size)
    {
        if(*(buffer->content + targetLineIndex) == '\n')
        {
            linesCountToMove--;
            if(linesCountToMove == 0)
                break;
        }

        targetLineIndex += direction;
    }

    i32 targetLineEnd = GetNewLineAfter(buffer, targetLineIndex + 1);
    
    if(targetLineEnd == -1)
        targetLineEnd = buffer->size;  

    i32 currentOffset = cursor.cursorIndex - GetNewLineBefore(buffer, cursor.cursorIndex);

    i32 targetIndex = targetLineIndex + currentOffset + (cursor.desiredCol - cursor.col);

    return MinI32(targetIndex, targetLineEnd);
}

#define IsWhiteSpace(ch) (ch == ' ' || ch == '\n')

i32 JumpWordRight(StringBuffer* buffer)
{
    i32 isInsideSpace = 0;
    for (int i = cursor.cursorIndex; i < buffer->size; i++)
    {
        u8 ch = *(buffer->content + i);
        if (IsWhiteSpace(ch))
            isInsideSpace = 1;
        
        // return first non-space char after space section
        if (!IsWhiteSpace(ch) && isInsideSpace)
            return i;
    }
    return buffer->size;
}

i32 JumpWordLeft(StringBuffer* buffer)
{
    i32 isInsideSpace = 0;
    for (int i = cursor.cursorIndex - 1; i >=0; i--)
    {
        u8 ch = *(buffer->content + i);
        if (IsWhiteSpace(ch))
            isInsideSpace = 1;
        
        // return first non-space char after space section
        if (!IsWhiteSpace(ch) && isInsideSpace)
            return i;
    }
    return buffer->size;
}

void MoveCursor(StringBuffer* buffer, CursorMovement movement, i32 isSelecting, f32 pageHeight)
{
    TrackSelection(isSelecting);
    i32 nextCursor = 0;

    if(movement == Left)
        nextCursor = cursor.cursorIndex - 1;
    else if (movement == Right)
        nextCursor = cursor.cursorIndex + 1;
    else if (movement == Up)
        nextCursor = MoveByLines(buffer, -1);
    else if (movement == Down)
        nextCursor = MoveByLines(buffer, 1);
    else if (movement == PageDown)
        nextCursor = MoveByLines(buffer, pageHeight);
    else if (movement == PageUp)
        nextCursor = MoveByLines(buffer, -pageHeight);
    else if (movement == LineEnd)
        nextCursor = GetNewLineAfter(buffer, cursor.cursorIndex);
    else if (movement == LineStart)
        nextCursor = GetNewLineBefore(buffer, cursor.cursorIndex) + 1;
    else if (movement == WordJumpRight)
        nextCursor = JumpWordRight(buffer);
    else if (movement == WordJumpLeft)
        nextCursor = JumpWordLeft(buffer);
    else if (movement == FileEnd)
        nextCursor = buffer->size;
    else if (movement == FileStart)
        nextCursor = 0;

    
    UpdateCursorPosition(buffer, ClampI32(nextCursor, 0, buffer->size));
    
    if(movement != Up && movement != Down && movement != PageDown && movement != PageUp)
        cursor.desiredCol = cursor.col;
}

void RemoveSelection(StringBuffer *buffer)
{
    i32 from = MinI32(cursor.selectionStart, cursor.cursorIndex);
    i32 to   = MaxI32(cursor.selectionStart, cursor.cursorIndex);
    RemoveBufferSegment(buffer, to, from);
    cursor.selectionStart = SELECTION_NONE;
    UpdateCursorPosition(buffer, from);
}

void RemoveCharFromLeft(StringBuffer *buffer)\
{
    if (cursor.selectionStart != SELECTION_NONE)
    {
        RemoveSelection(buffer);
    }
    else
    {
        if (cursor.cursorIndex > 0)
        {
            RemoveCharAt(buffer, cursor.cursorIndex - 1);
            UpdateCursorPosition(buffer, cursor.cursorIndex - 1);
            cursor.desiredCol = cursor.col;

            cursor.selectionStart = SELECTION_NONE;
        }
    }

    cursor.desiredCol = cursor.col;
}

void RemoveCurrentChar(StringBuffer* buffer)
{
    if (cursor.selectionStart != SELECTION_NONE)
    {
        RemoveSelection(buffer);
    }
    else 
    {
        if(cursor.cursorIndex < buffer->size)
        {
            RemoveCharAt(buffer, cursor.cursorIndex);

            cursor.selectionStart = SELECTION_NONE;
        }
    }

    cursor.desiredCol = cursor.col;
}

void InsertChartUnderCursor(StringBuffer* buffer, WPARAM ch)
{
    WPARAM code = ch == '\r' ? '\n' : ch;
    InsertCharAt(buffer, cursor.cursorIndex, code);
    UpdateCursorPosition(buffer, cursor.cursorIndex + 1);

    cursor.selectionStart = SELECTION_NONE;
}
