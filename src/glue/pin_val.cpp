#include "pin_val.h"
#include "utils/fail.h"

pin_val_t parse_json_pin_value(Json::Value &json_value)
{
    if (json_value.isString() && json_value.asString() == "Z") {
        return PIN_VAL_Z;
    }
    
    if (!json_value.isNumeric()) {
        fail("Pin value '%s' is not numeric or 'Z'", json_value.asString().c_str());
    }
    
    return json_value.asDouble();
}
