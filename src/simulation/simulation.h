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

class SimulationEventEntry {
public:
    sim_time_t timestamp;
    SimulatedDevice *device;
    int event_id;

    SimulationEventEntry(sim_time_t timestamp_, SimulatedDevice* device_, int event_id_)
        : timestamp(timestamp_), device(device_), event_id(event_id_) {}
    bool before(SimulationEventEntry &other);
};

class Simulation {
public:
    Simulation();
    Simulation(SystemDescription *sys_desc);
    
    void addDevice(SimulatedDevice *device);
    void removeDevice(SimulatedDevice *device);

    void run();
    void runToTime(sim_time_t to_time);

    void scheduleEvent(SimulatedDevice *device, int event, sim_time_t time);
    void scheduleEventIn(SimulatedDevice *device, int event, sim_time_t time);
    void unscheduleAll(SimulatedDevice *device);

    sim_time_t time;
    
    bool sync_with_real_time;
private:
    vector<SimulatedDevice *> devices;
    deque<SimulationEventEntry> event_queue;
};

#endif
