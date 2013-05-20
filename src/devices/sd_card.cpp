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
    { "SS", PIN_MODE_INPUT, PIN_VAL_VCC }  // SLAVE_SELECT
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

#define DEFAULT_NAME "SD card"

SdCard::SdCard(const char *backing_file_name, unsigned capacity)
    : Entity(DEFAULT_NAME), PinDevice(SDCARD_PIN_COUNT, PIN_INIT_DATA)
{
    init(backing_file_name, capacity);
}

SdCard::SdCard(Json::Value &json_data)
    : Entity(DEFAULT_NAME, json_data), PinDevice(SDCARD_PIN_COUNT, PIN_INIT_DATA)
{
    string backing_file_name;
    unsigned capacity = 0;

    parseJsonParam(backing_file_name, json_data, "image");
    parseJsonParam(capacity, json_data, "capacity");

    init(backing_file_name.c_str(), capacity);
}

void SdCard::init(const char *backing_file_name, unsigned capacity)
{
    if (capacity & 511)
        fail("SD capacity must be a multiple of 512 bytes");
    this->capacity = capacity;
    
    backing_file = fopen(backing_file_name, "rb+");
    if (!backing_file)
        fail("Cannot open SD card backing file '%s'", backing_file_name);
    if (fseek(backing_file, 0, SEEK_END) < 0)
        fail("Cannot seek in SD card backing file '%s'", backing_file_name);
    backing_file_len = ftell(backing_file);
    
    spi_selected = false;
    reset();
}

void SdCard::reset()
{
    in_sd_mode = true;
    idle = true;
    crc_enabled = true;
    expecting_acmd = false;
    flags = 0;
    block_size = 512;
}

void SdCard::_onPinChanged(int pin_id, pin_val_t value, pin_val_t old_value)
{
    switch (pin_id) {
        case SDCARD_PIN_SLAVE_SELECT:
            _spiSlaveSelect(!_pins[pin_id].readDigital());
            return;
    }
}

bool SdCard::spiReceiveData(uint8_t &data)
{
    if (!spi_selected)
        return false;
    
    data = handleSpiData(data);
    
    return true;
}

void SdCard::_onSpiSlaveSelect(bool select)
{
    if (select) {
        idle = true;
    }
}

uint8_t SdCard::handleSpiData(uint8_t data)
{
    if (this->idle)
        return handleCommandInIdle(data);
    if (this->receiving_command)
        return handleReceivingCommand(data);    
    if (this->responding)
        return handleResponding(data);    
    if (this->responding_with_data)
        return handleRespondingWithData(data);
    if (this->receiving_write_data)
        return handleReceivingWriteData(data);
    
    fail("Received SD command in unexpected state");
    
    return 0xff;
}

uint8_t SdCard::handleCommandInIdle(uint8_t data)
{
    if (data == 0xff)
        return 0xff;
    if ((data & 0xc0) != 0x40)
        fail("Command sent to SD card does not begin with '01'");
    
    cmd_buffer[0] = data;
    substate = 1;
    idle = false;
    receiving_command = true;
    
    return 0xff;
}

uint8_t SdCard::handleReceivingCommand(uint8_t data)
{
    cmd_buffer[substate++] = data;
    if (substate < 6)
        return 0xff;
    
    receiving_command = false;
    
    if (!bit_is_set(cmd_buffer[5], 0))
        fail("SD command does not end in '1' bit");
    if (crc_enabled && (compute_crc7(cmd_buffer, 5) != (cmd_buffer[5] >> 1)))
        fail("SD command CRC failed");
    
    uint32_t param = (cmd_buffer[1] << 24) + (cmd_buffer[2] << 16) +
        (cmd_buffer[3] << 8) + cmd_buffer[4];
    
    if (expecting_acmd) {
        expecting_acmd = false;
        execAppCommand(cmd_buffer[0], param);
    } else {
        execCommand(cmd_buffer[0], param);
    }
    
    return 0xff;
}

uint8_t SdCard::handleResponding(uint8_t data)
{
    if (data != 0xff)
        fail("SD card received %02x while responding", data);
    
    data = response[substate++];
    
    if (substate == response_length) {
        responding = false;
        idle = !responding_with_data && !receiving_write_data;
        if (responding_with_data || receiving_write_data)
            substate = 0;
    }
    
    return data;
}

uint8_t SdCard::handleRespondingWithData(uint8_t data)
{
    if (data != 0xff)
        fail("SD card received %02x while responding with data", data);
    
    if (substate == 0) {
        data = 0xfe;
    } else if ((substate >= 1) && (substate <= block_size)) {
        data = read_block_buffer[substate-1];
    } else if (substate == block_size+1) {
        data = high_byte(read_block_crc);
    } else {
        data = low_byte(read_block_crc);
        responding_with_data = false;
        idle = true;
    }
    
    substate++;
    
    return data;
}

