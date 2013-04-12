#include <assert.h>

#include "pin.h"
#include "pin_device.h"
#include "analog_bus.h"

using namespace std;

void Pin::initialize(PinDevice *owner, int pin_id, PinInitData const * init_data)
{
    this->owner = owner;
    this->pin_id = pin_id;
    this->pin_name = init_data->pin_name;
    this->mode = init_data->mode;
    this->drive_value = (init_data->mode == PIN_MODE_OUTPUT) ? init_data->float_value : PIN_VAL_Z;
    this->float_value = init_data->float_value;
    this->last_input = PIN_VAL_Z;
}

void Pin::connectToBus(AnalogBus *bus)
{
    for (vector<AnalogBus *>::iterator it = this->buses.begin(); it != this->buses.end(); it++)
        if (*it == bus)
            return;
    
    this->buses.push_back(bus);
    bus->addDevicePin(this->owner, this->pin_id);
}

void Pin::disconnectFromBus(AnalogBus *bus)
{
    for (vector<AnalogBus *>::iterator it = this->buses.begin(); it != this->buses.end(); it++)
        if (*it == bus)
            return;
    
    this->buses.push_back(bus);
    bus->addDevicePin(this->owner, this->pin_id);
}

/**
 * Gets the effective value of the pin as seen from the *inside*.
 * 
 * It is guaranteed that the read value will be a fully resolved voltage,
 * that is, neithen PIN_VAL_Z nor PIN_VAL_VCC.
 *
 * @return The effective value of the pin
 */
pin_val_t Pin::read(void)
{
    if (this->last_input == PIN_VAL_Z)
        return (this->float_value == PIN_VAL_VCC) ? this->owner->_vcc : this->float_value;
    
    return this->last_input;
}

/**
 * Like read(), and converts to a digital value.
 *
 * @see read()
 * @return The effective value of the pin
 */
bool Pin::readDigital(void)
{
    return (this->read() > this->owner->_logic_threshold);
}

/**
 * Drives the pin to a specific value from the *inside*.
 * 
 * @param value The value to set the pin to
 */
void Pin::write(pin_val_t value)
{
    this->drive_value = value;
    
    for (vector<AnalogBus *>::iterator it = this->buses.begin(); it != this->buses.end(); it++)
        (*it)->update();
}

/**
 * Like write(), and converts from a digital value. Z cannot be used.
 *
 * @see write()
 * @param value The value to set the pin to
 */
void Pin::writeDigital(bool value)
{
    this->write(value ? PIN_VAL_VCC : 0);
}

/**
 * Gets the effective value of the pin as seen from the *outide*.
 * 
 * It is guaranteed that the read value will be either Z, or a fully resolved
 * voltage (i.e. never PIN_VAL_VCC).
 * 
 * @return The effective value of the pin
 */
pin_val_t Pin::query(void)
{
    if (this->mode == PIN_MODE_INPUT)
        return PIN_VAL_Z;
    
    return (this->drive_value == PIN_VAL_VCC) ? this->owner->_vcc : this->drive_value;
}

/**
 * Drives the pin to a specific value from the *outside*.
 * 
 * @param value The value to set the pin to
 */
void Pin::drive(pin_val_t value)
{
    assert(value != PIN_VAL_VCC);
    
    pin_val_t prev_value = this->read();
    this->last_input = value;
    pin_val_t new_value = this->read();
    
    if ((this->mode == PIN_MODE_INPUT) && (new_value != prev_value)) {
        this->owner->_onPinChanged(this->pin_id, new_value, prev_value);
    }
}

void Pin::setFloatValue(pin_val_t value)
{
    assert(value != PIN_VAL_Z);
    
    pin_val_t prev_value = this->read();
    this->float_value = value;
    pin_val_t new_value = this->read();
    
    if ((this->mode == PIN_MODE_INPUT) && (new_value != prev_value)) {
        this->owner->_onPinChanged(this->pin_id, new_value, prev_value);
    }
}

void Pin::setFloatValueDigital(bool value)
{
    this->setFloatValue(value ? PIN_VAL_VCC : 0);
}

void Pin::setMode(int mode)
{
    if (mode == this->mode)
        return;
    
    pin_val_t prev_value = this->read();
    this->mode = mode;
    this->write(this->drive_value);
    pin_val_t new_value = this->read();
    
    if ((this->mode == PIN_MODE_INPUT) && (new_value != prev_value)) {
        this->owner->_onPinChanged(this->pin_id, new_value, prev_value);
    }
}
