#include <cstring>

#include "eth_frame.h"

#include "utils/bit_macros.h"
#include "utils/fail.h"

// EtherTypes
#define ETHERTYPE_VLAN                    0x8100
#define ETHERTYPE_MAC_CONTROL             0x8808


EthernetFrame::EthernetFrame(void)
{
    this->is_null = true;
}

EthernetFrame::EthernetFrame(const string& data, bool has_fcs)
    : EthernetFrame((uint8_t *)data.c_str(), data.length(), has_fcs)
{
}

EthernetFrame::EthernetFrame(const void *buffer, int data_len, bool has_fcs)
{
    const uint8_t *data = (uint8_t *)buffer;
    
    this->is_null = false;
    
    int min_size = 14 + 4 * has_fcs;
    
    if (data_len < min_size) {
        this->is_malformed = true;
        return;
    }
    
    this->dest_mac = mac_addr_t(data, 6);
    this->src_mac = mac_addr_t(data + 6, 6);
    this->ethertype = (data[12] << 8) + data[13];
    
    this->payload = string((const char *)(data + 14), data_len - min_size);
    
    this->has_fcs = has_fcs;
    if (has_fcs) {
        this->fcs = 0;
        for (int i = 0; i < 4; i++)
            this->fcs = (this->fcs << 8) + data[data_len - 4 + i];
    }
}
    
int EthernetFrame::totalLength(void) const
{
    return this->payload.length() + 18;
}

bool EthernetFrame::checkLengthValid(void) const
{
    return (this->ethertype <= 1500) || (this->ethertype >= 1536);
}

bool EthernetFrame::checkLengthCorrect(void) const
{
    return (this->ethertype <= 1500) && (this->ethertype == this->payload.length());
}

bool EthernetFrame::isMacControlFrame(void) const
{
    return (this->ethertype == ETHERTYPE_MAC_CONTROL);
}

bool EthernetFrame::isPauseFrame(void) const
{
    return (this->ethertype == ETHERTYPE_MAC_CONTROL) &&
        (this->payload.length() >= 2) &&
        (this->payload[0] == 0x00) && (this->payload[1] == 0x01);
}

bool EthernetFrame::isVlanFrame(void) const
{
    return (this->ethertype == ETHERTYPE_VLAN);
}

bool EthernetFrame::checkFcs(void) const
{
    if (!this->has_fcs)
        fail("Attempted to check FCS in frame that does not have one");
    
    return (this->computeFcs() == this->fcs);
}

string EthernetFrame::toBytes(void) const
{
    char data[65536];
    int data_len = this->toBuffer(data);
    
    return string(data, data_len);
}

int EthernetFrame::toBuffer(void *buffer) const
{
    uint8_t *data = (uint8_t *)buffer;
    int data_len = 0;
    
    for (int i = 0; i < 6; i++)
        data[data_len++] = this->dest_mac.octets[i];
    for (int i = 0; i < 6; i++)
        data[data_len++] = this->src_mac.octets[i];
    
    data[data_len++] = high_byte(this->ethertype);
    data[data_len++] = low_byte(this->ethertype);
    
    memcpy(data + data_len, this->payload.c_str(), this->payload.length());
    data_len += this->payload.length();
    
    if (this->has_fcs) {
        data[data_len++] = (this->fcs >> 24) & 0xff;
        data[data_len++] = (this->fcs >> 16) & 0xff;
        data[data_len++] = (this->fcs >> 8) & 0xff;
        data[data_len++] = this->fcs & 0xff;
    }
    
    return data_len;
}

static inline void crc32_feed(uint32_t &crc, uint8_t byte)
{
    crc ^= byte;
    
    for (int i = 0; i < 8; i++)
        if (crc & 1)
            crc = (crc >> 1) ^ 0xedb88320UL;
        else
            crc >>= 1;
}

uint32_t EthernetFrame::computeFcs(void) const
{
    uint8_t data[65536];
    int data_len = this->toBuffer(data) - (this->has_fcs ? 4 : 0);
    
    uint32_t crc = 0xffffffffUL;
    
    for (int i = 0; i < data_len; i++)
        crc32_feed(crc, data[i]);
    
    crc = ~crc;
    return (((crc >> 24) & 0xff) << 0) + 
          (((crc >> 16) & 0xff) << 8) + 
          (((crc >> 8) & 0xff) << 16) + 
          (((crc >> 0) & 0xff) << 24);
}

void EthernetFrame::addFcs(void)
{
    if (this->has_fcs)
        fail("Attempted to add FCS in frame that already has one");

    this->fcs = this->computeFcs();
    this->has_fcs = true;
}

void EthernetFrame::padTo(int pad_length)
{
    int extra = pad_length - this->totalLength();
    
    if (extra <= 0)
        return;
    
    this->payload += string(extra, 0);
}
