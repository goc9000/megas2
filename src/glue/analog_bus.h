#ifndef _H_ANALOG_BUS_H
#define _H_ANALOG_BUS_H

#include <vector>

using namespace std;

class PinDevice;

struct PinReference {
    PinReference(PinDevice *device, int pin_id) : device(device), pin_id(pin_id) {};
    
    PinDevice *device;
    int pin_id;
};

class AnalogBus {
public:
    AnalogBus();
    AnalogBus(PinDevice* device1, int pin_id1);
    AnalogBus(PinDevice* device1, int pin_id1, PinDevice* device2, int pin_id2);
    AnalogBus(PinDevice* device1, int pin_id1, PinDevice* device2, int pin_id2, PinDevice* device3, int pin_id3);

    void addDevicePin(PinDevice *device, int pin_id);
    void removeDevicePin(PinDevice *device, int pin_id);
    int query(void);
    void update(void);
private:
    int _value;
    
    vector<PinReference> _pins;
};

#endif
