#ifndef _H_I2C_BUS_H
#define _H_I2C_BUS_H

#include <inttypes.h>
#include <vector>

#include <json/json.h>

#include "simulation/entity.h"
#include "simulation/entity_lookup.h"

using namespace std;

class I2cDevice;

class I2cBus : public Entity {
public:
    I2cBus();
    I2cBus(Json::Value &json_data, EntityLookup *lookup);

    void addDevice(I2cDevice *device);
    void removeDevice(I2cDevice *device);
    void sendStart(I2cDevice *sender);
    bool sendAddress(I2cDevice *sender, uint8_t address, bool write);
    bool sendData(I2cDevice *sender, uint8_t data);
    bool queryData(I2cDevice *sender);
    void sendStop(I2cDevice *sender);
private:
    vector<I2cDevice *> devices;
};

#endif
