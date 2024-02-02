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


i32 GetNewLineBefore(StringBuffer* buffer, i32 pos)
{
    for (int i = pos - 1; i >= 0; i--)
    {
        if (*(buffer->content + i) == '\n')
            return i;
    }
    return -1;
}

i32 GetNewLineAfter(StringBuffer* buffer, i32 pos)
{
    for (int i = pos; i < buffer->size; i++)
    {
        if (*(buffer->content + i) == '\n')
            return i;
    }
    return buffer->size;
}

void MoveCursorUp(StringBuffer* buffer, i32 isSelecting)
{
    TrackSelection(isSelecting);

    i32 prevNewLineIndex = GetNewLineBefore(buffer, cursor.cursorIndex);

    i32 prevPrevNewLineIndex = GetNewLineBefore(buffer, prevNewLineIndex);

    i32 currentOffset = cursor.cursorIndex - prevNewLineIndex;
    i32 targetIndex = prevPrevNewLineIndex + currentOffset + (cursor.desiredCol - cursor.col);

    UpdateCursorPosition(buffer, MinI32(targetIndex, MaxI32(prevNewLineIndex, 0)));
}

void MoveCursorDown(StringBuffer* buffer, i32 isSelecting)
{
    TrackSelection(isSelecting);

    i32 prevNewLineIndex = GetNewLineBefore(buffer, cursor.cursorIndex);
    
    i32 nextNewLineIndex = GetNewLineAfter(buffer, cursor.cursorIndex);

    i32 nextNextNewLineIndex = GetNewLineAfter(buffer, nextNewLineIndex + 1);

    i32 currentOffset = cursor.cursorIndex - prevNewLineIndex;
    i32 targetIndex = nextNewLineIndex + currentOffset + (cursor.desiredCol - cursor.col);
    UpdateCursorPosition(buffer, MinI32(targetIndex, nextNextNewLineIndex));
}

void MoveCursorLeft(StringBuffer* buffer, i32 isSelecting)
{
    TrackSelection(isSelecting);

    UpdateCursorPosition(buffer, MaxI32(cursor.cursorIndex - 1, 0));
    cursor.desiredCol = cursor.col;
}

void MoveCursorRight(StringBuffer* buffer, i32 isSelecting)
{
    TrackSelection(isSelecting);

    UpdateCursorPosition(buffer, MinI32(cursor.cursorIndex + 1, buffer->size));
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

void RemoveCharFromLeft(StringBuffer *buffer)
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
}

void InsertChartUnderCursor(StringBuffer* buffer, WPARAM ch)
{
    WPARAM code = ch == '\r' ? '\n' : ch;
    InsertCharAt(buffer, cursor.cursorIndex, code);
    UpdateCursorPosition(buffer, cursor.cursorIndex + 1);

    cursor.selectionStart = SELECTION_NONE;
}