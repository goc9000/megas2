#ifndef _H_NET_DEVICE_H
#define _H_NET_DEVICE_H

#include <mutex>
#include <deque>
#include <string>

#include "eth_frame.h"

#include <json/json.h>

using namespace std;


class VirtualNetwork;

class NetworkDevice {
    friend class VirtualNetwork;
public:
    NetworkDevice(void);
    NetworkDevice(VirtualNetwork *network);
    
    void connectToNetwork(VirtualNetwork *network);
    void disconnectFromNetwork(void);
protected:
    VirtualNetwork *network;
    
    mutex receive_lock;
    deque<EthernetFrame> receive_buffer;
    
    EthernetFrame getPendingFrame(void);
    void sendFrame(const EthernetFrame& frame);
    
    virtual void onReceiveFrame(const EthernetFrame& frame);
};

#endif

