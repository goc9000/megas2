#include <cstdlib>

#include "sim_device.h"

using namespace std;

SimulatedDevice::SimulatedDevice(void)
{
    this->simulation = NULL;
}

void SimulatedDevice::setSimulation(Simulation *simulation)
{
    if (simulation == this->simulation) {
        return;
    }

    if (this->simulation) {
        Simulation *old_sim = this->simulation;
        this->simulation = NULL;
        old_sim->removeDevice(this);
    }

    if (simulation) {
        this->simulation = simulation;
        simulation->addDevice(this);
    }
}

void SimulatedDevice::act(int event)
{
}

void SimulatedDevice::scheduleEvent(int event, sim_time_t time)
{
    if (simulation)
        simulation->scheduleEvent(this, event, time);
}

void SimulatedDevice::scheduleEventIn(int event, sim_time_t time)
{
    if (simulation)
        simulation->scheduleEventIn(this, event, time);
}

void SimulatedDevice::unscheduleAll(void)
{
    if (simulation)
        simulation->unscheduleAll(this);
}
