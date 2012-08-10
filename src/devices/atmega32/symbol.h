#ifndef _H_SYMBOL_H
#define _H_SYMBOL_H

#include <string>

using namespace std;

class Symbol {
public:
    Symbol(char *name, int address, int length, bool is_data) :
        name(name), address(address), length(length), is_data(is_data) { }
    void dump();
    bool operator< (const Symbol &other) const;

    string name;
    int address;
    int length;
    bool is_data;
};

#endif
