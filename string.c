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

inline void PlaceLineEnd(StringBuffer *buffer)
{
    if(buffer->content)
        *(buffer->content + buffer->size) = '\0';
}

void DoubleCapacityIfFull(StringBuffer *buffer)
{
    if(buffer->size >= buffer->capacity)
    {
        char *currentStr = buffer->content;
        buffer->capacity = (buffer->capacity == 0) ? 4 : (buffer->capacity * 2);
        buffer->content = VirtualAllocateMemory(buffer->capacity);
        MoveMyMemory(currentStr, buffer->content, buffer->size);
        VirtualFreeMemory(currentStr);
    }
}


void InsertCharAt(StringBuffer *buffer, i32 at, i32 ch)
{
    DoubleCapacityIfFull(buffer);

    buffer->size += 1;
    MoveBytesRight(buffer->content + at, buffer->size - at);
    *(buffer->content + at) = ch;
    PlaceLineEnd(buffer);
}

void AppendStr(StringBuffer* buffer, char* str)
{
    while(*str)
    {
        DoubleCapacityIfFull(buffer);

        *(buffer->content + buffer->size) = *str;
        buffer->size++;

        str++;
    }

    DoubleCapacityIfFull(buffer);
    PlaceLineEnd(buffer);
}

void BufferClear(StringBuffer* buffer)
{
    buffer->size = 0;
}


void RemoveCharAt(StringBuffer *buffer, i32 at)
{
    MoveBytesLeft(buffer->content + at, buffer->size - at);
    buffer->size--;
    PlaceLineEnd(buffer);
}

void RemoveBufferSegment(StringBuffer *buffer, i32 from, i32 to)
{
    char * dest = buffer->content + to;
    char * source = buffer->content + from;
    for(int i = from; i < buffer->size; i++)
    {
        *dest = *source;
        dest++;
        source++;
    }

    i32 bytesRemoved = from - to;
    buffer->size -= bytesRemoved;
    PlaceLineEnd(buffer);
}


i32 IsStrEqualToStr(char* str1, char* str2)
{
    while(*str1 && *str2)
    {
        if(*str1 != *str2)
            return 0;

        str1++;
        str2++;
    }
    if(*str1 != *str2)  
        return 0;

    return 1;
}

i32 IsWStrEqualToWStr(wchar_t* str1, wchar_t* str2)
{
    while(*str1 && *str2)
    {
        if(*str1 != *str2)
            return 0;
            
        str1++;
        str2++;
    }

    return 1;
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

    PlaceLineEnd(&res);
    return res;
}


StringBuffer InitStringBufferWithCapacityFactor(char *content, i32 mult)
{
    StringBuffer res = {0}; 

    i32 size = 0;
    u8* ch = content;
    while(*ch)
    {
        size++;
        ch++;    
    }
    res.size = size;
    res.capacity = size * mult;
    res.content = VirtualAllocateMemory(res.capacity);

    MoveMyMemory(content, res.content, size);
    PlaceLineEnd(&res);
    return res;
}


inline StringBuffer EmptyStringBuffer()
{
    return InitStringBufferWithCapacityFactor("", 1);
}

inline StringBuffer InitStringBufferWithDoubledCapacity(char *content)
{
    return InitStringBufferWithCapacityFactor(content, 2);
}

inline StringBuffer InitStringBufferSameCapacity(char *content)
{
    return InitStringBufferWithCapacityFactor(content, 1);
}


#endif
