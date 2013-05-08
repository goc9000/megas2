#ifndef _H_PARSE_JSON_PARAM_H
#define _H_PARSE_JSON_PARAM_H

#include <string>
#include <cstring>
#include <limits>
#include <type_traits>

#include <json/json.h>

#include "glue/pin_val.h"

#include "fail.h"
#include "sdl_utils.h"
#include "net_utils.h"

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
    
    fail("Pin value '%s' is not numeric, 'VCC' or 'Z' for parameter '%s'",
        json_value.asString().c_str(), param_name);
}

template<typename T>
typename enable_if<is_same<T, ipv4_addr_t>::value, void>::type
parse_json_param_internal(T& value, Json::Value &json_value, const char *param_name)
{
    value = ipv4_addr_t(json_value);
}

template<typename T>
typename enable_if<is_same<T, SDLColor>::value, void>::type
parse_json_param_internal(T& value, Json::Value &json_value, const char *param_name)
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
    
    if (json_value.isObject()) {
        if (!json_value.isMember("r"))
            fail("Required parameter 'r' is missing in color specification");
        if (!json_value.isMember("g"))
            fail("Required parameter 'g' is missing in color specification");
        if (!json_value.isMember("b"))
            fail("Required parameter 'b' is missing in color specification");
        parse_json_param_internal(r, json_value["r"], "r");
        parse_json_param_internal(g, json_value["g"], "g");
        parse_json_param_internal(b, json_value["b"], "b");
        
        if (json_value.isMember("a"))
            parse_json_param_internal(a, json_value["a"], "a");
        
        value = SDLColor(r, g, b, a);
    } else if (json_value.isArray()) {
        int count = json_value.size();
        
        if (!((count == 3) || (count == 4)))
            fail("Color specification array must contain 3 or 4 values");
        
        parse_json_param_internal(r, json_value[0], "color array[0]");
        parse_json_param_internal(g, json_value[1], "color array[1]");
        parse_json_param_internal(b, json_value[2], "color array[2]");
        if (count == 4)
            parse_json_param_internal(a, json_value[3], "color array[3]");
        
        value = SDLColor(r, g, b, a);
    } else if (json_value.isString()) {
        const char *text = json_value.asCString();
        
        try {
            if (!*text)
                throw;
            if (*text == '#')
                text++;
            if ((strlen(text) != 6) && (strlen(text) != 8))
                throw;
            
            unsigned chars_read = 0;
            uint32_t rgba = 0;
            
            sscanf(text, "%x%n", &rgba, &chars_read);
            if (chars_read != strlen(text))
                throw;
            
            if (chars_read == 6)
                rgba = (rgba << 8) + 0xff;
            
            value = SDLColor(rgba);
        } catch (...) {
            fail("'%s' is not a valid HTML color specification", json_value.asCString());
        }
    } else
        fail("Invalid color specification for parameter '%s'", param_name);
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
