#ifndef _H_LED_H
#define _H_LED_H

#include <string>

#include "simulation/entity.h"
#include "glue/pin_device.h"
#include "dash_widget.h"

using namespace std;

#define LED_PIN_COUNT 1

#define LED_PIN_INPUT 0

class Led : public Entity, public DashboardWidget, public PinDevice
{
public:
    Led(const char *default_name);
    Led(const char *default_name, Json::Value &json_data);
    
    virtual void render(Dashboard *dash) = 0;
protected:
    bool _lit;
    
    virtual void _onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value);
};

const pin_val_t SIMPLE_LED_LIGHTING_TRESHOLD = 0.6;

class SimpleLed : public Led
{
public:
    SimpleLed(int x, int y, int size, int color, const char *caption);
    SimpleLed(Json::Value &json_data);
    
    virtual void render(Dashboard *dash);
protected:
    int _x;
    int _y;
    int _size;
    int _color;
    string _caption;
};

#endif

