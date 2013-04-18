#include "net_device.h"

#include "virtual_net.h"

NetworkDevice::NetworkDevice()
{
    this->network = NULL;
}

NetworkDevice::NetworkDevice(VirtualNetwork *network)
{
    this->network = NULL;
    this->connectToNetwork(network);
}
    
void NetworkDevice::connectToNetwork(VirtualNetwork *network)
{
    if (network == this->network)
        return;

    if (this->network != NULL) {
        this->disconnectFromNetwork();
    }

    this->network = network;
    network->addDevice(this);
}

void NetworkDevice::disconnectFromNetwork(void)
{
    if (this->network) {
        VirtualNetwork *network = this->network;
        this->network = NULL;
        network->removeDevice(this);
    }
}

EthernetFrame NetworkDevice::getPendingFrame(void)
{
    lock_guard<mutex> guard(this->receive_lock);
    
    if (this->receive_buffer.size()) {
        EthernetFrame frame = this->receive_buffer.front();
        this->receive_buffer.pop_front();
        
        return frame;
    } else
        return EthernetFrame();
}

void NetworkDevice::sendFrame(const EthernetFrame& frame)
{
    if (this->network)
        this->network->sendFrame(frame);
}

void NetworkDevice::receiveFrame(const EthernetFrame& frame)
{
    lock_guard<mutex> guard(this->receive_lock);
    
    this->receive_buffer.push_back(frame);
}
