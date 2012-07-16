#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdlib>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "enc28j60.h"

using namespace std;

#define E28J_REVISION_ID              0x05

#define OPCODE_READ_CONTROL_REG       0
#define OPCODE_WRITE_CONTROL_REG      2
#define OPCODE_BIT_FIELD_SET          4
#define OPCODE_BIT_FIELD_CLEAR        5

// Registers
#define REG_ECON1                     0x1f
#define REG_EREVID                    0x72

// Register bits

#define STATE_RECEIVING_COMMAND       0
#define STATE_RECEIVING_COMMAND_ARG   1
#define STATE_PRE_RESPONDING          2
#define STATE_RESPONDING              3

// Pin initialization data

PinInitData const PIN_INIT_DATA[E28J_PIN_COUNT] = {
    { PIN_MODE_INPUT, 0 }, // RESET
    { PIN_MODE_INPUT, 1 }  // SLAVE_SELECT
};

static bool is_common_reg(uint8_t reg)
{
    return (reg & 0x1f) >= 0x1b;
}

static bool is_mac_reg(uint8_t reg)
{
    return
        ((reg >= 0x40) && (reg <= 0x4d)) ||
        ((reg >= 0x60) && (reg <= 0x65)) ||
        (reg == 0x6A);
}

static bool is_mii_reg(uint8_t reg)
{
    return (reg >= 0x51) && (reg <= 0x59);
}

Enc28J60::Enc28J60() : PinDevice(E28J_PIN_COUNT, PIN_INIT_DATA)
{
    this->reset();
}

void Enc28J60::act()
{
    // ENC28J60 does nothing on its own (for now)
}

sim_time_t Enc28J60::nextEventTime()
{
    return SIM_TIME_NEVER;
}

void Enc28J60::reset()
{
    memset(this->regs, 0, E28J_REGS_COUNT);
    memset(this->phy_regs, 0, E28J_PHY_REGS_COUNT);
    memset(this->eth_buffer, 0, E28J_ETH_BUFFER_SIZE);
    
    this->regs[REG_EREVID] = E28J_REVISION_ID;
}

void Enc28J60::_onPinChanged(int pin_id, int value, int old_value)
{
    switch (pin_id) {
        case E28J_PIN_RESET:
            if (value)
                this->reset();
            return;
        case E28J_PIN_SLAVE_SELECT:
            this->_spiSlaveSelect(!value);
            return;
    }
}

bool Enc28J60::spiReceiveData(uint8_t &data)
{
    if (!this->spi_selected)
        return false;
    
    data = _handleSpiData(data);
    
    return true;
}

void Enc28J60::_onSpiSlaveSelect(bool select)
{
    if (select) {
        this->state = STATE_RECEIVING_COMMAND;
    }
}

uint8_t Enc28J60::_handleSpiData(uint8_t data)
{
    switch (this->state) {
        case STATE_RECEIVING_COMMAND:
            return this->_handleCommandStart(data);
        case STATE_RECEIVING_COMMAND_ARG:
            return this->_handleCommandArg(data);
        case STATE_PRE_RESPONDING:
            this->state = STATE_RESPONDING;
            return 0xff;
        case STATE_RESPONDING:
            this->state = STATE_RECEIVING_COMMAND;
            return this->response_byte;
    }
    
    return 0xff;
}

uint8_t Enc28J60::_handleCommandStart(uint8_t data)
{
    uint8_t opcode = (data >> 5) & 0x07;
    
    switch (opcode) {
        case OPCODE_READ_CONTROL_REG:
            return this->_execReadCtrlReg(data & 0x1f);
        case OPCODE_WRITE_CONTROL_REG:
        case OPCODE_BIT_FIELD_CLEAR:
        case OPCODE_BIT_FIELD_SET:
            this->cmd_byte = data;
            this->state = STATE_RECEIVING_COMMAND_ARG;
            return 0xff;
    }
    
    fail("Invalid command byte received by ENC28J60: %02x", data);
    
    return 0xff;
}

uint8_t Enc28J60::_handleCommandArg(uint8_t data)
{
    uint8_t opcode = (this->cmd_byte >> 5) & 0x07;

    switch (opcode) {
        case OPCODE_WRITE_CONTROL_REG:
            return this->_execWriteCtrlReg(this->cmd_byte & 0x1f, data);
        case OPCODE_BIT_FIELD_CLEAR:
            return this->_execBitFieldClear(this->cmd_byte & 0x1f, data);
        case OPCODE_BIT_FIELD_SET:
            return this->_execBitFieldSet(this->cmd_byte & 0x1f, data);
    }
    
    return 0xff;
}

uint8_t Enc28J60::_execReadCtrlReg(uint8_t reg)
{
    reg = this->_mapRegister(reg);
    
    this->response_byte = this->regs[reg];
    this->state = (is_mac_reg(reg) || is_mii_reg(reg)) ? STATE_PRE_RESPONDING : STATE_RESPONDING; 
    
    return 0xff;
}

uint8_t Enc28J60::_execWriteCtrlReg(uint8_t reg, uint8_t data)
{
    reg = this->_mapRegister(reg);
    
    // TODO: intercept this
    this->regs[reg] = data;
    
    this->state = STATE_RECEIVING_COMMAND;
    
    return 0xff;
}

uint8_t Enc28J60::_execBitFieldClear(uint8_t reg, uint8_t data)
{
    reg = this->_mapRegister(reg);
    
    if (is_mac_reg(reg) || is_mii_reg(reg))
        fail("Attempted Bit Field Clear on MAC/MII register %02x", reg);
    
    // TODO: intercept this
    this->regs[reg] &= ~data;
    
    this->state = STATE_RECEIVING_COMMAND;
    
    return 0xff;
}

uint8_t Enc28J60::_execBitFieldSet(uint8_t reg, uint8_t data)
{
    reg = this->_mapRegister(reg);
    
    if (is_mac_reg(reg) || is_mii_reg(reg))
        fail("Attempted Bit Field Set on MAC/MII register %02x", reg);
    
    // TODO: intercept this
    this->regs[reg] |= data;
    
    this->state = STATE_RECEIVING_COMMAND;
    
    return 0xff;
}

uint8_t Enc28J60::_mapRegister(uint8_t reg)
{
    if (reg > 0x1f)
        fail("ENC28J60: Tried to map already mapped register!");
    
    if (is_common_reg(reg))
        return reg;
    
    return ((this->regs[REG_ECON1] & 0x03) << 5) + reg;
}
