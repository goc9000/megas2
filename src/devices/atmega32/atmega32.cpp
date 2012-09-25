#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctype.h>
#include <algorithm>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "atmega32.h"
#include "defs.h"

using namespace std;

#define SIM_EVENT_TICK   0

extern PinInitData const MEGA32_PIN_INIT_DATA[MEGA32_PIN_COUNT];

Atmega32::Atmega32() :
    Entity("atmega32", "ATMEGA32"), PinDevice(MEGA32_PIN_COUNT, MEGA32_PIN_INIT_DATA)
{
    this->_init();
    this->reset();
}

Atmega32::Atmega32(Json::Value &json_data) :
    Entity(json_data), PinDevice(MEGA32_PIN_COUNT, MEGA32_PIN_INIT_DATA)
{
    this->_init();

    if (json_data.isMember("frequency")) {
        this->setFrequency(json_data["frequency"].asUInt64());
    }
    if (json_data.isMember("firmware")) {
        this->loadProgramFromElf(json_data["firmware"].asCString());
    }
    
    this->reset();
}

void Atmega32::_init()
{
    atmega32_core_init(&this->core, this);
    this->ports = this->core.ram + IO_BASE;
    this->setFrequency(16000000ULL);
}

void Atmega32::setFrequency(uint64_t frequency)
{
    this->frequency = frequency;
    this->clock_period = sec_to_sim_time(1) / frequency;
}

void Atmega32::loadProgramFromElf(const char *filename)
{
    this->core.prog_mem.loadElf(filename);
}

void Atmega32::reset()
{
    memset(this->core.ram, 0, RAM_SIZE);
    this->core.pc = 0;
    
    this->_twiInit();
    this->_spiInit();
    this->_timersInit();
    this->_pinsInit();
    this->_adcInit();

    if (this->simulation) {
        this->simulation->unscheduleAll(this);
        this->simulation->scheduleEvent(this, SIM_EVENT_TICK, this->simulation->time + this->clock_period);
    }
}

void Atmega32::act(int event)
{
    this->_handleIrqs();
    this->_runTimers();
    atmega32_core_step(&this->core);

    if (this->simulation) {
        this->simulation->scheduleEvent(this, SIM_EVENT_TICK, this->simulation->time + this->clock_period);
    }
}

void Atmega32::_handleIrqs()
{
    uint8_t irq;
    
    if (!get_flag(&this->core, FLAG_I))
        return;
    
    if ((irq = this->_handleTimerIrqs())) goto exec;
    if ((irq = this->_handleAdcIrqs())) goto exec;
    return;
exec:
    set_flag(&this->core, FLAG_I, false);
    push_word(&this->core, this->core.pc);
    this->core.pc = 2*(irq-1);
}

uint8_t Atmega32::_getPortWriteMask(uint8_t port)
{
    switch (port) {
        case PORT_TWCR: return 0x75;
        case PORT_TWSR: return 0x03;
        case PORT_SPSR: return 0x01;
        case PORT_TIFR: return 0x00;
        case PORT_ADCSRA: return 0xef;
        case PORT_ADCL: return 0x00;
        case PORT_ADCH: return 0x00;
        case PORT_SFIOR: return 0xef;
        case PORT_TCCR1A: return 0xf3;
        case PORT_TCCR1B: return 0xdf;
    }
    
    return 0xff;
}

uint8_t Atmega32::_getPortClearableMask(uint8_t port)
{
    switch (port) {
        case PORT_TWCR: return 0x80;
        case PORT_ADCSRA: return 0x10;
        case PORT_TIFR: return 0xff;
        case PORT_TCCR1A: return 0xc0;
    }
    
    return 0x00;
}

void Atmega32::_onPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
    if (port >= REG16_SP-IO_BASE)
        return;
    
    if (is_twi_port(port)) {
        this->_twiHandleRead(port, bit, value);
    } else if (is_spi_port(port)) {
        this->_spiHandleRead(port, bit, value);
    } else if (is_data_port(port)) {
        this->_handleDataPortRead(port, bit, value);
    } else if (is_timer_port(port)) {
        this->_timersHandleRead(port, bit, value);
    } else if (is_adc_port(port)) {
        this->_adcHandleRead(port, bit, value);
    } else {
        this->_dumpPortRead("UNSUPP", port, bit, value);
    }
}

uint8_t Atmega32::_onPortPreWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    uint8_t write_mask = this->_getPortWriteMask(port);
    uint8_t clear_mask = this->_getPortClearableMask(port);
    
    if (bit != -1) {
        write_mask &= (1 << bit);
        clear_mask &= (1 << bit);
    }
    
    uint8_t cleared = value & clear_mask;
    
    value = (prev_val & ~write_mask & ~cleared) | (value & write_mask);
    
    return cleared;
}

void Atmega32::_onPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    if (port >= REG16_SP-IO_BASE)
        return;
    
    if (is_twi_port(port)) {
        this->_twiHandleWrite(port, bit, value, prev_val, cleared);
    } else if (is_spi_port(port)) {
        this->_spiHandleWrite(port, bit, value, prev_val, cleared);
    } else if (is_data_port(port)) {
        this->_handleDataPortWrite(port, bit, value, prev_val, cleared);
    } else if (is_timer_port(port)) {
        this->_timersHandleWrite(port, bit, value, prev_val, cleared);
    } else if (is_adc_port(port)) {
        this->_adcHandleWrite(port, bit, value, prev_val, cleared);
    } else {
        this->_dumpPortWrite("UNSUPP", port, bit, value, prev_val, cleared);
    }
}

uint16_t Atmega32::_get16BitPort(uint8_t port)
{
    return (this->ports[port+1] << 8) + this->ports[port];
}

void Atmega32::_put16BitPort(uint8_t port, uint16_t value)
{
    this->ports[port] = low_byte(value);
    this->ports[port+1] = high_byte(value);
}

uint16_t Atmega32::_get16BitReg(uint8_t reg)
{
    return (this->core.ram[reg+1] << 8) + this->core.ram[reg];
}
