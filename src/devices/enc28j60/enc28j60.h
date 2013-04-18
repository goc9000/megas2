#ifndef _H_ENC28J60_H
#define _H_ENC28J60_H

#include <inttypes.h>

#include "glue/spi_device.h"
#include "glue/pin_device.h"
#include "devices/device.h"
#include "simulation/entity.h"
#include "simulation/sim_device.h"
#include "networking/net_device.h"

#define E28J_PIN_COUNT          2

#define E28J_PIN_RESET          0
#define E28J_PIN_SLAVE_SELECT   1

#define E28J_REGS_COUNT         0x80
#define E28J_PHY_REGS_COUNT     0x20
#define E28J_ETH_BUFFER_SIZE    0x2000


class Enc28J60 : public Entity, public PinDevice, public SpiDevice,
    public SimulatedDevice, public NetworkDevice {
public:
    Enc28J60();
    Enc28J60(Json::Value &json_data);
        
    virtual void reset();
    virtual void act(int event);
    void setFullDuplexWired(bool wired);
    void setLinkUp(bool link_up);
    
    void doReceiveFrames(void);
    bool spiReceiveData(uint8_t &data);
private:
    uint8_t regs[E28J_REGS_COUNT];
    uint16_t phy_regs[E28J_PHY_REGS_COUNT];
    uint8_t eth_buffer[E28J_ETH_BUFFER_SIZE];

    bool full_duplex_wired;
    bool link_up;

    int state;
    uint8_t cmd_byte;
    uint8_t response_byte;
    
    void _initRegs();
    
    void doTransmission(void);
    void doTransmissionResetChecks(void);
    void checkTxFrameBounds(void);
    void getTransmissionSettings(bool& add_crc, int& pad_to, bool& huge);
    void detectTransmissionType(uint64_t &tx_status);
    string prepareTransmisionData(bool add_crc, int pad_to, uint64_t& tx_status);
    void checkFinalTxFrameLength(string data, bool allow_huge, uint64_t& tx_status);

    virtual void _onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value);
    
    virtual void _onSpiSlaveSelect(bool select);
    uint8_t _handleSpiData(uint8_t data);
    uint8_t _handleCommandStart(uint8_t data);
    uint8_t _handleCommandArg(uint8_t data);
    uint8_t _handleBufferData(uint8_t data);
    
    uint8_t _execReadCtrlReg(uint8_t reg);
    uint8_t _execWriteCtrlReg(uint8_t reg, uint8_t data);
    uint8_t _execBitFieldClear(uint8_t reg, uint8_t data);
    uint8_t _execBitFieldSet(uint8_t reg, uint8_t data);
    
    uint8_t _mapRegister(uint8_t reg);
    uint8_t _getRegWriteMask(uint8_t reg);
    uint8_t _getRegClearableMask(uint8_t reg);
    uint8_t _getRegResetValue(uint8_t reg);
    uint16_t _getPhyRegWriteMask(uint8_t reg);
    uint16_t _getPhyRegResetValue(uint8_t reg);
    uint8_t _adjustRegWrite(uint8_t reg, uint8_t value, uint8_t prev_val);
    
    void _onRegRead(uint8_t reg, uint8_t &value);
    void _onRegWrite(uint8_t reg, uint8_t value, uint8_t prev_val);

    void _onMiiRegRead(uint8_t reg, uint8_t &value);
    void _onMiiRegWrite(uint8_t reg, uint8_t value, uint8_t prev_val);
    uint16_t _readPhyReg(uint8_t reg);
    void _writePhyReg(uint8_t reg, uint16_t value);
    
    uint16_t _get16BitReg(uint8_t low_byte_reg);
    void _set16BitReg(uint8_t low_byte_reg, uint16_t value);
};

#endif

