#ifndef _H_PIN_VAL_H
#define _H_PIN_VAL_H

#include <json/json.h>

typedef double pin_val_t;

const pin_val_t PIN_VAL_Z = 1000.0;
const pin_val_t PIN_VAL_VCC = 1001.0;

pin_val_t parse_json_pin_value(Json::Value &json_value);

#endif
