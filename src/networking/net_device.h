#ifndef _H_NET_DEVICE_H
#define _H_NET_DEVICE_H

#include <mutex>
#include <deque>
#include <string>

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
    deque<string> receive_buffer;
    
    string getPendingFrame(void);
    void sendFrame(const string& data);
    
    virtual void receiveFrame(const uint8_t *frame_data, int frame_len);
};

#endif

