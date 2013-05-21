#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "ds1307.h"

using namespace std;

#define SIM_EVENT_TICK   0

#define REG_SECONDS      0
#define REG_MINUTES      1
#define REG_HOURS        2
#define REG_DAY          3
#define REG_DATE         4
#define REG_MONTH        5
#define REG_YEAR         6
#define REG_CONTROL      7

#define B_CH             7
#define B_12HOUR         6
#define B_AMPM           5
#define B_OUT            7
#define B_SQWE           4
#define B_RS1            1
#define B_RS0            0

#define DEFAULT_NAME "DS1307"


static inline void bcd_write(uint8_t &bcd_value, uint8_t write_mask, uint8_t dec_value)
{
    uint8_t new_value = ((dec_value / 10) << 4) + (dec_value % 10);
    
    bcd_value = (bcd_value & ~write_mask) | (new_value & write_mask);
}

static inline bool bcd_read(uint8_t bcd_value, uint8_t &dec_value, uint8_t min, uint8_t max)
{
    if (((bcd_value & 0x0f) > 9) || ((bcd_value & 0xf0) > 0x90))
        return false;
    
    uint8_t value = ((bcd_value >> 4) & 0x0f) * 10 + (bcd_value & 0x0f);
    
    if ((value < min) || (value > max))
        return false;
    
    dec_value = value;
    
    return true;
}


Ds1307::Ds1307(uint8_t i2c_address) : Ds1307(i2c_address, string())
{
}

Ds1307::Ds1307(uint8_t i2c_address, string backing_file_name) : Entity(DEFAULT_NAME)
{
    i2c_addr = i2c_address;
    this->backing_file_name = backing_file_name;
    
    init(true);
}

Ds1307::Ds1307(Json::Value &json_data) : Entity(DEFAULT_NAME, json_data)
{
    bool init_with_current_time = true;
    
    backing_file_name = string();
    
    parseJsonParam(i2c_addr, json_data, "i2c_address");
    parseOptionalJsonParam(backing_file_name, json_data, "nvram_file");
    parseOptionalJsonParam(init_with_current_time, json_data, "init_with_current_time");
    
    init(init_with_current_time);
}

Ds1307::~Ds1307()
{
    saveNVRAM();
}

void Ds1307::init(bool init_with_current_time)
{
    resetNVRAM();
    
    if (loadNVRAM())
        saveNVRAM();
    
    if (init_with_current_time) {
        setTime(time(NULL));
        clear_bit(nvram[REG_SECONDS], B_CH);
        saveNVRAM();
    }
    
    reset();
}

void Ds1307::act(int event)
{
    tick();
    scheduleEventIn(SIM_EVENT_TICK, sec_to_sim_time(1));
}

void Ds1307::reset()
{
    i2c_listening = false;
    time_modified = false;
    reg_pointer = 0;
    
    resetDividerChain();
}

void Ds1307::i2cReceiveStart()
{
    i2c_listening = false;
    time_modified = false;
}

void Ds1307::i2cReceiveStop()
{
    i2c_listening = false;
    
    if (time_modified) {
        time_t time_val = getTimeAndCheck();
        
        struct tm date = *localtime(&time_val);
        
        info("RTC time modified: %04d-%02d-%02d %02d:%02d:%02d",
            1900 + date.tm_year, date.tm_mon + 1, date.tm_mday,
            date.tm_hour, date.tm_min, date.tm_sec);
        
        time_modified = false;
    }
}

bool Ds1307::i2cReceiveAddress(uint8_t address, bool write)
{
    if (address != i2c_addr)
        return false;
    
    i2c_listening = true;
    
    receiving_address = write;
    
    if (!write)
        memcpy(cached_time, nvram, 7);
    
    return true;
}

bool Ds1307::i2cReceiveData(uint8_t data)
{
    if (!i2c_listening)
        return false;
    
    if (receiving_address) {
        if (data >= DS1307_NVRAM_SIZE)
            fail("DS1307 received invalid register pointer %02x", data);
        
        reg_pointer = data;
        receiving_address = false;
    } else {
        if (reg_pointer == REG_SECONDS)
            resetDividerChain();
            
        if (reg_pointer < REG_CONTROL)
            time_modified = true;
        
        nvram[reg_pointer++] = data;
        
        if (reg_pointer >= DS1307_NVRAM_SIZE)
            reg_pointer = 0;
    }
    
    return true;
}

bool Ds1307::i2cQueryData(uint8_t &data)
{
    if (!i2c_listening)
        return false;
        
    data = (reg_pointer < REG_CONTROL) ? cached_time[reg_pointer] : nvram[reg_pointer];
    
    reg_pointer++;
    if (reg_pointer >= DS1307_NVRAM_SIZE)
            reg_pointer = 0;
    
    return true;
}

void Ds1307::resetDividerChain(void)
{
    unscheduleAll();
    scheduleEventIn(SIM_EVENT_TICK, sec_to_sim_time(1));
}

void Ds1307::tick(void)
{
    if (bit_is_set(nvram[REG_SECONDS], B_CH))
        return;
    
    setTime(getTimeAndCheck() + 1);
}

