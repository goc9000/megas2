#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "utils/fail.h"
#include "ds1307.h"

using namespace std;

Ds1307::Ds1307(uint8_t i2c_address)
{
    this->i2c_addr = i2c_address;
    this->reset();
}

void Ds1307::setSimulationTime(sim_time_t time)
{
    sim_time_t delta = time - this->sim_time;
    
    this->sim_time = time;
    this->next_tick_time += delta;
}

void Ds1307::act()
{
    this->sim_time = this->next_tick_time;
    
    printf("Ds1307 ticks!\n");
    
    this->next_tick_time = this->sim_time + sec_to_sim_time(1);
}

sim_time_t Ds1307::nextEventTime()
{
    return this->next_tick_time;
}

void Ds1307::reset()
{
    this->i2c_listening = false;
    
    this->next_tick_time = this->sim_time + sec_to_sim_time(1);
}

void Ds1307::i2cReceiveStart()
{
    this->i2c_listening = false;
}

void Ds1307::i2cReceiveStop()
{
    this->i2c_listening = false;
}

bool Ds1307::i2cReceiveAddress(uint8_t address, bool write)
{
    if (address != this->i2c_addr)
        return false;
    
    this->i2c_listening = true;
    
printf("***** RTC recognizes address (write:%d)\n", write);
    
    return true;
}

bool Ds1307::i2cReceiveData(uint8_t data)
{
    if (!this->i2c_listening)
        return false;
    
    printf("********* RTC: Received %02x\n", data);
    
    return true;
}

bool Ds1307::i2cQueryData(uint8_t &data)
{
    if (!this->i2c_listening)
        return false;
    
    data = 0xff;
    return true;
}
