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
    this->_reg_UCSRC = 0x86;
    this->ports[PORT_UBRRH] = 0x00;
    this->ports[PORT_UBRRL] = 0x00;
    
    this->port_metas[PORT_UCSRA].write_mask = 0x03;
    this->port_metas[PORT_UCSRA].clearable_mask = 0x40;
    this->port_metas[PORT_UCSRB].write_mask = 0xfd;
    
    for (int port = PORT_UBRRL; port <= PORT_UDR; port++) {
        this->port_metas[port].read_handler = &Atmega32::_usartHandleRead;
        this->port_metas[port].write_handler = &Atmega32::_usartHandleWrite;
    }
    this->port_metas[PORT_UBRRH].read_handler = &Atmega32::_usartHandleRead;
    this->port_metas[PORT_UBRRH].write_handler = &Atmega32::_usartHandleWrite;
}

void Atmega32::_usartHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    switch (port) {
        case PORT_UBRRH:
            if (this->_last_UBRRH_access == this->cycle_count - 1) {
                value = this->_reg_UCSRC;
                return;
            }
            this->_last_UBRRH_access = this->cycle_count;
            break;
    }
}

void Atmega32::_usartHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    switch (port) {
        case PORT_UBRRH:
            if (bit_is_set(value, B_URSEL)) {
                ports[PORT_UBRRH] = prev_val;
                _reg_UCSRC = value;
            } else {
                ports[PORT_UBRRH] = value & 0x0f;
            }
            break;
        case PORT_UDR:
            if (!bit_is_set(ports[PORT_UCSRA], B_UDRE))
                fail("Tried to transmit on USART with data register not empty");
            if (!bit_is_set(ports[PORT_UCSRB], B_TXEN))
                fail("Tried to transmit on USART with transmitter disabled");
            
            rs232Send(value);
            
            set_bit(ports[PORT_UCSRA], B_TXC);
            set_bit(ports[PORT_UCSRA], B_UDRE);
            break;
    }
}

void Atmega32::onRS232Receive(uint8_t data)
{
}
