#ifndef _H_DASHBOARD_WIDGET_H
#define _H_DASHBOARD_WIDGET_H

#include <SDL/SDL.h>
#include <utils/sdl_utils.h>

#include <json/json.h>

class Dashboard;

using namespace std;

class DashboardWidget
{
public:
    virtual void render(Dashboard *dash) = 0;
    virtual bool handleEvent(Dashboard *dash, SDL_Event *event);
};

#endif

