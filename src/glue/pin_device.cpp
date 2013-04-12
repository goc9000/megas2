#include <cstring>
#include <cstdlib>

#include "analog_bus.h"
#include "pin_device.h"
#include "utils/fail.h"

PinDevice::PinDevice(int num_pins, PinInitData const * const init_data)
{
    this->_vcc = DEFAULT_VCC;
    this->_logic_threshold = DEFAULT_LOGIC_THRESHOLD;
    
    this->_pins = new Pin[num_pins];
    this->_num_pins = num_pins;
    
    for (int i = 0; i < num_pins; i++) {
        this->_pins[i].initialize(this, i, &init_data[i]);
    }
}

void PinDevice::connectPinToBus(int pin_id, AnalogBus *bus)
{
    this->_pins[pin_id].connectToBus(bus);
}

void PinDevice::disconnectPinFromBus(int pin_id, AnalogBus *bus)
{
    this->_pins[pin_id].disconnectFromBus(bus);
}

int PinDevice::lookupPin(const char *pin_name)
{
    for (int i = 0; i < this->_num_pins; i++)
        if (!strcmp(pin_name, this->_pins[i].pin_name))
            return i;
    
    return -1;
}

void PinDevice::drivePin(int pin_id, pin_val_t value)
{
    this->_pins[pin_id].drive(value);
}

pin_val_t PinDevice::queryPin(int pin_id)
{
    return this->_pins[pin_id].query();
}
