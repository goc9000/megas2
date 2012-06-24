#ifndef _H_ENC28J60_H
#define _H_ENC28J60_H

#include <inttypes.h>

#include "glue/spi_device.h"
#include "devices/device.h"

#define E28J_REGS_COUNT         0x80
#define E28J_PHY_REGS_COUNT     0x20
#define E28J_ETH_BUFFER_SIZE    0x2000

class Enc28J60 : public Device, public SpiDevice {
public:
    Enc28J60();
    virtual void reset();
    virtual sim_time_t nextEventTime();
    void spiSlaveSelect(bool select);
    bool spiReceiveData(uint8_t &data);
private:
    uint8_t regs[E28J_REGS_COUNT];
    uint8_t phy_regs[E28J_PHY_REGS_COUNT];
    uint8_t eth_buffer[E28J_ETH_BUFFER_SIZE];

    bool spi_selected;
    
    int state;
    uint8_t cmd_byte;
    uint8_t response_byte;
    
    uint8_t _handleSpiData(uint8_t data);
    uint8_t _handleCommandStart(uint8_t data);
    uint8_t _handleCommandArg(uint8_t data);
    
    uint8_t _execReadCtrlReg(uint8_t reg);
    uint8_t _execWriteCtrlReg(uint8_t reg, uint8_t data);
    uint8_t _execBitFieldClear(uint8_t reg, uint8_t data);
    uint8_t _execBitFieldSet(uint8_t reg, uint8_t data);
    
    uint8_t _mapRegister(uint8_t reg);
};

#endif

