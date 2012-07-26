#include <cstdlib>

#include "pin_ref.h"
#include "pin_device.h"
#include "utils/fail.h"

using namespace std;

PinReference::PinReference(Json::Value &json_value, EntityLookup *lookup)
{
    if (!json_value.isMember("device"))
        fail("Member 'device' required in JSON object for pin reference");
    if (!json_value.isMember("pin"))
        fail("Member 'pin' required in JSON object for pin reference");

    const char *dev_id = json_value["device"].asCString();
    const char *pin_name = json_value["pin"].asCString();

    Entity *ent = lookup->lookupEntity(dev_id);
    if (!ent) {
        fail("Device '%s' not defined at this point", dev_id);
    }

    this->device = dynamic_cast<PinDevice *>(ent);
    if (!this->device) {
        fail("Device '%s' is not a pin device", dev_id);
    }

    this->pin_id = this->device->lookupPin(pin_name);
    if (this->pin_id < 0) {
        fail("Pin '%s' not found in devices '%s'", pin_name, dev_id);
    }
}
