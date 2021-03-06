#ifndef _H_LED_H
#define _H_LED_H

#include <string>

#include "simulation/entity.h"
#include "glue/pin_device.h"
#include "dash_widget.h"

using namespace std;

#define LED_PIN_COUNT 1

#define LED_PIN_INPUT 0

class Led : public DashboardWidget, public PinDevice
{
public:
    Led(const char *default_name, int x, int y);
    Led(const char *default_name, Json::Value &json_data);
protected:
    bool _lit;
    
    virtual void _onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value);
};

const pin_val_t SIMPLE_LED_LIGHTING_TRESHOLD = 0.6;

class SimpleLed : public Led
{
public:
    SimpleLed(int x, int y, int size, SDLColor color, const char *caption);
    SimpleLed(Json::Value &json_data);
    
    virtual void render(Dashboard *dash, SDLColor color, SDLColor bg_color,
        int font_size);
protected:
    int _size;
    string _caption;
};

#endif

