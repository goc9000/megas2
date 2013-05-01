#ifndef _H_RS232_CONSOLE_H
#define _H_RS232_CONSOLE_H

#include <string>
#include <vector>
#include <deque>

#include "simulation/entity.h"
#include "glue/rs232_device.h"
#include "dash_widget.h"

using namespace std;

class RS232Console : public DashboardWidget, public RS232Device
{
public:
    RS232Console(int x, int y, int rows, int columns);
    RS232Console(Json::Value &json_data, EntityLookup *lookup);
    
    virtual void render(Dashboard *dash, SDLColor color, SDLColor bg_color,
        int font_size);
    virtual bool handleEvent(Dashboard *dash, SDL_Event *event);
    
    void onRS232Receive(uint8_t data);
    
    static const unsigned DEFAULT_SCROLLBACK;
protected:
    int rows;
    int columns;
    
    int scrollback;
    
    deque<string> stored_text;
    string line_buffer;
    int scroll_position;
    int scrollbar_persist_frames;
    bool focused;
    
    void drawScrollbar(Dashboard *dash, SDLColor color);
    void getCharMetrics(Dashboard *dash, int &char_width, int &char_height);
    void getMetrics(Dashboard *dash, int &width, int &mid_height, int &height);
    
    int maxScrollPosition(void);
};

#endif
