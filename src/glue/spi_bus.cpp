#include <algorithm>

#include "spi_bus.h"
#include "spi_device.h"
#include "utils/fail.h"

using namespace std;

void SpiBus::addDevice(SpiDevice *device)
{
    if (find(this->devices.begin(), this->devices.end(), device) != this->devices.end())
        fail("Added device twice to SPI bus");
    
    this->devices.push_back(device);
}

void SpiBus::removeDevice(SpiDevice *device)
{
    vector<SpiDevice*>::iterator it = find(this->devices.begin(), this->devices.end(), device);
    
    this->devices.erase(it);
}

bool SpiBus::sendData(SpiDevice *sender, uint8_t &data)
{
    bool ack = false;
    
    for (unsigned int i = 0; i < this->devices.size(); i++)
        if (this->devices[i] != sender) {
            bool dev_ack = this->devices[i]->spiReceiveData(data);
            
            if (ack && dev_ack)
                fail("Multiple devices ACK'ed SPI data");
            
            ack |= dev_ack;
        }

    if (!ack)
        data = 0xff;
    
    return ack;
}
