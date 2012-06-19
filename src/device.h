#ifndef _H_DEVICE_H
#define _H_DEVICE_H

#include "simulation.h"

class Device {
public:
    virtual sim_time_t nextEventTime(void) = 0;
};

#endif
