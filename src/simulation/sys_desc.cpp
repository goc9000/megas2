#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "devices/atmega32/atmega32.h"
#include "devices/ds1307.h"
#include "devices/sd_card.h"
#include "devices/enc28j60.h"

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
    infile >> json_data;
    
    this->_initFromJson(json_data);
}

Entity * SystemDescription::getEntity(const char *name)
{
    for (vector<Entity *>::iterator it = this->entities.begin(); it != this->entities.end(); it++)
        if ((*it)->name == name)
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
    }
    
    fail("Unsupported entity type '%s'", type.c_str());

    return NULL;
}
