#ifndef _H_VOLTAGE_SRC_H
#define _H_VOLTAGE_SRC_H

#include <string>

#include <json/json.h>

#include "simulation/entity.h"
#include "glue/pin_device.h"

using namespace std;

#define VOLTAGE_SRC_PIN_COUNT 1

#define VOLTAGE_SRC_PIN_OUTPUT 0


class VoltageSource : public Entity, public PinDevice
{
public:
    VoltageSource();
    VoltageSource(pin_val_t value);
    VoltageSource(Json::Value &json_data);
    
    void setValue(pin_val_t value);
protected:
    pin_val_t _value;
    
    virtual void _onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value);
};

#endif

