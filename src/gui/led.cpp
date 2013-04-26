#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "led.h"
#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[LED_PIN_COUNT] = {
    { "IN", PIN_MODE_INPUT, 0 }  // INPUT
};

Led::Led(const char *default_name)
    : Entity(default_name), PinDevice(LED_PIN_COUNT, PIN_INIT_DATA)
{
    _lit = false;
}

Led::Led(const char *default_name, Json::Value &json_data)
    : Entity(default_name, json_data), PinDevice(LED_PIN_COUNT, PIN_INIT_DATA)
{
    _lit = false;
}

void Led::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
    _lit = value > SIMPLE_LED_LIGHTING_TRESHOLD;
}

SimpleLed::SimpleLed(int x, int y, int size, SDLColor color, const char *caption)
    : Led("Simple LED")
{
    this->_x = x;
    this->_y = y;
    this->_size = size;
    this->_color = color;
    this->_caption = caption;
}

SimpleLed::SimpleLed(Json::Value &json_data) : Led("Simple LED", json_data)
{
    _size = 16;
    _color = SDLColor::BLACK;
    _caption = "";
    
    parseJsonParam(_x, json_data, "x");
    parseJsonParam(_y, json_data, "y");
    parseJsonParam(_size, json_data, "size");
    
    if (json_data.isMember("color")) {
        _color = DashboardWidget::parseColor(json_data["color"]);
    }
    
    parseOptionalJsonParam(_caption, json_data, "caption");
}

void SimpleLed::render(Dashboard *dash)
{
    (_lit ? filledCircleColor : circleColor)(dash->screen,
        _x + _size/2, _y + _size/2, _size / 2, _color.toUInt32());
    
    dash->putText(_x + 3 * _size / 2, _y + 2, _caption, _size - 4, _color);
}
