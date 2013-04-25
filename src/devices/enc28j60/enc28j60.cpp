#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdlib>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "utils/net_utils.h"

#include "enc28j60.h"
#include "defs.h"

using namespace std;

#define MIN_ETHERNET_FRAME_SIZE      64

#define STATE_RECEIVING_COMMAND             0
#define STATE_RECEIVING_COMMAND_ARG         1
#define STATE_PRE_RESPONDING                2
#define STATE_RESPONDING                    3
#define STATE_RECEIVING_BUFFER_DATA         4
#define STATE_RESPONDING_WITH_BUFFER_DATA   5

const sim_time_t DEFAULT_RECEIVE_FRAMES_INTERVAL = ms_to_sim_time(100);

#define PADDING_NONE                  0
#define PADDING_AUTODETECT           -1


// Pin initialization data

PinInitData const PIN_INIT_DATA[E28J_PIN_COUNT] = {
    { "RESET", PIN_MODE_INPUT, 1 }, // RESET
    { "SS", PIN_MODE_INPUT, 1 }  // SLAVE_SELECT
};

#define DEFAULT_NAME "ENC28J60"

Enc28J60::Enc28J60()
    : Entity(DEFAULT_NAME), PinDevice(E28J_PIN_COUNT, PIN_INIT_DATA), NetworkDevice()
{
    this->full_duplex_wired = true;
    this->link_up = true;
    
    this->reset();
}

Enc28J60::Enc28J60(Json::Value &json_data)
    : Entity(DEFAULT_NAME, json_data), PinDevice(E28J_PIN_COUNT, PIN_INIT_DATA), NetworkDevice()
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
    
    if (this->simulation) {
        this->simulation->unscheduleAll(this);
        this->simulation->scheduleEvent(this, SIM_EVENT_RECEIVE_FRAMES,
            this->simulation->time + DEFAULT_RECEIVE_FRAMES_INTERVAL);
    }
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

mac_addr_t Enc28J60::getMacAddress(void) const
{
    uint8_t buffer[6];
    
    buffer[0] = this->regs[REG_MAADR5];
    buffer[1] = this->regs[REG_MAADR4];
    buffer[2] = this->regs[REG_MAADR3];
    buffer[3] = this->regs[REG_MAADR2];
    buffer[4] = this->regs[REG_MAADR1];
    buffer[5] = this->regs[REG_MAADR0];
    
    return mac_addr_t(buffer, 6);
}

void Enc28J60::act(int event)
{
    switch (event) {
        case SIM_EVENT_RECEIVE_FRAMES:
            this->doReceiveFrames();
            
            if (this->simulation) {
                this->simulation->scheduleEvent(this, SIM_EVENT_RECEIVE_FRAMES,
                    this->simulation->time + DEFAULT_RECEIVE_FRAMES_INTERVAL);
            }
            break;
    }
}

void Enc28J60::doReceiveFrames(void)
{
    while (true) {
        EthernetFrame frame = this->getPendingFrame();
        if (frame.is_null)
            return;
        
        this->doReceiveFrame(frame);
    }
}

void Enc28J60::doReceiveFrame(const EthernetFrame& frame)
{
    if (!this->receptionEnabled())
        return;
    if (!this->filterFrame(frame))
        return;
    
    uint32_t rx_status = this->getRxStatusFlags(frame);

    this->doRxBufferSanityChecks();
    
    if (!this->fitsInBuffer(frame)) {
        warn("ENC28J60 dropped frame because of insufficient buffer space");
        return;
    }
    
    this->loadReceivedFrame(frame, rx_status);
}

