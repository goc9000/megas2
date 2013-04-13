#ifndef _H_PIN_REFERENCE_H
#define _H_PIN_REFERENCE_H

#include <json/json.h>

#include "simulation/entity_lookup.h"

using namespace std;

class PinDevice;

class PinReference {
public:
    PinReference(PinDevice *device, int pin_id) : device(device), pin_id(pin_id) {};
    PinReference(Json::Value &json_data, EntityLookup *lookup);
    PinReference(PinDevice *device, const char *pin_name);
    
    PinDevice *device;
    int pin_id;
};

#endif
