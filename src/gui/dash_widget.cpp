#include "dash_widget.h"

SDLColor DashboardWidget::parseColor(Json::Value &json_data)
{
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;

    if (json_data.isMember("r")) {
        r = json_data["r"].asInt();
    }
    if (json_data.isMember("g")) {
        g = json_data["g"].asInt();
    }
    if (json_data.isMember("b")) {
        b = json_data["b"].asInt();
    }
    if (json_data.isMember("a")) {
        a = json_data["a"].asInt();
    }

    return SDLColor(r, g, b, a);
}

bool DashboardWidget::handleEvent(Dashboard *dash, SDL_Event *event)
{
    return false;
}
