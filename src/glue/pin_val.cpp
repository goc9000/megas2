#include <sstream>

#include "pin_val.h"
#include "utils/fail.h"

#define SPECIAL_NONE           0
#define SPECIAL_Z              1
#define SPECIAL_VCC            2

pin_val_t::pin_val_t(void) : pin_val_t(0)
{
}

pin_val_t::pin_val_t(double value)
{
    special = SPECIAL_NONE;
    this->value = value;
}

pin_val_t::pin_val_t(uint8_t special, double value)
{
    this->special = special;
    this->value = value;
}

pin_val_t pin_val_t::z(void)
{
    return pin_val_t(SPECIAL_Z, 0.0);
}

pin_val_t pin_val_t::vcc(void)
{
    return pin_val_t(SPECIAL_VCC, 0.0);
}

pin_val_t& pin_val_t::operator*= (int factor)
{
    check_operation("*=", *this);
    
    value *= factor;
    
    return *this;
}

pin_val_t& pin_val_t::operator*= (double factor)
{
    check_operation("*=", *this);
    
    value *= factor;
    
    return *this;
}

pin_val_t operator- (const pin_val_t &a)
{
    check_operation("unary -", a);
    
    return pin_val_t(-a.value);
}

pin_val_t operator+ (const pin_val_t &a, const pin_val_t &b)
{
    check_operation("+", a, b);
    
    return pin_val_t(a.value + b.value);
}

pin_val_t operator- (const pin_val_t &a, const pin_val_t &b)
{
    check_operation("-", a, b);
    
    return pin_val_t(a.value - b.value);
}

pin_val_t operator* (const pin_val_t &a, int factor)
{
    check_operation("* with scalar", a);
    
    return pin_val_t(a.value * factor);
}

pin_val_t operator* (const pin_val_t &a, double factor)
{
    check_operation("* with scalar", a);
    
    return pin_val_t(a.value * factor);
}

pin_val_t operator* (int factor, const pin_val_t &a)
{
    check_operation("* with scalar", a);
    
    return pin_val_t(factor * a.value);
}

pin_val_t operator* (double factor, const pin_val_t &a)
{
    check_operation("* with scalar", a);
    
    return pin_val_t(factor * a.value);
}

double operator/ (const pin_val_t &a, const pin_val_t &b)
{
    check_operation("/", a, b);
    
    return a.value / b.value;
}

bool operator== (const pin_val_t &a, const pin_val_t &b)
{
    return ((a.special == b.special) &&
            ((a.special != SPECIAL_NONE) || (a.value == b.value)));
}

bool operator!= (const pin_val_t &a, const pin_val_t &b)
{
    return ((a.special != b.special) ||
            ((a.special == SPECIAL_NONE) && (a.value != b.value)));
}

bool operator< (const pin_val_t &a, const pin_val_t &b)
{
    check_operation("<", a, b);
    
    return a.value < b.value;
}

bool operator> (const pin_val_t &a, const pin_val_t &b)
{
    check_operation(">", a, b);
    
    return a.value > b.value;
}

bool operator>= (const pin_val_t &a, const pin_val_t &b)
{
    check_operation(">=", a, b);
    
    return a.value >= b.value;
}

bool operator<= (const pin_val_t &a, const pin_val_t &b)
{
    check_operation("<=", a, b);
    
    return a.value <= b.value;
}

ostream& operator<< (ostream &out, const pin_val_t &a)
{
    switch (a.special) {
        case SPECIAL_NONE:
            out << a.value;
            break;
        case SPECIAL_Z:
            out << "Z";
            break;
        case SPECIAL_VCC:
            out << "VCC";
            break;
    }
    
    return out;
}

void check_operation(const char *op, const pin_val_t &a)
{
    if (a.special != SPECIAL_NONE) {
        stringstream buf;
        buf << "Attempted to perform operation '" << op << "' on pin_val_t value " << a;
        
        fail("%s", buf.str().c_str());
    }
}

void check_operation(const char *op, const pin_val_t &a, const pin_val_t &b)
{
    if ((a.special != SPECIAL_NONE) || (b.special != SPECIAL_NONE)) {
        stringstream buf;
        buf << "Attempted to perform operation '" << op << "' on pin_val_t values " << a << " vs. " << b;
        
        fail("%s", buf.str().c_str());
    }
}

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
