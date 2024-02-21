#ifndef FONT_C
#define FONT_C

#include <windows.h>
#include <gl/gl.h>
#include "core.c"

// this dependency is questionable
// #include "layout.c"

#define MAX_CHAR_CODE 126

typedef struct FontKerningPair
{
    u16 left;
    u16 right;
    i8 val;
} FontKerningPair;


typedef struct FontInfo
{
    u8* name;
    i32 size;
    i32 isClearType;
} FontInfo;

typedef struct FontData 
{
    MyBitmap textures[MAX_CHAR_CODE];
    GLuint cachedTextures[MAX_CHAR_CODE];

    // Need to use ABC structure for this 
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getcharabcwidthsa

    TEXTMETRIC textMetric;
    
    FontKerningPair* pairsHash;
} FontData;


const FontData *currentFont;

// Segoe UI has around 8k pairs, monospace has none pairs
#define PAIRS_HASH_LENGTH 16 * 1024
inline int HashAndProbeIndex(const FontData *font, u16 left, u16 right)
{
    i32 keysMask = PAIRS_HASH_LENGTH - 1;
    i32 index = (left * 19 + right * 7) & keysMask;
    int i = 1;
    while(font->pairsHash[index].left != 0 && font->pairsHash[index].left != left && font->pairsHash[index].right != right)
    {
        index += (i*i);
        i++;
        if(index >= PAIRS_HASH_LENGTH)
            index = index & keysMask;
        
    }
    return index;
}


// takes dimensions of destinations, reads rect from source at (0,0)
inline void CopyRectTo(MyBitmap *sourceT, MyBitmap *destination)
{
    u32 *row = (u32 *)destination->pixels + destination->width * (destination->height - 1);
    u32 *source = (u32 *)sourceT->pixels + sourceT->width * (sourceT->height - 1);
    for (u32 y = 0; y < destination->height; y += 1)
    {
        u32 *pixel = row;
        u32 *sourcePixel = source;
        for (u32 x = 0; x < destination->width; x += 1)
        {
            // u8 r = (*sourcePixel & 0xff0000) >> 16;
            // u8 g = (*sourcePixel & 0x00ff00) >> 8;
            // u8 b = (*sourcePixel & 0x0000ff) >> 0;
            // u32 alpha = (r == TRANSPARENT_R && g == TRANSPARENT_G && b == TRANSPARENT_B) ? 0x00000000 : 0xff000000;
            u32 alpha = 0xff000000;
            *pixel = *sourcePixel | alpha;
            sourcePixel += 1;
            pixel += 1;
        }
        source -= sourceT->width;
        row -= destination->width;
    }
}

