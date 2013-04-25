#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "pc_indicator.h"
#include "utils/fail.h"

#define DEFAULT_NAME "PC indicator"

PCIndicator::PCIndicator(Mcu* mcu, int x, int y, int size, int color)
    : Entity(DEFAULT_NAME)
{
    this->_mcu = mcu;
    this->_x = x;
    this->_y = y;
    this->_size = size;
    this->_color = color;
}

PCIndicator::PCIndicator(Json::Value &json_data, EntityLookup *lookup)
    : Entity(DEFAULT_NAME, json_data)
{
    this->_size = 16;
    this->_color = DashboardWidget::COLOR_BLACK;
    
    if (json_data.isMember("x")) {
        this->_x = json_data["x"].asInt();
    } else {
        fail("Member 'x' required for PCIndicator widget");
    }
    if (json_data.isMember("y")) {
        this->_y = json_data["y"].asInt();
    } else {
        fail("Member 'y' required for PCIndicator widget");
    }
    if (json_data.isMember("size")) {
        this->_size = json_data["size"].asInt();
    } else {
        fail("Member 'size' required for PCIndicator widget");
    }
    if (json_data.isMember("color")) {
        this->_color = DashboardWidget::parseColor(json_data["color"]);
    }
    
    if (json_data.isMember("mcu")) {
        const char *dev_id = json_data["mcu"].asCString();
        
        Entity *ent = lookup->lookupEntity(dev_id);
        if (!ent) {
            fail("Device '%s' not defined at this point", dev_id);
        }

        this->_mcu = dynamic_cast<Mcu *>(ent);
        if (this->_mcu == NULL) {
            fail("Device '%s' is not a MCU", dev_id);
        }
    } else {
        fail("Member 'mcu' required for PCIndicator widget");
    }
}

void PCIndicator::render(Dashboard *dash)
{
    char buf[512];
    int len = 0;
    
    int pc = this->_mcu->getPC();
    len += sprintf(buf + len, "%04x: ", 2 * pc);
    
    Symbol *sym = this->_mcu->getProgramSymbol(pc);
    if (!sym) {
        len += sprintf(buf + len, "???");
    } else {
        len += sprintf(buf + len, "%s", sym->name.c_str());
    }
    
    buf[len] = 0;
    
    dash->putMonoText(this->_x, this->_y, buf, this->_size, this->_color);
}
