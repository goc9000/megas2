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

uint64_t compute_twi_baud(uint64_t cpu_freq, uint8_t twbr_val, uint8_t twsr_val)
{
    return cpu_freq / (16 + (twbr_val << (1 + 2*(twsr_val & 0x03))));
}

void Atmega32::_twiInit()
{
    this->ports[PORT_TWBR] = 0x00;
    this->ports[PORT_TWSR] = 0xf8;
    this->ports[PORT_TWAR] = 0xfe;
    this->ports[PORT_TWDR] = 0xff;
    this->ports[PORT_TWCR] = 0x00;
    
    this->twi_has_floor = false;
    this->twi_start_just_sent = false;
}

void Atmega32::_twiHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
}

void Atmega32::_twiHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    switch (port) {
        case PORT_TWBR:
        case PORT_TWSR:
            info("TWI baud rate set to %llu", compute_twi_baud(this->frequency, this->ports[PORT_TWBR], this->ports[PORT_TWSR]));
            break;
        case PORT_TWCR:
            if (!bit_is_set(value, B_TWEN)) {
                this->twi_has_floor = false;
                return;
            }
            
            if (bit_is_set(cleared, B_TWINT)) {
                // Note: TWSTA and TWSTO may be set simultaneously. The effect
                // is to send the STOP first
                if (bit_is_set(value, B_TWSTO)) {
                    this->_twiDoSendStop();
                    if (bit_is_set(value, B_TWSTA)) {
                        this->_twiDoSendStart();
                    }
                } else if (bit_is_set(value, B_TWSTA)) {
                    this->_twiDoSendStart();
                } else if (this->twi_start_just_sent) {
                    this->_twiDoSendAddress();
                } else if (this->twi_xmit_mode) {
                    this->_twiDoSendData();
                } else {
                    this->_twiDoReceiveData();
                }
            }
            break;
    }
}

void Atmega32::_twiDoSendStart()
{
    this->_i2cSendStart();
    this->_twiCompleteOp(this->twi_has_floor ? TWI_STATUS_RESTART : TWI_STATUS_START);
    this->twi_has_floor = true;
    this->twi_start_just_sent = true;
}

void Atmega32::_twiDoSendStop()
{
    this->_i2cSendStop();
    this->_twiCompleteOp(TWI_STATUS_IDLE);
    clear_bit(this->ports[PORT_TWCR], B_TWSTO);
    this->twi_has_floor = false;
    this->twi_start_just_sent = false;
}

void Atmega32::_twiDoSendAddress()
{
    uint8_t data = this->ports[PORT_TWDR];
    if (!this->twi_has_floor)
        fail("Tried to send address on TWI without having the floor!");

    this->twi_xmit_mode = !bit_is_set(data, 0);
    bool ack = this->_i2cSendAddress((data >> 1) & 0x7f, twi_xmit_mode);
    this->_twiCompleteOp(twi_xmit_mode
        ? (ack ? TWI_STATUS_W_SL_ACK : TWI_STATUS_W_SL_NACK)
        : (ack ? TWI_STATUS_R_SL_ACK : TWI_STATUS_R_SL_NACK));
    this->twi_start_just_sent = false;
}

void Atmega32::_twiDoSendData()
{
    uint8_t data = this->ports[PORT_TWDR];
    if (!this->twi_has_floor)
        fail("Tried to send data on TWI without having the floor!");

    bool ack = this->_i2cSendData(data);
    this->_twiCompleteOp(ack ? TWI_STATUS_W_DATA_ACK : TWI_STATUS_W_DATA_NACK);
}

void Atmega32::_twiDoReceiveData()
{
    if (!this->twi_has_floor)
        fail("Tried to receive data on TWI without having the floor!");

    bool ack = this->_i2cQueryData();
    this->_twiCompleteOp(ack ? TWI_STATUS_R_DATA_ACK : TWI_STATUS_R_DATA_NACK);
}

void Atmega32::_twiCompleteOp(uint8_t status)
{
    // All TWI operations appear to complete immediately (not realistic of course)
    set_bit(this->ports[PORT_TWCR], B_TWINT);
    this->ports[PORT_TWSR] = status | (this->ports[PORT_TWSR] & 0x03);
}

void Atmega32::i2cReceiveStart()
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
}

void Atmega32::i2cReceiveStop()
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
}

bool Atmega32::i2cReceiveAddress(uint8_t address, bool write)
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
    
    return false;
}

bool Atmega32::i2cReceiveData(uint8_t data)
{
    if (this->twi_has_floor) {
        if (this->twi_xmit_mode)
            fail("ATMEGA32 Received I2C data while transmitting!");
        
        this->ports[PORT_TWDR] = data;
        
        return true;
    }
    
    fail("Slave behavior on I2C not supported for ATMEGA32");
    
    
    return false;
}

bool Atmega32::i2cQueryData(uint8_t &data)
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
    
    return false;
}
