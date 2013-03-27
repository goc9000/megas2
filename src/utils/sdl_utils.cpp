#include "sdl_utils.h"

SDLRect::SDLRect(Sint16 x, Sint16 y, Uint16 w, Uint16 h)
{
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

SDLColor::SDLColor(int color_spec)
{
    this->r = (color_spec >> 24) & 0xff;
    this->g = (color_spec >> 16) & 0xff;
    this->b = (color_spec >> 8) & 0xff;
}
