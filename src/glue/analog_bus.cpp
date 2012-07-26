#include <algorithm>

#include "analog_bus.h"
#include "utils/fail.h"

using namespace std;

AnalogBus::AnalogBus() : Entity("analogbus", "Analog Bus")
{
}

AnalogBus::AnalogBus(Json::Value &json_data, EntityLookup *lookup) : Entity(json_data)
{
    if (json_data.isMember("pins")) {
        if (!json_data["pins"].isArray()) {
            fail("'pins' should be an array");
        }

        for (Json::ValueIterator it = json_data["pins"].begin(); it != json_data["pins"].end(); it++) {
            PinReference pinref(*it, lookup);
            this->addDevicePin(pinref);
        }
    }
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

void AnalogBus::addDevicePin(PinReference &pinref)
{
    this->addDevicePin(pinref.device, pinref.pin_id);
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


void AnalogBus::removeDevicePin(PinReference &pinref)
{
    this->removeDevicePin(pinref.device, pinref.pin_id);
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
