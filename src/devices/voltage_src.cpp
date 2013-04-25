#include "voltage_src.h"

#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[VOLTAGE_SRC_PIN_COUNT] = {
    { "OUT", PIN_MODE_OUTPUT, 0 }  // OUTPUT
};

#define DEFAULT_NAME "Voltage source"

VoltageSource::VoltageSource(void)
    : Entity(DEFAULT_NAME), PinDevice(VOLTAGE_SRC_PIN_COUNT, PIN_INIT_DATA)
{
    setValue(PIN_VAL_0);
}

VoltageSource::VoltageSource(pin_val_t value)
    : Entity(DEFAULT_NAME), PinDevice(VOLTAGE_SRC_PIN_COUNT, PIN_INIT_DATA)
{
    setValue(value);
}

VoltageSource::VoltageSource(Json::Value &json_data)
    : Entity(DEFAULT_NAME, json_data), PinDevice(VOLTAGE_SRC_PIN_COUNT, PIN_INIT_DATA)
{
    pin_val_t value;
    
    parseJsonParam(value, json_data, "value");
    setValue(value);
}

void VoltageSource::setValue(pin_val_t value)
{
    this->_value = value;
    _pins[VOLTAGE_SRC_PIN_OUTPUT].write(value);
}

void VoltageSource::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
}
