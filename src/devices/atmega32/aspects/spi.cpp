#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctype.h>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "devices/atmega32/atmega32.h"
#include "devices/atmega32/defs.h"

using namespace std;

void Atmega32::_spiInit()
{
    this->ports[PORT_SPCR] = 0x00;
    this->ports[PORT_SPSR] = 0x00;
    this->ports[PORT_SPDR] = 0xff;
 
    this->port_metas[PORT_SPSR].write_mask = 0x01;
    
    for (int port = PORT_SPCR; port <= PORT_SPDR; port++) {
        this->port_metas[port].read_handler = &Atmega32::_spiHandleRead;
        this->port_metas[port].write_handler = &Atmega32::_spiHandleWrite;
    }
    
    this->spi_stat_read = false;
}

void Atmega32::_spiHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    switch (port) {
        case PORT_SPSR:
            if (bit_is_set(value, B_SPIF))
                this->spi_stat_read = true;
            break;
        case PORT_SPDR:
            if (this->spi_stat_read) {
                clear_bit(this->ports[PORT_SPSR], B_SPIF);
                this->spi_stat_read = false;
            }
            break;
    }
}

void Atmega32::_spiHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    switch (port) {
        case PORT_SPDR:
            if (!this->_spiIsEnabled())
                return;

            this->spi_stat_read = false;

            this->_spiSendData(value);
            this->ports[PORT_SPDR] = value;

            set_bit(this->ports[PORT_SPSR], B_SPIF);
            break;
    }
}

bool Atmega32::_spiIsEnabled()
{
    return bit_is_set(this->ports[PORT_SPCR], B_SPE);
}

void Atmega32::_onSpiSlaveSelect(bool select)
{
    if (select)
        fail("Slave behavior on SPI not supported for ATMEGA32");
}

bool Atmega32::spiReceiveData(uint8_t &data)
{
    fail("Slave behavior on SPI not supported for ATMEGA32");

    return false;
}
