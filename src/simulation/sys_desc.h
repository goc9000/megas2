#ifndef _H_SYSTEM_DESCRIPTION_H
#define _H_SYSTEM_DESCRIPTION_H

#include <string>
#include <map>

#include <json/json.h>

#include "entity.h"
#include "entity_lookup.h"

using namespace std;

class SystemDescription : public EntityLookup {
public:
    SystemDescription();
    SystemDescription(const char *filename);
    SystemDescription(Json::Value &json_data);

    virtual Entity * lookupEntity(const char *id);

    vector<Entity *> entities;
private:
    void _init();
    void _initFromJson(Json::Value &json_data);
    void _initEntitiesFromJson(Json::Value &json_data);
    Entity * _parseEntity(Json::Value &json_data);
};

#endif
