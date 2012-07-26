#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "push_button.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[PUSH_BUTTON_PIN_COUNT] = {
    { "OUT", PIN_MODE_OUTPUT, 0 }  // OUTPUT
};

PushButton::PushButton(void) : PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    this->_up_value = 0;
    this->_down_value = 1;
    this->_setPressed(false);
}

PushButton::PushButton(int up_value, int down_value) : PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    this->_up_value = up_value;
    this->_down_value = down_value;
    this->_setPressed(false);
}

void PushButton::_setPressed(bool pressed)
{
    this->_pressed = pressed;
    this->_pinWrite(PUSH_BUTTON_PIN_OUTPUT, this->_pressed ? this->_down_value : this->_up_value);
}

void PushButton::_onPinChanged(int pin_id, int value, int old_value)
{
}

SimplePushButton::SimplePushButton(int x, int y, int size, int color, const char *caption) : PushButton()
{
    this->_init(x, y, size, color, caption);
}

SimplePushButton::SimplePushButton(int up_value, int down_value, int x, int y, int size,
    int color, const char *caption) : PushButton(up_value, down_value)
{
    this->_init(x, y, size, color, caption);
}

void SimplePushButton::_init(int x, int y, int size, int color, const char *caption)
{
    this->_x = x;
    this->_y = y;
    this->_size = size;
    this->_color = color;
    this->_caption = caption;
}

void SimplePushButton::render(Dashboard *dash)
{
    rectangleColor(dash->screen,
        this->_x,
        this->_y,
        this->_x + this->_size - 1,
        this->_y + this->_size - 1,
        this->_color);
    
    if (this->_pressed) {
        boxColor(dash->screen,
            this->_x + 4,
            this->_y + 4,
            this->_x + this->_size - 4 - 1,
            this->_y + this->_size - 4 - 1,
            this->_color);
    }
    
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

bool SimplePushButton::handleEvent(Dashboard *dash, SDL_Event *event)
{
    bool hit_test = (event->button.x >= this->_x) &&
                    (event->button.y >= this->_y) &&
                    (event->button.x < this->_x + this->_size) &&
                    (event->button.y < this->_y + this->_size);
    
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if ((event->button.button = SDL_BUTTON_LEFT) && hit_test) {
                this->_setPressed(true);
            }
            return true;
        case SDL_MOUSEBUTTONUP:
            this->_setPressed(false);
            return false;
    }
    
    return false;
}
