#ifndef _H_DASHBOARD_H
#define _H_DASHBOARD_H

#include <SDL/SDL.h>

#include "simulation/sim_device.h"

using namespace std;

class Dashboard : public SimulatedDevice
{
public:
    Dashboard(const char *bkgd_image_filename);
    Dashboard(int width, int height);
    virtual void setSimulationTime(sim_time_t time);
    virtual void act();
    virtual sim_time_t nextEventTime();
private:
    sim_time_t _next_frame_time;
    
    SDL_Surface *_screen;
    SDL_Surface *_background;
    
    void _initScreen(int width, int height);
};

#endif

