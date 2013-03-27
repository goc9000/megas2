#ifndef _H_SDL_UTILS_H
#define _H_SDL_UTILS_H

#include <SDL/SDL.h>

class SDLRect : public SDL_Rect
{
    public:
        SDLRect(Sint16 x, Sint16 y, Uint16 w, Uint16 h);
};

class SDLColor : public SDL_Color
{
    public:
        SDLColor(int color_spec);
};

#endif
