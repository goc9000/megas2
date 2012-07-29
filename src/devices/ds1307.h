#ifndef _H_DS1307_H
#define _H_DS1307_H

#include <inttypes.h>

#include "simulation/entity.h"
#include "simulation/sim_device.h"
#include "glue/i2c_device.h"
#include "devices/device.h"

class Ds1307 : public Entity, public I2cDevice, public SimulatedDevice {
public:
    Ds1307(uint8_t i2c_address);
    Ds1307(Json::Value &json_data);
    
    virtual void reset();
    virtual void act(int event);
    
    virtual void i2cReceiveStart();
    virtual bool i2cReceiveAddress(uint8_t address, bool write);
    virtual bool i2cReceiveData(uint8_t data);
    virtual bool i2cQueryData(uint8_t &data);
    virtual void i2cReceiveStop();
private:
    uint8_t i2c_addr;
    bool i2c_listening;
};

#endif

