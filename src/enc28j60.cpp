#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdlib>

#include "bit_macros.h"
#include "fail.h"
#include "enc28j60.h"

using namespace std;

Enc28J60::Enc28J60()
{
    this->spi_selected = false;
    this->reset();
}

sim_time_t Enc28J60::nextEventTime()
{
    return 0;
}

void Enc28J60::reset()
{
    printf("ENC28J reset!\n");
}

void Enc28J60::spiSlaveSelect(bool select)
{
    printf("ENC28J select <- %d\n", select);
    
    if (!this->spi_selected && select) {
    }
    
    this->spi_selected = select;
}

bool Enc28J60::spiReceiveData(uint8_t &data)
{
    if (!this->spi_selected)
        return false;
    
    data = _handleSpiData(data);
    
    return true;
}

uint8_t Enc28J60::_handleSpiData(uint8_t data)
{
    printf("ENC28J gets: %02x\n", data);
    
    return 0xff;
}
