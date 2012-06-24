#include <cstdlib>

#include "sim_device.h"

using namespace std;

void SimulatedDevice::setSimulationTime(sim_time_t time)
{
    this->sim_time = time;
}
