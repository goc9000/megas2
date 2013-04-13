#ifndef _H_PIN_H
#define _H_PIN_H

#include <vector>

#include "pin_val.h"

using namespace std;

class AnalogBus;
class PinDevice;

struct PinInitData {
    const char *pin_name;
    int mode;
    pin_val_t float_value;
};

/**
 * This represents a pin in an emulated device.
 * 
 * Pins are a more complex device than it might appear at first, so some
 * understading of the model might be helpful. First of all, a pin has two
 * basic modes of operation:
 * 
 * - output mode (PIN_MODE_OUTPUT), in which the pin continuously transmits to
 *   the environment the 'drive value' last set by the owning device.
 * - input mode (PIN_MODE_INPUT), in which the pin passively receives data from
 *   the environment.
 * 
 * Note that the mode may be changed dynamically at runtime using the setMode()
 * function. Also note that the circuitry for the two modes is independent. The
 * received data does not affect the drive value or vice-versa.
 * 
 * Complicating things somewhat is the special 'high impedance' value PIN_VAL_Z.
 * If the drive value is set to Z in an output pin, it will be effectively
 * disconnected from the environment, neither receiving data, nor affecting the
 * outside world.
 * 
 * Conversely, if the pin receives the value Z from the environment (which
 * corresponds to it being physically disconnected, or connected to a currently
 * inactive bus), the read value will default to the 'float value' with which
 * the pin was initialized. Active-low command pins, for instance, have a float
 * value of VCC, so that they default to OFF when disconnected.
 * 
 * Finally, a pin has inherent differences in its behavior when accessed from
 * within vs. outside the device. As such, we have the read() and write()
 * functions for handling the pin from within the device, and the query() and
 * drive() methods, respectively, for handling the pin from the outside. This
 * convention must be strictly adhered to for proper operation of the emulator.
 */
class Pin {
public:
    PinDevice *owner;
    int pin_id;
    const char *pin_name;
    int mode;
    pin_val_t float_value;
    pin_val_t drive_value;
    pin_val_t last_input;
    vector<AnalogBus *> buses;

    void initialize(PinDevice *owner, int pin_id, PinInitData const * init_data);
    void connectToBus(AnalogBus *bus);
    void disconnectFromBus(AnalogBus *bus);
    pin_val_t read(void);
    bool readDigital(void);
    bool isDisconnected(void);
    void write(pin_val_t value);
    void writeDigital(bool value);
    pin_val_t query(void);
    void drive(pin_val_t value);
    void setFloatValue(pin_val_t value);
    void setFloatValueDigital(bool value);
    void setMode(int mode);
};

#endif
