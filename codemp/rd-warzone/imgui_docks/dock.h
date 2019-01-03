#ifndef DOCK_H
#define DOCK_H

#include "imgui/imgui_dock.h"

// creating a new dock: add it to imgui_api.cpp line 457, like: docks.push_back(new DockExplorer());
#ifdef __cplusplus

class Dock {
public:
	CDock *cdock = NULL;
	virtual const char *label();
	virtual void imgui();
};

#endif

#endif