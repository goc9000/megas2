#ifndef _H_PC_INDICATOR_H
#define _H_PC_INDICATOR_H

#include <string>

#include "simulation/entity.h"
#include "devices/mcu/mcu.h"
#include "dash_widget.h"

using namespace std;

class PCIndicator : public Entity, public DashboardWidget
{
public:
    PCIndicator(Mcu *mcu, int x, int y, int size, SDLColor color);
    PCIndicator(Json::Value &json_data, EntityLookup *lookup);
    
    virtual void render(Dashboard *dash);
protected:
    Mcu *_mcu;
    int _x;
    int _y;
    int _size;
    SDLColor _color;
};

#endif
