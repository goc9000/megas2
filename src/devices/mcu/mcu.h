#ifndef _H_MCU_H
#define _H_MCU_H

#include "symbol.h"

class Mcu {
public:
    virtual int getPC(void) = 0;
    virtual Symbol* getProgramSymbol(int pc) = 0;
};

#endif
