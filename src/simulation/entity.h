#ifndef _H_ENTITY_H
#define _H_ENTITY_H

#include <string>
#include <limits>
#include <type_traits>

#include <json/json.h>

#include "utils/fail.h"

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
        if (!json_data.isMember(param_name))
            fail("Missing parameter '%s' for %s object", param_name, type_name.c_str());
        
        parseJsonParamInternal(value, json_data[param_name], param_name);
    }
    
    template<typename T>
    bool parseOptionalJsonParam(T& value, Json::Value &json_data, const char *param_name)
    {
        if (!json_data.isMember(param_name))
            return false;
            
        parseJsonParamInternal(value, json_data[param_name], param_name);
        
        return true;
    }
    
    template<typename T>
    typename enable_if<is_integral<T>::value, void>::type
    parseJsonParamInternal(T& value, Json::Value &json_value, const char *param_name)
    {
        if (!json_value.isIntegral())
            fail("Parameter '%s' must be an integer", param_name);
        
        if (is_unsigned<T>::value) {
            if (json_value.asInt64() < 0)
                fail("Parameter '%s' must be non-negative", param_name);
            
            uint64_t widest_value = json_value.asUInt64();
            
            if (widest_value > numeric_limits<T>::max())
                fail("Parameter '%s' must be in range[0..%llu]", param_name,
                    (uint64_t)numeric_limits<T>::max());
            
            value = widest_value;
        } else {
            int64_t widest_value = json_value.asInt64();
            
            if ((widest_value < (int64_t)numeric_limits<T>::min()) ||
                (widest_value > (int64_t)numeric_limits<T>::max()))
                fail("Parameter '%s' must be in range[%lld..%lld]", param_name,
                    (int64_t)numeric_limits<T>::min(), (int64_t)numeric_limits<T>::max());
            
            value = widest_value;
        }
    }
    
    template<typename T>
    typename enable_if<is_same<T, string>::value, void>::type
    parseJsonParamInternal(T& value, Json::Value &json_value, const char *param_name)
    {
        if (!json_value.isString())
            fail("Parameter '%s' must be a string", param_name);
    
        value = json_value.asString();
    }
};

#endif
