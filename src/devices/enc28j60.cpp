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
#define REG_ERDPTL                    0x00
#define REG_ERDPTH                    0x01
#define REG_EWRPTL                    0x02
#define REG_EWRPTH                    0x03
#define REG_ETXSTL                    0x04
#define REG_ETXSTH                    0x05
#define REG_ETXNDL                    0x06
#define REG_ETXNDH                    0x07
#define REG_ERXSTL                    0x08
#define REG_ERXSTH                    0x09
#define REG_ERXNDL                    0x0a
#define REG_ERXNDH                    0x0b
#define REG_ERXRDPTL                  0x0c
#define REG_ERXRDPTH                  0x0d
#define REG_ERXWRPTL                  0x0e
#define REG_ERXWRPTH                  0x0f
#define REG_EDMASTL                   0x10
#define REG_EDMASTH                   0x11
#define REG_EDMANDL                   0x12
#define REG_EDMANDH                   0x13
#define REG_EDMADSTL                  0x14
#define REG_EDMADSTH                  0x15
#define REG_EDMACSL                   0x16
#define REG_EDMACSH                   0x17
#define REG_EIE                       0x1b
#define REG_EIR                       0x1c
#define REG_ESTAT                     0x1d
#define REG_ECON2                     0x1e
#define REG_ECON1                     0x1f

#define REG_EHT0                      0x20
#define REG_EHT1                      0x21
#define REG_EHT2                      0x22
#define REG_EHT3                      0x23
#define REG_EHT4                      0x24
#define REG_EHT5                      0x25
#define REG_EHT6                      0x26
#define REG_EHT7                      0x27
#define REG_EPMM0                     0x28
#define REG_EPMM1                     0x29
#define REG_EPMM2                     0x2a
#define REG_EPMM3                     0x2b
#define REG_EPMM4                     0x2c
#define REG_EPMM5                     0x2d
#define REG_EPMM6                     0x2e
#define REG_EPMM7                     0x2f
#define REG_EPMCSL                    0x30
#define REG_EPMCSH                    0x31
#define REG_EPMOL                     0x34
#define REG_EPMOH                     0x35
#define REG_EWOLIE                    0x36
#define REG_EWOLIR                    0x37
#define REG_ERXFCON                   0x38
#define REG_EPKTCNT                   0x39

#define REG_MACON1                    0x40
#define REG_MACON2                    0x41
#define REG_MACON3                    0x42
#define REG_MACON4                    0x43
#define REG_MABBIPG                   0x44
#define REG_MAIPGL                    0x46
#define REG_MAIPGH                    0x47
#define REG_MACLCON1                  0x48
#define REG_MACLCON2                  0x49
#define REG_MAMXFLL                   0x4a
#define REG_MAMXFLH                   0x4b
#define REG_MAPHSUP                   0x4d
#define REG_MICON                     0x51
#define REG_MICMD                     0x52
#define REG_MIREGADR                  0x54
#define REG_MIWRL                     0x56
#define REG_MIWRH                     0x57
#define REG_MIRDL                     0x58
#define REG_MIRDH                     0x59

#define REG_MAADR1                    0x60
#define REG_MAADR0                    0x61
#define REG_MAADR3                    0x62
#define REG_MAADR2                    0x63
#define REG_MAADR5                    0x64
#define REG_MAADR4                    0x65
#define REG_EBSTSD                    0x66
#define REG_EBSTCON                   0x67
#define REG_EBSTCSL                   0x68
#define REG_EBSTCSH                   0x69
#define REG_MISTAT                    0x6a
#define REG_EREVID                    0x72
#define REG_ECOCON                    0x75
#define REG_EFLOCON                   0x77
#define REG_EPAUSL                    0x78
#define REG_EPAUSH                    0x79

// Register bits

