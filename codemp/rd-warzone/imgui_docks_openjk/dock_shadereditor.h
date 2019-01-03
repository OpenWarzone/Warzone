#ifndef DOCK_SHADEREDITOR
#define DOCK_SHADEREDITOR

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockShaders : public Dock {
public:
	shaderProgram_t *shader = NULL;
	int currentItem = 4; // select lightall shader for start menu

	//std::string filename;
	/*ImGui::*/Dock *imguidock;
	DockShaders();
	virtual const char *label();
	virtual void imgui();

	
	void recompileShader();
	//void recompileFragmentShader();
};

#endif