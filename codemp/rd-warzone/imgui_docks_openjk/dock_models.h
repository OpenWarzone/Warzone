#ifndef DOCK_MODELS
#define DOCK_MODELS

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockModels : public Dock {
public:

	DockModels();
	virtual const char *label();
	virtual void imgui();
};

#endif