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

string NetworkDevice::getPendingFrame(void)
{
    lock_guard<mutex> guard(this->receive_lock);
    
    if (this->receive_buffer.size()) {
        string frame = this->receive_buffer.front();
        this->receive_buffer.pop_front();
        
        return frame;
    } else
        return string();
}

void NetworkDevice::sendFrame(const string& data)
{
    if (this->network)
        this->network->sendFrame(data);
}

void NetworkDevice::receiveFrame(const uint8_t *frame_data, int frame_len)
{
    lock_guard<mutex> guard(this->receive_lock);
    
    this->receive_buffer.push_back(string((char *)frame_data, frame_len));
}
