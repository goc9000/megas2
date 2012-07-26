#ifndef _H_SPI_DEVICE_H
#define _H_SPI_DEVICE_H

#include <inttypes.h>

class SpiBus;

class SpiDevice {
public:
    SpiDevice();
    
    void connectToSpiBus(SpiBus *bus);
    void disconnectFromSpiBus();
    virtual bool spiReceiveData(uint8_t &data) = 0;
protected:
    SpiBus *spi_bus;
    bool spi_selected;

    void _spiSlaveSelect(bool select);
    virtual void _onSpiSlaveSelect(bool select);
    bool _spiSendData(uint8_t &data);
};

#endif
