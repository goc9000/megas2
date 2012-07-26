#include <cstdlib>
#include <algorithm>
#include <unistd.h>

#include "simulation.h"
#include "sim_device.h"
#include "utils/fail.h"
#include "utils/time.h"

using namespace std;

Simulation::Simulation()
{
    this->sync_with_real_time = true;
}

Simulation::Simulation(SystemDescription *sys_desc)
{
    for (vector<Entity *>::iterator it = sys_desc->entities.begin(); it != sys_desc->entities.end(); it++) {
        SimulatedDevice *as_sim_dev = dynamic_cast<SimulatedDevice *>(*it);
        if (as_sim_dev != NULL) {
            this->addDevice(as_sim_dev);
        }
    }
    
    this->sync_with_real_time = true;
}

void Simulation::addDevice(SimulatedDevice *device)
{
    if (find(this->devices.begin(), this->devices.end(), device) != this->devices.end())
        fail("Added device twice to simulation");
    
    this->devices.push_back(device);
}

void Simulation::removeDevice(SimulatedDevice *device)
{
    vector<SimulatedDevice*>::iterator it = find(this->devices.begin(), this->devices.end(), device);
    
    this->devices.erase(it);
}

void Simulation::run()
{
    this->runToTime(SIM_TIME_NEVER);
}

void Simulation::runToTime(sim_time_t to_time)
{
    vector<SimulatedDevice*>::iterator it;
    
    if (this->devices.empty())
        fail("Can't run simulation with no devices present");

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
    
    sim_time_t time = 0;
    sim_time_t next_real_sync_time = time + ms_to_sim_time(1);
    
    for (it = this->devices.begin(); it != this->devices.end(); it++) {
        (*it)->setSimulationTime(0);
    }
    
    while (time < to_time) {
        sim_time_t earliest = SIM_TIME_NEVER;
        for (it = this->devices.begin(); it != this->devices.end(); it++)
            earliest = min(earliest, (*it)->nextEventTime());
        
        time = earliest;
        
        if (this->sync_with_real_time && (time >= next_real_sync_time)) {
            struct timespec t1;
            clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
            
            int64_t real_elapsed = timespec_delta_ns(&t1, &t0);
            int64_t delta = sim_time_to_ns(time) - real_elapsed;
            
            if (delta > 10000000) {
                usleep(delta/1000);
            }
            
            next_real_sync_time = time + ms_to_sim_time(1);
        }
        
        for (it = this->devices.begin(); it != this->devices.end(); it++)
            if ((*it)->nextEventTime() == time)
                (*it)->act();
    }
}
