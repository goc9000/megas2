#ifndef _H_DASHBOARD_WIDGET_H
#define _H_DASHBOARD_WIDGET_H

class Dashboard;

using namespace std;

class DashboardWidget
{
public:
    virtual void render(Dashboard *dash) = 0;
};

#endif

