#ifndef _H_PARSE_JSON_PARAM_H
#define _H_PARSE_JSON_PARAM_H

#include <string>
#include <limits>
#include <type_traits>

#include <json/json.h>

#include "glue/pin_val.h"
#include "fail.h"

using namespace std;

template<typename T>
typename enable_if<is_integral<T>::value && !is_same<T, bool>::value, void>::type
parse_json_param_internal(T& value, Json::Value &json_value, const char *param_name)
{
    if (!json_value.isIntegral())
        fail("Parameter '%s' must be an integer", param_name);
    
    if (is_unsigned<T>::value) {
        if (json_value.asInt64() < 0)
            fail("Parameter '%s' must be non-negative", param_name);
        
        uint64_t widest_value = json_value.asUInt64();
        
        if (widest_value > (uint64_t)numeric_limits<T>::max())
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
parse_json_param_internal(T& value, Json::Value &json_value, const char *param_name)
{
    if (!json_value.isString())
        fail("Parameter '%s' must be a string", param_name);

    value = json_value.asString();
}

template<typename T>
typename enable_if<is_same<T, bool>::value, void>::type
parse_json_param_internal(T& value, Json::Value &json_value, const char *param_name)
{
    if (!json_value.isBool())
        fail("Parameter '%s' must be a boolean value", param_name);

    value = json_value.asBool();
}

template<typename T>
typename enable_if<is_same<T, pin_val_t>::value, void>::type
parse_json_param_internal(T& value, Json::Value &json_value, const char *param_name)
{
    if (json_value.isString()) {
        if (json_value.asString() == "Z") {
            value = PIN_VAL_Z;
            return;
        } else if (json_value.asString() == "VCC") {
            value = PIN_VAL_VCC;
            return;
        }
    } else if (json_value.isNumeric()) {
        value = pin_val_t(json_value.asDouble());
        return;
    }
    
    fail("Pin value '%s' is not numeric, 'VCC' or 'Z'", json_value.asString().c_str());
}

template<typename T>
void parse_json_param(T& value, Json::Value &json_data, const char *param_name)
{
    if (!json_data.isMember(param_name))
        fail("Required parameter '%s' is missing", param_name);
    
    parse_json_param_internal(value, json_data[param_name], param_name);
}

template<typename T>
bool parse_optional_json_param(T& value, Json::Value &json_data, const char *param_name)
{
    if (!json_data.isMember(param_name))
        return false;
        
    parse_json_param_internal(value, json_data[param_name], param_name);
    
    return true;
}

#endif
