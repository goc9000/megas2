#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "utils/fail.h"
#include "ds1307.h"

using namespace std;

#define SIM_EVENT_TICK   0

Ds1307::Ds1307(uint8_t i2c_address) : Entity("ds1307", "DS1307")
{
    this->i2c_addr = i2c_address;
    this->reset();
}

Ds1307::Ds1307(Json::Value &json_data) : Entity(json_data)
{
    if (json_data.isMember("i2c_address")) {
        this->i2c_addr = json_data["i2c_address"].asInt();
    } else {
        fail("Attribute 'i2c_address' is required for DS1307");
    }
    
    this->reset();
}

void Ds1307::act(int event)
{
    //printf("Ds1307 ticks!\n");

    if (this->simulation) {
        this->simulation->scheduleEvent(this, SIM_EVENT_TICK, this->simulation->time + sec_to_sim_time(1));
    }
}

void Ds1307::reset()
{
    this->i2c_listening = false;
    
    if (this->simulation) {
        this->simulation->unscheduleAll(this);
        this->simulation->scheduleEvent(this, SIM_EVENT_TICK, this->simulation->time + sec_to_sim_time(1));
    }
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
    
//printf("***** RTC recognizes address (write:%d)\n", write);
    
    return true;
}

bool Ds1307::i2cReceiveData(uint8_t data)
{
    if (!this->i2c_listening)
        return false;
    
    //printf("********* RTC: Received %02x\n", data);
    
    return true;
}

bool Ds1307::i2cQueryData(uint8_t &data)
{
    if (!this->i2c_listening)
        return false;
    
    data = 0xff;
    return true;
}
