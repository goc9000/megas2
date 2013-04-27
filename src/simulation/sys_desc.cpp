#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "devices/atmega32/atmega32.h"
#include "devices/enc28j60/enc28j60.h"
#include "devices/ds1307.h"
#include "devices/sd_card.h"
#include "devices/voltage_src.h"

#include "glue/i2c_bus.h"
#include "glue/spi_bus.h"
#include "glue/analog_bus.h"

#include "gui/dashboard.h"
#include "gui/led.h"
#include "gui/push_button.h"
#include "gui/pc_indicator.h"
#include "gui/rs232_console.h"

#include "networking/virtual_net.h"

#include "sys_desc.h"
#include "utils/fail.h"

using namespace std;

SystemDescription::SystemDescription()
{
    this->_init();
}

SystemDescription::SystemDescription(Json::Value &json_data)
{
    this->_init();
    this->_initFromJson(json_data);
}

SystemDescription::SystemDescription(const char *filename)
{
    Json::Value json_data;

    this->_init();
    
    ifstream infile(filename);
    if (!infile.is_open()) {
        fail("Cannot open system description file '%s'", filename);
    }
    infile >> json_data;
    
    this->_initFromJson(json_data);
}

Entity * SystemDescription::lookupEntity(const char *id)
{
    for (vector<Entity *>::iterator it = this->entities.begin(); it != this->entities.end(); it++)
        if ((*it)->id == id)
            return *it;

    return NULL;
}

void SystemDescription::_init()
{
}

void SystemDescription::_initFromJson(Json::Value &json_data)
{
    if (json_data.isMember("entities")) {
        this->_initEntitiesFromJson(json_data["entities"]);
    }
}

void SystemDescription::_initEntitiesFromJson(Json::Value &json_data)
{
    if (!json_data.isArray()) {
        fail("'entities' should be an array");
    }

    for (Json::ValueIterator it = json_data.begin(); it != json_data.end(); it++) {
        Entity *ent = this->_parseEntity(*it);
        this->entities.push_back(ent);
        this->_parseEntityConnections(ent, *it);
    }
}

Entity * SystemDescription::_parseEntity(Json::Value &json_data)
{
    if (!json_data.isMember("type")) {
        fail("Entity object lacks member 'type'");
    }

    string type = json_data["type"].asString();

    if (type == "Atmega32") {
        return new Atmega32(json_data, this);
    } else if (type == "Ds1307") {
        return new Ds1307(json_data);
    } else if (type == "SdCard") {
        return new SdCard(json_data);
    } else if (type == "Enc28J60") {
        return new Enc28J60(json_data);
    } else if (type == "I2cBus") {
        return new I2cBus(json_data, this);
    } else if (type == "SpiBus") {
        return new SpiBus(json_data, this);
    } else if (type == "AnalogBus") {
        return new AnalogBus(json_data, this);
    } else if (type == "VoltageSource") {
        return new VoltageSource(json_data);
    } else if (type == "VirtualNetwork") {
        return new VirtualNetwork(json_data, this);
    } else if (type == "SimpleLed") {
        return new SimpleLed(json_data);
    } else if (type == "SimplePushButton") {
        return new SimplePushButton(json_data);
    } else if (type == "PCIndicator") {
        return new PCIndicator(json_data, this);
    } else if (type == "RS232Console") {
        return new RS232Console(json_data, this);
    } else if (type == "Dashboard") {
        return new Dashboard(json_data, this);
    }
    
    fail("Unsupported entity type '%s'", type.c_str());

    return NULL;
}

void SystemDescription::_parseEntityConnections(Entity *entity, Json::Value &json_data)
{
    if (!json_data.isMember("connect"))
        return;
     
    PinDevice *pin_device = dynamic_cast<PinDevice *>(entity);
    if (!pin_device) {
        fail("Device '%s' is not a pin device", entity->id.c_str());
    }
    
    Json::Value pin_dict = json_data["connect"];
    if (!pin_dict.isObject())
        return;
    
    for (Json::ValueIterator it = pin_dict.begin(); it != pin_dict.end() ; it++) {
        if (!it.key().isString())
            fail("Keys in 'connect' object should be strings denoting pin names");
        
        PinReference pin1(pin_device, it.key().asString().c_str());
        PinReference pin2(*it, this);
        
        AnalogBus *wire = new AnalogBus();
        wire->addDevicePin(pin1);
        wire->addDevicePin(pin2);
        
        this->entities.push_back(wire);
    }
}
