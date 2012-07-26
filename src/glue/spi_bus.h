#ifndef _H_SPI_BUS_H
#define _H_SPI_BUS_H

#include <inttypes.h>
#include <vector>

#include <json/json.h>

#include "simulation/entity.h"
#include "simulation/entity_lookup.h"

using namespace std;

class SpiDevice;

class SpiBus : public Entity {
public:
    SpiBus();
    SpiBus(Json::Value &json_data, EntityLookup *lookup);

    void addDevice(SpiDevice *device);
    void removeDevice(SpiDevice *device);
    bool sendData(SpiDevice *sender, uint8_t &data);
private:
    vector<SpiDevice *> devices;
};

#endif
