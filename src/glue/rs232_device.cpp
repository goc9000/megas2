#include <cstdlib>

#include "rs232_device.h"
#include "utils/fail.h"

using namespace std;

RS232Device::RS232Device()
{
    this->rs232_peer = NULL;
}

void RS232Device::connectToRS232Peer(RS232Device *peer)
{
    if (peer == this->rs232_peer)
        return;

    if (this->rs232_peer != NULL) {
        this->disconnectFromRS232Peer();
    }

    this->rs232_peer = peer;
    peer->connectToRS232Peer(this);
}

void RS232Device::disconnectFromRS232Peer(void)
{
    if (this->rs232_peer) {
        RS232Device *peer = this->rs232_peer;
        this->rs232_peer = NULL;
        peer->disconnectFromRS232Peer();
    }
}

void RS232Device::rs232Send(uint8_t data)
{
    if (!this->rs232_peer)
        fail("Attempted to send data via RS232 with no peer connected!");
        
    this->rs232_peer->onRS232Receive(data);
}
