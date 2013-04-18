#include <cstring>
#include <algorithm>

#include <arpa/inet.h>

#include "bit_macros.h"
#include "fail.h"
#include "net_utils.h"

string mac_addr_t::toString(void) const
{
    char buf[18];

    for (int i = 0; i < 6; i++)	
        sprintf(buf + i * 3, "%02x:", this->octets[i]);
    
    buf[17] = 0;
    
    return string(buf);
}

mac_addr_t::mac_addr_t(void)
{
}

mac_addr_t::mac_addr_t(const void *octets, int addr_size)
{
    if (addr_size != 6)
        fail("Only 48-bit MAC addresses are supported");
    
    memcpy(this->octets, octets, addr_size);
}

bool mac_addr_t::isNull(void) const
{
    return all_of(this->octets, this->octets + 6, [](uint8_t x) { return !x; });
}

bool mac_addr_t::isLocal(void) const
{
    return bit_is_set(this->octets[0], 1);
}

bool mac_addr_t::isMulticast(void) const
{
    return bit_is_set(this->octets[0], 0) && !this->isBroadcast();
}

bool mac_addr_t::isBroadcast(void) const
{
    return all_of(this->octets, this->octets + 6, [](uint8_t x) { return x == 0xff; });
}

ostream& operator << (std::ostream& os, const mac_addr_t& addr) 
{
    os << addr.toString();

    return os;
}

ipv4_addr_t::ipv4_addr_t(void)
{
}

ipv4_addr_t::ipv4_addr_t(Json::Value& json_data)
{
    if (!json_data.isString())
        fail("IPv4 address must be given as a string");
    
    const char *addr_text = json_data.asCString();
    if (!inet_pton(AF_INET, addr_text, this->octets))
        fail("Invalid IPv4 address '%s'", addr_text);
}

string ipv4_addr_t::toString(void) const
{
    char buf[18];

    buf[sprintf(buf, "%u.%u.%u.%u", this->octets[0], this->octets[1],
        this->octets[2], this->octets[3])] = 0;
    
    return string(buf);
}

struct sockaddr_in ipv4_addr_t::toSockAddr(void) const
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, this->octets, 4);
    
    return addr;
}

ostream& operator << (std::ostream& os, const ipv4_addr_t& addr) 
{
    os << addr.toString();

    return os;
}

static inline void crc32_feed(uint32_t &crc, unsigned char byte)
{
    crc ^= byte;
    
    for (int i = 0; i < 8; i++)
        if (crc & 1)
            crc = (crc >> 1) ^ 0xedb88320UL;
        else
            crc >>= 1;
}

uint32_t compute_fcs(const void *data, int length)
{
    uint32_t crc = 0xffffffffUL;
    
    for (int i = 0; i < length; i++)
        crc32_feed(crc, ((const char *)data)[i]);
    
    crc = ~crc;
    crc = (((crc >> 24) & 0xff) << 0) + 
          (((crc >> 16) & 0xff) << 8) + 
          (((crc >> 8) & 0xff) << 16) + 
          (((crc >> 0) & 0xff) << 24);
    
    return crc;
}

uint32_t compute_fcs(string& data)
{
    return compute_fcs(data.c_str(), data.length());
}
