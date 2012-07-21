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

// Pin initialization data

extern PinInitData const MEGA32_PIN_INIT_DATA[MEGA32_PIN_COUNT] = {
    { PIN_MODE_INPUT, 0 }, // A0
    { PIN_MODE_INPUT, 0 }, // A1
    { PIN_MODE_INPUT, 0 }, // A2
    { PIN_MODE_INPUT, 0 }, // A3
    { PIN_MODE_INPUT, 0 }, // A4
    { PIN_MODE_INPUT, 0 }, // A5
    { PIN_MODE_INPUT, 0 }, // A6
    { PIN_MODE_INPUT, 0 }, // A7
    { PIN_MODE_INPUT, 0 }, // B0
    { PIN_MODE_INPUT, 0 }, // B1
    { PIN_MODE_INPUT, 0 }, // B2
    { PIN_MODE_INPUT, 0 }, // B3
    { PIN_MODE_INPUT, 0 }, // B4
    { PIN_MODE_INPUT, 0 }, // B5
    { PIN_MODE_INPUT, 0 }, // B6
    { PIN_MODE_INPUT, 0 }, // B7
    { PIN_MODE_INPUT, 0 }, // C0
    { PIN_MODE_INPUT, 0 }, // C1
    { PIN_MODE_INPUT, 0 }, // C2
    { PIN_MODE_INPUT, 0 }, // C3
    { PIN_MODE_INPUT, 0 }, // C4
    { PIN_MODE_INPUT, 0 }, // C5
    { PIN_MODE_INPUT, 0 }, // C6
    { PIN_MODE_INPUT, 0 }, // C7
    { PIN_MODE_INPUT, 0 }, // D0
    { PIN_MODE_INPUT, 0 }, // D1
    { PIN_MODE_INPUT, 0 }, // D2
    { PIN_MODE_INPUT, 0 }, // B3
    { PIN_MODE_INPUT, 0 }, // D4
    { PIN_MODE_INPUT, 0 }, // D5
    { PIN_MODE_INPUT, 0 }, // D6
    { PIN_MODE_INPUT, 0 }  // D7
};

static inline bool is_data_port(uint8_t port)
{
    return ((port >= PORT_PIND) && (port <= PORT_PORTA));
}

static inline bool is_PIN_port(uint8_t port)
{
    return is_data_port(port) && (((port - PORT_PIND) % 3) == 0);
}

static inline bool is_DDR_port(uint8_t port)
{
    return is_data_port(port) && (((port - PORT_PIND) % 3) == 1);
}

static inline bool is_PORT_port(uint8_t port)
{
    return is_data_port(port) && (((port - PORT_PIND) % 3) == 2);
}

static inline uint8_t PIN_for_PORT(uint8_t port)
{
    return port - 2;
}

static inline uint8_t PORT_for_DDR(uint8_t port)
{
    return port + 1;
}

static inline uint8_t DDR_for_PORT(uint8_t port)
{
    return port - 1;
}

static inline int pin_for_port(uint8_t port)
{
    if (is_data_port(port)) {
        return MEGA32_PIN_A + 8 * ((PORT_PORTA - port)/3);
    }

    return -1;
}

static inline bool is_dataport_pin(int pin_id)
{
    return (pin_id >= MEGA32_PIN_A0) && (pin_id <= MEGA32_PIN_D7);
}

static inline uint8_t PIN_for_pin(int pin_id)
{
    return PORT_PINA - 3 * (pin_id / 8);
}

void Atmega32::_pinsInit()
{
    // all pins revert to inputs again
    for (int i = MEGA32_PIN_A0; i <= MEGA32_PIN_D7; i++) {
        this->_setPinMode(i, PIN_MODE_INPUT);
        this->_setPinFloatValue(i, 0);
        this->_pinWrite(i, 0);
    }
}

void Atmega32::_onPinChanged(int pin_id, int value, int old_value)
{
    if (is_dataport_pin(pin_id)) {
        uint8_t port = PIN_for_pin(pin_id);
        
        // TODO: feed into ADC
        if (value) {
            set_bit(this->ports[port], pin_id & 7);
        } else {
            clear_bit(this->ports[port], pin_id & 7);
        }
    }
}

void Atmega32::_handleDataPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
}

void Atmega32::_handleDataPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    int pin = pin_for_port(port);
    
    if (is_PIN_port(port)) {
        fail("Tried to write to PIN port");
    } else if (is_PORT_port(port)) {
        uint8_t mask = this->ports[DDR_for_PORT(port)]; // read corresponding DDR register
        if (bit != -1)
            mask &= (1 << bit);
        
        // copy data to PIN buffer (for pins set as output)
        uint8_t pin_port = PIN_for_PORT(port);
        this->ports[pin_port] = (this->ports[pin_port] & ~mask) | (value & mask);
        
        // toggle outputs proper
        for (uint8_t i = 0; i < 8; i++)
            if (bit_is_set(mask, i)) {
                this->_pinWrite(pin + i, bit_is_set(value, i));
            } else {
                this->_setPinFloatValue(pin + i, bit_is_set(value, i));
            }
    } else if (is_DDR_port(port)) {
        for (uint8_t i = 0; i < 8; i++)
            if (bit_is_set(value, i) != bit_is_set(prev_val, i)) {
                this->_setPinMode(pin + i, bit_is_set(value, i) ? PIN_MODE_OUTPUT : PIN_MODE_INPUT);
            }
    }
}
