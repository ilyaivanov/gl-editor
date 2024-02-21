#include "core.c"

#define HexColor(hex) {(float)((hex >> 16) & 0xff) / 255.0f, \
                       (float)((hex >>  8) & 0xff) / 255.0f, \
                       (float)((hex >>  0) & 0xff) / 255.0f}

#define Grey(val) ((V3f){val, val, val})
#define Grey4(val) ((V4f){val, val, val, val})



// example of colors on github
// https://gist.github.com/hasenj/1bba3ca00af1a3c0b2035c9bd14a85ef

// https://github.com/ayu-theme/ayu-vim

#define textColorHex 0xdcdcdc
#define lineColorHex 0x817770
#define selectedLineColorHex 0xcccccc
#define bgColorHex 0x121212
#define cursorHex 0xFFFFFF

#define textColor HexColor(textColorHex)
#define bgColor HexColor(bgColorHex)

// V3f bgColor           = HexColor(0x121212);
// V3f footerColor       = HexColor(0x181818);

// V3f header            = HexColor(0x151515);
// V3f headerHover       = HexColor(0x252525);

// V3f headerButtonHover = HexColor(0x333333);
// V3f headerCrossButtonHover = HexColor(0xDB291D);
// V3f headerCrossButtonDown = HexColor(0xDB291D);
// V3f uiColor           = HexColor(0xBBBBBB);

// V3f white             = HexColor(0xffffff);
// V3f back              = HexColor(0x000000);