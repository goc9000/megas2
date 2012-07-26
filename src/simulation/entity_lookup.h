#ifndef _H_ENTITY_LOOKUP_H
#define _H_ENTITY_LOOKUP_H

#include "entity.h"

using namespace std;

class EntityLookup {
public:
    virtual Entity * lookupEntity(const char *id) = 0;
};

#endif
