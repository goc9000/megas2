#include <algorithm>

#include "i2c_bus.h"
#include "i2c_device.h"
#include "utils/fail.h"

using namespace std;

#define DEFAULT_NAME "I2C bus"

I2cBus::I2cBus() : Entity(DEFAULT_NAME)
{
}

I2cBus::I2cBus(Json::Value &json_data, EntityLookup *lookup)
    : Entity(DEFAULT_NAME, json_data)
{
    if (json_data.isMember("devices")) {
        if (!json_data["devices"].isArray()) {
            fail("'devices' should be an array");
        }

        for (Json::ValueIterator it = json_data["devices"].begin(); it != json_data["devices"].end(); it++) {
            const char *dev_id = (*it).asCString();
            Entity *ent = lookup->lookupEntity(dev_id);
            
            if (!ent) {
                fail("Device '%s' not defined at this point", dev_id);
            }

            I2cDevice *as_i2c_dev = dynamic_cast<I2cDevice *>(ent);
            if (as_i2c_dev == NULL) {
                fail("Device '%s' is not a I2C device", dev_id);
            }

            this->addDevice(as_i2c_dev);
        }
    }
}

void I2cBus::addDevice(I2cDevice *device)
{
    if (find(this->devices.begin(), this->devices.end(), device) != this->devices.end())
        return;
    
    this->devices.push_back(device);
    device->connectToI2cBus(this);
}

void I2cBus::removeDevice(I2cDevice *device)
{
    vector<I2cDevice*>::iterator it = find(this->devices.begin(), this->devices.end(), device);
    if (it != this->devices.end()) {
        this->devices.erase(it);
        device->disconnectFromI2cBus();
    }
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
    I2cDevice *emitter = NULL;
    
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
