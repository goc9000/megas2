#ifndef _H_SD_CARD_H
#define _H_SD_CARD_H

#include <cstdio>
#include <inttypes.h>

#include "glue/spi_device.h"
#include "glue/pin_device.h"
#include "devices/device.h"
#include "simulation/sim_device.h"

#define SDCARD_PIN_COUNT           1

#define SDCARD_PIN_SLAVE_SELECT    0

class SdCard : public Device, public SpiDevice, public PinDevice, public SimulatedDevice {
public:
    SdCard(const char *backing_file_name, unsigned capacity);
    virtual void reset();
    
    virtual void act();
    virtual sim_time_t nextEventTime();
    
    bool spiReceiveData(uint8_t &data);
private:
    FILE *backing_file;
    unsigned backing_file_len;
    unsigned capacity;
    
    bool in_sd_mode;
    bool idle;
    bool receiving_command;
    bool expecting_acmd;
    bool responding;
    bool responding_with_data;
    bool receiving_write_data;
    int substate;
    uint16_t flags;
    
    uint8_t cmd_buffer[6];
    uint8_t response[5];
    int response_length;
    uint8_t read_block_buffer[512];
    uint16_t read_block_crc;
    unsigned write_block_addr; 
    uint8_t write_block_buffer[512];
    uint16_t write_block_crc;
    
    bool crc_enabled;
    uint16_t block_size;
    
    virtual void _onPinChanged(int pin_id, int value, int old_value);
    
    virtual void _onSpiSlaveSelect(bool select);
    uint8_t _handleSpiData(uint8_t data);
    uint8_t _handleCommandInIdle(uint8_t data);
    uint8_t _handleReceivingCommand(uint8_t data);
    uint8_t _handleResponding(uint8_t data);
    uint8_t _handleRespondingWithData(uint8_t data);
    uint8_t _handleReceivingWriteData(uint8_t data);
    
    void _execCommand(uint8_t command, uint32_t param);
    void _execAppCommand(uint8_t command, uint32_t param);
    void _prepareR1Response();
    void _prepareDataResponse(bool crc_error, bool write_error);
    void _readBlockFromBackingFile(uint8_t *buffer, unsigned offset, unsigned length);
    bool _writeBlockToBackingFile(uint8_t *buffer, unsigned offset, unsigned length);
    void _expandBackingFile(unsigned minimum_size);
};

#endif
