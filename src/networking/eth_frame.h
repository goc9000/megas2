#ifndef _H_ETHERNET_FRAME_H
#define _H_ETHERNET_FRAME_H

#include <string>

#include "utils/net_utils.h"

using namespace std;


class EthernetFrame {
public:
    mac_addr_t dest_mac;
    mac_addr_t src_mac;
    uint16_t ethertype;
    string payload;
    bool has_fcs;
    uint32_t fcs;
    
    bool is_malformed;
    bool is_null;
    
    EthernetFrame(void);
    EthernetFrame(const string& data, bool has_fcs);
    EthernetFrame(const void *data, int data_len, bool has_fcs);
    
    int totalLength(void) const;
    bool checkFcs(void) const;
    bool checkLengthValid(void) const;
    bool checkLengthCorrect(void) const;
    bool isMacControlFrame(void) const;
    bool isPauseFrame(void) const;
    bool isVlanFrame(void) const;
    
    uint32_t computeFcs(void) const;
    
    void addFcs(void);
    void padTo(int pad_length);
    
    string toBytes(void) const;
    int toBuffer(void *data) const;
protected:
};

#endif
