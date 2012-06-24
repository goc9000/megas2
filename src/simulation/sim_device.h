#ifndef _H_SIM_DEVICE_H
#define _H_SIM_DEVICE_H

#include "simulation.h"

class SimulatedDevice {
public:
    virtual void act() = 0;
    virtual void setSimulationTime(sim_time_t time);
    virtual sim_time_t nextEventTime() = 0;
protected:
    sim_time_t sim_time;
};

#endif
