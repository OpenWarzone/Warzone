#ifndef DOCK_POSTPROCESS
#define DOCK_POSTPROCESS

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockPostProcess : public Dock {
public:
	DockPostProcess();
	virtual const char *label();
	virtual void imgui();

	void ClearFrame();
	void AddCvar(cvar_t *cvar, int max);
	void AddCheckBox(int ID);
	void AddInt(int ID);
	void UpdateUI();
	void UpdateCvars();
	void MakeCvarList();
};

#endif