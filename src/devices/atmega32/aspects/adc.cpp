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

const pin_val_t ADC_REF_VBG = 1.22;
const pin_val_t ADC_REF_V256 = 2.56;

const pin_val_t ADC_REF_EPSILON = 0.0001;

const int MAX_ADC_VALUE = 0x3ff;

const int ADC_CYCLES_FOR_CONVERSION = 15;


void Atmega32::_adcInit()
{
    for (int port = PORT_ADCL; port <= PORT_ADMUX; port++)
        this->ports[port] = 0;
    
    this->port_metas[PORT_ADCSRA].write_mask = 0xff - _BV(B_ADIF);
    this->port_metas[PORT_ADCL].write_mask = 0x00;
    this->port_metas[PORT_ADCH].write_mask = 0x00;
    
    this->port_metas[PORT_ADCSRA].clearable_mask = _BV(B_ADIF);
    this->port_metas[PORT_ADCSRA].unclearable_mask = _BV(B_ADSC);
    
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
    }
}

void Atmega32::_adcHandleWrite(uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    bool enable;
    
    switch (port) {
        case PORT_ADCSRA:
            enable = bit_is_set(value, B_ADEN);
            if (enable != this->adc_enabled) {
                this->_setAdcEnabled(enable);
            }
            if (!enable) break;
            
            if (bit_was_set(prev_val, value, B_ADSC)) {
                this->_startAdcConversion();
            }
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
        for (int i = MEGA32_PIN_PA0; i <= MEGA32_PIN_PA7; i++)
            this->_enablePinOverride(i, PIN_MODE_INPUT, PIN_VAL_VCC);
    } else {
        for (int i = MEGA32_PIN_PA0; i <= MEGA32_PIN_PA7; i++)
            this->_disablePinOverride(i);
    }
    
    clear_bit(this->ports[PORT_ADCSRA], B_ADSC);
    this->adc_result = 0;
    this->adc_result_locked = false;
}

void Atmega32::_startAdcConversion()
{
    set_bit(this->ports[PORT_ADCSRA], B_ADSC);
    this->adc_last_admux = this->ports[PORT_ADMUX];
    
    int adc_prescaler_factor = max(2, (1 << (this->ports[PORT_ADCSRA] & 7)));
    sim_time_t adc_clock_period = this->clock_period * adc_prescaler_factor;
    
    scheduleEventIn(SIM_EVENT_ADC_COMPLETE_CONVERSION,
        adc_clock_period * ADC_CYCLES_FOR_CONVERSION);
}

void Atmega32::_completeAdcConversion()
{
    if (!this->adc_result_locked) {
        this->adc_result = this->_getAdcMeasurement();
        set_bit(this->ports[PORT_ADCSRA], B_ADIF);
    }
    
    clear_bit(this->ports[PORT_ADCSRA], B_ADSC);
}

uint16_t Atmega32::_getAdcMeasurement()
{
    int up_bound = this->_adcInDifferentialMode() ? (MAX_ADC_VALUE >> 1) : MAX_ADC_VALUE;
    int lo_bound = this->_adcInDifferentialMode() ? (-1 - (MAX_ADC_VALUE >> 1)) : 0;
    
    pin_val_t diff_input = this->_getAdcPosVoltage() - this->_getAdcNegVoltage();
    pin_val_t ref = max(this->_getAdcRefVoltage(), ADC_REF_EPSILON);
    
    diff_input *= this->_getAdcGain();
    
    double normalized = (up_bound * diff_input) / ref;
    
    normalized = max(normalized, (double)lo_bound);
    normalized = min(normalized, (double)up_bound);
    
    return ((int)normalized) & MAX_ADC_VALUE;
}

pin_val_t Atmega32::_getAdcRefVoltage()
{
    uint8_t ref = (this->adc_last_admux >> 6) & 3;
    
    bool has_aref = !this->_pins[MEGA32_PIN_AREF].isDisconnected();
    
    switch (ref) {
        case 0:
            if (!has_aref)
                fail("Used AREF as ADC reference with nothing connected to AREF pin");
            return this->_pins[MEGA32_PIN_AREF].read();
        case 1:
            if (has_aref)
                fail("Used AVCC as ADC reference with AREF pin connected");
            return this->_vcc;
        case 2:
            fail("Used reserved ADC reference REFS=10");
            break;
        case 3:
            if (has_aref)
                fail("Used V2.56 as ADC reference with AREF pin connected");
            return ADC_REF_V256;
    }
    
    return 0.0;
}

pin_val_t Atmega32::_getAdcPosVoltage()
{
    uint8_t mux = this->adc_last_admux & 0x1f;
    
    if (((mux >> 3) & 0x03) == 0x01) {
        return this->_pins[MEGA32_PIN_PA0 + ((mux & 4) >> 1) + (mux & 1)].read();
    } else if (mux == 0x1e) {
        return ADC_REF_VBG;
    } else if (mux == 0x1f) {
        return 0.0;
    }
    
    return this->_pins[MEGA32_PIN_PA0 + (mux & 7)].read();
}

pin_val_t Atmega32::_getAdcNegVoltage()
{
    uint8_t mux = this->adc_last_admux & 0x1f;
    
    if (((mux >> 3) & 0x03) == 0x01) {
        return this->_pins[MEGA32_PIN_PA0 + ((mux & 4) >> 1)].read();
    } else if ((mux >= 0x10) && (mux <= 0x1d)) {
        return this->_pins[MEGA32_PIN_PA0 + 1 + ((mux >> 3) & 1)].read();
    }
    
    return 0.0;
}

double Atmega32::_getAdcGain()
{
    switch ((this->adc_last_admux >> 1) & 0x0f) {
        case 4: case 6:
            return 10.0;
        case 5: case 7:
            return 200.0;
        default:
            return 1.0;
    }
}

bool Atmega32::_adcInDifferentialMode()
{
    uint8_t mux = this->adc_last_admux & 0x1f;
    
    return (mux >= 0x10) && (mux <= 0x1d);
}
