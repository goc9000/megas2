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

static bool is_timer0_port(uint8_t port)
{
    return (port == PORT_TCNT0) || (port == PORT_TCCR0) || (port == PORT_TIFR);
}

static bool is_timer1_port(uint8_t port)
{
    return (port >= PORT_ICR1L) && (port <= PORT_TCCR1A);
}

static bool is_timer2_port(uint8_t port)
{
    return (port >= PORT_ASSR) && (port <= PORT_TCCR2);
}

static int prescaler_bit_01(int clksel)
{
    switch (clksel) {
        case 1: return 1;
        case 2: return 8;
        case 3: return 64;
        case 4: return 256;
        case 5: return 1024;
    }
    
    return 0;
}

static int prescaler_bit_2(int clksel)
{
    switch (clksel) {
        case 1: return 1;
        case 2: return 8;
        case 3: return 32;
        case 4: return 64;
        case 5: return 128;
        case 6: return 256;
        case 7: return 1024;
    }
    
    return 0;
}

void Atmega32::_timersInit()
{
    // all timer ports default to 0
    this->prescaler01 = 0;
    this->prescaler2 = 0;
    
    this->_timer0Init();
    this->_timer1Init();
    this->_timer2Init();
}

void Atmega32::_runTimers()
{
    int old_value;
    int clksel0 = this->ports[PORT_TCCR0] & 7;
    int clksel1 = this->ports[PORT_TCCR1B] & 7;
    int clksel2 = this->ports[PORT_TCCR2] & 7;
    
    old_value = this->prescaler01;
    this->prescaler01++;
    if ((this->prescaler01 ^ old_value) & prescaler_bit_01(clksel0))
        this->_timer0Tick();
    if ((this->prescaler01 ^ old_value) & prescaler_bit_01(clksel1))
        this->_timer1Tick();
    
    old_value = this->prescaler2;
    this->prescaler2++;
    if ((this->prescaler2 ^ old_value) & prescaler_bit_2(clksel2))
        this->_timer2Tick();
}

void Atmega32::_timersHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    if (is_timer0_port(port)) {
        this->_timer0HandleRead(port, bit, value);
    } else if (is_timer1_port(port)) {
        this->_timer1HandleRead(port, bit, value);
    } else if (is_timer2_port(port)) {
        this->_timer2HandleRead(port, bit, value);
    } else {
        // handle common ports
    }
}

void Atmega32::_timersHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    if (is_timer0_port(port)) {
        this->_timer0HandleWrite(port, bit, value, prev_val);
    } else if (is_timer1_port(port)) {
        this->_timer1HandleWrite(port, bit, value, prev_val);
    } else if (is_timer2_port(port)) {
        this->_timer2HandleWrite(port, bit, value, prev_val);
    } else {
        // handle common ports
        if (port == PORT_TIFR) {
            this->_handleFlagBitsInPortWrite(255, bit, value, prev_val);
        }
    }
}

void Atmega32::_triggerTimerIrq(uint8_t flags)
{
    this->ports[PORT_TIFR] |= flags;
}

uint8_t Atmega32::_handleTimerIrqs()
{
    uint8_t irqs = this->ports[PORT_TIFR] & this->ports[PORT_TIMSK];
    
    if (irqs)
        for (int bit = 7; bit >= 0; bit--)
            if (bit_is_set(irqs, bit)) {
                clear_bit(this->ports[PORT_TIFR], bit);
                return IRQ_TIMER0_OVF - bit;
            }
    
    return 0;
}

void Atmega32::_timer0Init()
{
}

void Atmega32::_timer0Tick()
{
}

void Atmega32::_timer0HandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    fail("Timer 0 not supported");
}

void Atmega32::_timer0HandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    fail("Timer 0 not supported");
}

void Atmega32::_timer1Init()
{
    this->timer1_temp_high_byte = 0;
}

void Atmega32::_timer1Tick()
{
    uint16_t tcnt, top;
    int wgm = ((this->ports[PORT_TCCR1B] >> (B_WGM12-2)) & 0x0c) |
        ((this->ports[PORT_TCCR1A] >> B_WGM10) & 0x03);
    
    switch (wgm) {
        case 0:
        case 4:
        case 12:
            tcnt = this->_get16BitPort(PORT_TCNT1);
            top = this->_get16BitPort((wgm == 4) ? PORT_OCR1A : PORT_ICR1);
            
            // TODO: if TOP==0xFFFF, does TOV get triggered too? I assume not.
            if (((wgm == 4) || (wgm == 12)) && (tcnt == top)) {
                this->_triggerTimerIrq(_BV(B_OCF1A));
                tcnt = 0x0000;
            } else if (tcnt == 0xffff) {
                this->_triggerTimerIrq(_BV(B_TOV1));
                tcnt = 0x0000;
            } else {
                tcnt++;
            }
            
            this->_put16BitPort(PORT_TCNT1, tcnt);
            break;
        default:
            fail("Unsupported WGM for timer 1: %d", wgm);
    }
}

void Atmega32::_timer1HandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    switch (port) {
        case PORT_ICR1H:
        case PORT_TCNT1H:
            value = this->timer1_temp_high_byte;
            break;
        case PORT_ICR1L:
        case PORT_TCNT1L:
            this->timer1_temp_high_byte = this->ports[port+1];
            break;
    }
}

void Atmega32::_timer1HandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    switch (port) {
        case PORT_ICR1H:
        case PORT_OCR1BH:
        case PORT_OCR1AH:
        case PORT_TCNT1H:
            this->timer1_temp_high_byte = value;
            break;
        case PORT_ICR1L:
        case PORT_OCR1BL:
        case PORT_OCR1AL:
        case PORT_TCNT1L:
            this->ports[port+1] = this->timer1_temp_high_byte;
            break;
    }
}

void Atmega32::_timer2Init()
{
}

void Atmega32::_timer2Tick()
{
}

void Atmega32::_timer2HandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    fail("Timer 2 not supported");
}

void Atmega32::_timer2HandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    fail("Timer 2 not supported");
}
