#include <cstdlib>

#include "spi_bus.h"
#include "spi_device.h"
#include "fail.h"

using namespace std;

SpiDevice::SpiDevice()
{
    this->spi_bus = NULL;
}

void SpiDevice::connectToSpiBus(SpiBus *bus)
{
    if (this->spi_bus) {
        this->spi_bus->removeDevice(this);
    }
    
    this->spi_bus = bus;
    
    bus->addDevice(this);
}

bool SpiDevice::_spiSendData(uint8_t &data)
{
    if (!this->spi_bus)
        fail("Attempted to read/write SPI data while not connected to bus!");
        
    return this->spi_bus->sendData(this, data);
}
