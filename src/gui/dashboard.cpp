#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>

#include "utils/fail.h"
#include "dashboard.h"

Dashboard::Dashboard(const char *bkgd_image_filename, const char *font_filename)
{
    SDL_Surface *image = IMG_Load(bkgd_image_filename);
    if (!image)
        fail("Error loading dashboard background image: %s", IMG_GetError());

    this->_init(image->w, image->h, image, font_filename);
}

Dashboard::Dashboard(int width, int height, const char *font_filename)
{
    this->_init(width, height, NULL, font_filename);
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

TTF_Font * Dashboard::getFont(int size)
{
    if (this->_font_cache.find(size) != this->_font_cache.end()) {
        return this->_font_cache[size];
    }
    
    TTF_Font *font = TTF_OpenFont(this->_font_filename, size);
    if (!font) {
        fail("Could not load dashboard font: %s", TTF_GetError());
    }
    
    this->_font_cache[size] = font;
    
    return font;
}

void Dashboard::setSimulationTime(sim_time_t time)
{
    sim_time_t delta = time - this->sim_time;
    
    this->sim_time = time;
    this->_next_frame_time += delta;
}

void Dashboard::act()
{
    this->sim_time = this->_next_frame_time;
    
    SDL_Event event;
    
    if (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
            exit(EXIT_SUCCESS);
    }
    
    if (this->_background) {
        SDL_BlitSurface(this->_background, NULL, this->screen, NULL);
    } else {
        boxColor(this->screen, 0, 0, this->screen->w, this->screen->h, 0x000000ff);
    }
    
    for (vector<DashboardWidget *>::iterator it = this->_widgets.begin(); it != this->_widgets.end(); it++)
        (*it)->render(this);
    
    SDL_Flip(this->screen);
    
    this->_next_frame_time += ms_to_sim_time(20);
}

sim_time_t Dashboard::nextEventTime()
{
    return this->_next_frame_time;
}

void Dashboard::_init(int width, int height, SDL_Surface *background, const char *font_filename)
{
    static char ENV1[100] = "SDL_VIDEO_WINDOW_POS";
    static char ENV2[100] = "SDL_VIDEO_CENTERED=1";
    putenv(ENV1);
    putenv(ENV2);
    
    TTF_Init();
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Surface *screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen)
        fail("Could not initialize SDL display");
    atexit(SDL_Quit);
    
    SDL_WM_SetCaption("Dashboard", 0);
    
    this->screen = screen;
    this->_background = background;
    this->_font_filename = font_filename;
    
    this->_next_frame_time = 0;
}
