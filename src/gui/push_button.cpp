#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "push_button.h"

#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[PUSH_BUTTON_PIN_COUNT] = {
    { "OUT", PIN_MODE_OUTPUT, 0 }  // OUTPUT
};

PushButton::PushButton(void) : Entity("pushbutton", "Push Button"), PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    this->_up_value = 0;
    this->_down_value = 1;
    this->_setPressed(false);
}

PushButton::PushButton(int up_value, int down_value) : Entity("pushbutton", "Push Button"), PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    this->_up_value = up_value;
    this->_down_value = down_value;
    this->_setPressed(false);
}

PushButton::PushButton(Json::Value &json_data) : Entity(json_data), PinDevice(PUSH_BUTTON_PIN_COUNT, PIN_INIT_DATA)
{
    this->_up_value = 0;
    this->_down_value = 1;
    this->_setPressed(false);
    
    if (json_data.isMember("up_value")) {
        this->_up_value = (json_data["up_value"].isString() && (json_data["up_value"].asString() == "Z"))
            ? PIN_VAL_Z : json_data["up_value"].asInt();
    }
    if (json_data.isMember("down_value")) {
        this->_down_value = (json_data["down_value"].isString() && (json_data["down_value"].asString() == "Z"))
            ? PIN_VAL_Z : json_data["down_value"].asInt();
    }
}

void PushButton::_setPressed(bool pressed)
{
    this->_pressed = pressed;
    this->_pinWrite(PUSH_BUTTON_PIN_OUTPUT, this->_pressed ? this->_down_value : this->_up_value);
}

void PushButton::_onPinChanged(int pin_id, int value, int old_value)
{
}

SimplePushButton::SimplePushButton(int x, int y, int size, int color, const char *caption)
    : PushButton()
{
    this->_init(x, y, size, color, caption);
}

SimplePushButton::SimplePushButton(int up_value, int down_value, int x, int y, int size,
    int color, const char *caption)
    : PushButton(up_value, down_value)
{
    this->_init(x, y, size, color, caption);
}

SimplePushButton::SimplePushButton(Json::Value &json_data) : PushButton(json_data)
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
    
    if (this->_caption == "")
        return;
    
    TTF_Font *font = dash->getFont(this->_size-4);
    if (!font)
        return;
    
    SDL_Color color = {(this->_color >> 24) & 0xff,
                       (this->_color >> 16) & 0xff,
                       (this->_color >> 8) & 0xff};
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, this->_caption.c_str(), color);
    
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