bool Enc28J60::filterFrame(const EthernetFrame& frame) const
{
    if (frame.totalLength() < MIN_ETHERNET_FRAME_SIZE) {
        warn("Received runt frame (%d bytes)", frame.totalLength());
        return false;
    }
    if (!bit_is_set(this->regs[REG_MACON3], B_HFRMEN) &&
        (frame.totalLength() > this->_get16BitReg(REG_MAMXFLL)))
        return false;
    
    uint8_t enabled_filters = this->regs[REG_ERXFCON] & ~(_BV(B_ANDOR) | _BV(B_CRCEN));
    uint8_t triggered_filters = 0;
    
    if (frame.dest_mac == this->getMacAddress())
        set_bit(triggered_filters, B_UCEN);
    if (bit_is_set(this->regs[REG_ERXFCON], B_PMEN))
        fail("Pattern Match filter not supported");
    if (bit_is_set(this->regs[REG_ERXFCON], B_MPEN))
        fail("Magic Packet filter not supported");
    if (bit_is_set(this->regs[REG_ERXFCON], B_HTEN))
        fail("Hash Table filter not supported");
    if (frame.dest_mac.isMulticast())
        set_bit(triggered_filters, B_MCEN);
    if (frame.dest_mac.isBroadcast())
        set_bit(triggered_filters, B_BCEN);
    
    if (bit_is_set(this->regs[REG_ERXFCON], B_ANDOR)) {
        if (enabled_filters != triggered_filters)
            return false;
    } else {
        if ((enabled_filters & triggered_filters) == 0)
            return false;
    }
    
    if (bit_is_set(this->regs[REG_ERXFCON], B_CRCEN) && frame.has_fcs && !frame.checkFcs())
        return false;
    
    if (!bit_is_set(this->regs[REG_MACON1], B_PASSALL) && frame.isMacControlFrame())
        return false;
    
    return true;
}

bool Enc28J60::receptionEnabled(void) const
{
    return
        bit_is_set(this->regs[REG_ECON1], B_RXEN) &&
        bit_is_set(this->regs[REG_MACON1], B_MARXEN) &&
        !bit_is_set(this->regs[REG_ECON1], B_RXRST) &&
        ((this->regs[REG_MACON2] & 0x8c) == 0) &&
        !bit_is_set(this->regs[REG_MAPHSUP], B_RSTRMII) &&
        !bit_is_set(this->phy_regs[REG_PHCON1], B_PRST);
}

uint32_t Enc28J60::getRxStatusFlags(const EthernetFrame& frame) const
{
    uint32_t rx_status = 0;
    
    chg_bit(rx_status, B_RXSTAT_IS_VLAN, frame.isVlanFrame());
    chg_bit(rx_status, B_RXSTAT_IS_UNSUPP_CONTROL,
        frame.isMacControlFrame() && !frame.isPauseFrame());
    chg_bit(rx_status, B_RXSTAT_IS_PAUSE, frame.isPauseFrame());
    chg_bit(rx_status, B_RXSTAT_IS_CONTROL, frame.isMacControlFrame());
    chg_bit(rx_status, B_RXSTAT_BROADCAST, frame.dest_mac.isBroadcast());
    chg_bit(rx_status, B_RXSTAT_MULTICAST, frame.dest_mac.isMulticast());

    chg_bit(rx_status, B_RXSTAT_OK, frame.checkFcs() && frame.checkLengthValid());
    chg_bit(rx_status, B_RXSTAT_INVALID_LENGTH, !frame.checkLengthValid());
    chg_bit(rx_status, B_RXSTAT_LENGTH_CHECK_ERROR,
        bit_is_set(this->regs[REG_MACON3], B_FRMLNEN) && !frame.checkLengthCorrect());

    chg_bit(rx_status, B_RXSTAT_CRC_ERROR, !frame.checkFcs());
    
    rx_status += high_byte(frame.totalLength()) + (low_byte(frame.totalLength()) << 8);
    
    return rx_status;
}

void Enc28J60::doRxBufferSanityChecks(void) const
{
    uint16_t ERXWRPT = this->_get16BitReg(REG_ERXWRPTL);
    uint16_t ERXRDPT = this->_get16BitReg(REG_ERXRDPTL);
    uint16_t ERXST = this->_get16BitReg(REG_ERXSTL);
    uint16_t ERXND = this->_get16BitReg(REG_ERXNDL);
    
    if (ERXST > ERXND)
        fail("Sanity check ERXST <= ERXND failed");
    if ((ERXWRPT < ERXST) || (ERXWRPT > ERXND))
        fail("ERXWRPT outside receive buffer");
    if ((ERXRDPT < ERXST) || (ERXRDPT > ERXND))
        fail("ERXRDPT outside receive buffer");
    if (ERXWRPT & 1)
        fail("ERXWRPT is odd");
}

bool Enc28J60::fitsInBuffer(const EthernetFrame& frame) const
{
    int frame_size = 6 + frame.totalLength();
    
    if (frame_size & 1)
        frame_size++;
    
    return (this->regs[REG_EPKTCNT] < 0xff) ||
        (frame_size <= this->getFreeBufferSpace());
}

