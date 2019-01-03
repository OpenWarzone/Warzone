#ifndef DOCK_CHARACTEREDITOR
#define DOCK_CHARACTEREDITOR

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockCharacterEditor : public Dock {
public:
	DockCharacterEditor();
	virtual const char *label();
	virtual void imgui();

	void UpdateUI();
};

#endif