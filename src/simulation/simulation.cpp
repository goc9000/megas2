#include <cstdlib>
#include <algorithm>
#include <time.h>
#include <unistd.h>

#include "simulation.h"
#include "sim_device.h"
#include "utils/fail.h"

using namespace std;

Simulation::Simulation()
{
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
    vector<SimulatedDevice*>::iterator it;
    
    if (this->devices.empty())
        fail("Can't run simulation with no devices present");

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
    
    sim_time_t time = 0;
    sim_time_t next_overrun_check_time = time + ms_to_sim_time(1);
    
    for (it = this->devices.begin(); it != this->devices.end(); it++) {
        (*it)->setSimulationTime(0);
    }
    
    while (true) {
        sim_time_t earliest = SIM_TIME_NEVER;
        for (it = this->devices.begin(); it != this->devices.end(); it++)
            earliest = min(earliest, (*it)->nextEventTime());
        
        time = earliest;
        
        if (time >= next_overrun_check_time) {
            struct timespec t1;
            clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
            
            int64_t real_elapsed = (t1.tv_sec - t0.tv_sec)*1000000000LL + (t1.tv_nsec - t0.tv_nsec);
            int64_t delta = sim_time_to_ns(time) - real_elapsed;
            
            if (delta > 10000000) {
                usleep(delta/1000);
            }
            
            next_overrun_check_time = time + ms_to_sim_time(1);
        }
        
        for (it = this->devices.begin(); it != this->devices.end(); it++)
            if ((*it)->nextEventTime() == time)
                (*it)->act();
    }
}