// EIE
#define B_INTIE                       7
#define B_PKTIE                       6
#define B_DMAIE                       5
#define B_LINKIE                      4
#define B_TXIE                        3
#define B_WOLIE                       2
#define B_TXERIE                      1
#define B_RXERIE                      0
// EIR
#define B_PKTIF                       6
#define B_DMAIF                       5
#define B_LINKIF                      4
#define B_TXIF                        3
#define B_WOLIF                       2
#define B_TXERIF                      1
#define B_RXERIF                      0
// ESTAT
#define B_INT                         7
#define B_LATECOL                     4
#define B_RXBUSY                      2
#define B_TXABRT                      1
#define B_CLKRDY                      0
// ECON2
#define B_AUTOINC                     7
#define B_PKTDEC                      6
#define B_PWRSV                       5
#define B_VRPS                        3
// ECON1
#define B_TXRST                       7
#define B_RXRST                       6
#define B_DMAST                       5
#define B_CSUMEN                      4
#define B_TXRTS                       3
#define B_RXEN                        2
#define B_BSEL1                       1
#define B_BSEL0                       0
// EWOLIE
#define B_UCWOLIE                     7
#define B_AWOLIE                      6
#define B_PMWOLIE                     4
#define B_MPWOLIE                     3
#define B_HTWOLIE                     2
#define B_MCWOLIE                     1
#define B_BCWOLIE                     0
// EWOLIR
#define B_UCWOLIF                     7
#define B_AWOLIF                      6
#define B_PMWOLIF                     4
#define B_MPWOLIF                     3
#define B_HTWOLIF                     2
#define B_MCWOLIF                     1
#define B_BCWOLIF                     0
// ERXFCON
#define B_UCEN                        7
#define B_ANDOR                       6
#define B_CRCEN                       5
#define B_PMEN                        4
#define B_MPEN                        3
#define B_HTEN                        2
#define B_MCEN                        1
#define B_BCEN                        0
// MACON1
#define B_LOOPBK                      4
#define B_TXPAUS                      3
#define B_RXPAUS                      2
#define B_PASSALL                     1
#define B_MARXEN                      0
// MACON2
#define B_MARST                       7
#define B_RNDRST                      6
#define B_MARXRST                     3
#define B_RFUNRST                     2
#define B_MATXRST                     1
#define B_TFUNRST                     0
// MACON3
#define B_PADCFG2                     7
#define B_PADCFG1                     6
#define B_PADCFG0                     5
#define B_TXCRCEN                     4
#define B_PHDRLEN                     3
#define B_HFRMEN                      2
#define B_FRMLNEN                     1
#define B_FULDPX                      0
// MACON4
#define B_DEFER                       6
#define B_BPEN                        5
#define B_NOBKOFF                     4
#define B_LONGPRE                     1
#define B_PUREPRE                     0
// MAPHSUP
#define B_RSTINTFC                    7
#define B_RSTRMII                     3
// MICON
#define B_RSTMII                      7
// MICMD
#define B_MIISCAN                     1
#define B_MIIRD                       0
// EBSTCON
#define B_PSV2                        7
#define B_PSV1                        6
#define B_PSV0                        5
#define B_PSEL                        4
#define B_TMSEL1                      3
#define B_TMSEL0                      2
#define B_TME                         1
#define B_BISTST                      0
// MISTAT
#define B_NVALID                      2
#define B_SCAN                        1
#define B_BUSY                        0
// ECOCON
#define B_COCON2                      2
#define B_COCON1                      1
#define B_COCON0                      0
// EFLOCON
#define B_FULDPXS                     2
#define B_FCEN1                       1
#define B_FCEN0                       0

