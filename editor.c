#include "string.c"
#include "core.c"

i32 GetNewLineBefore(StringBuffer *buffer, i32 cursorIndex)
{
    for (int i = cursorIndex - 1; i >= 0; i--)
    {
        if (*(buffer->content + i) == '\n')
            return i;
    }
    return -1;
}

i32 GetNewLineAfter(StringBuffer *buffer, i32 cursorIndex)
{
    for (int i = cursorIndex; i < buffer->size; i++)
    {
        if (*(buffer->content + i) == '\n')
            return i;
    }
    return buffer->size;
}

i32 MoveCursorUp(StringBuffer *buffer, i32 cursorIndex)
{
    i32 prevNewLineIndex = GetNewLineBefore(buffer, cursorIndex);

    i32 prevPrevNewLineIndex = GetNewLineBefore(buffer, prevNewLineIndex);

    return MinI32(prevPrevNewLineIndex + (cursorIndex - prevNewLineIndex), MaxI32(prevNewLineIndex, 0));
}

i32 MoveCursorDown(StringBuffer *buffer, i32 cursorIndex)
{
    i32 prevNewLineIndex = GetNewLineBefore(buffer, cursorIndex);
    
    i32 nextNewLineIndex = GetNewLineAfter(buffer, cursorIndex);

    i32 nextNextNewLineIndex = GetNewLineAfter(buffer, nextNewLineIndex + 1);

    return MinI32(nextNewLineIndex + (cursorIndex - prevNewLineIndex), nextNextNewLineIndex);
}

i32 MoveCursorLeft(StringBuffer *buffer, i32 cursorIndex)
{
    return MaxI32(cursorIndex - 1, 0);
}

i32 MoveCursorRight(StringBuffer *buffer, i32 cursorIndex)
{
    return MinI32(cursorIndex + 1, buffer->size);
}


i32 RemoveCharFromLeft(StringBuffer *buffer, i32 cursorIndex)
{
    if(cursorIndex > 0)
    {
        RemoveCharAt(buffer, cursorIndex - 1);
        return cursorIndex - 1;
    }
    return cursorIndex;
}


i32 RemoveCurrentChar(StringBuffer *buffer, i32 cursorIndex)
{
    if(cursorIndex < buffer->size)
    {
        RemoveCharAt(buffer, cursorIndex);
        return cursorIndex;
    }
    return cursorIndex;
}