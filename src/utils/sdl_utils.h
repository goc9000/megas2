#ifndef _H_SDL_UTILS_H
#define _H_SDL_UTILS_H

#include <SDL/SDL.h>

class SDLRect : public SDL_Rect
{
public:
    SDLRect(Sint16 x, Sint16 y, Uint16 w, Uint16 h);
};

class SDLColor;

class SDLColor : public SDL_Color
{
public:
    SDLColor(void);
    SDLColor(uint8_t r, uint8_t g, uint8_t b);
    SDLColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    SDLColor(uint32_t rgba);
    
    uint32_t toUInt32(void) const;
    
    static const SDLColor BLACK;
};

#endif
