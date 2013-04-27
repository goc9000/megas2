#ifndef _H_PC_INDICATOR_H
#define _H_PC_INDICATOR_H

#include <string>

#include "simulation/entity.h"
#include "devices/mcu/mcu.h"
#include "dash_widget.h"

using namespace std;

class PCIndicator : public DashboardWidget
{
public:
    PCIndicator(Mcu *mcu, int x, int y);
    PCIndicator(Json::Value &json_data, EntityLookup *lookup);
    
    virtual void render(Dashboard *dash, SDLColor color, SDLColor bg_color,
        int font_size);
protected:
    Mcu *_mcu;
};

#endif
