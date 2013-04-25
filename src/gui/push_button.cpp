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
    
    this->_setPressed(false);
}

void PushButton::_setPressed(bool pressed)
{
    this->_pressed = pressed;
    this->_pins[PUSH_BUTTON_PIN_OUTPUT].write(this->_pressed ? this->_down_value : this->_up_value);
}

void PushButton::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
}

SimplePushButton::SimplePushButton(int x, int y, int size, int color, const char *caption)
    : PushButton("Simple push button")
{
    this->_init(x, y, size, color, caption);
}

SimplePushButton::SimplePushButton(int up_value, int down_value, int x, int y, int size,
    int color, const char *caption)
    : PushButton("Simple push button", up_value, down_value)
{
    this->_init(x, y, size, color, caption);
}

SimplePushButton::SimplePushButton(Json::Value &json_data)
    : PushButton("Simple push button", json_data)
{
    this->_size = 16;
    this->_color = DashboardWidget::COLOR_BLACK;
    this->_caption = "";
    
    if (json_data.isMember("x")) {
        this->_x = json_data["x"].asInt();
    } else {
        fail("Member 'x' required for SimplePushButton widget");
    }
    if (json_data.isMember("y")) {
        this->_y = json_data["y"].asInt();
    } else {
        fail("Member 'y' required for SimplePushButton widget");
    }
    if (json_data.isMember("size")) {
        this->_size = json_data["size"].asInt();
    } else {
        fail("Member 'size' required for SimplePushButton widget");
    }
    if (json_data.isMember("color")) {
        this->_color = DashboardWidget::parseColor(json_data["color"]);
    }
    if (json_data.isMember("caption")) {
        this->_caption = json_data["caption"].asString();
    }
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
    rectangleColor(dash->screen, this->_x, this->_y,
        this->_x + this->_size - 1, this->_y + this->_size - 1, this->_color);
    
    if (this->_pressed) {
        boxColor(dash->screen, this->_x + 4, this->_y + 4,
            this->_x + this->_size - 4 - 1, this->_y + this->_size - 4 - 1,
            this->_color);
    }
    
    dash->putText(this->_x + 3 * this->_size / 2, this->_y + 2, this->_caption,
        this->_size - 4, this->_color);
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
