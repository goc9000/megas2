#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>

#include "utils/cpp_macros.h"
#include "utils/fail.h"
#include "dashboard.h"

#define SIM_EVENT_DO_FRAME  0

// TODO: add cleanup for TTF_* objects

#define DEFAULT_NAME "Dashboard"

// NB: We don't use SDLColor::BLACK here, lest we fall afoul of the SIOF
const SDLColor Dashboard::DEFAULT_COLOR = SDLColor(0,0,0);
const int Dashboard::DEFAULT_FONT_SIZE = 11;
const int Dashboard::FRAME_INTERVAL_MSEC = 50;

Dashboard::Dashboard(const char *bkgd_image_filename) : Entity(DEFAULT_NAME)
{
    init(0, 0, bkgd_image_filename);
}

Dashboard::Dashboard(int width, int height) : Entity(DEFAULT_NAME)
{
    init(width, height, NULL);
}

Dashboard::Dashboard(Json::Value &json_data, EntityLookup *lookup)
    : Entity(DEFAULT_NAME, json_data)
{
    string bkgd_filename;
    int width = 0;
    int height = 0;
    
    if (parseOptionalJsonParam(bkgd_filename, json_data, "background"))
        init(0, 0, bkgd_filename.c_str());
    else {
        parseJsonParam(width, json_data, "width");
        parseJsonParam(height, json_data, "height");
        init(width, height, NULL);
    }
    
    if (json_data.isMember("font")) {
        setFont(json_data["font"].asCString());
    }
    if (json_data.isMember("monoFont")) {
        setMonoFont(json_data["monoFont"].asCString());
    }
    if (json_data.isMember("widgets")) {
        if (!json_data["widgets"].isArray()) {
            fail("Member 'widgets' must be an array");
        }
        
        for (Json::ValueIterator it = json_data["widgets"].begin(); it != json_data["widgets"].end(); it++) {
            const char *dev_id = (*it).asCString();
            Entity *ent = lookup->lookupEntity(dev_id);
            
            if (!ent) {
                fail("Device '%s' not defined at this point", dev_id);
            }

            DashboardWidget *as_widget = dynamic_cast<DashboardWidget *>(ent);
            if (as_widget == NULL) {
                fail("Device '%s' is not a dashboard widget", dev_id);
            }

            addWidget(as_widget);
        }
    }
}

void Dashboard::addWidget(DashboardWidget *widget)
{
    if (CONTAINS(widgets, widget))
        return;
    
    widgets.push_back(widget);
}

void Dashboard::removeWidget(DashboardWidget *widget)
{
    auto widget_it = FIND(widgets, widget);
    
    if (widget_it != widgets.end())
        widgets.erase(widget_it);
}

void Dashboard::setFont(const char *font_filename)
{
    this->font_filename = font_filename;
    
    clearFontCaches();
}

void Dashboard::setMonoFont(const char *mono_font_filename)
{
    this->mono_font_filename = mono_font_filename;
    
    clearFontCaches();
}

TTF_Font * Dashboard::getFont(int size)
{
    if (font_cache.find(size) != font_cache.end())
        return font_cache[size];
    
    if (font_filename == "")
        fail("No font set for dashboard");
    
    TTF_Font *font = TTF_OpenFont(font_filename.c_str(), size);
    if (!font)
        fail("Could not load dashboard font: %s", TTF_GetError());
    
    font_cache[size] = font;
    
    return font;
}

TTF_Font * Dashboard::getMonoFont(int size)
{
    if (mono_font_cache.find(size) != mono_font_cache.end())
        return mono_font_cache[size];
    
    if (mono_font_filename == "")
        fail("No monospace font set for dashboard");
    
    TTF_Font *font = TTF_OpenFont(mono_font_filename.c_str(), size);
    if (!font)
        fail("Could not load dashboard monospaced font: %s", TTF_GetError());
    
    mono_font_cache[size] = font;
    
    return font;
}

void Dashboard::clearFontCaches(void)
{
    for (auto& item : font_cache)
        TTF_CloseFont(item.second);
    for (auto& item : mono_font_cache)
        TTF_CloseFont(item.second);
    
    font_cache.clear();
    mono_font_cache.clear();
}

void Dashboard::putText(int x, int y, const char *text, int size, SDLColor color)
{
    putText(x, y, text, size, color, false);
}

void Dashboard::putText(int x, int y, const string& text, int size, SDLColor color)
{
    putText(x, y, text.c_str(), size, color, false);
}

void Dashboard::putMonoText(int x, int y, const char *text, int size, SDLColor color)
{
    putText(x, y, text, size, color, true);
}

void Dashboard::putMonoText(int x, int y, const string& text, int size, SDLColor color)
{
    putText(x, y, text.c_str(), size, color, true);
}

void Dashboard::putText(int x, int y, const char *text, int size, SDLColor color,
    bool mono)
{
    if (strlen(text) == 0)
        return;
    
    TTF_Font *font = mono ? getMonoFont(size) : getFont(size);
    if (!font)
        return;
    
    SDL_Surface *text_surface;
    text_surface = TTF_RenderText_Solid(font, text, color);
    if (!text_surface)
        return;
    
    SDLRect where(x, y, 0, 0);
    SDL_BlitSurface(text_surface, NULL, screen, &where);
    SDL_FreeSurface(text_surface);
}

void Dashboard::measureText(const string& text, int size, bool mono, int& width, int& height)
{
    measureText(text.c_str(), size, mono, width, height);
}

void Dashboard::measureText(const char *text, int size, bool mono, int& width, int& height)
{
    TTF_Font *font = mono ? getMonoFont(size) : getFont(size);
    if (!font)
        return;
    
    TTF_SizeText(font, text, &width, &height);
}

void Dashboard::reset()
{
    if (simulation) {
        simulation->unscheduleAll(this);
        simulation->scheduleEvent(this, SIM_EVENT_DO_FRAME, simulation->time + ms_to_sim_time(20));
    }    
}

void Dashboard::act(int event)
{
    SDL_Event evt;
    
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT)
            exit(EXIT_SUCCESS);
        
        for (auto &widget : widgets)
            if (widget->handleEvent(this, &evt))
                break;
    }
    
    if (background)
        SDL_BlitSurface(background, NULL, screen, NULL);
    else
        boxColor(screen, 0, 0, screen->w, screen->h, 0x000000ff);
    
    for (auto &widget : widgets)
        widget->render(this);
    
    SDL_Flip(screen);

    if (simulation)
        simulation->scheduleEvent(this, SIM_EVENT_DO_FRAME,
            simulation->time + ms_to_sim_time(Dashboard::FRAME_INTERVAL_MSEC));
}

void Dashboard::init(int width, int height, const char *bkgd_filename)
{    
    if (bkgd_filename) {
        background = IMG_Load(bkgd_filename);
        if (!background)
            fail("Error loading dashboard background image: %s", IMG_GetError());
        
        width = background->w;
        height = background->h;
    } else
        background = NULL;
    
    TTF_Init();
 
    static char ENV1[100] = "SDL_VIDEO_WINDOW_POS";
    static char ENV2[100] = "SDL_VIDEO_CENTERED=1";
    putenv(ENV1);
    putenv(ENV2);
    SDL_Init(SDL_INIT_VIDEO);
    
    screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen)
        fail("Could not initialize SDL display");
    atexit(SDL_Quit);

    SDL_WM_SetCaption("Dashboard", 0);

    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
    
    font_filename = "";
    mono_font_filename = "";
    
    color = Dashboard::DEFAULT_COLOR;
    font_size = Dashboard::DEFAULT_FONT_SIZE;
}
