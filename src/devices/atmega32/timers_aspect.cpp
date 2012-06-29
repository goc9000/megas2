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
        this->_tickTimer0();
    if ((this->prescaler01 ^ old_value) & prescaler_bit_01(clksel1))
        this->_tickTimer1();
    
    old_value = this->prescaler2;
    this->prescaler2++;
    if ((this->prescaler2 ^ old_value) & prescaler_bit_2(clksel2))
        this->_tickTimer2();
}

void Atmega32::_tickTimer0()
{
}

void Atmega32::_tickTimer1()
{
}

void Atmega32::_tickTimer2()
{
}

void Atmega32::_timersHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
}

void Atmega32::_timersHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
}
