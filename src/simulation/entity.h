#ifndef _H_ENTITY_H
#define _H_ENTITY_H

#include <string>

#include <json/json.h>

using namespace std;

class Entity {
public:
    Entity(const char *defaultId, const char *defaultName);
    Entity(Json::Value &json_data);

    string id;
    string name;
};

#endif
