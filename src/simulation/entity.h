#ifndef _H_ENTITY_H
#define _H_ENTITY_H

#include <string>
#include <exception>

#include <json/json.h>

#include "utils/parse_json_param.h"

using namespace std;

class Entity {
public:
    Entity(const char *default_name);
    Entity(const char *default_name, Json::Value &json_data);
    virtual ~Entity() { }; // needed to make this polymorphic

    string id;
    string name;
    string type_name;
    
    template<typename T>
    void parseJsonParam(T& value, Json::Value &json_data, const char *param_name)
    {
        try {
            return parse_json_param(value, json_data, param_name);
        } catch (exception& e) {
            fail("%s (for object '%s')", e.what(), type_name.c_str());
        }
    }
    
    template<typename T>
    bool parseOptionalJsonParam(T& value, Json::Value &json_data, const char *param_name)
    {
        try {
            return parse_optional_json_param(value, json_data, param_name);
        } catch (exception& e) {
            fail("%s (for object '%s')", e.what(), type_name.c_str());
        }
    }
};

#endif
