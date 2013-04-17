#ifndef _H_NET_UTILS_H
#define _H_NET_UTILS_H

#include <iostream>

#include <netinet/in.h>

#include <json/json.h>

using namespace std;

struct mac_addr_t {
    uint8_t octets[6];
    
    mac_addr_t(void);
    mac_addr_t(const void *octets, int addr_size);

    bool isNull(void) const;
    bool isLocal(void) const;
    bool isMulticast(void) const;
    bool isBroadcast(void) const;
    
    string toString(void) const;
} __attribute__((packed));

struct ipv4_addr_t {
    uint8_t octets[4];
    
    ipv4_addr_t(void);
    ipv4_addr_t(Json::Value& json_data);
    
    string toString(void) const;
    struct sockaddr_in toSockAddr(void) const;
} __attribute__((packed));

ostream& operator << (std::ostream& os, const mac_addr_t& addr);
ostream& operator << (std::ostream& os, const ipv4_addr_t& addr);

#endif
