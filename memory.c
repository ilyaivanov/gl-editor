#ifndef MEMORY_C
#define MEMORY_C

#include <windows.h>

inline void *VirtualAllocateMemory(size_t size)
{
     return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
};

inline void VirtualFreeMemory(void * ptr)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
};

#endif