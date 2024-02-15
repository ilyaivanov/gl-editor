#ifndef CORE_C
#define CORE_C

#include <stdint.h>
#include "sincos.c"

#define ArrayLength(array) (sizeof(array) / sizeof(array[0]))
#define Assert(cond) if (!(cond)) { *(u32*)0 = 0; }
#define Fail(msg) Assert(0)


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;


typedef struct V2i { i32 x, y; } V2i;
typedef struct V3i { i32 x, y, z; } V3i;
typedef struct V2f { f32 x, y; } V2f;
typedef struct V3f { 
    union { 
        struct { f32 x, y, z; };
        struct { f32 r, g, b; };
    };
}  V3f;

typedef struct V4f { 
    union { 
        struct { f32 x, y, z, t; };
        struct { f32 r, g, b, a; };
    };
}  V4f;

typedef struct Mat4 {
    float values[16];
} Mat4;

typedef struct MyBitmap
{
    i32 width;
    i32 height;
    i32 bytesPerPixel;
    u32 *pixels;
} MyBitmap;



inline i32 Clampi32(i32 val, i32 min, i32 max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

inline i32 MaxI32(i32 v1, i32 v2)
{
    if(v1 > v2)
        return v1;
    return v2;
}

inline i32 AbsI32(i32 val)
{
    //TODO: bit manipulation?
    return val < 0 ? -val : val;
}

inline i32 MinI32(i32 v1, i32 v2)
{
    if(v1 < v2)
        return v1;
    return v2;
}


inline f32 Clampf32(f32 val, f32 min, f32 max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

inline f32 ClampI32(i32 val, i32 min, i32 max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

inline void* VirtualAllocateMemory(size_t size)
{
     return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
};

inline void VirtualFreeMemory(void * ptr)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
};


#endif