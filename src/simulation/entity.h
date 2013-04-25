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
    string type_name;
    
    void parseJsonParam(int& value, Json::Value &json_data, const char *param_name);
    void parseOptionalJsonParam(int& value, Json::Value &json_data, const char *param_name);
    void parseJsonParam(string& value, Json::Value &json_data, const char *param_name);
    void parseOptionalJsonParam(string& value, Json::Value &json_data, const char *param_name);
protected:
    void checkJsonParamExists(Json::Value &json_data, const char *param_name);
};

#endif
