#include "dock.h"
#include "../imgui/include_imgui.h"

// creating a new dock: add it to imgui_api.cpp line 457, like: docks.push_back(new DockExplorer());

const char *Dock::label() {
	return "implement Dock::label()";
}

void Dock::imgui() {
	ImGui::Button("implement Dock::imgui()");
}