#include <cstring>
#include <cstdlib>

#include "analog_bus.h"
#include "pin_device.h"
#include "utils/fail.h"

using namespace std;

class Pin {
public:
    PinDevice *owner;
    int pin_id;
    const char *pin_name;
    int mode;
    int float_value;
    int drive_value;
    int last_input;
    vector<AnalogBus *> buses;

    void initialize(PinDevice *owner, int pin_id, PinInitData const * init_data);
    void connectToBus(AnalogBus *bus);
    void disconnectFromBus(AnalogBus *bus);
    int read(void);
    int query(void);
    void write(int value);
    void drive(int value);
    void setFloatValue(int value);
    void setMode(int mode);
};

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
 * @return The effective value of the pin
 */
int Pin::read(void)
{
    return (this->last_input != PIN_VAL_Z) ? this->last_input : this->float_value;
}

/**
 * Drives the pin to a specific value from the *inside*.
 * 
 * @param value The value to set the pin to
 */
void Pin::write(int value)
{
    this->drive_value = value;
    
    for (vector<AnalogBus *>::iterator it = this->buses.begin(); it != this->buses.end(); it++)
        (*it)->update();
}

/**
 * Gets the effective value of the pin as seen from the *outide*.
 * 
 * @return The effective value of the pin
 */
int Pin::query(void)
{
    return (this->mode == PIN_MODE_OUTPUT) ? this->drive_value : PIN_VAL_Z;
}

/**
 * Drives the pin to a specific value from the *outside*.
 * 
 * @param value The value to set the pin to
 */
void Pin::drive(int value)
{
    int prev_value = this->read();
    this->last_input = value;
    int new_value = this->read();
    
    if ((this->mode == PIN_MODE_INPUT) && (new_value != prev_value)) {
        this->owner->_onPinChanged(this->pin_id, new_value, prev_value);
    }
}

void Pin::setFloatValue(int value)
{
    int prev_value = this->read();
    this->float_value = value;
    int new_value = this->read();
    
    if ((this->mode == PIN_MODE_INPUT) && (new_value != prev_value)) {
        this->owner->_onPinChanged(this->pin_id, new_value, prev_value);
    }
}

void Pin::setMode(int mode)
{
    if (mode == this->mode)
        return;
    
    int prev_value = this->read();
    this->mode = mode;
    this->write(this->drive_value);
    int new_value = this->read();
    
    if ((this->mode == PIN_MODE_INPUT) && (new_value != prev_value)) {
        this->owner->_onPinChanged(this->pin_id, new_value, prev_value);
    }
}

PinDevice::PinDevice(int num_pins, PinInitData const * const init_data)
{
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

void PinDevice::drivePin(int pin_id, int value)
{
    this->_pins[pin_id].drive(value);
}

int PinDevice::queryPin(int pin_id)
{
    return this->_pins[pin_id].query();
}

void PinDevice::_setPinMode(int pin_id, int mode)
{
    this->_pins[pin_id].setMode(mode);
}

void PinDevice::_setPinFloatValue(int pin_id, int float_value)
{
    this->_pins[pin_id].setFloatValue(float_value);
}

void PinDevice::_pinWrite(int pin_id, int value)
{
    this->_pins[pin_id].write(value);
}

int PinDevice::_pinRead(int pin_id)
{
    return this->_pins[pin_id].read();
}
