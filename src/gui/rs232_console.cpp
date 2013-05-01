#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "rs232_console.h"
#include "utils/fail.h"

#define DEFAULT_NAME "RS232 console"

#define SCROLLBAR_PERSIST_TIME_MSEC 1000

const unsigned RS232Console::DEFAULT_SCROLLBACK = 1000;

RS232Console::RS232Console(int x, int y, int rows, int columns)
    : DashboardWidget(DEFAULT_NAME, x, y)
{
    this->rows = rows;
    this->columns = columns;
    this->scrollback = DEFAULT_SCROLLBACK;
    
    scroll_position = 0;
    scrollbar_persist_frames = 0;
    focused = false;
}

RS232Console::RS232Console(Json::Value &json_data, EntityLookup *lookup)
    : DashboardWidget(DEFAULT_NAME, json_data), RS232Device(json_data, lookup)
{
    rows = 80;
    columns = 25;
    scrollback = DEFAULT_SCROLLBACK;
    
    parseOptionalJsonParam(columns, json_data, "columns");
    parseOptionalJsonParam(rows, json_data, "rows");
    parseOptionalJsonParam(scrollback, json_data, "scrollback");
    
    scroll_position = 0;
    scrollbar_persist_frames = 0;
    focused = false;
}

void RS232Console::render(Dashboard *dash, SDLColor color, SDLColor bg_color,
    int font_size)
{
    int char_width, char_height, height, width, mid_height;
    
    getCharMetrics(dash, char_width, char_height);
    getMetrics(dash, width, mid_height, height);
    
    rectangleColor(dash->screen, x, y,  x + width - 1, y + height - 1,
        color.toUInt32());
    lineColor(dash->screen, x, y + mid_height - 1, x + width - 1, y + mid_height - 1, color.toUInt32());
    
    int current_y = y + 2;
    int to_line = min((int)stored_text.size(), scroll_position + rows);
    
    for (int i = scroll_position; i < to_line; i++) {
        dash->putMonoText(x + 2, current_y, stored_text[i].substr(0, columns),
            font_size + 2, color);
        current_y += char_height;
    }
    
    if (scrollbar_persist_frames) {
        drawScrollbar(dash, color);
        scrollbar_persist_frames--;
    }
}

void RS232Console::drawScrollbar(Dashboard *dash, SDLColor color)
{
    const int PADDING = 6;
    const int BAR_WIDTH = 12;
    
    if ((int)stored_text.size() <= rows)
        return;
    
    int height, width, mid_height;
    
    getMetrics(dash, width, mid_height, height);
    
    int bar_x = x + width - PADDING - BAR_WIDTH;
    int bar_y = y + PADDING;
    int bar_w = BAR_WIDTH;
    int bar_h = mid_height - 2 * PADDING;
    
    int thumb_y = bar_y + bar_h * scroll_position / stored_text.size();
    int thumb_h = bar_h * rows / stored_text.size();
    
    rectangleColor(dash->screen, bar_x, bar_y, bar_x + bar_w - 1, bar_y + bar_h - 1,
        color.toUInt32());
    boxColor(dash->screen, bar_x, thumb_y, bar_x + bar_w - 1, thumb_y + thumb_h - 1,
        color.toUInt32());
}

bool RS232Console::handleEvent(Dashboard *dash, SDL_Event *event)
{
    int width, height, dummy;
    
    getMetrics(dash, width, dummy, height);
    
    bool hit_test = (event->button.x >= x) &&
                    (event->button.y >= y) &&
                    (event->button.x < x + width) &&
                    (event->button.y < y + height);
    
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT)
                focused = hit_test;
            break;
        case SDL_MOUSEBUTTONUP:
            if (!hit_test)
                break;
            
            if (event->button.button == SDL_BUTTON_WHEELDOWN) {
                scroll_position = min(scroll_position + 2, maxScrollPosition());
                scrollbar_persist_frames = SCROLLBAR_PERSIST_TIME_MSEC / Dashboard::FRAME_INTERVAL_MSEC;
            } else if (event->button.button == SDL_BUTTON_WHEELUP) {
                scroll_position = max(scroll_position - 2, 0);
                scrollbar_persist_frames = SCROLLBAR_PERSIST_TIME_MSEC / Dashboard::FRAME_INTERVAL_MSEC;
            }
            break;
    }
    
    return false;
}

void RS232Console::getCharMetrics(Dashboard *dash, int &char_width, int &char_height)
{
    dash->measureText("X", font_size + 2, true, char_width, char_height);
}

void RS232Console::getMetrics(Dashboard *dash, int &width, int &mid_height, int &height)
{
    int char_width, char_height;
    
    getCharMetrics(dash, char_width, char_height);
    
    width = columns * char_width + 4;
    mid_height = rows * char_height + 4;
    height = mid_height + char_height + 3;
}

int RS232Console::maxScrollPosition(void)
{
    return max(0, (int)stored_text.size() - rows);
}

void RS232Console::onRS232Receive(uint8_t data)
{
    if (stored_text.size() == 0)
        stored_text.push_back(string());
    
    if (data == '\n')
        stored_text.push_back(string());
    else
        stored_text[stored_text.size() - 1].push_back((char)data);
}
