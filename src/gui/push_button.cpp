#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "push_button.h"

#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[PUSH_BUTTON_PIN_COUNT] = {
    { "OUT", PIN_MODE_OUTPUT, 0 }  // OUTPUT
};

PushButton::PushButton(const char *default_name)
    : Entity(default_name), PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    _up_value = PUSH_BUTTON_DEFAULT_UP_VALUE;
    _down_value = PUSH_BUTTON_DEFAULT_DOWN_VALUE;
    _setPressed(false);
}

PushButton::PushButton(const char *default_name, pin_val_t up_value, pin_val_t down_value)
    : Entity(default_name), PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    this->_up_value = up_value;
    this->_down_value = down_value;
    _setPressed(false);
}

PushButton::PushButton(const char *default_name, Json::Value &json_data)
    : Entity(default_name, json_data), PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    _up_value = PUSH_BUTTON_DEFAULT_UP_VALUE;
    _down_value = PUSH_BUTTON_DEFAULT_DOWN_VALUE;
    
    parseOptionalJsonParam(_up_value, json_data, "up_value");
    parseOptionalJsonParam(_down_value, json_data, "down_value");
    
    _setPressed(false);
}

void PushButton::_setPressed(bool pressed)
{
    this->_pressed = pressed;
    _pins[PUSH_BUTTON_PIN_OUTPUT].write(_pressed ? _down_value : _up_value);
}

void PushButton::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
}

SimplePushButton::SimplePushButton(int x, int y, int size, SDLColor color, const char *caption)
    : PushButton("Simple push button")
{
    _init(x, y, size, color, caption);
}

SimplePushButton::SimplePushButton(int up_value, int down_value, int x, int y, int size,
    SDLColor color, const char *caption)
    : PushButton("Simple push button", up_value, down_value)
{
    _init(x, y, size, color, caption);
}

SimplePushButton::SimplePushButton(Json::Value &json_data)
    : PushButton("Simple push button", json_data)
{
    _size = 16;
    _color = SDLColor::BLACK;
    _caption = "";
    
    parseJsonParam(_x, json_data, "x");
    parseJsonParam(_y, json_data, "y");
    parseJsonParam(_size, json_data, "size");
    parseOptionalJsonParam(_color, json_data, "color");
    parseOptionalJsonParam(_caption, json_data, "caption");
}

void SimplePushButton::_init(int x, int y, int size, SDLColor color, const char *caption)
{
    this->_x = x;
    this->_y = y;
    this->_size = size;
    this->_color = color;
    this->_caption = caption;
}

void SimplePushButton::render(Dashboard *dash)
{
    rectangleColor(dash->screen, _x, _y, _x + _size - 1, _y + _size - 1,
        _color.toUInt32());
    
    if (_pressed) {
        boxColor(dash->screen, _x + 4, _y + 4, _x + _size - 4 - 1, _y + _size - 4 - 1,
            _color.toUInt32());
    }
    
    dash->putText(_x + 3 * _size / 2, _y + 2, _caption, _size - 4, _color);
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
