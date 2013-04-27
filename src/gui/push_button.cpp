#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "push_button.h"

#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[PUSH_BUTTON_PIN_COUNT] = {
    { "OUT", PIN_MODE_OUTPUT, 0 }  // OUTPUT
};

PushButton::PushButton(const char *default_name, int x, int y)
    : DashboardWidget(default_name, x, y),
      PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    _up_value = PUSH_BUTTON_DEFAULT_UP_VALUE;
    _down_value = PUSH_BUTTON_DEFAULT_DOWN_VALUE;
    _setPressed(false);
}

PushButton::PushButton(const char *default_name, pin_val_t up_value,
    pin_val_t down_value, int x, int y)
    : DashboardWidget(default_name, x, y),
      PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    this->_up_value = up_value;
    this->_down_value = down_value;
    _setPressed(false);
}

PushButton::PushButton(const char *default_name, Json::Value &json_data)
    : DashboardWidget(default_name, json_data),
      PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
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

SimplePushButton::SimplePushButton(int x, int y, int size,
    const char *caption)
    : PushButton("Simple push button", x, y)
{
    this->_size = size;
    this->_caption = caption;
}

SimplePushButton::SimplePushButton(int up_value, int down_value, int x, int y,
    int size, const char *caption)
    : PushButton("Simple push button", up_value, down_value, x, y)
{
    this->_size = size;
    this->_caption = caption;
}

SimplePushButton::SimplePushButton(Json::Value &json_data)
    : PushButton("Simple push button", json_data)
{
    _caption = "";
    
    parseJsonParam(_size, json_data, "size");
    parseOptionalJsonParam(_caption, json_data, "caption");
}

void SimplePushButton::render(Dashboard *dash, SDLColor color, SDLColor bg_color,
    int font_size)
{
    rectangleColor(dash->screen, x, y, x + _size - 1, y + _size - 1,
        color.toUInt32());
    
    if (_pressed) {
        boxColor(dash->screen, x + 4, y + 4, x + _size - 4 - 1, y + _size - 4 - 1,
            color.toUInt32());
    }
    
    dash->putText(x + 3 * _size / 2, y + (_size - font_size) / 2, _caption,
        font_size, color);
}

bool SimplePushButton::handleEvent(Dashboard *dash, SDL_Event *event)
{
    bool hit_test = (event->button.x >= x) &&
                    (event->button.y >= y) &&
                    (event->button.x < x + _size) &&
                    (event->button.y < y + _size);
    
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if ((event->button.button = SDL_BUTTON_LEFT) && hit_test) {
                _setPressed(true);
            }
            return true;
        case SDL_MOUSEBUTTONUP:
            _setPressed(false);
            return false;
    }
    
    return false;
}
