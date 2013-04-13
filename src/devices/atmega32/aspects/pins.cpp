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

// Pin initialization data

extern PinInitData const MEGA32_PIN_INIT_DATA[MEGA32_PIN_COUNT] = {
    { "A0", PIN_MODE_INPUT, 0 }, // A0
    { "A1", PIN_MODE_INPUT, 0 }, // A1
    { "A2", PIN_MODE_INPUT, 0 }, // A2
    { "A3", PIN_MODE_INPUT, 0 }, // A3
    { "A4", PIN_MODE_INPUT, 0 }, // A4
    { "A5", PIN_MODE_INPUT, 0 }, // A5
    { "A6", PIN_MODE_INPUT, 0 }, // A6
    { "A7", PIN_MODE_INPUT, 0 }, // A7
    { "B0", PIN_MODE_INPUT, 0 }, // B0
    { "B1", PIN_MODE_INPUT, 0 }, // B1
    { "B2", PIN_MODE_INPUT, 0 }, // B2
    { "B3", PIN_MODE_INPUT, 0 }, // B3
    { "B4", PIN_MODE_INPUT, 0 }, // B4
    { "B5", PIN_MODE_INPUT, 0 }, // B5
    { "B6", PIN_MODE_INPUT, 0 }, // B6
    { "B7", PIN_MODE_INPUT, 0 }, // B7
    { "C0", PIN_MODE_INPUT, 0 }, // C0
    { "C1", PIN_MODE_INPUT, 0 }, // C1
    { "C2", PIN_MODE_INPUT, 0 }, // C2
    { "C3", PIN_MODE_INPUT, 0 }, // C3
    { "C4", PIN_MODE_INPUT, 0 }, // C4
    { "C5", PIN_MODE_INPUT, 0 }, // C5
    { "C6", PIN_MODE_INPUT, 0 }, // C6
    { "C7", PIN_MODE_INPUT, 0 }, // C7
    { "D0", PIN_MODE_INPUT, 0 }, // D0
    { "D1", PIN_MODE_INPUT, 0 }, // D1
    { "D2", PIN_MODE_INPUT, 0 }, // D2
    { "D3", PIN_MODE_INPUT, 0 }, // B3
    { "D4", PIN_MODE_INPUT, 0 }, // D4
    { "D5", PIN_MODE_INPUT, 0 }, // D5
    { "D6", PIN_MODE_INPUT, 0 }, // D6
    { "D7", PIN_MODE_INPUT, 0 }, // D7
    { "AREF", PIN_MODE_INPUT, PIN_VAL_VCC } // AREF
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
        return MEGA32_PIN_PA0 + 8 * ((PORT_PORTA - port)/3);
    }

    return -1;
}

static inline bool is_dataport_pin(int pin_id)
{
    return (pin_id >= MEGA32_PIN_PA0) && (pin_id <= MEGA32_PIN_PD7);
}

static inline uint8_t PIN_for_pin(int pin_id)
{
    return PORT_PINA - 3 * (pin_id / 8);
}

static inline uint8_t PORT_for_pin(int pin_id)
{
    return PORT_PORTA - 3 * (pin_id / 8);
}

static inline uint8_t DDR_for_pin(int pin_id)
{
    return PORT_DDRA - 3 * (pin_id / 8);
}

static inline uint8_t bit_for_pin(int pin_id)
{
    return pin_id % 8;
}

void Atmega32::_pinsInit()
{
    for (int port = PORT_PIND; port <= PORT_PORTA; port++) {
        this->ports[port] = 0;
        this->port_metas[port].read_handler = &Atmega32::_handleDataPortRead;
        this->port_metas[port].write_handler = &Atmega32::_handleDataPortWrite;
    }
    
    this->_pin_overrides = vector<bool>(this->_num_pins);
    
    // all pins revert to inputs again
    for (int i = MEGA32_PIN_PA0; i <= MEGA32_PIN_PD7; i++) {
        this->_pins[i].setMode(PIN_MODE_INPUT);
        this->_pins[i].setFloatValueDigital(0);
        this->_pins[i].writeDigital(0);
    }
}

void Atmega32::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
    if (is_dataport_pin(pin_id)) {
        uint8_t port = PIN_for_pin(pin_id);
        
        chg_bit(this->ports[port], bit_for_pin(pin_id), this->_pins[pin_id].readDigital());
    }
}

void Atmega32::_handleDataPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
}

void Atmega32::_handleDataPortWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
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
        for (uint8_t i = 0; i < 8; i++) {
            if (!this->_pin_overrides[pin + i]) {
                if (bit_is_set(mask, i)) {
                    this->_pins[pin + i].writeDigital(bit_is_set(value, i));
                } else {
                    this->_pins[pin + i].setFloatValueDigital(bit_is_set(value, i));
                }
            }
        }
    } else if (is_DDR_port(port)) {
        for (uint8_t i = 0; i < 8; i++) {
            if (!this->_pin_overrides[pin + i]) {
                if (bit_is_set(value, i) != bit_is_set(prev_val, i)) {
                    this->_pins[pin + i].setMode(bit_is_set(value, i) ? PIN_MODE_OUTPUT : PIN_MODE_INPUT);
                }
            }
        }
    }
}

void Atmega32::_enablePinOverride(int pin_id, int mode, pin_val_t float_value)
{
    if (!is_dataport_pin(pin_id))
        fail("Tried to override non-dataport pin #%d", pin_id);
    
    this->_pin_overrides[pin_id] = true;
    this->_pins[pin_id].setMode(mode);
    this->_pins[pin_id].setFloatValue(float_value);
}

void Atmega32::_disablePinOverride(int pin_id)
{
    if (!is_dataport_pin(pin_id))
        fail("Tried to unoverride non-dataport pin #%d", pin_id);
 
    uint8_t bit = bit_for_pin(pin_id);
    bool is_out = bit_is_set(DDR_for_pin(pin_id), bit);
    
    this->_pin_overrides[pin_id] = false;
    this->_pins[pin_id].setMode(is_out ? PIN_MODE_OUTPUT : PIN_MODE_INPUT);
    if (is_out) {
        this->_pins[pin_id].writeDigital(bit_is_set(PORT_for_pin(pin_id), bit));
    } else {
        this->_pins[pin_id].setFloatValueDigital(bit_is_set(PORT_for_pin(pin_id), bit));
    }
}
