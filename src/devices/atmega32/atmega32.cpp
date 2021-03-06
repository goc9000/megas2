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

extern PinInitData const MEGA32_PIN_INIT_DATA[MEGA32_PIN_COUNT];

#define DEFAULT_NAME "ATMEGA32"

Atmega32::Atmega32() :
    Entity(DEFAULT_NAME), PinDevice(MEGA32_PIN_COUNT, MEGA32_PIN_INIT_DATA)
{
    _init();
    reset();
}

Atmega32::Atmega32(Json::Value &json_data, EntityLookup *lookup) :
    Entity(DEFAULT_NAME, json_data),
    RS232Device(json_data, lookup),
    PinDevice(MEGA32_PIN_COUNT, MEGA32_PIN_INIT_DATA)
{
    _init();

    uint64_t frequency;
    if (parseOptionalJsonParam(frequency, json_data, "frequency"))
        setFrequency(frequency);
    
    string firmware;
    if (parseOptionalJsonParam(firmware, json_data, "firmware"))
        loadProgramFromElf(firmware.c_str());
    
    reset();
}

void Atmega32::_init(void)
{
    atmega32_core_init(&this->core, this);
    this->setFrequency(16000000ULL);
    
    this->ports = this->core.ram + IO_BASE;
    for (unsigned int i = 0; i < MEGA32_PORT_COUNT; i++) {
        this->port_metas[i].write_mask = 0xff;
        this->port_metas[i].clearable_mask = 0x00;
        this->port_metas[i].unclearable_mask = 0x00;
        this->port_metas[i].read_handler = &Atmega32::_unsuppPortRead;
        this->port_metas[i].write_handler = &Atmega32::_unsuppPortWrite;
    }
    
    this->port_metas[PORT_SFIOR].write_mask = 0xef;
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

void Atmega32::reset(void)
{
    memset(core.ram, 0, RAM_SIZE);
    core.pc = 0;
    
    cycle_count = 0;
    
    _twiInit();
    _spiInit();
    _usartInit();
    _timersInit();
    _pinsInit();
    _adcInit();

    unscheduleAll();
    scheduleEventIn(SIM_EVENT_TICK, clock_period);
}

void Atmega32::act(int event)
{
    _handleIrqs();
    
    switch (event) {
        case SIM_EVENT_TICK:        
            _runTimers();
            
            atmega32_core_step(&core);
            cycle_count++;

            scheduleEventIn(SIM_EVENT_TICK, clock_period);
            break;
        case SIM_EVENT_ADC_COMPLETE_CONVERSION:
            _completeAdcConversion();
            break;
    }
}

int Atmega32::getPC(void)
{
    return this->core.pc;
}

Symbol* Atmega32::getProgramSymbol(int pc)
{
    return this->core.prog_mem.flashSymbolAt(2 * pc);
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

void Atmega32::_onPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
    if (port < REG16_SP-IO_BASE)
        (this->*this->port_metas[port].read_handler)(port, bit, value);
}

uint8_t Atmega32::_onPortPreWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    uint8_t write_mask = this->port_metas[port].write_mask;
    uint8_t clear_mask = this->port_metas[port].clearable_mask;
    uint8_t uncl_mask = this->port_metas[port].unclearable_mask;
    
    if (bit != -1) {
        write_mask &= (1 << bit);
        clear_mask &= (1 << bit);
        uncl_mask &= (1 << bit);
    }
    
    value |= uncl_mask & prev_val;
    
    uint8_t cleared = value & clear_mask;
    
    value = (prev_val & ~write_mask & ~cleared) | (value & write_mask);
    
    return cleared;
}

void Atmega32::_onPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    if (port < REG16_SP-IO_BASE)
        (this->*this->port_metas[port].write_handler)(port, bit, value, prev_val, cleared);
}

void Atmega32::_unsuppPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
    this->_dumpPortRead("UNSUPP", port, bit, value);
}

void Atmega32::_unsuppPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    this->_dumpPortWrite("UNSUPP", port, bit, value, prev_val, cleared);
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
