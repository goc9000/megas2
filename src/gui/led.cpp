#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "led.h"
#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[LED_PIN_COUNT] = {
    { "IN", PIN_MODE_INPUT, 0 }  // INPUT
};

Led::Led(const char *default_name, int x, int y)
    : DashboardWidget(default_name, x, y), PinDevice(LED_PIN_COUNT, PIN_INIT_DATA)
{
    _lit = false;
}

Led::Led(const char *default_name, Json::Value &json_data)
    : DashboardWidget(default_name, json_data), PinDevice(LED_PIN_COUNT, PIN_INIT_DATA)
{
    _lit = false;
}

void Led::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
    _lit = value > SIMPLE_LED_LIGHTING_TRESHOLD;
}

SimpleLed::SimpleLed(int x, int y, int size, SDLColor color, const char *caption)
    : Led("Simple LED", x, y)
{
    this->_size = size;
    this->color = color;
    this->_caption = caption;
}

SimpleLed::SimpleLed(Json::Value &json_data)
    : Led("Simple LED", json_data)
{
    _size = 16;
    _caption = "";
    
    parseJsonParam(_size, json_data, "size");
    parseOptionalJsonParam(_caption, json_data, "caption");
}

void SimpleLed::render(Dashboard *dash, SDLColor color, SDLColor bg_color,
    int font_size)
{
    if (_lit)
        filledCircleColor(dash->screen, x + _size / 2, y + _size / 2,
            _size / 2, color.toUInt32());
    else
        circleColor(dash->screen, x + _size / 2, y + _size / 2,
            _size / 2, color.toUInt32());
    
    dash->putText(x + 3 * _size / 2, y + (_size - font_size) / 2, _caption,
        font_size, color);
}
