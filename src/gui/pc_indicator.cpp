#include <SDL/SDL_gfxPrimitives.h>

#include "dashboard.h"
#include "pc_indicator.h"
#include "utils/fail.h"

#define DEFAULT_NAME "PC indicator"

PCIndicator::PCIndicator(Mcu* mcu, int x, int y)
    : DashboardWidget(DEFAULT_NAME, x, y)
{
    this->_mcu = mcu;
}

PCIndicator::PCIndicator(Json::Value &json_data, EntityLookup *lookup)
    : DashboardWidget(DEFAULT_NAME, json_data)
{
    if (json_data.isMember("mcu")) {
        const char *dev_id = json_data["mcu"].asCString();
        
        Entity *ent = lookup->lookupEntity(dev_id);
        if (!ent)
            fail("Device '%s' not defined at this point", dev_id);

        _mcu = dynamic_cast<Mcu *>(ent);
        if (_mcu == NULL)
            fail("Device '%s' is not a MCU", dev_id);
    } else {
        fail("Member 'mcu' required for PCIndicator widget");
    }
}

void PCIndicator::render(Dashboard *dash, SDLColor color, SDLColor bg_color,
    int font_size)
{
    char buf[512];
    int len = 0;
    
    int pc = _mcu->getPC();
    len += sprintf(buf + len, "%04x: ", 2 * pc);
    
    Symbol *sym = _mcu->getProgramSymbol(pc);
    if (!sym)
        len += sprintf(buf + len, "???");
    else
        len += sprintf(buf + len, "%s", sym->name.c_str());
    
    buf[len] = 0;
    
    dash->putMonoText(x, y, buf, font_size, color);
}
