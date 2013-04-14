#include "voltage_src.h"

#include "utils/fail.h"

// Pin initialization data

PinInitData const PIN_INIT_DATA[VOLTAGE_SRC_PIN_COUNT] = {
    { "OUT", PIN_MODE_OUTPUT, 0 }  // OUTPUT
};

VoltageSource::VoltageSource(void)
    : Entity("voltage_src", "Voltage Source"), PinDevice(VOLTAGE_SRC_PIN_COUNT, PIN_INIT_DATA)
{
    this->setValue(0.0);
}

VoltageSource::VoltageSource(pin_val_t value)
    : Entity("voltage_src", "Voltage Source"), PinDevice(VOLTAGE_SRC_PIN_COUNT, PIN_INIT_DATA)
{
    this->setValue(value);
}

VoltageSource::VoltageSource(Json::Value &json_data)
    : Entity(json_data), PinDevice(VOLTAGE_SRC_PIN_COUNT, PIN_INIT_DATA)
{
    if (json_data.isMember("value"))
        this->setValue(parse_json_pin_value(json_data["value"]));
    else
        fail("Missing parameter 'value' for entity of type VoltageSource");
}

void VoltageSource::setValue(pin_val_t value)
{
    this->_value = value;
    this->_pins[VOLTAGE_SRC_PIN_OUTPUT].write(value);
}

void VoltageSource::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
}
