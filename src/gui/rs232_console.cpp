#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "rs232_console.h"
#include "utils/fail.h"

#define DEFAULT_NAME "RS232 console"

RS232Console::RS232Console(int x, int y, int rows, int columns)
    : DashboardWidget(DEFAULT_NAME, x, y)
{
    this->rows = rows;
    this->columns = columns;
}

RS232Console::RS232Console(Json::Value &json_data, EntityLookup *lookup)
    : DashboardWidget(DEFAULT_NAME, json_data)
{
    rows = 80;
    columns = 25;
    
    parseOptionalJsonParam(columns, json_data, "columns");
    parseOptionalJsonParam(rows, json_data, "rows");
}

void RS232Console::render(Dashboard *dash, SDLColor color, SDLColor bg_color,
    int font_size)
{
    int char_width;
    int char_height;
    
    dash->measureText("X", font_size, true, char_width, char_height);
    
    int width = columns * char_width;
    int height = rows * char_height;
    
    rectangleColor(dash->screen, x, y,  x + width - 1, y + height - 1,
        color.toUInt32());
}

void RS232Console::onRS232Receive(uint8_t data)
{
}
