#include <cstdlib>

#include "rs232_device.h"
#include "utils/fail.h"

using namespace std;

RS232Device::RS232Device(void)
{
    this->rs232_peer = NULL;
}

RS232Device::RS232Device(Json::Value &json_data, EntityLookup *lookup)
{
    this->rs232_peer = NULL;
    
    if (json_data.isMember("rs232_peer")) {
        const char *dev_id = json_data["rs232_peer"].asCString();
        
        Entity *ent = lookup->lookupEntity(dev_id);
        if (!ent) {
            fail("Device '%s' not defined at this point", dev_id);
        }

        RS232Device* peer = dynamic_cast<RS232Device *>(ent);
        if (peer == NULL) {
            fail("Device '%s' is not an RS232 device", dev_id);
        }
        
        connectToRS232Peer(peer);
    }
}

void RS232Device::connectToRS232Peer(RS232Device *peer)
{
    if (peer == rs232_peer)
        return;

    if (rs232_peer != NULL)
        this->disconnectFromRS232Peer();

    rs232_peer = peer;
    
    peer->connectToRS232Peer(this);
}

void RS232Device::disconnectFromRS232Peer(void)
{
    if (rs232_peer) {
        RS232Device *peer = rs232_peer;
        rs232_peer = NULL;
        peer->disconnectFromRS232Peer();
    }
}

void RS232Device::rs232Send(uint8_t data)
{
    if (!rs232_peer)
        fail("Attempted to send data via RS232 with no peer connected!");
        
    rs232_peer->onRS232Receive(data);
}
