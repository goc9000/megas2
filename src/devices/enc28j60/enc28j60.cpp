#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdlib>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "enc28j60.h"
#include "defs.h"

using namespace std;

#define STATE_RECEIVING_COMMAND       0
#define STATE_RECEIVING_COMMAND_ARG   1
#define STATE_PRE_RESPONDING          2
#define STATE_RESPONDING              3
#define STATE_RECEIVING_BUFFER_DATA   4

// Pin initialization data

PinInitData const PIN_INIT_DATA[E28J_PIN_COUNT] = {
    { "RESET", PIN_MODE_INPUT, 1 }, // RESET
    { "SS", PIN_MODE_INPUT, 1 }  // SLAVE_SELECT
};

Enc28J60::Enc28J60()
    : Entity("enc28j60", "ENC28J60"), PinDevice(E28J_PIN_COUNT, PIN_INIT_DATA)
{
    this->full_duplex_wired = true;
    this->link_up = true;
    
    this->reset();
}

Enc28J60::Enc28J60(Json::Value &json_data)
    : Entity(json_data), PinDevice(E28J_PIN_COUNT, PIN_INIT_DATA)
{
    this->full_duplex_wired = true;
    this->link_up = true;

    if (json_data.isMember("full_duplex")) {
        this->setFullDuplexWired(json_data["full_duplex"].asBool());
    }
    
    this->reset();
}

void Enc28J60::reset()
{
    this->_initRegs();
    memset(this->eth_buffer, 0, E28J_ETH_BUFFER_SIZE);
}

void Enc28J60::setFullDuplexWired(bool wired)
{
    this->full_duplex_wired = wired;
}

void Enc28J60::setLinkUp(bool link_up)
{
    this->link_up = link_up;
    chg_bit(this->phy_regs[REG_PHSTAT2], B_LSTAT, link_up);
    
    if (!link_up)
        clear_bit(this->phy_regs[REG_PHSTAT1], B_LLSTAT);
}

void Enc28J60::_initRegs()
{
    memset(this->regs, 0, E28J_REGS_COUNT);
    this->regs[REG_ECON2] = _BV(B_AUTOINC);
    this->regs[REG_ERDPTL] = 0xfa;
    this->regs[REG_ERDPTH] = 0x05;
    this->regs[REG_ERXSTL] = 0xfa;
    this->regs[REG_ERXSTH] = 0x05;
    this->regs[REG_ERXNDL] = 0xff;
    this->regs[REG_ERXNDH] = 0x1f;
    this->regs[REG_ERXRDPTL] = 0xfa;
    this->regs[REG_ERXRDPTH] = 0x05;
    this->regs[REG_ERXFCON] = _BV(B_UCEN) | _BV(B_CRCEN) | _BV(B_BCEN);
    this->regs[REG_MACON2] = _BV(B_MARST);
    this->regs[REG_MACLCON1] = 0x0f;
    this->regs[REG_MACLCON2] = 0x37;
    this->regs[REG_MAMXFLH] = 0x06;
    this->regs[REG_MAPHSUP] = 0x10;
    this->regs[REG_EREVID] = E28J_REVISION_ID;
    this->regs[REG_ECOCON] = 0x04;
    this->regs[REG_EPAUSH] = 0x10;

    memset(this->phy_regs, 0, E28J_PHY_REGS_COUNT);
    this->phy_regs[REG_PHCON1] = _BV(B_PPWRSV) | (this->full_duplex_wired * _BV(B_PDPXMD));
    this->phy_regs[REG_PHSTAT1] = _BV(B_PFDPX) | _BV(B_PHDPX);
    this->phy_regs[REG_PHID1] = 0x0083;
    this->phy_regs[REG_PHID2] = 0x1400;
    this->phy_regs[REG_PHSTAT2] = (this->full_duplex_wired * _BV(B_DPXSTAT)) |
        (this->link_up * _BV(B_LSTAT));
    this->phy_regs[REG_PHLCON] = 0x3422;
}

