#ifndef UI_C
#define UI_C

#include "core.c"

inline Mat4 DrawScrollbar(Layout *layout, float scrollWidth)
{
    if(layout->pageHeight > layout->height)
    {
        i32 scrollY = layout->offsetY * (layout->height / layout->pageHeight);
        i32 scrollHeight = layout->height * (layout->height / layout->pageHeight);
        
        glColor3f(0.3f, 0.3f, 0.3f);

        f32 x = layout->x + layout->width - scrollWidth;
        f32 y = layout->y + layout->height - scrollHeight + scrollY;
        return (Mat4) {
            scrollWidth, 0, 0, x,
            0, scrollHeight, 0, y,
            0, 0, 1, 0,
            0, 0, 0, 1,
        };
    }
    return (Mat4) {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
        };
}

#endif