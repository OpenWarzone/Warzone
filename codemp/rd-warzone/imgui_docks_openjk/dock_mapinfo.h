#ifndef DOCK_MAPINFO
#define DOCK_MAPINFO

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockMapInfo : public Dock {
public:
	DockMapInfo();
	virtual const char *label();
	virtual void imgui();
};

#endif