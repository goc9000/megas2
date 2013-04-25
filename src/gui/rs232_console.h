#ifndef _H_RS232_CONSOLE_H
#define _H_RS232_CONSOLE_H

#include <string>

#include "simulation/entity.h"
#include "glue/rs232_device.h"
#include "dash_widget.h"

using namespace std;

class RS232Console : public Entity, public DashboardWidget, public RS232Device
{
public:
    RS232Console(int x, int y, int rows, int columns, int font_size);
    RS232Console(Json::Value &json_data, EntityLookup *lookup);
    
    virtual void render(Dashboard *dash);
    
    void onRS232Receive(uint8_t data);
protected:
    int x;
    int y;
    int rows;
    int columns;
    int font_size;
};

#endif
