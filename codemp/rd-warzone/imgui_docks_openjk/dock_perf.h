#ifndef DOCK_PERF
#define DOCK_PERF

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockPerf : public Dock {
public:
	DockPerf();
	virtual const char *label();
	virtual void imgui();
};

#endif