uint8_t SdCard::handleReceivingWriteData(uint8_t data)
{
    if (substate == 0) {
        if (data == 0xff)
            return 0xff;
        if (data == 0xfe) {
            substate = 1;
            return 0xff;
        }
        fail("SD card received %02x while expecting data for writing", data);
    } else if (substate <= block_size) {
        write_block_buffer[substate-1] = data;
        substate++;
    } else if (substate == block_size+1) {
        write_block_crc = data << 8;
        substate++;
    } else {
        write_block_crc += data;
        
        bool crc_error = crc_enabled &&
            (write_block_crc != compute_crc16(write_block_buffer, block_size));
        bool write_error = !writeBlockToBackingFile(
            write_block_buffer, write_block_addr, block_size);
        
        prepareDataResponse(crc_error, write_error);
        receiving_write_data = false;
    }
    
    return 0xff;
}

void SdCard::execCommand(uint8_t command, uint32_t param)
{
    if (in_sd_mode && (command != CMD_GO_IDLE_STATE))
        fail("SD card received command %02x in SD mode instead of GO_IDLE_STATE", command);
    
    responding_with_data = false;
    receiving_write_data = false;
    
    switch (command) {
        case CMD_GO_IDLE_STATE:
            if (in_sd_mode)
                crc_enabled = false;
            in_sd_mode = false;
            break;
        case CMD_APP_CMD:
            expecting_acmd = true;
            break;
        case CMD_SET_BLOCKLEN:
            if (param != 512)
                fail("Only 512 supported as block length for SD card (tried %d)", param);
            break;
        case CMD_READ_SINGLE_BLOCK:
            if (param & 511)
                fail("SD card read address not aligned (=%08x)", param);
            if (param + block_size >= capacity)
                fail("Out of range read from SD card (addr=%08x)", param);
            readBlockFromBackingFile(read_block_buffer, param, block_size);
            read_block_crc = compute_crc16(read_block_buffer, block_size);
            responding_with_data = true;
            break;
        case CMD_WRITE_SINGLE_BLOCK:
            if (param & 511)
                fail("SD card write address not aligned (=%08x)", param);
            if (param + block_size >= capacity)
                fail("Out of range write to SD card (addr=%08x)", param);
            write_block_addr = param;
            receiving_write_data = true;
            break;
        default:
            fail("Unrecognized SD command: %02x %08x\n", command, param);
            break;
    }
    
    prepareR1Response();
}

void SdCard::execAppCommand(uint8_t command, uint32_t param)
{
    switch (command) {
        case ACMD_SD_SEND_OP_CMD:
            break;
        default:
            fail("Unrecognized app-specific SD command: %02x %08x\n", command, param);
            break;
    }
    
    prepareR1Response();
}

void SdCard::prepareR1Response()
{
    response[0] = low_byte(flags);
    response_length = 1;
    substate = 0;
    responding = true;
    
    flags &= 0xff00;
}

void SdCard::prepareDataResponse(bool crc_error, bool write_error)
{
    if (crc_error)
        response[0] = 0x0b;
    else if (write_error)
        response[0] = 0x0d;
    else
        response[0] = 0x05;
    
    response_length = 1;
    substate = 0;
    responding = true;
}

void SdCard::readBlockFromBackingFile(uint8_t *buffer, unsigned offset, unsigned length)
{
    memset(buffer, 0xff, length);
    
    if (offset >= backing_file_len)
        return;
    
    fseek(backing_file, offset, SEEK_SET);
    int count = fread(buffer, 1, length, backing_file);
    
    if (count < 0)
        fail("Error reading from SD card backing file");
}

bool SdCard::writeBlockToBackingFile(uint8_t *buffer, unsigned offset, unsigned length)
{
    expandBackingFile(offset + length);
    
    fseek(backing_file, offset, SEEK_SET);
    if (fwrite(buffer, length, 1, backing_file) != 1)
        fail("Error writing to SD card backing file");
    
    return true;
}

void SdCard::expandBackingFile(unsigned minimum_size)
{
    uint8_t buf[65536];
    
    memset(buf, 0xff, 65536);
    
    while (backing_file_len < minimum_size) {
        unsigned count = min(65536U, minimum_size - backing_file_len);
        
        fseek(backing_file, 0, SEEK_END);
        if (fwrite(buf, count, 1, backing_file) != 1)
            fail("Error expanding SD card backing file");
        backing_file_len += count;
    }
}
