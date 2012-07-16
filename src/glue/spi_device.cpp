#include <cstdlib>

#include "spi_bus.h"
#include "spi_device.h"
#include "utils/fail.h"

using namespace std;

SpiDevice::SpiDevice()
{
    this->spi_bus = NULL;
    this->spi_selected = false;
}

void SpiDevice::connectToSpiBus(SpiBus *bus)
{
    if (this->spi_bus) {
        this->spi_bus->removeDevice(this);
    }
    
    this->spi_bus = bus;
    
    bus->addDevice(this);
}

void SpiDevice::_spiSlaveSelect(bool select)
{
    if (this->spi_selected != select) {
        this->spi_selected = select;
        this->_onSpiSlaveSelect(select);
    }
}

void SpiDevice::_onSpiSlaveSelect(bool select)
{
    // do nothing
}

bool SpiDevice::_spiSendData(uint8_t &data)
{
    if (!this->spi_bus)
        fail("Attempted to read/write SPI data while not connected to bus!");
        
    return this->spi_bus->sendData(this, data);
}