void Ds1307::setTime(time_t unix_time)
{
    time_t old_time;
    bool valid = getTime(old_time);
    struct tm old_cal_time = *localtime(&old_time);
        
    struct tm cal_time = *localtime(&unix_time);
    
    bcd_write(nvram[REG_SECONDS], 0x7f, cal_time.tm_sec);
    bcd_write(nvram[REG_MINUTES], 0xff, cal_time.tm_min);
    
    if (bit_is_set(nvram[REG_HOURS], B_12HOUR)) {
        bcd_write(nvram[REG_HOURS], 0x9f,
            (cal_time.tm_hour % 12) ? (cal_time.tm_hour % 12) : 12);
        chg_bit(nvram[REG_HOURS], B_AMPM, cal_time.tm_hour >= 12);
    } else {
        bcd_write(nvram[REG_HOURS], 0xbf, cal_time.tm_hour);
    }
    
    bcd_write(nvram[REG_DATE], 0xff, cal_time.tm_mday);
    bcd_write(nvram[REG_MONTH], 0xff, cal_time.tm_mon + 1);
    bcd_write(nvram[REG_YEAR], 0xff, (cal_time.tm_year % 100));
    
    if (valid) {
        nvram[REG_DAY] = ((nvram[REG_DAY] - 1 + cal_time.tm_wday -
            old_cal_time.tm_wday + 7) % 7) + 1;
    } else {
        nvram[REG_DAY] = 1 + cal_time.tm_wday;
    }
}

time_t Ds1307::getTimeAndCheck(void)
{
    time_t time_val;
    
    bool valid = getTime(time_val);
    
    if (!valid) {
        fail("Invalid RTC time value: %02x %02x %02x %02x %02x %02x %02x",
            nvram[0], nvram[1], nvram[2], nvram[3], nvram[4], nvram[5], nvram[6]);
    }
    
    return time_val;
}

bool Ds1307::getTime(time_t &unix_time)
{
    uint8_t seconds = 0, minutes = 0, hours = 0, dow = 0, date = 0, month = 0, year = 0;
    
    bool valid = true;
    
    valid &= bcd_read(nvram[REG_SECONDS] & 0x7f, seconds, 0, 59);
    valid &= bcd_read(nvram[REG_MINUTES], minutes, 0, 59);
    
    if (bit_is_set(nvram[REG_HOURS], B_12HOUR)) {
        valid &= bcd_read(nvram[REG_HOURS] & 0x9f, hours, 1, 12);
        if (hours == 12)
            hours -= 12;
        if (bit_is_set(nvram[REG_HOURS], B_AMPM))
            hours += 12;
    } else {
        valid &= bcd_read(nvram[REG_HOURS] & 0xbf, hours, 0, 23);
    }
    
    valid &= bcd_read(nvram[REG_DAY], dow, 1, 7);
    valid &= bcd_read(nvram[REG_DATE], date, 1, 31);
    valid &= bcd_read(nvram[REG_MONTH], month, 1, 12);
    valid &= bcd_read(nvram[REG_YEAR], year, 0, 99);
    
    struct tm cal_time;
    cal_time.tm_sec = seconds;
    cal_time.tm_min = minutes;
    cal_time.tm_hour = hours;
    cal_time.tm_mday = date;
    cal_time.tm_mon = month - 1;
    cal_time.tm_year = ((year < 70) ? 100 : 0) + year;
    
    time_t time_val = mktime(&cal_time);
    valid &= (time_val != -1);
    
    if (valid)
        unix_time = time_val;
    
    return valid;
}

void Ds1307::resetNVRAM(void)
{
    memset(nvram, 0, DS1307_NVRAM_SIZE);
    
    set_bit(nvram[REG_SECONDS], B_CH);
    nvram[REG_DAY] = 0x01;
    nvram[REG_DATE] = 0x01;
    nvram[REG_MONTH] = 0x01;
    nvram[REG_CONTROL] = 0x03;
}

bool Ds1307::loadNVRAM(void)
{
    if (backing_file_name == "")
        return false;
    
    FILE *file = fopen(backing_file_name.c_str(), "rb");
    if (!file)
        return false;
    if (fseek(file, 0, SEEK_END) < 0)
        fail("Cannot seek in DS1307 NVRAM file '%s'", backing_file_name.c_str());
    if (ftell(file) != 64)
        fail("DS1307 NVRAM file has incorrect size (!= 64 bytes)");
    if ((fseek(file, 0, SEEK_SET) < 0) || (fread(nvram, 64, 1, file) != 1))
        fail("Error reading DS1307 NVRAM file");
    
    fclose(file);
    
    return true;
}

void Ds1307::saveNVRAM(void)
{
    if (backing_file_name == "")
        return;
    
    FILE *file = fopen(backing_file_name.c_str(), "wb");
    if (!file || (fwrite(nvram, 64, 1, file) != 1)) {
        warn("Cannot save DS1307 backing file '%s'", backing_file_name.c_str());
        return;
    }
    fclose(file);
}
