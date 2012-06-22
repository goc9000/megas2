#include "reset_pin_monitor.h"

using namespace std;

ResetPinMonitor::ResetPinMonitor(Device *device, bool active_low)
{
    this->device = device;
    this->active_low = active_low;
}

void ResetPinMonitor::onPinChanged(int pin, int value)
{
    if (!(value ^ active_low))
        this->device->reset();
}
