#ifndef _H_RS232_DEVICE_H
#define _H_RS232_DEVICE_H

#include <inttypes.h>

#include "simulation/entity.h"
#include "simulation/entity_lookup.h"

class RS232Device {
public:
    RS232Device(void);
    RS232Device(Json::Value &json_data, EntityLookup *lookup);
    
    void connectToRS232Peer(RS232Device *peer);
    void disconnectFromRS232Peer(void);
    
    virtual void onRS232Receive(uint8_t data) = 0;
protected:
    RS232Device *rs232_peer;

    void rs232Send(uint8_t data);
};

#endif
