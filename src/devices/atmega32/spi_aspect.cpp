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
#include "atmega32.h"
#include "defs.h"

using namespace std;

void Atmega32::_spiInit()
{
    this->ports[PORT_SPCR] = 0x00;
    this->ports[PORT_SPSR] = 0x00;
    this->ports[PORT_SPDR] = 0xff;

    this->spi_stat_read = false;
}

void Atmega32::_spiHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    switch (port) {
        case PORT_SPSR:
            if (bit_is_set(value, B_SPIF))
                this->spi_stat_read = true;
        case PORT_SPDR:
            if (this->spi_stat_read) {
                clear_bit(this->ports[PORT_SPSR], B_SPIF);
                this->spi_stat_read = false;
            }
    }
}

void Atmega32::_spiHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    switch (port) {
        case PORT_SPSR:
            // reject writes to read-only bits
            value = (prev_val & 0xfe) + (value & 0x01);
            break;
        case PORT_SPDR:
            if (!this->_spiIsEnabled())
                return;

            this->spi_stat_read = false;

            this->_spiSendData(value);

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