#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "led.h"
#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[LED_PIN_COUNT] = {
    { "IN", PIN_MODE_INPUT, 0 }  // INPUT
};

Led::Led(void) : Entity("led", "LED"), PinDevice(LED_PIN_COUNT, PIN_INIT_DATA)
{
    this->_lit = false;
}

Led::Led(Json::Value &json_data) : Entity(json_data), PinDevice(LED_PIN_COUNT, PIN_INIT_DATA)
{
    this->_lit = false;
}

void Led::_onPinChanged(int pin_id, int value, int old_value)
{
    this->_lit = !!value;
}

SimpleLed::SimpleLed(int x, int y, int size, int color, const char *caption)
{
    this->_x = x;
    this->_y = y;
    this->_size = size;
    this->_color = color;
    this->_caption = caption;
}

SimpleLed::SimpleLed(Json::Value &json_data) : Led(json_data)
{
    this->_size = 16;
    this->_color = DashboardWidget::COLOR_BLACK;
    this->_caption = "";
    
    if (json_data.isMember("x")) {
        this->_x = json_data["x"].asInt();
    } else {
        fail("Member 'x' required for SimpleLed widget");
    }
    if (json_data.isMember("y")) {
        this->_y = json_data["y"].asInt();
    } else {
        fail("Member 'y' required for SimpleLed widget");
    }
    if (json_data.isMember("size")) {
        this->_size = json_data["size"].asInt();
    } else {
        fail("Member 'size' required for SimpleLed widget");
    }
    if (json_data.isMember("color")) {
        this->_color = DashboardWidget::parseColor(json_data["color"]);
    }
    if (json_data.isMember("caption")) {
        this->_caption = json_data["caption"].asString();
    }
}

void SimpleLed::render(Dashboard *dash)
{
    (this->_lit ? filledCircleColor : circleColor)(dash->screen,
        this->_x + this->_size/2, this->_y + this->_size/2,
        this->_size / 2, this->_color);
    
    dash->putText(this->_x + 3 * this->_size / 2, this->_y + 2,
        this->_caption, this->_size - 4, this->_color);
}
