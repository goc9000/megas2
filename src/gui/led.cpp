#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "led.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[LED_PIN_COUNT] = {
    { PIN_MODE_INPUT, 0 }  // INPUT
};

Led::Led(void) : PinDevice(LED_PIN_COUNT, PIN_INIT_DATA)
{
    this->_value = 0;
}

void Led::_onPinChanged(int pin_id, int value, int old_value)
{
    this->_value = value;
}

SimpleLed::SimpleLed(int x, int y, int size, int color, const char *caption)
{
    this->_x = x;
    this->_y = y;
    this->_size = size;
    this->_color = color;
    this->_caption = caption;
}

void SimpleLed::render(Dashboard *dash)
{
    (this->_value ? filledCircleColor : circleColor)(dash->screen,
        this->_x+this->_size/2,
        this->_y+this->_size/2,
        this->_size/2,
        this->_color);
    
    if (!this->_caption)
        return;
    
    TTF_Font *font = dash->getFont(this->_size-4);
    if (!font)
        return;
    
    SDL_Color color = {(this->_color >> 24) & 0xff,
                       (this->_color >> 16) & 0xff,
                       (this->_color >> 8) & 0xff};
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, this->_caption, color);
    
    if (text_surface) {
        SDL_Rect where = {this->_x+this->_size+this->_size/2, this->_y + 2, 0, 0};
        SDL_BlitSurface(text_surface, NULL, dash->screen, &where);
        SDL_FreeSurface(text_surface);
    }
}
