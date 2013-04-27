#ifndef _H_DASHBOARD_WIDGET_H
#define _H_DASHBOARD_WIDGET_H

#include <SDL/SDL.h>
#include <utils/sdl_utils.h>

#include <json/json.h>

#include "simulation/entity.h"

class Dashboard;

using namespace std;

class DashboardWidget: public Entity
{
public:
    DashboardWidget(const char *default_name, int x, int y);
    DashboardWidget(const char *default_name, int x, int y, SDLColor color);
    DashboardWidget(const char *default_name, Json::Value &json_data);

    void render(Dashboard *dash);
    virtual void render(Dashboard *dash, SDLColor color, SDLColor bg_color,
        int font_size) = 0;
    virtual bool handleEvent(Dashboard *dash, SDL_Event *event);
protected:
    int x;
    int y;
    SDLColor color;
    SDLColor bg_color;
    int font_size;
    
    static const SDLColor COLOR_DEFAULT;
    static const int FONT_SIZE_DEFAULT;
};

#endif
