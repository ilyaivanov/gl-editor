#ifndef STRING_C
#define STRING_C
#include "core.c"
#include "win32.c"


typedef struct StringBuffer {
    char *content;
    i32 size;
    i32 capacity;
} StringBuffer;



inline void MoveBytesLeft(char *ptr, int length) 
{
    for (int i = 0; i < length - 1; i++) {
        ptr[i] = ptr[i + 1];
    }
}

inline void MoveBytesRight(char *ptr, int length) 
{
    for (int i = length - 1; i > 0; i--) {
        ptr[i] = ptr[i - 1];
    }
}

inline void MoveMyMemory(char *source, char *dest, int length) 
{
    for (int i = 0; i < length; i++)
    {
        *dest = *source;
        source++;
        dest++;
    }
}

void InsertCharAt(StringBuffer *buffer, i32 at, i32 ch)
{
    if(buffer->size >= buffer->capacity)
    {
        char *currentStr = buffer->content;
        buffer->capacity *= 2;
        buffer->content = VirtualAllocateMemory(buffer->capacity);
        MoveMyMemory(currentStr, buffer->content, buffer->size);
        VirtualFreeMemory(currentStr);
    }

    buffer->size += 1;
    MoveBytesRight(buffer->content + at, buffer->size - at);
    *(buffer->content + at) = ch;
}


void RemoveCharAt(StringBuffer *buffer, i32 at)
{
    MoveBytesLeft(buffer->content + at, buffer->size - at);
    buffer->size--;
}


StringBuffer ReadFileIntoDoubledSizedBuffer(char *path)
{
    u32 fileSize = GetMyFileSize(path);
    StringBuffer res = {
        .capacity = fileSize * 2,
        .size = fileSize,
        .content = 0
    }; 
    res.content = VirtualAllocateMemory(res.capacity);
    ReadFileInto(path, fileSize, res.content);

    //removing windows new lines delimeters, assuming no two CR are next to each other
    for(int i = 0; i < fileSize; i++){
        if(*(res.content + i) == '\r')
            RemoveCharAt(&res, i);
    }

    return res;
}


// Ugly function taken from the ChatFuckYouT
// Will need to rewrite this with custom string type which has a length
void ReverseString(char *str) {
    if (str == NULL)
        return;

    int length = 0;
    while (str[length] != '\0') {
        length++;
    }

    int start = 0;
    int end = length - 1;
    char temp;

    while (start < end) {
        // Swap characters at start and end indices
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;

        // Move towards the center
        start++;
        end--;
    }
}

#endif