void InitFontSystem(FontData *fontData, int fontSize, char* fontName, u32 foreground, u32 background)
{

    HDC deviceContext = CreateCompatibleDC(0);
 
 
    int h = -MulDiv(fontSize, GetDeviceCaps(deviceContext, LOGPIXELSY), USER_DEFAULT_SCREEN_DPI);
    HFONT font = CreateFontA(h, 0, 0, 0,
                             FW_DONTCARE, // Weight
                             0,           // Italic
                             0,           // Underline
                             0,           // Strikeout
                             DEFAULT_CHARSET,
                             OUT_TT_ONLY_PRECIS,
                             CLIP_DEFAULT_PRECIS,

                             //I've experimented with the Chrome and it doesn't render LCD quality for fonts above 32px
                            //  ANTIALIASED_QUALITY,
                              CLEARTYPE_QUALITY,
                             
                             DEFAULT_PITCH,
                             fontName);

    BITMAPINFO info = {0};
    int textureSize = 256;
    InitBitmapInfo(&info, textureSize, textureSize);

    void *bits;
    HBITMAP bitmap = CreateDIBSection(deviceContext, &info, DIB_RGB_COLORS, &bits, 0, 0);
    MyBitmap fontCanvas = {.bytesPerPixel = 4, .height = textureSize, .width = textureSize, .pixels = bits};

    SelectObject(deviceContext, bitmap);
    SelectObject(deviceContext, font);

     i32 kerningPairCount = GetKerningPairsW(deviceContext, 0, 0);

     
     if (kerningPairCount > 1)
     {
         
         fontData->pairsHash = VirtualAllocateMemory(PAIRS_HASH_LENGTH * sizeof(FontKerningPair));
         
         i32 pairsSizeAllocated = sizeof(KERNINGPAIR) * kerningPairCount;
         KERNINGPAIR *pairs = VirtualAllocateMemory(pairsSizeAllocated);
         GetKerningPairsW(deviceContext, kerningPairCount, pairs);

         for (int i = 0; i < kerningPairCount; i++)
         {
             KERNINGPAIR *pair = pairs + i;
             i32 index = HashAndProbeIndex(fontData, pair->wFirst, pair->wSecond);

             fontData->pairsHash[index].left = pair->wFirst;
             fontData->pairsHash[index].right = pair->wSecond;
             fontData->pairsHash[index].val = pair->iKernAmount;
         }
         VirtualFreeMemory(pairs);
     }

    // TRANSPARENT still leaves 00 as alpha value for non-trasparent pixels. I love GDI
    // SetBkColor(deviceContext, TRANSPARENT);
    SetBkColor(deviceContext, background);
    SetTextColor(deviceContext, foreground);


    // i32 count = 0;
    // StringBuffer buff = EmptyStringBuffer();

    SIZE size;
    for (wchar_t ch = 32; ch <= MAX_CHAR_CODE; ch += 1)
    {
        int len = 1;
        GetTextExtentPoint32W(deviceContext, &ch, len, &size);

        TextOutW(deviceContext, 0, 0, &ch, len);

        MyBitmap* texture = &fontData->textures[ch];
        texture->width = size.cx;
        texture->height = size.cy;
        texture->bytesPerPixel = 4;

        // count += texture->height * texture->width * texture->bytesPerPixel;
        texture->pixels = VirtualAllocateMemory(texture->height * texture->width * texture->bytesPerPixel);
        

        CopyRectTo(&fontCanvas, texture);
    }

    // AppendStr(&buff, "Allocated: ");
    // AppendNumber(&buff, count);
    // AppendStr(&buff, "\n");
    // OutputDebugStringA(buff.content);
    // // VirtualFreeMemory(buff.content);

    

    GetTextMetrics(deviceContext, &fontData->textMetric);
    DeleteObject(bitmap);
    DeleteDC(deviceContext);

}

void CreateFontTexturesForOpenGl(FontData *font)
{
    glGenTextures(MAX_CHAR_CODE, font->cachedTextures);
    for (u32 i = ' '; i < MAX_CHAR_CODE; i++)
    {
        MyBitmap *bit = &font->textures[i];
        glBindTexture(GL_TEXTURE_2D, font->cachedTextures[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bit->width, bit->height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bit->pixels);
    }
}

void InitFont(FontInfo* info, FontData *font, u32 foreground, u32 backgorund)
{
    InitFontSystem(font, info->size, info->name, foreground, backgorund);
    CreateFontTexturesForOpenGl(font);
}

i32 IsMonospaced(const FontData* font)
{
    return font->pairsHash == NULL;
}

inline int GetKerningValue(u16 left, u16 right)
{
    if(IsMonospaced(currentFont))
        return 0;

    i32 index = HashAndProbeIndex(currentFont, left, right);

    if(currentFont->pairsHash[index].left != left && currentFont->pairsHash[index].right != right)
    {
        return 0;
    }

    return currentFont->pairsHash[index].val;
}


i32 GetTextWidth(u8 *text){
    i32 res = 0;
    while(*text)
    {
        u8 ch = *text;
        // skips new lines and other control symbols
        if(ch >= ' ')
            res += currentFont->textures[ch].width + (GetKerningValue(ch, *(text + 1)));

        text++;
    }
    return res;
}


#endif