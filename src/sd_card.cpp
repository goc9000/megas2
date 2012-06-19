#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "fail.h"
#include "sd_card.h"

using namespace std;

#define STATE_IDLE           0

SdCard::SdCard()
{
    this->spi_selected = false;
    this->reset();
}

sim_time_t SdCard::nextEventTime()
{
    return 0;
}

void SdCard::reset()
{
    this->state = STATE_IDLE;
}

void SdCard::spiSlaveSelect(bool select)
{
    printf("SDCARD select=%d\n", select);
}

bool SdCard::spiReceiveData(uint8_t &data)
{
    if (!this->spi_selected)
        return false;
    
    printf("SDCARD receives: %02x\n", data);
    
    data = 0xff;
    
    return true;
}
