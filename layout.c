#ifndef LAYOUT_C
#define LAYOUT_C

#include "core.c"

typedef struct Layout
{
    f32 x, y, width, height;
    f32 runningX, runningY;
    f32 offsetY, pageHeight;
} Layout;


Layout layoutStack[20];
i32 currentLayoutIndex = -1;

#define currentLayout layoutStack[currentLayoutIndex]

void PushLayout(f32 x, f32 y, f32 width, f32 height)
{
    layoutStack[++currentLayoutIndex] = (Layout){.x = x, .y = y, .runningX = x, .runningY = y, .width = width, .height = height};
}

void PushPersistedLayout(Layout layout)
{
    layout.runningX = 0;
    layout.runningY = 0;

    layoutStack[++currentLayoutIndex] = layout;
}

void PushRow(f32 height)
{
    Layout* current = &currentLayout;
    
    f32 x = current->x;
    f32 y = current->y + current->height - current->runningY - currentLayout.offsetY - height;
    layoutStack[++currentLayoutIndex] = (Layout){.x = x, .y = y, .width = current->width, .height = height};
    current->runningY += height;

}
void PushColumn(f32 width)
{
    Layout* current = &currentLayout;
    
    f32 x = current->x + current->runningX;
    f32 y = current->y;
    layoutStack[++currentLayoutIndex] = (Layout){.x = x, .y = y, .width = width, .height = current->height};
    current->runningX += width;

}
void PushColumnWithOffset(f32 width, f32 offset)
{
    PushColumn(width);
    currentLayout.offsetY = offset;
}

void ShrinkLayout(f32 val)
{
    currentLayout.x += val;
    currentLayout.y += val;

    currentLayout.height -= val*2;
    currentLayout.width -= val*2;
}

void ShrinkHorizontal(f32 val)
{
    currentLayout.x += val;

    currentLayout.width -= val*2;
}

void LayoutMoveRight(f32 val)
{
    currentLayout.runningX += val;
}

void LayoutMoveDown(f32 val)
{
    currentLayout.runningY += val;
}

void PlaceFromRight(f32 val)
{
    currentLayout.runningX = currentLayout.width - val;
}

void PlaceFromBottom(f32 val)
{
    currentLayout.runningY = currentLayout.height - val;
}

f32 RemainingHeight()
{
    return currentLayout.height - currentLayout.runningY;
}

void ResetLayout(V2i screen)
{
    currentLayoutIndex = -1;
    PushLayout(0, 0, screen.x, screen.y);
}

void PopLayout()
{
    currentLayoutIndex--;
}

i32 IsMouseOverLayout(V2i mouse)
{
    return mouse.x >= currentLayout.x && 
           mouse.y >= currentLayout.y && 
           mouse.x <= (currentLayout.x + currentLayout.width) &&
           mouse.y <= (currentLayout.y + currentLayout.height);
}


#endif