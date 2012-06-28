#ifndef _H_ATMEGA32_H
#define _H_ATMEGA32_H

#include <vector>

#include "atmega32_core.h"
#include "glue/pin_monitor.h"
#include "glue/i2c_device.h"
#include "glue/spi_device.h"
#include "simulation/sim_device.h"
#include "device.h"

#define MEGA32_PIN_A       0x00
#define MEGA32_PIN_B       0x08
#define MEGA32_PIN_C       0x10
#define MEGA32_PIN_D       0x18
#define MEGA32_PIN_COUNT   0x20

class Atmega32 : public Device, public I2cDevice, public SpiDevice, public SimulatedDevice {
public:
    Atmega32();
    void load_program_from_elf(const char *filename);
    void setFrequency(unsigned frequency_khz);

    void step();
    
    virtual void reset();
    
    virtual void setSimulationTime(sim_time_t time);
    virtual void act();
    virtual sim_time_t nextEventTime();

    void addPinMonitor(int pin, PinMonitor* monitor);
    void removePinMonitor(int pin, PinMonitor* monitor);
    
    virtual void i2cReceiveStart();
    virtual bool i2cReceiveAddress(uint8_t address, bool write);
    virtual bool i2cReceiveData(uint8_t data);
    virtual bool i2cQueryData(uint8_t &data);
    virtual void i2cReceiveStop();

    virtual void spiSlaveSelect(bool select);
    virtual bool spiReceiveData(uint8_t &data);

    // should be protected, but...
    void _onPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _onPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
protected:
    unsigned frequency_khz;
    sim_time_t clock_period;

    sim_time_t next_fetch_time;

    Atmega32Core core;
    
    vector<PinMonitor *> pin_monitors[MEGA32_PIN_COUNT];
    
    bool twi_has_floor;
    bool twi_start_just_sent;
    bool twi_xmit_mode;

    bool spi_stat_read;
    
    // shortcuts
    uint8_t *ports;

    // methods
    uint8_t _read16BitReg(uint8_t reg);
    
    void _triggerPinMonitors(int pin, int value);

    void _handleDataPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _handleDataPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    
    void _twiInit();
    void _twiHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _twiHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    void _twiStatus(uint8_t status);
    
    void _spiInit();
    void _spiHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _spiHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    bool _spiIsEnabled();

    void _timersInit();
    void _timersHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _timersHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);

    void _dumpRegisters();
    void _dumpSram();
};

#endif
