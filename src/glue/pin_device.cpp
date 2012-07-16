#include <cstdlib>

#include "pin_device.h"
#include "utils/fail.h"

using namespace std;

PinDevice::PinDevice(int num_pins, PinInitData const * const init_data)
{
    for (int i = 0; i < num_pins; i++) {
        Pin pin;
        pin.mode = init_data[i].mode;
        pin.last_output = (init_data[i].mode == PIN_MODE_OUTPUT) ? init_data[i].float_value : PIN_VAL_Z;
        pin.float_value = init_data[i].float_value;
        pin.last_input = PIN_VAL_Z;
        this->pins.push_back(pin);
    }
}

void PinDevice::_setPinMode(int pin_id, int mode)
{
    Pin *pin = &this->pins[pin_id];
    
    if (pin->mode == mode)
        return;
    
    uint8_t eff_val = this->_pinRead(pin_id);
    
    pin->mode = mode;
    
    uint8_t new_eff_val = this->_pinRead(pin_id);
    
    if ((mode == PIN_MODE_INPUT) && (new_eff_val != eff_val)) {
        this->_onPinChanged(pin_id, new_eff_val, eff_val);
    } else {
        int out_val = pin->last_output;
        pin->last_output = !out_val; // to force resending it over the bus
        this->_pinWrite(pin_id, out_val);
    }
}

void PinDevice::_setPinFloatValue(int pin_id, int float_value)
{
    Pin *pin = &this->pins[pin_id];
    
    if (pin->float_value == float_value)
        return;
    
    uint8_t eff_val = this->_pinRead(pin_id);
    
    pin->float_value = float_value;
    
    uint8_t new_eff_val = this->_pinRead(pin_id);
    
    if ((pin->mode == PIN_MODE_INPUT) && (new_eff_val != eff_val)) {
        this->_onPinChanged(pin_id, new_eff_val, eff_val);
    }
}

void PinDevice::_pinWrite(int pin_id, int value)
{
    Pin *pin = &this->pins[pin_id];
    
    if (pin->mode == PIN_MODE_OUTPUT) {
        if (pin->last_output == value)
            return;
        
        pin->last_output = value;
        
        // TODO: propagate over bus
    } else {
        fail("Writing to pin set as input");
    }
}

int PinDevice::_pinRead(int pin_id)
{
    Pin *pin = &this->pins[pin_id];
    
    if (pin->mode == PIN_MODE_OUTPUT) {
        return pin->last_output;
    } else {
        if (pin->last_input == PIN_VAL_Z) {
            return pin->float_value;
        }
        return pin->last_input;
    }
}

Pin::Pin(void)
{
    this->mode = PIN_MODE_INPUT;
    this->float_value = 1;
    this->last_input = PIN_VAL_Z;
    this->last_output = 0;
}
