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

SimpleLed::SimpleLed(int x, int y, int size, int color, const char *caption)
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
    _color = DashboardWidget::COLOR_BLACK;
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
    (this->_lit ? filledCircleColor : circleColor)(dash->screen,
        this->_x + this->_size/2, this->_y + this->_size/2,
        this->_size / 2, this->_color);
    
    dash->putText(this->_x + 3 * this->_size / 2, this->_y + 2,
        this->_caption, this->_size - 4, this->_color);
}
