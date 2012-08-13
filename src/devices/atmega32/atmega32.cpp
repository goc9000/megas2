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
        case PORT_ASSR: return 0x08;
        case PORT_ADCSRA: return 0xef;
        case PORT_ADCL: return 0x00;
        case PORT_ADCH: return 0x00;
        case PORT_SFIOR: return 0xe7;
    }
    
    return 0xff;
}

uint8_t Atmega32::_getPortClearableMask(uint8_t port)
{
    switch (port) {
        case PORT_TWCR: return 0x80;
        case PORT_TIFR: return 0xff;
        case PORT_ADCSRA: return 0x10;
    }
    
    return 0x00;
}

uint8_t Atmega32::_adjustPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val)
{
    uint8_t clr_mask = this->_getPortClearableMask(port);
    uint8_t wr_mask = this->_getPortWriteMask(port);

    if (bit != -1) {
        clr_mask &= _BV(bit);
        wr_mask &= _BV(bit);
    }

    return ((prev_val & ~(value & clr_mask)) & ~wr_mask) | (value & wr_mask);
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
        if (bit < 0) {
            printf("%04x: IN %s(%02x) == %02x\n", this->core.last_inst_pc * 2, PORT_NAMES[port], port,
                value);
        } else {
            printf("%04x: IN %s(%02x).%d == %02x\n", this->core.last_inst_pc * 2, PORT_NAMES[port], port, bit,
                (value >> bit) & 1);
        }
    }
}

void Atmega32::_onPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val)
{
    if (port >= REG16_SP-IO_BASE)
        return;
    
    if (is_twi_port(port)) {
        this->_twiHandleWrite(port, bit, value, prev_val);
    } else if (is_spi_port(port)) {
        this->_spiHandleWrite(port, bit, value, prev_val);
    } else if (is_data_port(port)) {
        this->_handleDataPortWrite(port, bit, value, prev_val);
    } else if (is_timer_port(port)) {
        this->_timersHandleWrite(port, bit, value, prev_val);
    } else if (is_adc_port(port)) {
        this->_adcHandleWrite(port, bit, value, prev_val);
    } else {
        if (bit < 0) {
            printf("%04x: OUT %s(%02x) <- %02x (was %02x)\n", this->core.last_inst_pc * 2, PORT_NAMES[port], port,
                value, prev_val);
        } else {
            printf("%04x: OUT %s(%02x).%d <- %d (was %d)\n", this->core.last_inst_pc * 2, PORT_NAMES[port], port, bit,
                (value >> bit) & 1, (prev_val >> bit) & 1);
        }
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

void Atmega32::_dumpRegisters()
{
    char const *SREG_REP = "ithsvnzc";
    char sreg_rep[9];
    
    printf("--- Registers ---\n");
    
    for (int i=0; i<IO_BASE; i++) {
        printf("%c%02d", i ? ' ' : 'r', i);
    }
    printf("\n");
    for (int i=0; i<IO_BASE; i++) {
        printf(" %02x", this->core.ram[i]);
    }
    printf("\n");
    
    uint8_t sreg = this->core.ram[REG_SREG];
    for (int i=0; i<8; i++)
        sreg_rep[i] = bit_is_set(sreg, 7-i) ? toupper(SREG_REP[i]) : SREG_REP[i];
    sreg_rep[8] = 0;

    printf("X=%04x Y=%04x Z=%04x SP=%04x SREG=%02x (%s)\n",
        this->_get16BitReg(REG16_X), this->_get16BitReg(REG16_Y),
        this->_get16BitReg(REG16_Z), this->_get16BitReg(REG16_SP),
        sreg, sreg_rep);
    
    printf("Last fetch @PC=%04x (in bytes: %04x)\n", this->core.last_inst_pc, 2*this->core.last_inst_pc);
}

void Atmega32::_dumpSram()
{
    const int BYTES_PER_BLOCK = 32;
    int size = RAM_SIZE - SRAM_BASE;
    int blocks = (size + BYTES_PER_BLOCK-1) / BYTES_PER_BLOCK;
    
    printf("--- SRAM contents ---\n");
    
    printf("     ");
    for (int i = 0; i < BYTES_PER_BLOCK; i++)
        printf("%c%02x", i ? ' ' : '+', i);
    printf("\n");
    
    for (int i = 0; i < blocks; i++) {
        int addr = i * BYTES_PER_BLOCK;
        printf("%04x:", addr);
        
        int count = min(size - addr, BYTES_PER_BLOCK);
        for (int j = 0; j < count; j++)
            printf(" %02x", this->core.ram[SRAM_BASE + addr + j]);
        for (int j = BYTES_PER_BLOCK; j < count; j++)
            printf("   ");
        printf(" | ");
        for (int j = 0; j < count; j++) {
            uint8_t byte = this->core.ram[SRAM_BASE + addr + j];
            printf("%c", ((byte >= 32) && (byte <= 126)) ? byte : '.');
        }
        printf("\n");
    }
}
