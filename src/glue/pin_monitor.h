#ifndef _H_PIN_MONITOR_H
#define _H_PIN_MONITOR_H

#include <inttypes.h>
#include <vector>

using namespace std;

class PinMonitor {
public:
    virtual void onPinChanged(int pin, int value) = 0;
};

#endif
