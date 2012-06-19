#include <algorithm>

#include "i2c_bus.h"
#include "i2c_device.h"
#include "fail.h"

using namespace std;

void I2cBus::addDevice(I2cDevice *device)
{
    if (find(this->devices.begin(), this->devices.end(), device) != this->devices.end())
        fail("Added device twice to I2C bus");
    
    this->devices.push_back(device);
}

void I2cBus::removeDevice(I2cDevice *device)
{
    vector<I2cDevice*>::iterator it = find(this->devices.begin(), this->devices.end(), device);
    
    this->devices.erase(it);
}

void I2cBus::sendStart(I2cDevice *sender)
{
    for (unsigned int i = 0; i < this->devices.size(); i++)
        if (this->devices[i] != sender)
            this->devices[i]->i2cReceiveStart();
}

bool I2cBus::sendAddress(I2cDevice *sender, uint8_t address, bool write)
{
    bool ack = false;
    
    for (unsigned int i = 0; i < this->devices.size(); i++)
        if (this->devices[i] != sender) {
            bool dev_ack = this->devices[i]->i2cReceiveAddress(address, write);
            
            if (ack && dev_ack)
                fail("Multiple devices ACK'ed I2C address %02x", address);
            
            ack |= dev_ack;
        }
    
    return ack;
}

bool I2cBus::sendData(I2cDevice *sender, uint8_t data)
{
    bool ack = false;
    
    for (unsigned int i = 0; i < this->devices.size(); i++)
        if (this->devices[i] != sender) {
            bool dev_ack = this->devices[i]->i2cReceiveData(data);
            
            if (ack && dev_ack)
                fail("Multiple devices ACK'ed data");
            
            ack |= dev_ack;
        }
    
    return ack;
}

bool I2cBus::queryData(I2cDevice *sender)
{
    bool ack = false;
    uint8_t data;
    I2cDevice *emitter;
    
    for (unsigned int i = 0; i < this->devices.size(); i++)
        if (this->devices[i] != sender) {
            bool dev_ack = this->devices[i]->i2cQueryData(data);
            
            if (ack && dev_ack)
                fail("Multiple devices ACK'ed query");
            
            if (dev_ack) {
                ack = true;
                emitter = this->devices[i];
            }
        }
        
    if (!ack)
        return false;
    
    for (unsigned int i = 0; i < this->devices.size(); i++)
        if (this->devices[i] != emitter)
            this->devices[i]->i2cReceiveData(data);
    
    return true;
}

void I2cBus::sendStop(I2cDevice *sender)
{
    for (unsigned int i = 0; i < this->devices.size(); i++)
        if (this->devices[i] != sender)
            this->devices[i]->i2cReceiveStop();
}
