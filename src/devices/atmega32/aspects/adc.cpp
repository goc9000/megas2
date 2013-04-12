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

void Atmega32::_adcInit()
{
    for (int port = PORT_ADCL; port <= PORT_ADMUX; port++)
        this->ports[port] = 0;
    
    this->port_metas[PORT_ADCSRA].write_mask = 0xef;
    this->port_metas[PORT_ADCL].write_mask = 0x00;
    this->port_metas[PORT_ADCH].write_mask = 0x00;
    
    this->port_metas[PORT_ADCSRA].clearable_mask = 0x10;
    
    for (int port = PORT_ADCL; port <= PORT_ADMUX; port++) {
        this->port_metas[port].read_handler = &Atmega32::_adcHandleRead;
        this->port_metas[port].write_handler = &Atmega32::_adcHandleWrite;
    }
    
    this->adc_enabled = false;
    this->adc_result_locked = false;
}

void Atmega32::_adcHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    int result_shift = bit_is_set(this->ports[PORT_ADMUX], B_ADLAR) ? 6 : 0;
    
    switch (port) {
        case PORT_ADCL:
            value = low_byte(this->adc_result << result_shift);
            this->adc_result_locked = true;
            break;
        case PORT_ADCH:
            value = high_byte(this->adc_result << result_shift);
            this->adc_result_locked = false;
            break;
        default:
            this->_dumpPortRead("ADC", port, bit, value);
            break;
    }
}

void Atmega32::_adcHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    bool enable;
    
    this->_dumpPortWrite("ADC", port, bit, value, prev_val, cleared);
    
    switch (port) {
        case PORT_ADCSRA:
            enable = bit_is_set(value, B_ADEN);
            if (enable != this->adc_enabled) {
                this->_setAdcEnabled(enable);
            }
            if (!enable) break;
            break;
    }
}

uint8_t Atmega32::_handleAdcIrqs()
{
    if (bit_is_set(this->ports[PORT_ADCSRA], B_ADIF) &&
        bit_is_set(this->ports[PORT_ADCSRA], B_ADIE)) {
        clear_bit(this->ports[PORT_ADCSRA], B_ADIF);
        return IRQ_ADC;
    }
    
    return 0;
}

void Atmega32::_setAdcEnabled(bool enabled)
{
    if (this->adc_enabled == enabled)
        return;
    
    this->adc_enabled = enabled;
    
    if (enabled) {
        for (int i = PIN_PA0; i <= PIN_PA7; i++)
            this->_enablePinOverride(i, PIN_MODE_INPUT, PIN_VAL_VCC);
        
        this->adc_result = 0;
        this->adc_result_locked = false;
    } else {
        for (int i = PIN_PA0; i <= PIN_PA7; i++)
            this->_disablePinOverride(i);
    }
}
