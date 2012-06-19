#ifndef _H_SS_PIN_MONITOR_H
#define _H_SS_PIN_MONITOR_H

#include <inttypes.h>
#include <vector>

#include "pin_monitor.h"
#include "spi_device.h"

using namespace std;

class SlaveSelectPinMonitor : public PinMonitor {
public:
    SlaveSelectPinMonitor(SpiDevice *device, bool active_low);
    
    virtual void onPinChanged(int pin, int value);
private:
    SpiDevice *device;
    bool active_low;
};

#endif
