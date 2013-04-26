#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>

#include "utils/fail.h"
#include "dashboard.h"

#define SIM_EVENT_DO_FRAME  0

// TODO: add cleanup for TTF_* objects

#define DEFAULT_NAME "Dashboard"

Dashboard::Dashboard(const char *bkgd_image_filename) : Entity(DEFAULT_NAME)
{
    _init(0, 0, bkgd_image_filename);
}

Dashboard::Dashboard(int width, int height) : Entity(DEFAULT_NAME)
{
    _init(width, height, NULL);
}

Dashboard::Dashboard(Json::Value &json_data, EntityLookup *lookup)
    : Entity(DEFAULT_NAME, json_data)
{
    if (json_data.isMember("background")) {
        this->_init(0, 0, json_data["background"].asCString());
    } else if (json_data.isMember("width") && json_data.isMember("height")) {
        this->_init(json_data["width"].asInt(), json_data["height"].asInt(), NULL);
    } else {
        fail("Either 'background' or 'width' and 'height' attributes required for Dashboard object");
    }
    
    if (json_data.isMember("font")) {
        this->setFont(json_data["font"].asCString());
    }
    if (json_data.isMember("monoFont")) {
        this->setMonoFont(json_data["monoFont"].asCString());
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

            this->addWidget(as_widget);
        }
    }
}

void Dashboard::addWidget(DashboardWidget *widget)
{
    for (vector<DashboardWidget *>::iterator it = this->_widgets.begin(); it != this->_widgets.end(); it++)
        if (*it == widget)
            return;
    
    this->_widgets.push_back(widget);
}

void Dashboard::removeWidget(DashboardWidget *widget)
{
    for (vector<DashboardWidget *>::iterator it = this->_widgets.begin(); it != this->_widgets.end(); it++)
        if (*it == widget) {
            this->_widgets.erase(it);
            return;
        }
}

void Dashboard::setFont(const char *font_filename)
{
    this->_font_filename = font_filename;
}

void Dashboard::setMonoFont(const char *mono_font_filename)
{
    this->_mono_font_filename = mono_font_filename;
}

TTF_Font * Dashboard::getFont(int size)
{
    if (this->_font_cache.find(size) != this->_font_cache.end()) {
        return this->_font_cache[size];
    }
    
    if (this->_font_filename == "") {
        fail("No font set for dashboard");
    }
    TTF_Font *font = TTF_OpenFont(this->_font_filename.c_str(), size);
    if (!font) {
        fail("Could not load dashboard font: %s", TTF_GetError());
    }
    
    this->_font_cache[size] = font;
    
    return font;
}

TTF_Font * Dashboard::getMonoFont(int size)
{
    if (this->_mono_font_cache.find(size) != this->_mono_font_cache.end()) {
        return this->_mono_font_cache[size];
    }
    
    if (this->_mono_font_filename == "") {
        fail("No monospace font set for dashboard");
    }
    TTF_Font *font = TTF_OpenFont(this->_mono_font_filename.c_str(), size);
    if (!font) {
        fail("Could not load dashboard monospaced font: %s", TTF_GetError());
    }
    
    this->_mono_font_cache[size] = font;
    
    return font;
}

void Dashboard::putText(int x, int y, const char *text, int size, SDLColor color)
{
    _putText(x, y, text, size, color, false);
}

void Dashboard::putText(int x, int y, const string& text, int size, SDLColor color)
{
    _putText(x, y, text.c_str(), size, color, false);
}

void Dashboard::putMonoText(int x, int y, const char *text, int size, SDLColor color)
{
    _putText(x, y, text, size, color, true);
}

void Dashboard::putMonoText(int x, int y, const string& text, int size, SDLColor color)
{
    _putText(x, y, text.c_str(), size, color, true);
}

void Dashboard::_putText(int x, int y, const char *text, int size, SDLColor color,
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
    this->measureText(text.c_str(), size, mono, width, height);
}

void Dashboard::measureText(const char *text, int size, bool mono, int& width, int& height)
{
    TTF_Font *font = mono ? this->getMonoFont(size) : this->getFont(size);
    if (!font)
        return;
    
    TTF_SizeText(font, text, &width, &height);
}

void Dashboard::reset()
{
    if (this->simulation) {
        this->simulation->unscheduleAll(this);
        this->simulation->scheduleEvent(this, SIM_EVENT_DO_FRAME, this->simulation->time + ms_to_sim_time(20));
    }    
}

void Dashboard::act(int event)
{
    SDL_Event evt;
    
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT)
            exit(EXIT_SUCCESS);
        
        for (vector<DashboardWidget *>::iterator it = this->_widgets.begin(); it != this->_widgets.end(); it++)
            if ((*it)->handleEvent(this, &evt))
                break;
    }
    
    if (this->_background) {
        SDL_BlitSurface(this->_background, NULL, this->screen, NULL);
    } else {
        boxColor(this->screen, 0, 0, this->screen->w, this->screen->h, 0x000000ff);
    }
    
    for (vector<DashboardWidget *>::iterator it = this->_widgets.begin(); it != this->_widgets.end(); it++)
        (*it)->render(this);
    
    SDL_Flip(this->screen);

    if (this->simulation) {
        this->simulation->scheduleEvent(this, SIM_EVENT_DO_FRAME, this->simulation->time + ms_to_sim_time(20));
    }
}

void Dashboard::_init(int width, int height, const char *bkgd_filename)
{    
    if (bkgd_filename) {
        this->_background = IMG_Load(bkgd_filename);
        if (!this->_background)
            fail("Error loading dashboard background image: %s", IMG_GetError());
        
        width = this->_background->w;
        height = this->_background->h;
    } else {
        this->_background = NULL;
    }
    
    TTF_Init();
 
    static char ENV1[100] = "SDL_VIDEO_WINDOW_POS";
    static char ENV2[100] = "SDL_VIDEO_CENTERED=1";
    putenv(ENV1);
    putenv(ENV2);
    SDL_Init(SDL_INIT_VIDEO);
    
    this->screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!this->screen)
        fail("Could not initialize SDL display");
    atexit(SDL_Quit);

    SDL_WM_SetCaption("Dashboard", 0);

    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
    
    this->_font_filename = "";
    this->_mono_font_filename = "";
}
