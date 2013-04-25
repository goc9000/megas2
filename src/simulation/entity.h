#ifndef _H_ENTITY_H
#define _H_ENTITY_H

#include <string>

#include <json/json.h>

using namespace std;

class Entity {
public:
    Entity(const char *default_name);
    Entity(const char *default_name, Json::Value &json_data);
    virtual ~Entity() { }; // needed to make this polymorphic

    string id;
    string name;
};

#endif