uint16_t Enc28J60::getFreeBufferSpace(void) const
{
    uint16_t ERXWRPT = this->_get16BitReg(REG_ERXWRPTL);
    uint16_t ERXRDPT = this->_get16BitReg(REG_ERXRDPTL);
    uint16_t ERXST = this->_get16BitReg(REG_ERXSTL);
    uint16_t ERXND = this->_get16BitReg(REG_ERXNDL);
    
    if (ERXWRPT > ERXRDPT)
        return (ERXND - ERXST) - (ERXWRPT - ERXRDPT);
    else if (ERXWRPT == ERXRDPT)
        return ERXND - ERXST;
    else
        return ERXRDPT - ERXWRPT - 1;
}

uint16_t Enc28J60::wrapToRxBuffer(uint16_t pointer) const
{
    uint16_t ERXST = this->_get16BitReg(REG_ERXSTL);
    uint16_t ERXND = this->_get16BitReg(REG_ERXNDL);
    
    while (pointer > ERXND)
        pointer -= ERXND + 1 - ERXST;
    
    return pointer;
}

void Enc28J60::loadReceivedFrame(const EthernetFrame& frame, uint32_t rx_status)
{
    uint8_t data[65536];
    int data_len = 6 + frame.toBuffer(data + 6);
    
    if (data_len & 1)
        data[data_len++] = 0;
    
    uint16_t ERXWRPT = this->_get16BitReg(REG_ERXWRPTL);
    
    int next_pkt_ptr = this->wrapToRxBuffer(ERXWRPT + data_len);
    data[0] = low_byte(next_pkt_ptr);
    data[1] = high_byte(next_pkt_ptr);
    data[2] = rx_status & 0xff;
    data[3] = (rx_status >> 8) & 0xff;
    data[4] = (rx_status >> 16) & 0xff;
    data[5] = (rx_status >> 24) & 0xff;
    
    for (int i = 0; i < data_len; i++) {
        this->eth_buffer[ERXWRPT] = data[i];
        ERXWRPT = this->wrapToRxBuffer(ERXWRPT + 1);
    }
    this->_set16BitReg(REG_ERXWRPTL, ERXWRPT);
    
    this->regs[REG_EPKTCNT]++;
}

void Enc28J60::doTransmission(void)
{
    this->doTransmissionResetChecks();
    this->checkTxFrameBounds();
    
    bool add_crc = false;
    int pad_to = PADDING_NONE;
    bool huge_frame = false;
    this->getTransmissionSettings(add_crc, pad_to, huge_frame);
    
    EthernetFrame frame = this->getFrameForTransmission(!add_crc);
    
    uint64_t tx_status = 0;
    this->detectTransmissionType(frame, tx_status);
    this->prepareTransmisionData(frame, add_crc, pad_to, tx_status);
    this->checkFinalTxFrameLength(frame, huge_frame, tx_status);
    
    this->sendFrame(frame);
    
    tx_status |= B_TXSTAT_DONE;
    uint16_t frame_end = this->_get16BitReg(REG_ETXNDL);
    for (int i = 0; i < 7; i++)
        this->eth_buffer[frame_end + 1 + i] = (tx_status >> (8*i)) & 0xff;
    
    clear_bit(this->regs[REG_ECON1], B_TXRTS);
}

void Enc28J60::doTransmissionResetChecks(void)
{
    if (bit_is_set(this->regs[REG_ECON1], B_TXRST))
        fail("Attempted transmit with TXRST on");
    if (((this->regs[REG_MACON2] & 0xc3) != 0) || bit_is_set(this->regs[REG_MAPHSUP], B_RSTRMII))
        fail("Attempted transmit with some MAC RST bits on");
    if (bit_is_set(this->phy_regs[REG_PHCON1], B_PRST))
        fail("Attempted transmit with PHY in reset");
}

void Enc28J60::checkTxFrameBounds(void)
{
    uint16_t frame_start = this->_get16BitReg(REG_ETXSTL);
    uint16_t frame_end = this->_get16BitReg(REG_ETXNDL);
    
    if (frame_start >= E28J_ETH_BUFFER_SIZE)
        fail("ETXST outside buffer");
    if (frame_end >= E28J_ETH_BUFFER_SIZE)
        fail("ETXND outside buffer");
    if (frame_end + 7 >= E28J_ETH_BUFFER_SIZE)
        fail("No space for status vector after ETXND");
    if (frame_start > frame_end)
        fail("ETXND < ETXST when attempting transmission");
    if ((1 + frame_end - frame_start) < 16)
        fail("Frame too short (stops before EtherType)");
    if (frame_start & 1)
        warn("ETXST is not even");
}

