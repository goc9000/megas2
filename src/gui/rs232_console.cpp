#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "rs232_console.h"
#include "utils/fail.h"

#define DEFAULT_NAME "RS232 console"

RS232Console::RS232Console(int x, int y, int rows, int columns, int font_size)
    : Entity(DEFAULT_NAME)
{
    this->x = x;
    this->y = y;
    this->rows = rows;
    this->columns = columns;
    this->font_size = font_size;
}

RS232Console::RS232Console(Json::Value &json_data, EntityLookup *lookup)
    : Entity(DEFAULT_NAME, json_data)
{
    this->rows = 40;
    this->columns = 20;
    this->font_size = 10;
    
    if (json_data.isMember("x")) {
        x = json_data["x"].asInt();
    } else {
        fail("Member 'x' required for RS232Console widget");
    }
    if (json_data.isMember("y")) {
        y = json_data["y"].asInt();
    } else {
        fail("Member 'y' required for RS232Console widget");
    }
    if (json_data.isMember("columns")) {
        columns = json_data["columns"].asInt();
    } else {
        fail("Member 'columns' required for RS232Console widget");
    }
    if (json_data.isMember("rows")) {
        rows = json_data["rows"].asInt();
    } else {
        fail("Member 'rows' required for RS232Console widget");
    }
    if (json_data.isMember("font_size")) {
        font_size = json_data["font_size"].asInt();
    }
}

void RS232Console::render(Dashboard *dash)
{
    int char_width;
    int char_height;
    
    dash->measureText("X", font_size, true, char_width, char_height);
    
    int width = font_size + columns * char_width;
    int height = font_size + rows * char_height;
        
    rectangleColor(dash->screen, x, y, 
        x + width - 1, y + height - 1,
        DashboardWidget::COLOR_BLACK);
}

void RS232Console::onRS232Receive(uint8_t data)
{
    
}
