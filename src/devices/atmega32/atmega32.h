#ifndef _H_ATMEGA32_H
#define _H_ATMEGA32_H

#include <vector>

#include "cpu_core.h"
#include "glue/pin_device.h"
#include "glue/i2c_device.h"
#include "glue/spi_device.h"
#include "simulation/entity.h"
#include "simulation/sim_device.h"
#include "devices/device.h"
#include "devices/mcu/mcu.h"

#define MEGA32_PORT_COUNT    64

#define MEGA32_PIN_COUNT     32

#define MEGA32_PIN_A0         0
#define MEGA32_PIN_A          0
#define MEGA32_PIN_B          8
#define MEGA32_PIN_C         16
#define MEGA32_PIN_D         24
#define MEGA32_PIN_D7        31

class Atmega32;

struct Atmega32PortMeta
{
    uint8_t write_mask;
    uint8_t clearable_mask;
    void (Atmega32::*read_handler)(uint8_t, int8_t, uint8_t &);
    void (Atmega32::*write_handler)(uint8_t, int8_t, uint8_t, uint8_t, uint8_t);
};

class Atmega32 : public Entity, public Mcu, public I2cDevice, public SpiDevice,
    public PinDevice, public SimulatedDevice {
    friend uint8_t read_port(Atmega32Core *core, uint8_t port);
    friend bool read_port_bit(Atmega32Core *core, uint8_t port, uint8_t bit);
    friend void write_port(Atmega32Core *core, uint8_t port, uint8_t value);
    friend void write_port_bit(Atmega32Core *core, uint8_t port, uint8_t bit, bool value);
public:
    Atmega32();
    Atmega32(Json::Value &json_data);
    
    void loadProgramFromElf(const char *filename);
    void setFrequency(uint64_t frequency);

    virtual void reset(void);
    virtual void act(int event);
    
    virtual int getPC(void);
    virtual Symbol* getProgramSymbol(int pc);
protected:
    uint64_t frequency;
    sim_time_t clock_period;

    Atmega32Core core;
    
    uint8_t *ports; // shortcut
    Atmega32PortMeta port_metas[MEGA32_PORT_COUNT];

    void _init();

    void _onPortRead(uint8_t port, int8_t bit, uint8_t &value);
    uint8_t _onPortPreWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    void _onPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);
    
    void _unsuppPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _unsuppPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared);
    
    uint16_t _get16BitPort(uint8_t port);
    void _put16BitPort(uint8_t port, uint16_t value);
    uint16_t _get16BitReg(uint8_t reg);
  
    void _handleIrqs();
#include "aspects/debug.h"
#include "aspects/twi.h"
#include "aspects/spi.h"
#include "aspects/timers.h"
#include "aspects/pins.h"
#include "aspects/adc.h"
};

#endif
