#ifndef _H_PIN_DEVICE_H
#define _H_PIN_DEVICE_H

#include "pin.h"
#include "pin_ref.h"
#include "simulation/entity_lookup.h"

using namespace std;

#define PIN_MODE_INPUT   0
#define PIN_MODE_OUTPUT  1

const pin_val_t DEFAULT_VCC = 5.0;
const pin_val_t DEFAULT_LOGIC_THRESHOLD = 1.5;

class AnalogBus;

class PinDevice {
    friend class Pin;
public:
    PinDevice(int num_pins, PinInitData const * const init_data);
    void connectPinToBus(int pin_id, AnalogBus *bus);
    void disconnectPinFromBus(int pin_id, AnalogBus *bus);
    void drivePin(int pin_id, pin_val_t data);
    pin_val_t queryPin(int pin_id);
    int lookupPin(const char *pin_name);
protected:
    Pin *_pins;
    int _num_pins;
    
    pin_val_t _vcc;
    pin_val_t _logic_threshold;
    
    virtual void _onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value) = 0;
};

#endif

