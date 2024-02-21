#ifndef ARENA_C
#define ARENA_C

#include "core.c"

typedef struct Arena
{
    u8* start;
    u32 bytesAllocated;
    u32 size;
} Arena;


inline Arena CreateArena(u32 size)
{
    return (Arena){
        .size = size,
        .bytesAllocated = 0,
        .start = VirtualAllocateMemory(size)
    };
}


u8* ArenaPush(Arena* arena, u32 size)
{
    u8* res = arena->start + arena->bytesAllocated;
    arena->bytesAllocated += size;
    return res;
}

void ArenaClear(Arena* arena)
{
    arena->bytesAllocated = 0;
}


#endif 