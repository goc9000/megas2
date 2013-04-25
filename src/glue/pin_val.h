#ifndef _H_PIN_VAL_H
#define _H_PIN_VAL_H

#include <iostream>

#include <json/json.h>

using namespace std;

class pin_val_t {
    friend pin_val_t operator- (const pin_val_t &a);
    friend pin_val_t operator+ (const pin_val_t &a, const pin_val_t &b);
    friend pin_val_t operator- (const pin_val_t &a, const pin_val_t &b);
    friend pin_val_t operator* (const pin_val_t &a, int factor);
    friend pin_val_t operator* (const pin_val_t &a, double factor);
    friend pin_val_t operator* (int factor, const pin_val_t &a);
    friend pin_val_t operator* (double factor, const pin_val_t &a);
    friend double operator/ (const pin_val_t &a, const pin_val_t &b);
    friend bool operator== (const pin_val_t &a, const pin_val_t &b);
    friend bool operator!= (const pin_val_t &a, const pin_val_t &b);
    friend bool operator< (const pin_val_t &a, const pin_val_t &b);
    friend bool operator> (const pin_val_t &a, const pin_val_t &b);
    friend bool operator>= (const pin_val_t &a, const pin_val_t &b);
    friend bool operator<= (const pin_val_t &a, const pin_val_t &b);
    friend ostream& operator<< (ostream &out, const pin_val_t &a);
    
    friend void check_operation(const char *op, const pin_val_t &a);
    friend void check_operation(const char *op, const pin_val_t &a, const pin_val_t &b);
public:
    pin_val_t(void);
    pin_val_t(double);
    
    pin_val_t& operator*= (int factor);
    pin_val_t& operator*= (double factor);
    
    static pin_val_t z(void);
    static pin_val_t vcc(void);
protected:
    uint8_t special;
    double value;
    
    pin_val_t(uint8_t special, double value);
};

const pin_val_t PIN_VAL_0 = pin_val_t(0.0);
const pin_val_t PIN_VAL_Z = pin_val_t::z();
const pin_val_t PIN_VAL_VCC = pin_val_t::vcc();

#endif
