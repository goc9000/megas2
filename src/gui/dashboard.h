#ifndef _H_DASHBOARD_H
#define _H_DASHBOARD_H

#include <vector>
#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "dash_widget.h"
#include "simulation/sim_device.h"

using namespace std;

class Dashboard : public SimulatedDevice
{
public:
    SDL_Surface *screen;

    Dashboard(const char *bkgd_image_filename, const char *font_filename);
    Dashboard(int width, int height, const char *font_filename);
    
    void addWidget(DashboardWidget *widget);
    void removeWidget(DashboardWidget *widget);
    
    TTF_Font *getFont(int size);
    
    virtual void setSimulationTime(sim_time_t time);
    virtual void act();
    virtual sim_time_t nextEventTime();
private:
    vector<DashboardWidget *> _widgets;

    sim_time_t _next_frame_time;
    
    const char *_font_filename;
    map<int, TTF_Font *> _font_cache;
    SDL_Surface *_background;
    
    void _init(int width, int height, SDL_Surface *background, const char *font_filename);
};

#endif

