#ifndef _H_DASHBOARD_H
#define _H_DASHBOARD_H

#include <vector>
#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <json/json.h>

#include "dash_widget.h"
#include "simulation/entity.h"
#include "simulation/entity_lookup.h"
#include "simulation/sim_device.h"

using namespace std;

class Dashboard : public Entity, public SimulatedDevice
{
public:
    SDL_Surface *screen;

    Dashboard(const char *bkgd_image_filename);
    Dashboard(int width, int height);
    Dashboard(Json::Value &json_data, EntityLookup *lookup);
    
    void addWidget(DashboardWidget *widget);
    void removeWidget(DashboardWidget *widget);
    
    void setFont(const char *font_filename);
    void setMonoFont(const char *mono_font_filename);
    
    TTF_Font *getFont(int size);
    TTF_Font *getMonoFont(int size);

    virtual void reset();
    virtual void act(int event);
private:
    vector<DashboardWidget *> _widgets;

    string _font_filename;
    map<int, TTF_Font *> _font_cache;
    string _mono_font_filename;
    map<int, TTF_Font *> _mono_font_cache;
    
    SDL_Surface *_background;
    
    void _init(int width, int height, const char *bkgd_filename);
};

#endif

