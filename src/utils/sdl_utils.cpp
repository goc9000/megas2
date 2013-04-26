#include "sdl_utils.h"

SDLRect::SDLRect(Sint16 x, Sint16 y, Uint16 w, Uint16 h)
{
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

const SDLColor SDLColor::BLACK = SDLColor(0, 0, 0, 0xff);

SDLColor::SDLColor(void) : SDLColor(0, 0, 0, 0xff)
{
}

SDLColor::SDLColor(uint8_t r, uint8_t g, uint8_t b) : SDLColor(r, g, b, 0xff)
{
}

SDLColor::SDLColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->unused = a;
}

SDLColor::SDLColor(uint32_t rgba)
{
    this->r = (rgba >> 24) & 0xff;
    this->g = (rgba >> 16) & 0xff;
    this->b = (rgba >> 8) & 0xff;
    this->unused = rgba & 0xff;
}

uint32_t SDLColor::toUInt32(void) const
{
    return (this->r << 24) + (this->g << 16) + (this->b << 8) + this->unused;
}
