#ifndef IMGUI_OPENJK_GLUECODE
#define IMGUI_OPENJK_GLUECODE

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

void Cvar_SetInt(cvar_t *cvar, int value);

namespace ImGui {
	extern bool Checkbox(char *label, qboolean *var);
	extern bool CvarBool(cvar_t *cvar);
	extern bool CvarInt(cvar_t *cvar);
	extern bool Cvar(cvar_t *cvar);
}

#endif