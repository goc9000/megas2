#ifndef _H_DS1307_H
#define _H_DS1307_H

#include <inttypes.h>
#include <time.h>

#include "simulation/entity.h"
#include "simulation/sim_device.h"
#include "glue/i2c_device.h"
#include "devices/device.h"

#define DS1307_NVRAM_SIZE  64

class Ds1307 : public Entity, public I2cDevice, public SimulatedDevice {
public:
    Ds1307(uint8_t i2c_address);
    Ds1307(uint8_t i2c_address, string backing_file_name);
    Ds1307(Json::Value &json_data);
    ~Ds1307();
    
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
    
    bool receiving_address;
    uint8_t reg_pointer;
    
    bool time_modified;
    
    uint8_t nvram[DS1307_NVRAM_SIZE];
    uint8_t cached_time[7];
    
    string backing_file_name;
    
    void init(bool init_with_current_time);
    void tick(void);
    void setTime(time_t unix_time);
    bool getTime(time_t &unix_time);
    time_t getTimeAndCheck(void);
    void resetNVRAM(void);
    bool loadNVRAM(void);
    void saveNVRAM(void);
    void resetDividerChain(void);
};

#endif

