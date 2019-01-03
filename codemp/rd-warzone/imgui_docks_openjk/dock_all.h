#ifndef DOCK_ALL
#define DOCK_ALL

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockAll : public Dock {
public:
	DockAll();
	virtual const char *label();
	virtual void imgui();
};

#endif