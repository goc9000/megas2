#ifndef _H_VIRTUAL_NET_H
#define _H_VIRTUAL_NET_H

#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>

#include <json/json.h>

#include "simulation/entity.h"
#include "simulation/entity_lookup.h"
#include "simulation/sim_device.h"
#include "networking/net_device.h"

#include "eth_frame.h"

#include "utils/net_utils.h"

using namespace std;

const string DEFAULT_VNET_NAME = "megasnet0";


class VirtualNetwork : public Entity, public SimulatedDevice
{
public:
    VirtualNetwork(void);
    VirtualNetwork(ipv4_addr_t ipv4_address);
    VirtualNetwork(string interface_name);
    VirtualNetwork(string interface_name, ipv4_addr_t ipv4_address);
    VirtualNetwork(Json::Value &json_data, EntityLookup *lookup);
    ~VirtualNetwork();
    
    void addDevice(NetworkDevice *device);
    void removeDevice(NetworkDevice *device);
    
    void sendFrame(const EthernetFrame& frame);
    
    void reset();
    void act(int event);
protected:
    string interface_name;
    int interface_fd;
    ipv4_addr_t ipv4_address;
    
    thread receive_frames_thread;
    int shutdown_fds[2];
    recursive_mutex lock;
    
    vector<NetworkDevice *> devices;
    deque<EthernetFrame> pending_frames;

    void init(void);
    void setInterfaceIpv4(ipv4_addr_t address);
    void ifupdown(bool bring_up);
    void receiveFramesThreadCode(void);
};

#endif

