#include "dashboard.h"
#include "dash_widget.h"

const SDLColor DashboardWidget::COLOR_DEFAULT = SDLColor(0, 0, 0, 254);
const int DashboardWidget::FONT_SIZE_DEFAULT = -1;

DashboardWidget::DashboardWidget(const char *default_name, int x, int y)
    : Entity(default_name)
{
    this->x = x;
    this->y = y;
    this->color = COLOR_DEFAULT;
    this->bg_color = COLOR_DEFAULT;
    this->font_size = FONT_SIZE_DEFAULT;
}
    
DashboardWidget::DashboardWidget(const char *default_name, int x, int y, SDLColor color)
    : Entity(default_name)
{
    this->x = x;
    this->y = y;
    this->color = color;
    this->bg_color = COLOR_DEFAULT;
    this->font_size = FONT_SIZE_DEFAULT;
}

DashboardWidget::DashboardWidget(const char *default_name, Json::Value &json_data)
    : Entity(default_name, json_data)
{
    color = COLOR_DEFAULT;
    bg_color = COLOR_DEFAULT;
    font_size = FONT_SIZE_DEFAULT;
    
    parseJsonParam(x, json_data, "x");
    parseJsonParam(y, json_data, "y");
    parseOptionalJsonParam(color, json_data, "color");
    parseOptionalJsonParam(bg_color, json_data, "bg_color");
    parseOptionalJsonParam(font_size, json_data, "font_size");
}

void DashboardWidget::render(Dashboard *dash)
{
    SDLColor color = (this->color == COLOR_DEFAULT) ? dash->color : this->color;
    SDLColor bg_color = (this->bg_color == COLOR_DEFAULT) ? SDLColor::TRANSPARENT : this->bg_color;
    int font_size = (this->font_size == FONT_SIZE_DEFAULT) ? dash->font_size : this->font_size;

    render(dash, color, bg_color, font_size);
}

bool DashboardWidget::handleEvent(Dashboard *dash, SDL_Event *event)
{
    return false;
}

