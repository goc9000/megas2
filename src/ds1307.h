#ifndef _H_DS1307_H
#define _H_DS1307_H

#include <inttypes.h>

#include "i2c_device.h"
#include "device.h"

class Ds1307 : public Device, public I2cDevice {
public:
    Ds1307(uint8_t i2c_address);
    void reset();
    virtual sim_time_t nextEventTime();
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

