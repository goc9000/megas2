#ifndef _H_PUSH_BUTTON_H
#define _H_PUSH_BUTTON_H

#include "glue/pin_device.h"
#include "dash_widget.h"

using namespace std;

#define PUSH_BUTTON_PIN_COUNT 1

#define PUSH_BUTTON_PIN_OUTPUT 0

class PushButton : public DashboardWidget, public PinDevice
{
public:
    PushButton();
    PushButton(int up_value, int down_value);
    
    virtual void render(Dashboard *dash) = 0;
protected:
    int _up_value;
    int _down_value;
    bool _pressed;
    
    void _setPressed(bool pressed);
    virtual void _onPinChanged(int pin_id, int value, int old_value);
};

class SimplePushButton : public PushButton
{
public:
    SimplePushButton(int x, int y, int size, int color, const char *caption);
    SimplePushButton(int up_value, int down_value, int x, int y, int size, int color, const char *caption);
    
    virtual void render(Dashboard *dash);
    virtual bool handleEvent(Dashboard *dash, SDL_Event *event);
protected:
    int _x;
    int _y;
    int _size;
    int _color;
    const char *_caption;
    
    void _init(int x, int y, int size, int color, const char *caption);
};

#endif