static char const * const REG_NAMES[128] = {
    "ERDPTL", "ERDPTH", "EWRPTL", "EWRPTH", "ETXSTL", "ETXSTH", "ETXNDL", "ETXNDH",
    "ERXSTL", "ERXSTH", "ERXNDL", "ERXNDH", "ERXRDPTL", "ERXRDPTH", "ERXWRPTL", "ERXWRPTH",
    "EDMASTL", "EDMASTH", "EDMANDL", "EDMANDH", "EDMADSTL", "EDMADSTH", "EDMACSL", "EDMACSH",
    "(18H)", "(19H)", "(1AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1",
    "EHT0", "EHT1", "EHT2", "EHT3", "EHT4", "EHT5", "EHT6", "EHT7",
    "EPMM0", "EPMM1", "EPMM2", "EPMM3", "EPMM4", "EPMM5", "EPMM6", "EPMM7",
    "EPMCSL", "EPMCSH", "(32H)", "(33H)", "EPMOL", "EPMOH", "EWOLIE", "EWOLIR",
    "ERXFCON", "EPKTCNT", "(3AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1",
    "MACON1", "MACON2", "MACON3", "MACON4", "MABBIPG", "(45H)", "MAIPGL", "MAIPGH",
    "MACLCON1", "MACLCON2", "MAMXFLL", "MAMXFLH", "(4CH)", "MAPHSUP", "(4EH)", "(4FH)",
    "(50H)", "MICON", "MICMD", "(53H)", "MIREGADR", "(55H)", "MIWRL", "MIWRH",
    "MIRDL", "MIRDH", "(5AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1",
    "MAADR1", "MAADR0", "MAADR3", "MAADR2", "MAADR5", "MAADR4", "EBSTSD", "EBSTCON",
    "EBSTCSL", "EBSTCSH", "MISTAT", "(6BH)", "(6CH)", "(6DH)", "(6EH)", "(6FH)",
    "(70H)", "(71H)", "EREVID", "(73H)", "(74H)", "ECOCON", "(76H)", "EFLOCON",
    "EPAUSL", "EPAUSH", "(7AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1"
};

#define STATE_RECEIVING_COMMAND       0
#define STATE_RECEIVING_COMMAND_ARG   1
#define STATE_PRE_RESPONDING          2
#define STATE_RESPONDING              3

// Pin initialization data

PinInitData const PIN_INIT_DATA[E28J_PIN_COUNT] = {
    { "RESET", PIN_MODE_INPUT, 1 }, // RESET
    { "SS", PIN_MODE_INPUT, 1 }  // SLAVE_SELECT
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

Enc28J60::Enc28J60()
    : Entity("enc28j60", "ENC28J60"), PinDevice(E28J_PIN_COUNT, PIN_INIT_DATA)
{
    this->reset();
}

Enc28J60::Enc28J60(Json::Value &json_data)
    : Entity(json_data), PinDevice(E28J_PIN_COUNT, PIN_INIT_DATA)
{
    this->reset();
}

void Enc28J60::reset()
{
    this->_initRegs();
    memset(this->eth_buffer, 0, E28J_ETH_BUFFER_SIZE);
}

void Enc28J60::_initRegs()
{
    memset(this->regs, 0, E28J_REGS_COUNT);
    memset(this->phy_regs, 0, E28J_PHY_REGS_COUNT);

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
}

void Enc28J60::_onPinChanged(int pin_id, int value, int old_value)
{
    switch (pin_id) {
        case E28J_PIN_RESET:
            if (!value)
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

//printf("E28J: read %s\n", REG_NAMES[reg]);
    
    this->response_byte = this->regs[reg];
    this->state = (is_mac_reg(reg) || is_mii_reg(reg)) ? STATE_PRE_RESPONDING : STATE_RESPONDING; 
    
    return 0xff;
}

uint8_t Enc28J60::_execWriteCtrlReg(uint8_t reg, uint8_t data)
{
    reg = this->_mapRegister(reg);

//printf("E28J: write %s, %02x\n", REG_NAMES[reg], data);
    
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

//printf("E28J: and %s, %02x\n", REG_NAMES[reg], data);    
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

//printf("E28J: or %s, %02x\n", REG_NAMES[reg], data);
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
