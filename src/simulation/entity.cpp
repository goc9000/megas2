#include <cstdio>

#include "entity.h"

using namespace std;

Entity::Entity(const char *defaultId, const char *defaultName)
{
    char buf[256];

    buf[sprintf(buf, "anon_%s_%p", defaultId, this)] = 0;
    this->id = string(buf);

    buf[sprintf(buf, "%s @ %p", defaultName, this)] = 0;
    this->name = string(buf);
}

Entity::Entity(Json::Value &json_data)
{
    if (json_data.isMember("id")) {
        this->id = json_data["id"].asString();
    }
    if (json_data.isMember("name")) {
        this->name = json_data["name"].asString();
    }
}
