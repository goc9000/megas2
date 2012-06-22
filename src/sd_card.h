#ifndef _H_SD_CARD_H
#define _H_SD_CARD_H

#include <cstdio>
#include <inttypes.h>

#include "spi_device.h"
#include "device.h"

class SdCard : public Device, public SpiDevice {
public:
    SdCard(const char *backing_file_name, unsigned capacity);
    void reset();
    virtual sim_time_t nextEventTime();
    void spiSlaveSelect(bool select);
    bool spiReceiveData(uint8_t &data);
private:
    FILE *backing_file;
    unsigned capacity;

    bool spi_selected;
    
    bool in_sd_mode;
    bool idle;
    bool receiving_command;
    bool expecting_acmd;
    bool responding;
    bool responding_with_data;
    int substate;
    uint16_t flags;
    
    uint8_t cmd_buffer[6];
    uint8_t response[5];
    int response_length;
    uint8_t read_block_buffer[512];
    uint16_t read_block_crc;
    
    bool crc_enabled;
    uint16_t block_size;
    
    uint8_t _handleSpiData(uint8_t data);
    void _execCommand(uint8_t command, uint32_t param);
    void _execAppCommand(uint8_t command, uint32_t param);
    void _prepareR1Response();
    void _readBlockFromBackingFile(uint8_t *buffer, int offset, int length);
};

#endif

