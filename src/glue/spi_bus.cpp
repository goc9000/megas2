#include <algorithm>

#include "spi_bus.h"
#include "spi_device.h"
#include "utils/fail.h"

using namespace std;

SpiBus::SpiBus() : Entity("spibus", "SPI Bus")
{
}

SpiBus::SpiBus(Json::Value &json_data, EntityLookup *lookup) : Entity(json_data)
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

            SpiDevice *as_spi_dev = dynamic_cast<SpiDevice *>(ent);
            if (as_spi_dev == NULL) {
                fail("Device '%s' is not a SPI device", dev_id);
            }

            this->addDevice(as_spi_dev);
        }
    }
}

void SpiBus::addDevice(SpiDevice *device)
{
    if (find(this->devices.begin(), this->devices.end(), device) != this->devices.end())
        return;
    
    this->devices.push_back(device);
    device->connectToSpiBus(this);
}

void SpiBus::removeDevice(SpiDevice *device)
{
    vector<SpiDevice*>::iterator it = find(this->devices.begin(), this->devices.end(), device);
    if (it != this->devices.end()) {
        this->devices.erase(it);
        device->disconnectFromSpiBus();
    }
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
