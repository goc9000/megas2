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

#include "utils/fail.h"


VirtualNetwork::VirtualNetwork(ipv4_addr_t ipv4_address)
    : Entity("virtual_net", "Virtual Network")
{
    this->interface_name = DEFAULT_VNET_NAME;
    this->ipv4_address = ipv4_address;
}

VirtualNetwork::VirtualNetwork(string interface_name, ipv4_addr_t ipv4_address)
    : Entity("virtual_net", "Virtual Network")
{
    this->interface_name = interface_name;
    this->ipv4_address = ipv4_address;
    
    this->init();
}

VirtualNetwork::VirtualNetwork(Json::Value &json_data, EntityLookup *lookup)
    : Entity(json_data)
{
    this->interface_name = DEFAULT_VNET_NAME;
    
    if (json_data.isMember("interface_name"))
        this->interface_name = json_data["interface_name"].asString();
    
    if (json_data.isMember("ipv4_address")) {
        this->ipv4_address = ipv4_addr_t(json_data["ipv4_address"]);
    } else
        fail("Missing 'ipv4_address' parameter for VirtualNetwork object");
    
    this->init();
    
    if (json_data.isMember("devices")) {
        if (!json_data["devices"].isArray()) {
            fail("'devices' should be an array");
        }

        for (Json::ValueIterator it = json_data["devices"].begin(); it != json_data["devices"].end(); it++) {
            const char *dev_id = (*it).asCString();
            Entity *ent = lookup->lookupEntity(dev_id);
            
            if (!ent) {
                fail("Device '%s' not defined at this point", dev_id);
            }

            NetworkDevice *as_net_dev = dynamic_cast<NetworkDevice *>(ent);
            if (as_net_dev == NULL) {
                fail("Device '%s' is not a network device", dev_id);
            }

            this->addDevice(as_net_dev);
        }
    }
}

void VirtualNetwork::addDevice(NetworkDevice *device)
{
    lock_guard<recursive_mutex> guard(this->lock);
    
    if (find(this->devices.begin(), this->devices.end(), device) != this->devices.end())
        return;
    
    this->devices.push_back(device);
    device->connectToNetwork(this);
}

void VirtualNetwork::removeDevice(NetworkDevice *device)
{
    lock_guard<recursive_mutex> guard(this->lock);
    
    vector<NetworkDevice*>::iterator it = find(this->devices.begin(), this->devices.end(), device);
    if (it != this->devices.end()) {
        this->devices.erase(it);
        device->disconnectFromNetwork();
    }
}

void VirtualNetwork::init(void)
{
    this->interface_fd = open("/dev/net/tun", O_RDWR);
    if (this->interface_fd < 0)
        fail("Could not open /dev/net/tun. Try running with superuser rights.");
 
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strcpy(ifr.ifr_name, this->interface_name.c_str());

    if (ioctl(this->interface_fd, TUNSETIFF, &ifr) != 0)
        fail("Could not configure virtual Ethernet interface. Try running with root rights.");
    
    this->interface_name = ifr.ifr_name;
    
    this->setInterfaceIpv4(this->ipv4_address);
    this->ifupdown(true);
    
    this->receive_frames_thread = thread(&VirtualNetwork::receiveFramesThreadCode, this);
    
    info("Virtual network '%s' accessible as %s at %s", this->id.c_str(),
        ifr.ifr_name, this->ipv4_address.toString().c_str());
}

void VirtualNetwork::receiveFramesThreadCode(void)
{
    uint8_t buffer[65536];
    
    while (true) {
        int count = read(this->interface_fd, buffer, sizeof(buffer));
        if (count <= 0)
            break;
    
        try {
            this->lock.lock();
            
            for (auto& device : this->devices) {
                device->receiveFrame(buffer, count);
            }
        } catch (exception& e) {
            this->lock.unlock();
            throw e;
        }
    }
}

void VirtualNetwork::setInterfaceIpv4(ipv4_addr_t address)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, this->interface_name.c_str());
    
    struct sockaddr_in addr = address.toSockAddr();
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0)
        fail("Cannot set IP address for interface %s", this->interface_name.c_str());
}

void VirtualNetwork::ifupdown(bool bring_up)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, this->interface_name.c_str());
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);    
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
        fail("Cannot read flags for interface %s", this->interface_name.c_str());

    if (((bool)(ifr.ifr_flags & IFF_UP)) != bring_up) {
        ifr.ifr_flags = (ifr.ifr_flags & ~IFF_UP) | (IFF_UP * bring_up);
        
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
            fail("Cannot bring %s interface %s", bring_up ? "up" : "down",
                this->interface_name.c_str());
    }
}
