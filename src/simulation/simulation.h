#ifndef _H_SIMULATION_H
#define _H_SIMULATION_H

#include "sys_desc.h"

#include <inttypes.h>
#include <vector>

using namespace std;

#define SIM_TIME_NEVER 0x0fffffffffffffffLL

typedef int64_t sim_time_t;

#define ns_to_sim_time(x) (x)
#define ms_to_sim_time(x) (1000000LL*(x))
#define sec_to_sim_time(x) (1000000000LL*(x))

#define sim_time_to_ns(x) (x)

class SimulatedDevice;

class Simulation {
public:
    Simulation();
    Simulation(SystemDescription *sys_desc);
    
    void addDevice(SimulatedDevice *device);
    void removeDevice(SimulatedDevice *device);
    void run();
    void runToTime(sim_time_t to_time);
    
    bool sync_with_real_time;
private:
    vector<SimulatedDevice *> devices;
};

#endif
