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
    static const SDLColor DEFAULT_COLOR;
    static const int DEFAULT_FONT_SIZE;

    SDL_Surface *screen;

    SDLColor color;
    SDLColor bg_color;
    int font_size;

    Dashboard(const char *bkgd_image_filename);
    Dashboard(int width, int height);
    Dashboard(Json::Value &json_data, EntityLookup *lookup);
    
    void addWidget(DashboardWidget *widget);
    void removeWidget(DashboardWidget *widget);
    
    void setFont(const char *font_filename);
    void setMonoFont(const char *mono_font_filename);
    
    TTF_Font *getFont(int size);
    TTF_Font *getMonoFont(int size);
    
    void putText(int x, int y, const string& text, int size, SDLColor color);
    void putText(int x, int y, const char *text, int size, SDLColor color);
    void putMonoText(int x, int y, const string& text, int size, SDLColor color);
    void putMonoText(int x, int y, const char *text, int size, SDLColor color);
    
    void measureText(const string& text, int size, bool mono, int& with, int& height);
    void measureText(const char *text, int size, bool mono, int& with, int& height);

    virtual void reset();
    virtual void act(int event);
private:
    vector<DashboardWidget *> widgets;

    string font_filename;
    map<int, TTF_Font *> font_cache;
    string mono_font_filename;
    map<int, TTF_Font *> mono_font_cache;
    
    SDL_Surface *background;
    
    void putText(int x, int y, const char *text, int size, SDLColor color, bool mono);
    
    void init(int width, int height, const char *bkgd_filename);
    void clearFontCaches(void);
};

#endif

