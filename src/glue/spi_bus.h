#ifndef _H_SPI_BUS_H
#define _H_SPI_BUS_H

#include <inttypes.h>
#include <vector>

using namespace std;

class SpiDevice;

class SpiBus {
public:
    void addDevice(SpiDevice *device);
    void removeDevice(SpiDevice *device);
    bool sendData(SpiDevice *sender, uint8_t &data);
private:
    vector<SpiDevice *> devices;
};

#endif