void Enc28J60::getTransmissionSettings(bool& add_crc, int& pad_to, bool& huge)
{
    uint16_t frame_start = this->_get16BitReg(REG_ETXSTL);
    
    pad_to = PADDING_NONE;
    switch ((this->regs[REG_MACON3] >> 5) & 7) {
        case 1: pad_to = 60; break;
        case 3: case 7: pad_to = 64; break;
        case 5: pad_to = PADDING_AUTODETECT; break;
    }
    
    add_crc = bit_is_set(this->regs[REG_MACON3], B_TXCRCEN);
    huge = bit_is_set(this->regs[REG_MACON3], B_HFRMEN);
    
    uint8_t overrides = this->eth_buffer[frame_start];
    if (bit_is_set(overrides, B_POVERRIDE)) {
        huge = bit_is_set(overrides, B_PHUGEEN);
        add_crc = bit_is_set(overrides, B_PCRCEN);
        pad_to = 60 * bit_is_set(overrides, B_PPADEN);
    }
}

EthernetFrame Enc28J60::getFrameForTransmission(bool has_crc)
{
    uint16_t frame_start = this->_get16BitReg(REG_ETXSTL);
    uint16_t frame_end = this->_get16BitReg(REG_ETXNDL);
    
    return EthernetFrame(this->eth_buffer + frame_start + 1,
        frame_end - frame_start, has_crc);
}

void Enc28J60::detectTransmissionType(const EthernetFrame& frame, uint64_t &tx_status)
{
    chg_bit(tx_status, B_TXSTAT_IS_CONTROL_FRAME, frame.isMacControlFrame());
    chg_bit(tx_status, B_TXSTAT_IS_PAUSE_FRAME, frame.isPauseFrame());
    chg_bit(tx_status, B_TXSTAT_IS_VLAN, frame.isVlanFrame());
    chg_bit(tx_status, B_TXSTAT_BROADCAST, frame.dest_mac.isBroadcast());
    chg_bit(tx_status, B_TXSTAT_MULTICAST, frame.dest_mac.isMulticast());
}

void Enc28J60::prepareTransmisionData(EthernetFrame& frame, bool add_crc,
    int pad_to, uint64_t& tx_status)
{
    if (pad_to == PADDING_AUTODETECT)
        pad_to = frame.isVlanFrame() ? 64 : 60;
    
    if (pad_to > 0) {
        if (!add_crc)
            fail("Automatic padding not allowed when TXCRCEN = 0");
        frame.padTo(pad_to + 4);
    }
    
    if (add_crc)
        frame.addFcs();
    else if (!frame.checkFcs()) {
        tx_status |= B_TXSTAT_CRC_ERROR;
        warn("Incorrect CRC for transmitted packet");
    }
}

void Enc28J60::checkFinalTxFrameLength(const EthernetFrame& frame, bool allow_huge,
    uint64_t& tx_status)
{
    if (!frame.checkLengthValid())
        tx_status |= B_TXSTAT_INVALID_LENGTH;
    if (bit_is_set(this->regs[REG_MACON3], B_FRMLNEN) && !frame.checkLengthCorrect())
        tx_status = B_TXSTAT_LENGTH_CHECK_ERROR;
    
    uint16_t max_frame_len = this->_get16BitReg(REG_MAMXFLL);
    
    if (frame.totalLength() > max_frame_len) {
        tx_status |= B_TXSTAT_IS_GIANT;
        
        if (!allow_huge)
            fail("Frame exceeds MAXMXFL=%d in non-huge mode", max_frame_len);
    }
    
    uint16_t frame_len = frame.totalLength();
    uint64_t len_le = high_byte(frame_len) + (low_byte(frame_len) << 8);
    
    tx_status = (tx_status & 0xffff0000ffff0000ULL) + len_le + (len_le << 32);
}

void Enc28J60::_initRegs()
{
    for (int i = 0; i < E28J_REGS_COUNT; i++)
        this->regs[i] = this->_getRegResetValue(i);
    for (int i = 0; i < E28J_PHY_REGS_COUNT; i++)
        this->phy_regs[i] = this->_getPhyRegResetValue(i);
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
        case STATE_RESPONDING_WITH_BUFFER_DATA:
            return this->_respondWithBufferData();
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
        case OPCODE_READ_BUFFER_MEMORY:
            if ((data & 0x1f) != 0x1a)
                break;
            this->state = STATE_RESPONDING_WITH_BUFFER_DATA;
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
    uint16_t ptr = this->_get16BitReg(REG_EWRPTL);
    
    if (ptr >= E28J_ETH_BUFFER_SIZE)
        fail("EWRPTL set beyond ENC28J60 available memory");
    
    this->eth_buffer[ptr] = data;
    
    if (bit_is_set(this->regs[REG_ECON2], B_AUTOINC)) {
        ptr = (ptr + 1) & (E28J_ETH_BUFFER_SIZE - 1);
        this->_set16BitReg(REG_EWRPTL, ptr);
    }
    
    return 0xff;
}

