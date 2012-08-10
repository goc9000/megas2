#include <cstdio>

#include "symbol.h"

using namespace std;

void Symbol::dump()
{
    printf("%c %-32s @ %04x : %04x\n", this->is_data ? 'D' : 'C',
        this->name.c_str(), this->address, this->length);
}

bool Symbol::operator< (const Symbol &other) const
{
    if (this->address != other.address)
        return (this->address < other.address);
    if (this->length != other.length)
        return (this->length > other.length);
    if (this->is_data != other.is_data)
        return (this->is_data < other.is_data);
    
    return this->name < other.name;
}
