#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdlib>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "sd_card.h"

using namespace std;

#define CMD_GO_IDLE_STATE          0x40
#define CMD_SET_BLOCKLEN           0x50
#define CMD_READ_SINGLE_BLOCK      0x51
#define CMD_WRITE_SINGLE_BLOCK     0x58
#define CMD_APP_CMD                0x77

#define ACMD_SD_SEND_OP_CMD        0x69

#define FLAG_OUT_OF_RANGE            15
#define FLAG_CSD_OVERWRITE           15
#define FLAG_ERASE_PARAM             14
#define FLAG_WP_VIOLATION            13
#define FLAG_CARD_ECC_FAILED         12
#define FLAG_CC_ERROR                11
#define FLAG_ERROR                   10
#define FLAG_LOCK_CMD_FAILED          9
#define FLAG_WP_ERASE_SKIP            9
#define FLAG_CARD_IS_LOCKED           8
#define FLAG_PARAMETER_ERROR          6
#define FLAG_ADDRESS_ERROR            5
#define FLAG_ERASE_SEQ_ERROR          4
#define FLAG_COMMAND_CRC_ERROR        3
#define FLAG_ILLEGAL_COMMAND          2
#define FLAG_ERASE_RESET              1
#define FLAG_IN_IDLE_STATE            0

// Pin initialization data

PinInitData const PIN_INIT_DATA[SDCARD_PIN_COUNT] = {
    { PIN_MODE_INPUT, 1 }  // SLAVE_SELECT
};

uint8_t compute_crc7(uint8_t *buffer, int length)
{
    uint8_t crc = 0;
    for (int i = 0; i < length; i++) {
        uint8_t data = buffer[i];
        for (int j = 0; j < 8; j++) {
            crc <<= 1;
            if (bit_is_set(data ^ crc, 7))
                crc ^=0x09;
            data <<= 1;
        }
    }
    
    return crc;
}

uint16_t compute_crc16(uint8_t *buffer, int length)
{
    uint16_t crc = 0;
    
    for (int i = 0; i < length; i++) {
        uint8_t data = buffer[i];
        crc = crc ^ (data << 8);
        for (int j = 0; j < 8; j++) {
            crc = (crc << 1) ^ (bit_is_set(crc, 15) ? 0x1021 : 0);
        }
    }
    
    return crc;
}

SdCard::SdCard(const char *backing_file_name, unsigned capacity) : PinDevice(SDCARD_PIN_COUNT, PIN_INIT_DATA)
{
    if (capacity & 511)
        fail("SD capacity must be a multiple of 512 bytes");
    this->capacity = capacity;
    
    this->backing_file = fopen(backing_file_name, "rb+");
    if (!this->backing_file)
        fail("Cannot open SD card backing file '%s'", backing_file_name);
    if (fseek(this->backing_file, 0, SEEK_END) < 0)
        fail("Cannot seek in SD card backing file '%s'", backing_file_name);
    this->backing_file_len = ftell(this->backing_file);
    
    this->spi_selected = false;
    this->reset();
}

void SdCard::act()
{
    // SD cards do nothing on their own
}

sim_time_t SdCard::nextEventTime()
{
    return SIM_TIME_NEVER;
}

void SdCard::reset()
{
    this->in_sd_mode = true;
    this->idle = true;
    this->crc_enabled = true;
    this->expecting_acmd = false;
    this->flags = 0;
    this->block_size = 512;
}

void SdCard::_onPinChanged(int pin_id, int value, int old_value)
{
    switch (pin_id) {
        case SDCARD_PIN_SLAVE_SELECT:
            this->_spiSlaveSelect(!value);
            return;
    }
}

bool SdCard::spiReceiveData(uint8_t &data)
{
    if (!this->spi_selected)
        return false;
    
    data = _handleSpiData(data);
    
    return true;
}

void SdCard::_onSpiSlaveSelect(bool select)
{
    if (select) {
        this->idle = true;
    }
}

