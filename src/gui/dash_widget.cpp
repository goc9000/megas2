#include "dash_widget.h"

int DashboardWidget::parseColor(Json::Value &json_data)
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

    return (r << 24) + (g << 16) + (b << 8) + a;
}

bool DashboardWidget::handleEvent(Dashboard *dash, SDL_Event *event)
{
    return false;
}