void Enc28J60::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
    bool digi_val = this->_pins[pin_id].readDigital();
    
    switch (pin_id) {
        case E28J_PIN_RESET:
            if (!digi_val)
                this->reset();
            return;
        case E28J_PIN_SLAVE_SELECT:
            this->_spiSlaveSelect(!digi_val);
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
        case STATE_RECEIVING_BUFFER_DATA:
            return this->_handleBufferData(data);
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
        case OPCODE_WRITE_BUFFER_MEMORY:
            if ((data & 0x1f) != 0x1a)
                break;
            this->state = STATE_RECEIVING_BUFFER_DATA;
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

uint8_t Enc28J60::_handleBufferData(uint8_t data)
{
    uint16_t ptr = this->_get16BitReg(REG_EWRPTL) & (E28J_ETH_BUFFER_SIZE - 1);
    
    this->eth_buffer[ptr] = data;
    
    if (bit_is_set(this->regs[REG_ECON2], B_AUTOINC)) {
        ptr = (ptr + 1) & (E28J_ETH_BUFFER_SIZE - 1);
        this->_set16BitReg(REG_EWRPTL, ptr);
    }
    
    return 0xff;
}

uint8_t Enc28J60::_execReadCtrlReg(uint8_t reg)
{
    reg = this->_mapRegister(reg);

    uint8_t value = this->regs[reg];

    this->_onRegRead(reg, value);

    this->response_byte = value;
    this->state = (is_mac_reg(reg) || is_mii_reg(reg)) ? STATE_PRE_RESPONDING : STATE_RESPONDING; 
    
    return 0xff;
}

uint8_t Enc28J60::_execWriteCtrlReg(uint8_t reg, uint8_t data)
{
    reg = this->_mapRegister(reg);

    uint8_t prev_value = this->regs[reg];
    uint8_t value = data;
    value = this->_adjustRegWrite(reg, value, prev_value);
    this->regs[reg] = value;
    this->_onRegWrite(reg, value, prev_value);
    
    this->state = STATE_RECEIVING_COMMAND;
    
    return 0xff;
}

uint8_t Enc28J60::_execBitFieldClear(uint8_t reg, uint8_t data)
{
    reg = this->_mapRegister(reg);
    
    if (is_mac_reg(reg) || is_mii_reg(reg))
        fail("Attempted Bit Field Clear on MAC/MII register %02x", reg);

    uint8_t prev_value = this->regs[reg];
    uint8_t value = prev_value & ~data;
    value = this->_adjustRegWrite(reg, value, prev_value);
    this->regs[reg] = value;
    this->_onRegWrite(reg, value, prev_value);
    
    this->state = STATE_RECEIVING_COMMAND;
    
    return 0xff;
}

uint8_t Enc28J60::_execBitFieldSet(uint8_t reg, uint8_t data)
{
    reg = this->_mapRegister(reg);
    
    if (is_mac_reg(reg) || is_mii_reg(reg))
        fail("Attempted Bit Field Set on MAC/MII register %02x", reg);

    uint8_t prev_value = this->regs[reg];
    uint8_t value = prev_value | data;
    value = this->_adjustRegWrite(reg, value, prev_value);
    this->regs[reg] = value;
    this->_onRegWrite(reg, value, prev_value);
    
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

uint8_t Enc28J60::_getRegWriteMask(uint8_t reg)
{
    switch (reg) {
        case REG_EIR: return 0x00;
        case REG_ESTAT: return 0x01;
        case REG_ECON2: return 0xe8;
        case REG_ERDPTH: case REG_EWRPTH: case REG_ETXSTH: case REG_ETXNDH:
        case REG_ERXSTH: case REG_ERXNDH: case REG_ERXRDPTH: case REG_ERXWRPTH:
        case REG_EDMASTH: case REG_EDMANDH: case REG_EDMADSTH: case REG_EDMACSH:
            return 0x1f;
        case REG_EPMOH: return 0x1f;
        case REG_EPKTCNT: return 0x00;
        case REG_EWOLIE: return 0xdf;
        case REG_EWOLIR: return 0x00;
        case REG_MACON1: return 0x1f;
        case REG_MACON2: return 0xcf;
        case REG_MACON4: return 0x73;
        case REG_MABBIPG: return 0x7f;
        case REG_MAIPGL: return 0x7f;
        case REG_MAIPGH: return 0x7f;
        case REG_MACLCON1: return 0x0f;
        case REG_MACLCON2: return 0x3f;
        case REG_MAPHSUP: return 0x99;
        case REG_MICON: return 0x80;
        case REG_MICMD: return 0x03;
        case REG_MIREGADR: return 0x1f;
        case REG_MISTAT: return 0x00;
        case REG_EREVID: return 0x00;
        case REG_ECOCON: return 0x07;
        case REG_EFLOCON: return 0x03;
    }
    
    return 0xff;
}

uint8_t Enc28J60::_getRegClearableMask(uint8_t reg)
{
    switch (reg) {
        case REG_ESTAT: return 0x52;
        case REG_EIR: return 0x2b;
        case REG_EWOLIR: return 0xdf;
    }
    
    return 0x00;
}

uint16_t Enc28J60::_getPhyRegWriteMask(uint8_t reg)
{
    switch (reg) {
        case REG_PHCON1: return 0xc480;
        case REG_PHSTAT1: return 0x0000;
        case REG_PHID1: return 0x0000;
        case REG_PHID2: return 0x0000;
        case REG_PHSTAT2: return 0x0000;
        case REG_PHCON2: return 0x7fff;
        case REG_PHLCON: return 0xffff;
        case REG_PHIE: return 0xffff;
        case REG_PHIR: return 0x0000;
    }
    
    return 0x0000;
}

uint8_t Enc28J60::_adjustRegWrite(uint8_t reg, uint8_t value, uint8_t prev_val)
{
    uint8_t clr_mask = this->_getRegClearableMask(reg);
    uint8_t wr_mask = this->_getRegWriteMask(reg);

    return ((prev_val & ~(~value & clr_mask)) & ~wr_mask) | (value & wr_mask);
}

void Enc28J60::_onRegRead(uint8_t reg, uint8_t &value)
{
    if (is_mii_reg(reg)) {
        this->_onMiiRegRead(reg, value);
    }
}

void Enc28J60::_onRegWrite(uint8_t reg, uint8_t value, uint8_t prev_val)
{
    if (is_mii_reg(reg)) {
        this->_onMiiRegWrite(reg, value, prev_val);
    }
}

void Enc28J60::_onMiiRegRead(uint8_t reg, uint8_t &value)
{
    switch (reg) {
        case REG_MIRDL:
            if (bit_is_set(this->regs[REG_MICMD], B_MIISCAN)) {
                uint16_t val = this->_readPhyReg(this->regs[REG_MIREGADR]);
                this->regs[REG_MIRDH] = high_byte(val);
                this->regs[REG_MIRDL] = low_byte(val);
            }
            break;
    }
}

void Enc28J60::_onMiiRegWrite(uint8_t reg, uint8_t value, uint8_t prev_val)
{
    switch (reg) {
        case REG_MICMD:
            chg_bit(this->regs[REG_MISTAT], B_SCAN, bit_is_set(value, B_MIISCAN));
            
            if (bit_is_set(value, B_MIIRD)) {
                uint16_t val = this->_readPhyReg(this->regs[REG_MIREGADR]);
                this->regs[REG_MIRDH] = high_byte(val);
                this->regs[REG_MIRDL] = low_byte(val);
            }
            break;
        case REG_MIWRH:
            this->_writePhyReg(this->regs[REG_MIREGADR],
                (this->regs[REG_MIWRH] << 8) + this->regs[REG_MIWRL]);
            break;
    }
}

uint16_t Enc28J60::_readPhyReg(uint8_t reg)
{
    uint16_t value = this->phy_regs[reg];

    // clear status bits
    switch (reg) {
        case REG_PHSTAT1:
            this->phy_regs[reg] = (value & ~(_BV(B_JABBER) | _BV(B_LLSTAT))) | (_BV(B_LLSTAT) * this->link_up);
            break;
        case REG_PHIR:
            this->phy_regs[reg] = value & ~(_BV(B_PLNKIF) | _BV(B_PGIF));
            break;
    }
    
    return value;
}

void Enc28J60::_writePhyReg(uint8_t reg, uint16_t value)
{
    this->phy_regs[reg] = value;
}

uint16_t Enc28J60::_get16BitReg(uint8_t low_byte_reg)
{
    return ((uint16_t)this->regs[low_byte_reg + 1] << 8) + this->regs[low_byte_reg];
}

void Enc28J60::_set16BitReg(uint8_t low_byte_reg, uint16_t value)
{
    this->regs[low_byte_reg] = low_byte(value);
    this->regs[low_byte_reg + 1] = high_byte(value);
}
