#ifndef _H_PIN_DEVICE_H
#define _H_PIN_DEVICE_H

#include <inttypes.h>
#include <vector>

using namespace std;

#define PIN_MODE_INPUT   0
#define PIN_MODE_OUTPUT  1

#define PIN_VAL_Z        0x0FFFFFFF

class Pin;
class AnalogBus;

struct PinInitData {
    int mode;
    int float_value;
};

class PinDevice {
    friend class Pin;
public:
    PinDevice(int num_pins, PinInitData const * const init_data);
    void connectPinToBus(int pin_id, AnalogBus *bus);
    void disconnectPinFromBus(int pin_id, AnalogBus *bus);
    void drivePin(int pin_id, int data);
    int queryPin(int pin_id);
protected:
    Pin *_pins;
    
    void _pinWrite(int pin_id, int data);
    int _pinRead(int pin_id);
    void _setPinMode(int pin_id, int mode);
    void _setPinFloatValue(int pin_id, int float_value);
    
    virtual void _onPinChanged(int pin_id, int value, int old_value) = 0;
};

#endif

