#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>

#include "simulation.h"
#include "sim_device.h"
#include "utils/fail.h"
#include "utils/time.h"

using namespace std;

bool SimulationEventEntry::before(SimulationEventEntry &other)
{
    if (this->timestamp != other.timestamp)
        return (this->timestamp < other.timestamp);
    if (this->event_id != other.event_id)
        return (this->event_id < other.event_id);
    
    return this->device < other.device;
}

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
        return;
        
    this->devices.push_back(device);
    device->setSimulation(this);
}

void Simulation::removeDevice(SimulatedDevice *device)
{
    vector<SimulatedDevice*>::iterator it = find(this->devices.begin(), this->devices.end(), device);
    if (it != this->devices.end()) {
        this->unscheduleAll(*it);
        this->devices.erase(it);
        (*it)->setSimulation(NULL);
    }
}

void Simulation::scheduleEvent(SimulatedDevice *device, int event, sim_time_t time)
{
    SimulationEventEntry new_evt(time, device, event);

    // fast path: insert in front
    if (this->event_queue.empty() || new_evt.before(this->event_queue.front())) {
        this->event_queue.push_front(new_evt);
        return;
    }

    // fast path: insert in back
    if (this->event_queue.back().before(new_evt)) {
        this->event_queue.push_back(new_evt);
        return;
    }

    for (deque<SimulationEventEntry>::iterator it = this->event_queue.begin(); it != this->event_queue.end(); it++) {
        if (!it->before(new_evt)) {
            this->event_queue.insert(it, new_evt);
            break;
        }
    }
}

void Simulation::unscheduleAll(SimulatedDevice *device)
{
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
    
    this->time = 0;
    sim_time_t next_real_sync_time = this->time + ms_to_sim_time(1);

    this->event_queue.clear();
    for (it = this->devices.begin(); it != this->devices.end(); it++) {
        (*it)->reset();
    }
    
    while (time < to_time) {
        if (this->event_queue.empty()) {
            fail("Deadlock - all devices quiescent");
        }

        SimulationEventEntry evt = this->event_queue.front();
        this->event_queue.pop_front();

        this->time = evt.timestamp;
        
        if (this->sync_with_real_time && (this->time >= next_real_sync_time)) {
            struct timespec t1;
            clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
            
            int64_t real_elapsed = timespec_delta_ns(&t1, &t0);
            int64_t delta = sim_time_to_ns(this->time) - real_elapsed;
            
            if (delta > 10000000) {
                usleep(delta/1000);
            }
            
            next_real_sync_time = this->time + ms_to_sim_time(1);
        }
        
        evt.device->act(evt.event_id);
    }
}
