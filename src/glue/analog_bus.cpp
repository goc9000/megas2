#include <algorithm>

#include "analog_bus.h"
#include "pin_device.h"
#include "utils/fail.h"

using namespace std;

AnalogBus::AnalogBus()
{
}

AnalogBus::AnalogBus(PinDevice* device1, int pin_id1)
{
    this->addDevicePin(device1, pin_id1);
}

AnalogBus::AnalogBus(PinDevice* device1, int pin_id1, PinDevice* device2, int pin_id2)
{
    this->addDevicePin(device1, pin_id1);
    this->addDevicePin(device2, pin_id2);
}

AnalogBus::AnalogBus(PinDevice* device1, int pin_id1, PinDevice* device2, int pin_id2, PinDevice* device3, int pin_id3)
{
    this->addDevicePin(device1, pin_id1);
    this->addDevicePin(device2, pin_id2);
    this->addDevicePin(device3, pin_id3);
}

void AnalogBus::addDevicePin(PinDevice *device, int pin_id)
{
    for (vector<PinReference>::iterator it = this->_pins.begin(); it != this->_pins.end(); it++)
        if ((it->device == device) && (it->pin_id = pin_id))
            return;
    
    PinReference pin_ref(device, pin_id);
    this->_pins.push_back(pin_ref);
    
    device->connectPinToBus(pin_id, this);
    
    this->update();
}

void AnalogBus::removeDevicePin(PinDevice *device, int pin_id)
{
    for (vector<PinReference>::iterator it = this->_pins.begin(); it != this->_pins.end(); it++)
        if ((it->device == device) && (it->pin_id = pin_id)) {
            this->_pins.erase(it);
            device->disconnectPinFromBus(pin_id, this);
            
            this->update();
            return;
        }
}

int AnalogBus::query(void)
{
    return this->_value;
}

void AnalogBus::update(void)
{
    int prev_value = this->query();
 
    int value = PIN_VAL_Z;
    
    for (vector<PinReference>::iterator it = this->_pins.begin(); it != this->_pins.end(); it++) {
        int v = it->device->queryPin(it->pin_id);

        if (v != PIN_VAL_Z)
            value = v;
    }
    
    if (value != prev_value) {
        this->_value = value;
        
        for (vector<PinReference>::iterator it = this->_pins.begin(); it != this->_pins.end(); it++)
            it->device->drivePin(it->pin_id, this->_value);
    }
}
