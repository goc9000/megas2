#include <cstring>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <resolv.h>

#include <chrono>

#include "virtual_net.h"

#include "utils/cpp_macros.h"
#include "utils/fail.h"


#define SIM_EVENT_CHECK_FRAMES         1

const sim_time_t DEFAULT_CHECK_FRAMES_INTERVAL = ms_to_sim_time(10);


#define DEFAULT_NAME "Virtual network"

VirtualNetwork::VirtualNetwork(void)
    : VirtualNetwork(DEFAULT_VNET_NAME, ipv4_addr_t())
{
}

VirtualNetwork::VirtualNetwork(string interface_name)
    : VirtualNetwork(interface_name, ipv4_addr_t())
{
}

VirtualNetwork::VirtualNetwork(ipv4_addr_t ipv4_address)
    : VirtualNetwork(DEFAULT_VNET_NAME, ipv4_address)
{
}

VirtualNetwork::VirtualNetwork(string interface_name, ipv4_addr_t ipv4_address)
    : Entity(DEFAULT_NAME)
{
    this->interface_name = interface_name;
    this->ipv4_address = ipv4_address;
    
    init();
}

VirtualNetwork::VirtualNetwork(Json::Value &json_data, EntityLookup *lookup)
    : Entity(DEFAULT_NAME, json_data)
{
    interface_name = DEFAULT_VNET_NAME;
    
    parseOptionalJsonParam(interface_name, json_data, "interface_name");
    parseOptionalJsonParam(ipv4_address, json_data, "ipv4_address");
    
    init();
    
    if (json_data.isMember("devices")) {
        if (!json_data["devices"].isArray()) {
            fail("'devices' should be an array");
        }

        for (auto& it : json_data["devices"]) {
            const char *dev_id = it.asCString();
            Entity *ent = lookup->lookupEntity(dev_id);
            
            if (!ent) {
                fail("Device '%s' not defined at this point", dev_id);
            }

            NetworkDevice *as_net_dev = dynamic_cast<NetworkDevice *>(ent);
            if (as_net_dev == NULL) {
                fail("Device '%s' is not a network device", dev_id);
            }

            addDevice(as_net_dev);
        }
    }
}

void VirtualNetwork::addDevice(NetworkDevice *device)
{
    if (CONTAINS(devices, device))
        return;
    
    devices.push_back(device);
    device->connectToNetwork(this);
}

void VirtualNetwork::removeDevice(NetworkDevice *device)
{
    if (CONTAINS(devices, device)) {
        this->devices.erase(FIND(devices, device));
        device->disconnectFromNetwork();
    }
}

void VirtualNetwork::init(void)
{
    interface_fd = open("/dev/net/tun", O_RDWR);
    if (interface_fd < 0)
        fail("Could not open /dev/net/tun. Try running with superuser rights.");
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strcpy(ifr.ifr_name, interface_name.c_str());

    if (ioctl(interface_fd, TUNSETIFF, &ifr) != 0)
        fail("Could not configure virtual Ethernet interface. Try running with root rights.");
    
    interface_name = ifr.ifr_name;
    
    if (!ipv4_address.isNull()) {
        setInterfaceIpv4(ipv4_address);
        ifupdown(true);
    }
    
    receive_frames_thread = thread(&VirtualNetwork::receiveFramesThreadCode, this);
    
    info("Virtual network '%s' accessible as %s", id.c_str(), ifr.ifr_name);
}

void VirtualNetwork::receiveFramesThreadCode(void)
{
    uint8_t buffer[65536];
    
    while (true) {
        int count = read(interface_fd, buffer, sizeof(buffer));
        if (count <= 0)
            break;
    
        EthernetFrame frame = EthernetFrame(buffer, count, false);
        frame.padTo(64);
        frame.addFcs();
    
        lock.lock();
        pending_frames.push_back(frame);
        lock.unlock();
    }
}

void VirtualNetwork::reset(void)
{
    unscheduleAll();
    scheduleEventIn(SIM_EVENT_CHECK_FRAMES, DEFAULT_CHECK_FRAMES_INTERVAL);
}

void VirtualNetwork::act(int event)
{
    EthernetFrame frame;
    
    if (event == SIM_EVENT_CHECK_FRAMES) {
        while (true) {
            lock.lock();
            if (pending_frames.empty()) {
                lock.unlock();
                break;
            }
            frame = pending_frames.front();
            pending_frames.pop_front();
            lock.unlock();
        
            for (auto& device : devices)
                device->onReceiveFrame(frame);
        }
        
        scheduleEventIn(SIM_EVENT_CHECK_FRAMES, DEFAULT_CHECK_FRAMES_INTERVAL);
    }
}

void VirtualNetwork::sendFrame(const EthernetFrame& frame)
{
    char data[65536];
    unsigned int data_len = frame.toBuffer(data);
    
    unsigned int ptr = 0;
    int count;
    
    while (ptr < data_len) {
        count = write(interface_fd, data + ptr, data_len - ptr);
        if (count <= 0)
            break;
        
        ptr += count;
    }
}

void VirtualNetwork::setInterfaceIpv4(ipv4_addr_t address)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface_name.c_str());
    
    struct sockaddr_in addr = address.toSockAddr();
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0)
        fail("Cannot set IP address for interface %s", interface_name.c_str());
}

void VirtualNetwork::ifupdown(bool bring_up)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface_name.c_str());
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);    
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
        fail("Cannot read flags for interface %s", interface_name.c_str());

    if (((bool)(ifr.ifr_flags & IFF_UP)) != bring_up) {
        ifr.ifr_flags = (ifr.ifr_flags & ~IFF_UP) | (IFF_UP * bring_up);
        
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
            fail("Cannot bring %s interface %s", bring_up ? "up" : "down",
                interface_name.c_str());
    }
}
