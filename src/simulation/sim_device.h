#ifndef _H_SIM_DEVICE_H
#define _H_SIM_DEVICE_H

#include "devices/device.h"
#include "simulation.h"

class SimulatedDevice : public Device {
public:
    SimulatedDevice();

    virtual void act(int event);
    void setSimulation(Simulation *simulation);
protected:
    Simulation *simulation;
    
    void scheduleEvent(int event, sim_time_t time);
    void scheduleEventIn(int event, sim_time_t time);
    void unscheduleAll(void);
};

#endif
