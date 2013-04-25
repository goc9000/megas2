#include <cstdio>

#include "entity.h"
#include "utils/fail.h"

using namespace std;

Entity::Entity(const char *default_name)
{
    char buf[256];
    
    char *ptr = buf + sprintf(buf, "anon_");
    const char *read_ptr = default_name;
    while (*read_ptr) {
        char c = tolower(*(read_ptr++));
        
        *(ptr++) = (c == ' ') ? '_' : c;
    }
    ptr += sprintf(ptr, "_%p", this);
    
    id = string(buf);

    buf[sprintf(buf, "Anonymous %s @ %p", default_name, this)] = 0;
    name = string(buf);
    
    type_name = string(default_name);
}

Entity::Entity(const char *default_name, Json::Value &json_data)
    : Entity(default_name)
{
    parseOptionalJsonParam(this->id, json_data, "id");
    parseOptionalJsonParam(this->name, json_data, "name");
}

void Entity::parseJsonParam(int& value, Json::Value &json_data, const char *param_name)
{
    checkJsonParamExists(json_data, param_name);
    parseOptionalJsonParam(value, json_data, param_name);
}

void Entity::parseOptionalJsonParam(int& value, Json::Value &json_data, const char *param_name)
{
    if (json_data.isMember(param_name)) {
        Json::Value json_value = json_data[param_name];
        
        if (!json_value.isInt())
            fail("Parameter '%s' must be an integer", param_name);
        
        value = json_value.asInt();
    }
}

void Entity::parseJsonParam(string& value, Json::Value &json_data, const char *param_name)
{
    checkJsonParamExists(json_data, param_name);
    parseOptionalJsonParam(value, json_data, param_name);
}

void Entity::parseOptionalJsonParam(string& value, Json::Value &json_data, const char *param_name)
{
    if (json_data.isMember(param_name)) {
        Json::Value json_value = json_data[param_name];
        
        if (!json_value.isString())
            fail("Parameter '%s' must be a string", param_name);
        
        value = json_value.asString();
    }
}

void Entity::checkJsonParamExists(Json::Value &json_data, const char *param_name)
{
    if (!json_data.isMember(param_name))
        fail("Missing parameter '%s' for %s object", param_name, type_name.c_str());
}
