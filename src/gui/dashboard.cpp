#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>

#include "utils/fail.h"
#include "dashboard.h"

Dashboard::Dashboard(const char *bkgd_image_filename)
{
    SDL_Surface *image = IMG_Load(bkgd_image_filename);
    if (!image)
        fail("Error loading dashboard background image: %s", IMG_GetError());
    this->_background = image;

    this->_initScreen(image->w, image->h);
    this->_next_frame_time = 0;
}

Dashboard::Dashboard(int width, int height)
{
    this->_background = NULL;
    this->_initScreen(width, height);
    this->_next_frame_time = 0;
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
        SDL_BlitSurface(this->_background, NULL, this->_screen, NULL);
    } else {
        boxColor(this->_screen, 0, 0, this->_screen->w, this->_screen->h, 0x000000ff);
    }
    
    SDL_Flip(this->_screen);
    
    this->_next_frame_time += ms_to_sim_time(20);
}

sim_time_t Dashboard::nextEventTime()
{
    return this->_next_frame_time;
}

void Dashboard::_initScreen(int width, int height)
{
    static char ENV1[100] = "SDL_VIDEO_WINDOW_POS";
    static char ENV2[100] = "SDL_VIDEO_CENTERED=1";
    putenv(ENV1);
    putenv(ENV2);

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Surface *screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen)
        fail("Could not initialize SDL display");
    atexit(SDL_Quit);
    
    SDL_WM_SetCaption("Dashboard", 0);
    
    this->_screen = screen;
}
