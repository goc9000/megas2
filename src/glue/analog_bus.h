#ifndef _H_ANALOG_BUS_H
#define _H_ANALOG_BUS_H

#include <vector>

#include <json/json.h>

#include "pin_device.h"
#include "pin_ref.h"
#include "simulation/entity.h"
#include "simulation/entity_lookup.h"

using namespace std;

class AnalogBus : public Entity {
public:
    AnalogBus();
    AnalogBus(Json::Value &json_data, EntityLookup *lookup);
    
    void addDevicePin(PinDevice *device, int pin_id);
    void addDevicePin(PinReference &pinref);
    void removeDevicePin(PinDevice *device, int pin_id);
    void removeDevicePin(PinReference &pinref);
    int query(void);
    void update(void);
private:
    int _value;
    
    vector<PinReference> _pins;
};

#endif
