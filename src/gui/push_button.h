#ifndef _H_PUSH_BUTTON_H
#define _H_PUSH_BUTTON_H

#include <string>

#include <json/json.h>

#include "simulation/entity.h"
#include "glue/pin_device.h"
#include "dash_widget.h"

using namespace std;

#define PUSH_BUTTON_PIN_COUNT 1

#define PUSH_BUTTON_PIN_OUTPUT 0

const pin_val_t PUSH_BUTTON_DEFAULT_UP_VALUE = 0.0;
const pin_val_t PUSH_BUTTON_DEFAULT_DOWN_VALUE = PIN_VAL_VCC;


class PushButton : public DashboardWidget, public PinDevice
{
public:
    PushButton(const char *default_name, int x, int y);
    PushButton(const char *default_name, pin_val_t up_value, pin_val_t down_value,
        int x, int y);
    PushButton(const char *default_name, Json::Value &json_data);
protected:
    pin_val_t _up_value;
    pin_val_t _down_value;
    bool _pressed;
    
    void _setPressed(bool pressed);
    virtual void _onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value);
};

class SimplePushButton : public PushButton
{
public:
    SimplePushButton(int x, int y, int size, const char *caption);
    SimplePushButton(int up_value, int down_value, int x, int y, int size,
        const char *caption);
    SimplePushButton(Json::Value &json_data);
    
    virtual void render(Dashboard *dash, SDLColor color, SDLColor bg_color,
        int font_size);
    virtual bool handleEvent(Dashboard *dash, SDL_Event *event);
protected:
    int _size;
    string _caption;
};

#endif

