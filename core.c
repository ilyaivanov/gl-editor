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

#endif