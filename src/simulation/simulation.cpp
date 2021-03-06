#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>

#include "simulation.h"
#include "sim_device.h"

#include "utils/cpp_macros.h"
#include "utils/fail.h"
#include "utils/time.h"

using namespace std;

bool SimulationEventEntry::before(SimulationEventEntry &other)
{
    if (timestamp != other.timestamp)
        return timestamp < other.timestamp;
    if (event_id != other.event_id)
        return event_id < other.event_id;
    
    return device < other.device;
}

Simulation::Simulation()
{
    sync_with_real_time = true;
}

Simulation::Simulation(SystemDescription &sys_desc)
{
    for (auto& ent : sys_desc.entities) {
        auto as_sim_dev = dynamic_cast<SimulatedDevice *>(ent);
        if (as_sim_dev != NULL)
            addDevice(as_sim_dev);
    }
    
    sync_with_real_time = true;
}

void Simulation::addDevice(SimulatedDevice *device)
{
    if (CONTAINS(devices, device))
        return;
        
    devices.push_back(device);
    device->setSimulation(this);
}

void Simulation::removeDevice(SimulatedDevice *device)
{
    auto it = FIND(devices, device);
    if (it != devices.end()) {
        unscheduleAll(*it);
        devices.erase(it);
        (*it)->setSimulation(NULL);
    }
}

void Simulation::scheduleEvent(SimulatedDevice *device, int event, sim_time_t time)
{
    SimulationEventEntry new_evt(time, device, event);

    // fast path: insert in front
    if (event_queue.empty() || new_evt.before(event_queue.front())) {
        event_queue.push_front(new_evt);
        return;
    }

    // fast path: insert in back
    if (event_queue.back().before(new_evt)) {
        event_queue.push_back(new_evt);
        return;
    }

    for (auto it = event_queue.begin(); it != event_queue.end(); it++) {
        if (!it->before(new_evt)) {
            event_queue.insert(it, new_evt);
            break;
        }
    }
}

void Simulation::scheduleEventIn(SimulatedDevice *device, int event, sim_time_t time)
{
    scheduleEvent(device, event, this->time + time);
}

void Simulation::unscheduleAll(SimulatedDevice *device)
{
    event_queue.erase(remove_if(event_queue.begin(), event_queue.end(),
            [device](const SimulationEventEntry& ev) {
                return ev.device == device;
            }),
        event_queue.end());
}

void Simulation::run()
{
    runToTime(SIM_TIME_NEVER);
}

void Simulation::runToTime(sim_time_t to_time)
{
    vector<SimulatedDevice*>::iterator it;
    
    if (devices.empty())
        fail("Can't run simulation with no devices present");
    
    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
    
    time = 0;
    sim_time_t next_real_sync_time = time + ms_to_sim_time(1);

    event_queue.clear();
    for (auto dev : devices)
        dev->reset();
    
    while (time < to_time) {
        if (event_queue.empty())
            fail("Deadlock - all devices quiescent");

        SimulationEventEntry evt = event_queue.front();
        event_queue.pop_front();

        time = evt.timestamp;
        
        if (sync_with_real_time && (time >= next_real_sync_time)) {
            struct timespec t1;
            clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
            
            int64_t real_elapsed = timespec_delta_ns(&t1, &t0);
            int64_t delta = sim_time_to_ns(time) - real_elapsed;
            
            if (delta > 10000000) {
                usleep(delta/1000);
            }
            
            next_real_sync_time = time + ms_to_sim_time(1);
        }
        
        if (evt.device) {
            evt.device->act(evt.event_id);
        } else { // System event
            if (evt.event_id == SIM_EVENT_END)
                break;
        }
    }
}

void Simulation::end()
{
    scheduleEventIn(NULL, SIM_EVENT_END, 0);
}
