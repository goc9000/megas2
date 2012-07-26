#include <cstdlib>

#include "i2c_bus.h"
#include "i2c_device.h"
#include "utils/fail.h"

using namespace std;

I2cDevice::I2cDevice()
{
    this->i2c_bus = NULL;
}

void I2cDevice::connectToI2cBus(I2cBus *bus)
{
    if (bus == this->i2c_bus)
        return;

    if (this->i2c_bus != NULL) {
        this->disconnectFromI2cBus();
    }

    this->i2c_bus = bus;
    bus->addDevice(this);
}

void I2cDevice::disconnectFromI2cBus()
{
    if (this->i2c_bus) {
        I2cBus *bus = this->i2c_bus;
        this->i2c_bus = NULL;
        bus->removeDevice(this);
    }
}

void I2cDevice::_i2cSendStart(void)
{
    if (!this->i2c_bus)
        fail("Attempted to send I2C START while not connected to bus!");
        
    this->i2c_bus->sendStart(this);
}

bool I2cDevice::_i2cSendAddress(uint8_t address, bool write)
{
    if (!this->i2c_bus)
        fail("Attempted to send I2C address while not connected to bus!");
        
    return this->i2c_bus->sendAddress(this, address, write);
}

bool I2cDevice::_i2cSendData(uint8_t data)
{
    if (!this->i2c_bus)
        fail("Attempted to send I2C data while not connected to bus!");
        
    return this->i2c_bus->sendData(this, data);
}

bool I2cDevice::_i2cQueryData()
{
    if (!this->i2c_bus)
        fail("Attempted to query I2C data while not connected to bus!");
        
    return this->i2c_bus->queryData(this);
}

void I2cDevice::_i2cSendStop(void)
{
    if (!this->i2c_bus)
        fail("Attempted to send I2C STOP while not connected to bus!");
        
    this->i2c_bus->sendStop(this);
}