uint8_t SdCard::_handleSpiData(uint8_t data)
{
    if (this->idle)
        return this->_handleCommandInIdle(data);
    if (this->receiving_command)
        return this->_handleReceivingCommand(data);    
    if (this->responding)
        return this->_handleResponding(data);    
    if (this->responding_with_data)
        return this->_handleRespondingWithData(data);
    if (this->receiving_write_data)
        return this->_handleReceivingWriteData(data);
    
    fail("Received SD command in unexpected state");
    
    return 0xff;
}

uint8_t SdCard::_handleCommandInIdle(uint8_t data)
{
    if (data == 0xff)
        return 0xff;
    if ((data & 0xc0) != 0x40)
        fail("Command sent to SD card does not begin with '01'");
    
    this->cmd_buffer[0] = data;
    this->substate = 1;
    this->idle = false;
    this->receiving_command = true;
    
    return 0xff;
}

uint8_t SdCard::_handleReceivingCommand(uint8_t data)
{
    this->cmd_buffer[this->substate++] = data;
    if (this->substate < 6)
        return 0xff;
    
    this->receiving_command = false;
    
    if (!bit_is_set(this->cmd_buffer[5], 0))
        fail("SD command does not end in '1' bit");
    if (this->crc_enabled && (compute_crc7(this->cmd_buffer, 5) != (this->cmd_buffer[5] >> 1)))
        fail("SD command CRC failed");
    
    uint32_t param = (this->cmd_buffer[1] << 24) +
        (this->cmd_buffer[2] << 16) +
        (this->cmd_buffer[3] << 8) +
        (this->cmd_buffer[4]);
    
    if (this->expecting_acmd) {
        this->expecting_acmd = false;
        this->_execAppCommand(this->cmd_buffer[0], param);
    } else {
        this->_execCommand(this->cmd_buffer[0], param);
    }
    
    return 0xff;
}

uint8_t SdCard::_handleResponding(uint8_t data)
{
    if (data != 0xff)
        fail("SD card received %02x while responding", data);
    
    data = this->response[this->substate++];
    
    if (this->substate == this->response_length) {
        this->responding = false;
        this->idle = !this->responding_with_data && !this->receiving_write_data;
        if (this->responding_with_data || this->receiving_write_data)
            this->substate = 0;
    }
    
    return data;
}

uint8_t SdCard::_handleRespondingWithData(uint8_t data)
{
    if (data != 0xff)
        fail("SD card received %02x while responding with data", data);
    
    if (this->substate == 0) {
        data = 0xfe;
    } else if ((this->substate >= 1) && (this->substate <= this->block_size)) {
        data = this->read_block_buffer[this->substate-1];
    } else if (this->substate == this->block_size+1) {
        data = high_byte(this->read_block_crc);
    } else {
        data = low_byte(this->read_block_crc);
        this->responding_with_data = false;
        this->idle = true;
    }
    
    this->substate++;
    
    return data;
}

uint8_t SdCard::_handleReceivingWriteData(uint8_t data)
{
    if (this->substate == 0) {
        if (data == 0xff)
            return 0xff;
        if (data == 0xfe) {
            this->substate = 1;
            return 0xff;
        }
        fail("SD card received %02x while expecting data for writing", data);
    } else if (this->substate <= this->block_size) {
        this->write_block_buffer[this->substate-1] = data;
        this->substate++;
    } else if (this->substate == this->block_size+1) {
        this->write_block_crc = data << 8;
        this->substate++;
    } else {
        this->write_block_crc += data;
        
        bool crc_error = this->crc_enabled &&
            (this->write_block_crc != compute_crc16(this->write_block_buffer, this->block_size));
        bool write_error = !this->_writeBlockToBackingFile(
            this->write_block_buffer, this->write_block_addr, this->block_size);
        
        this->_prepareDataResponse(crc_error, write_error);
        this->receiving_write_data = false;
    }
    
    return 0xff;
}

