#ifndef _H_RESET_PIN_MONITOR_H
#define _H_RESET_PIN_MONITOR_H

#include <inttypes.h>
#include <vector>

#include "pin_monitor.h"
#include "device.h"

using namespace std;

class ResetPinMonitor : public PinMonitor {
public:
    ResetPinMonitor(Device *device, bool active_low);
    
    virtual void onPinChanged(int pin, int value);
private:
    Device *device;
    bool active_low;
};

#endif
