#ifndef DOCK_CONTROLFLOW
#define DOCK_CONTROLFLOW

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockControlFlow : public Dock {
public:
	DockControlFlow();
	virtual const char *label();
	virtual void imgui();
};

#endif