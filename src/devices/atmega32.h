#ifndef _H_ATMEGA32_H
#define _H_ATMEGA32_H

#include <vector>

#include "glue/pin_monitor.h"
#include "glue/i2c_device.h"
#include "glue/spi_device.h"
#include "simulation/sim_device.h"
#include "device.h"

#define MEGA32_PIN_A       0x00
#define MEGA32_PIN_B       0x08
#define MEGA32_PIN_C       0x10
#define MEGA32_PIN_D       0x18
#define MEGA32_PIN_COUNT   0x20

// Flash size is in 16-bit words
#define MEGA32_FLASH_SIZE  0x4000
// Others are in bytes
#define MEGA32_RAM_SIZE    0x0860
#define MEGA32_IO_BASE     0x0020   
#define MEGA32_SRAM_BASE   0x0060

class Atmega32 : public Device, public I2cDevice, public SpiDevice, public SimulatedDevice {
public:
    Atmega32();
    void load_program_from_elf(const char *filename);
    void setFrequency(unsigned frequency_khz);

    void step();
    
    virtual void reset();
    
    virtual void setSimulationTime(sim_time_t time);
    virtual void act();
    virtual sim_time_t nextEventTime();

    void addPinMonitor(int pin, PinMonitor* monitor);
    void removePinMonitor(int pin, PinMonitor* monitor);
    
    virtual void i2cReceiveStart();
    virtual bool i2cReceiveAddress(uint8_t address, bool write);
    virtual bool i2cReceiveData(uint8_t data);
    virtual bool i2cQueryData(uint8_t &data);
    virtual void i2cReceiveStop();

    virtual void spiSlaveSelect(bool select);
    virtual bool spiReceiveData(uint8_t &data);
private:
    unsigned frequency_khz;
    sim_time_t clock_period;

    sim_time_t next_fetch_time;

    uint16_t flash[MEGA32_FLASH_SIZE];
    
    uint8_t ram[MEGA32_RAM_SIZE]; // note: includes regs and I/O space
    int pc;
    int last_inst_pc;

    vector<PinMonitor *> pin_monitors[MEGA32_PIN_COUNT];
    
    bool twi_has_floor;
    bool twi_start_just_sent;
    bool twi_xmit_mode;

    bool spi_stat_read;
    
    // shortcuts
    uint8_t *ports;

    // methods
    uint16_t _fetchNextOpcode();
    void _skipInstruction();
    void _doJump(int address);
    void _doRelJump(int displacement);
    
    void _execMovW(uint16_t &opcode);
    void _execMultiplications(uint16_t &opcode);
    void _execRegRegOp(uint16_t &opcode);
    void _execRegImmOp(uint16_t &opcode);
    void _execLdd(uint16_t &opcode);
    void _execRegMemOp(uint16_t &opcode);
    void _execFlagOp(uint16_t &opcode);
    void _execLongJump(uint16_t &opcode);
    void _execSingleRegOp(uint16_t &opcode);
    void _execIndirectJump(uint16_t &opcode);
    void _execReturn(uint16_t &opcode);
    void _execMcuControlOp(uint16_t &opcode);
    void _execProgMemOp(uint16_t &opcode);
    void _execWordImmOp(uint16_t &opcode);
    void _execIoBitOp(uint16_t &opcode);
    void _execIo(uint16_t &opcode);
    void _execRelativeJump(uint16_t &opcode);
    void _execBranch(uint16_t &opcode);
    void _execBitOp(uint16_t &opcode);
    
    void _setLogicalOpFlags(uint8_t result);
    void _doAdd(uint8_t dest_reg, uint8_t value, bool carry);
    void _doCpOrSub(uint8_t dest_reg, uint8_t value, bool carry, bool store);
    
    void _setFlag(uint8_t bit, bool set);
    bool _getFlag(uint8_t bit);
    uint8_t _readReg(uint8_t reg);
    uint16_t _read16BitReg(uint8_t reg);
    void _writeReg(uint8_t reg, uint8_t value);
    void _write16BitReg(uint8_t reg, uint16_t value);
    bool _readRegBit(uint8_t reg, uint8_t bit);
    void _writeRegBit(uint8_t reg, uint8_t bit, bool value);
    uint8_t _readPort(uint8_t port);
    void _writePort(uint8_t port, uint8_t value);
    bool _readPortBit(uint8_t port, uint8_t bit);
    void _writePortBit(uint8_t port, uint8_t bit, bool value);
    uint8_t _readMem(int addr);
    void _writeMem(int addr, uint8_t value);
    void _loadProgMem(uint8_t dest_reg, bool post_increment, bool extended);
    void _doLoadStore(uint8_t addr_reg, uint16_t displ, uint8_t dest_reg, uint8_t incrementing, bool store);
    void _push(uint8_t value);
    void _pushWord(uint16_t value);
    uint8_t _pop();
    uint16_t _popWord();
    
    void _onPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _onPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);

    void _triggerPinMonitors(int pin, int value);

    void _handleDataPortRead(uint8_t port, int8_t bit, uint8_t &value);
    void _handleDataPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    
    void _twiInit();
    void _twiHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _twiHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    void _twiStatus(uint8_t status);
    
    void _spiInit();
    void _spiHandleRead(uint8_t port, int8_t bit, uint8_t &value);
    void _spiHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val);
    bool _spiIsEnabled();

    void _dumpRegisters();
    void _dumpSram();
};

#endif
