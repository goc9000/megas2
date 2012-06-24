#ifndef _H_I2C_DEVICE_H
#define _H_I2C_DEVICE_H

#include <inttypes.h>

class I2cBus;

class I2cDevice {
public:
    I2cDevice();
    void connectToI2cBus(I2cBus *bus);
    virtual void i2cReceiveStart() = 0;
    virtual bool i2cReceiveAddress(uint8_t address, bool write) = 0;
    virtual bool i2cReceiveData(uint8_t data) = 0;
    virtual bool i2cQueryData(uint8_t &data) = 0;
    virtual void i2cReceiveStop() = 0;
protected:
    I2cBus *i2c_bus;

    void _i2cSendStart();
    bool _i2cSendAddress(uint8_t address, bool write);
    bool _i2cSendData(uint8_t data);
    bool _i2cQueryData();
    void _i2cSendStop();
};

#endif
