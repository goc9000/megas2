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

void Atmega32::_usartInit()
{
    this->ports[PORT_UDR] = 0x00;
    this->ports[PORT_UCSRA] = 0x20;
    this->ports[PORT_UCSRB] = 0x00;
    this->ports[PORT_UCSRC] = 0x86;
    this->_reg_UBRRH = 0x00;
    this->ports[PORT_UBRRL] = 0x00;
    
    this->port_metas[PORT_UCSRA].write_mask = 0x43;
    this->port_metas[PORT_UCSRB].write_mask = 0xfd;
    
    for (int port = PORT_UBRRL; port <= PORT_UDR; port++) {
        this->port_metas[port].read_handler = &Atmega32::_usartHandleRead;
        this->port_metas[port].write_handler = &Atmega32::_usartHandleWrite;
    }
    this->port_metas[PORT_UCSRC].read_handler = &Atmega32::_usartHandleRead;
    this->port_metas[PORT_UCSRC].write_handler = &Atmega32::_usartHandleWrite;
}

void Atmega32::_usartHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
}

void Atmega32::_usartHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
}
