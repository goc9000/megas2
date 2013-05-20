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
    ~SystemDescription();

    virtual Entity * lookupEntity(const char *id);

    vector<Entity *> entities;
private:
    void initFromJson(Json::Value &json_data);
    void initEntitiesFromJson(Json::Value &json_data);
    Entity * parseEntity(Json::Value &json_data);
    void parseEntityConnections(Entity *entity, Json::Value &json_data);
};

#endif
