#ifndef _H_ENC28J60_H
#define _H_ENC28J60_H

#include <inttypes.h>

#include "spi_device.h"
#include "device.h"

class Enc28J60 : public Device, public SpiDevice {
public:
    Enc28J60();
    virtual void reset();
    virtual sim_time_t nextEventTime();
    void spiSlaveSelect(bool select);
    bool spiReceiveData(uint8_t &data);
private:
    bool spi_selected;
    
    uint8_t _handleSpiData(uint8_t data);
};

#endif

