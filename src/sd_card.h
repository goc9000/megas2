#ifndef _H_SD_CARD_H
#define _H_SD_CARD_H

#include <inttypes.h>

#include "spi_device.h"
#include "device.h"

class SdCard : public Device, public SpiDevice {
public:
    SdCard();
    void reset();
    virtual sim_time_t nextEventTime();
    void spiSlaveSelect(bool select);
    bool spiReceiveData(uint8_t &data);
private:
    bool spi_selected;
    
    int state;
};

#endif