uint8_t Enc28J60::_respondWithBufferData(void)
{
    uint16_t ptr = this->_get16BitReg(REG_ERDPTL);
    
    if (ptr >= E28J_ETH_BUFFER_SIZE)
        fail("EWRPTL set beyond ENC28J60 available memory");
    
    uint8_t data = this->eth_buffer[ptr];
    
    if (bit_is_set(this->regs[REG_ECON2], B_AUTOINC)) {
        if (ptr == this->_get16BitReg(REG_ERXNDL))
            ptr = this->_get16BitReg(REG_ERXSTL);
        else
            ptr = (ptr + 1) & (E28J_ETH_BUFFER_SIZE - 1);
        
        this->_set16BitReg(REG_ERDPTL, ptr);
    }
    
    return data;
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

uint8_t Enc28J60::_getRegResetValue(uint8_t reg)
{
    switch (reg) {
        case REG_ECON2: return _BV(B_AUTOINC);
        case REG_ERDPTL: return 0xfa;
        case REG_ERDPTH: return 0x05;
        case REG_ERXSTL: return 0xfa;
        case REG_ERXSTH: return 0x05;
        case REG_ERXNDL: return 0xff;
        case REG_ERXNDH: return 0x1f;
        case REG_ERXRDPTL: return 0xfa;
        case REG_ERXRDPTH: return 0x05;
        case REG_ERXFCON: return _BV(B_UCEN) | _BV(B_CRCEN) | _BV(B_BCEN);
        case REG_MACON2: return _BV(B_MARST);
        case REG_MACLCON1: return 0x0f;
        case REG_MACLCON2: return 0x37;
        case REG_MAMXFLH: return 0x06;
        case REG_MAPHSUP: return 0x10;
        case REG_EREVID: return E28J_REVISION_ID;
        case REG_ECOCON: return 0x04;
        case REG_EPAUSH: return 0x10;
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

uint16_t Enc28J60::_getPhyRegResetValue(uint8_t reg)
{
    switch (reg) {
        case REG_PHCON1: return _BV(B_PPWRSV) | (this->full_duplex_wired * _BV(B_PDPXMD));
        case REG_PHSTAT1: return _BV(B_PFDPX) | _BV(B_PHDPX);
        case REG_PHID1: return 0x0083;
        case REG_PHID2: return 0x1400;
        case REG_PHSTAT2: return (this->full_duplex_wired * _BV(B_DPXSTAT)) |
            (this->link_up * _BV(B_LSTAT));
        case REG_PHLCON: return 0x3422;
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
        return;
    }
}

void Enc28J60::_onRegWrite(uint8_t reg, uint8_t value, uint8_t prev_val)
{
    if (is_mii_reg(reg)) {
        this->_onMiiRegWrite(reg, value, prev_val);
        return;
    }
    
    switch (reg) {
        case REG_ECON1:
            if (bit_is_set(value, B_TXRTS))
                this->doTransmission();
            break;
        case REG_ECON2:
            if (bit_is_set(value, B_PKTDEC) && this->regs[REG_EPKTCNT]) {
                this->regs[REG_EPKTCNT]--;
                if (!this->regs[REG_EPKTCNT])
                    clear_bit(this->regs[REG_EIR], B_PKTIF);
            }
            break;
        case REG_ERXRDPTL:
            this->regs[REG_ERXRDPTL] = prev_val;
            this->reg_ERXRDPTL_shadow = value;
            break;
        case REG_ERXRDPTH:
            this->regs[REG_ERXRDPTL] = this->reg_ERXRDPTL_shadow;
            break;
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

uint16_t Enc28J60::_get16BitReg(uint8_t low_byte_reg) const
{
    return ((uint16_t)this->regs[low_byte_reg + 1] << 8) + this->regs[low_byte_reg];
}

void Enc28J60::_set16BitReg(uint8_t low_byte_reg, uint16_t value)
{
    this->regs[low_byte_reg] = low_byte(value);
    this->regs[low_byte_reg + 1] = high_byte(value);
}
