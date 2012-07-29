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

#define MEGA32_PIN_COUNT     32

#define MEGA32_PIN_A0         0
#define MEGA32_PIN_A          0
#define MEGA32_PIN_B          8
#define MEGA32_PIN_C         16
#define MEGA32_PIN_D         24
#define MEGA32_PIN_D7        31

class Atmega32 : public Entity, public I2cDevice, public SpiDevice, public PinDevice, public SimulatedDevice {
public:
    Atmega32();
    Atmega32(Json::Value &json_data);
    
    void loadProgramFromElf(const char *filename);
    void setFrequency(uint64_t frequency);

    virtual void reset();
    virtual void act(int event);

    // should be protected, but...
    void _onPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _onPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
protected:
    uint64_t frequency;
    sim_time_t clock_period;

    Atmega32Core core;
    
    uint8_t *ports; // shortcut

    void _init();

    uint16_t _get16BitPort(uint8_t port);
    void _put16BitPort(uint8_t port, uint16_t value);
    uint16_t _get16BitReg(uint8_t reg);
  
    void _handleIrqs();
    
    uint8_t _handleFlagBitsInPortWrite(uint8_t flag_bits, int8_t bit, uint8_t &value, uint8_t prev_val);

    void _dumpRegisters();
    void _dumpSram();
#include "inc_twi_aspect.h"
#include "inc_spi_aspect.h"
#include "inc_timers_aspect.h"
#include "inc_pins_aspect.h"
#include "inc_adc_aspect.h"
};

#endif
