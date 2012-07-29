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