void SdCard::_execCommand(uint8_t command, uint32_t param)
{
    if (this->in_sd_mode && (command != CMD_GO_IDLE_STATE))
        fail("SD card received command %02x in SD mode instead of GO_IDLE_STATE", command);
    
    this->responding_with_data = false;
    this->receiving_write_data = false;
    
    switch (command) {
        case CMD_GO_IDLE_STATE:
            if (this->in_sd_mode)
                this->crc_enabled = false;
            this->in_sd_mode = false;
            break;
        case CMD_APP_CMD:
            this->expecting_acmd = true;
            break;
        case CMD_SET_BLOCKLEN:
            if (param != 512)
                fail("Only 512 supported as block length for SD card (tried %d)", param);
            break;
        case CMD_READ_SINGLE_BLOCK:
            if (param & 511)
                fail("SD card read address not aligned (=%08x)", param);
            if (param + this->block_size >= capacity)
                fail("Out of range read from SD card (addr=%08x)", param);
            this->_readBlockFromBackingFile(this->read_block_buffer, param, this->block_size);
            this->read_block_crc = compute_crc16(this->read_block_buffer, this->block_size);
            this->responding_with_data = true;
            break;
        case CMD_WRITE_SINGLE_BLOCK:
            if (param & 511)
                fail("SD card write address not aligned (=%08x)", param);
            if (param + this->block_size >= capacity)
                fail("Out of range write to SD card (addr=%08x)", param);
            this->write_block_addr = param;
            this->receiving_write_data = true;
            break;
        default:
            fail("Unrecognized SD command: %02x %08x\n", command, param);
            break;
    }
    
    this->_prepareR1Response();
}

void SdCard::_execAppCommand(uint8_t command, uint32_t param)
{
    switch (command) {
        case ACMD_SD_SEND_OP_CMD:
            break;
        default:
            fail("Unrecognized app-specific SD command: %02x %08x\n", command, param);
            break;
    }
    
    this->_prepareR1Response();
}

void SdCard::_prepareR1Response()
{
    this->response[0] = low_byte(this->flags);
    this->response_length = 1;
    this->substate = 0;
    this->responding = true;
    
    this->flags &= 0xff00;
}

void SdCard::_prepareDataResponse(bool crc_error, bool write_error)
{
    if (crc_error)
        this->response[0] = 0x0b;
    else if (write_error)
        this->response[0] = 0x0d;
    else
        this->response[0] = 0x05;
    
    this->response_length = 1;
    this->substate = 0;
    this->responding = true;
}

void SdCard::_readBlockFromBackingFile(uint8_t *buffer, unsigned offset, unsigned length)
{
    memset(buffer, 0xff, length);
    
    if (offset >= this->backing_file_len)
        return;
    
    fseek(this->backing_file, offset, SEEK_SET);
    int count = fread(buffer, 1, length, this->backing_file);
    
    if (count < 0)
        fail("Error reading from SD card backing file");
}

bool SdCard::_writeBlockToBackingFile(uint8_t *buffer, unsigned offset, unsigned length)
{
    this->_expandBackingFile(offset+length);
    
    fseek(this->backing_file, offset, SEEK_SET);
    if (fwrite(buffer, length, 1, this->backing_file) != 1)
        fail("Error writing to SD card backing file");
    
    return true;
}

void SdCard::_expandBackingFile(unsigned minimum_size)
{
    uint8_t buf[65536];
    
    memset(buf, 0xff, 65536);
    
    while (this->backing_file_len < minimum_size) {
        unsigned count = min(65536U, minimum_size - this->backing_file_len);
        
        fseek(this->backing_file, 0, SEEK_END);
        if (fwrite(buf, count, 1, this->backing_file) != 1)
            fail("Error expanding SD card backing file");
        this->backing_file_len += count;
    }
}
