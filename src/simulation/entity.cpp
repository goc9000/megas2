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
