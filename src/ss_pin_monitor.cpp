#include "ss_pin_monitor.h"

using namespace std;

SlaveSelectPinMonitor::SlaveSelectPinMonitor(SpiDevice *device, bool active_low)
{
    this->device = device;
    this->active_low = active_low;
}

#include "fail.h"
void SlaveSelectPinMonitor::onPinChanged(int pin, int value)
{
    this->device->spiSlaveSelect(value ^ active_low);
}
