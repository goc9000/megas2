#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "devices/atmega32/atmega32.h"
#include "devices/enc28j60/enc28j60.h"
#include "devices/ds1307.h"
#include "devices/sd_card.h"

#include "glue/i2c_bus.h"
#include "glue/spi_bus.h"
#include "glue/analog_bus.h"

#include "gui/dashboard.h"
#include "gui/led.h"
#include "gui/push_button.h"
#include "gui/pc_indicator.h"

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
    }
}

Entity * SystemDescription::_parseEntity(Json::Value &json_data)
{
    if (!json_data.isMember("type")) {
        fail("Entity object lacks member 'type'");
    }

    string type = json_data["type"].asString();

    if (type == "Atmega32") {
        return new Atmega32(json_data);
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
    } else if (type == "SimpleLed") {
        return new SimpleLed(json_data);
    } else if (type == "SimplePushButton") {
        return new SimplePushButton(json_data);
    } else if (type == "PCIndicator") {
        return new PCIndicator(json_data, this);
    } else if (type == "Dashboard") {
        return new Dashboard(json_data, this);
    }
    
    fail("Unsupported entity type '%s'", type.c_str());

    return NULL;
}
