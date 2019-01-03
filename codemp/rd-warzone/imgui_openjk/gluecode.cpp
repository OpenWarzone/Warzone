
#include "gluecode.h"

// not exactly an imgui function lol
void Cvar_SetInt(cvar_t *cvar, int value) {
	ri->Cvar_Set(cvar->name, va("%i", value));
}

namespace ImGui {

	// roses are red, qboolean is no bool
	bool Checkbox(char *label, qboolean *var) {
		bool tmp = (*var) ? true : false;
		bool ret = ImGui::Checkbox(label, &tmp);
		*var = (qboolean) tmp;
		return ret;
	}

	bool CvarBool(cvar_t *cvar) {
		bool changed = false;
		if (cvar->displayInfoSet && cvar->displayName && cvar->displayName[0]) {
			changed = ImGui::Checkbox(cvar->displayName, (qboolean *)&cvar->integer);
			if (cvar->displayInfoSet && cvar->description && cvar->description[0]) {
				ImGui::SetTooltip(cvar->description);
			}
		} else {
			changed = ImGui::Checkbox(cvar->name, (qboolean *)&cvar->integer);
			if (cvar->displayInfoSet && cvar->description && cvar->description[0]) {
				ImGui::SetTooltip(cvar->description);
			}
		}
		if (changed)
			Cvar_SetInt(cvar, cvar->integer);
		return changed;
	}
	bool CvarInt(cvar_t *cvar) {
		bool changed = false;
		if (cvar->displayInfoSet && cvar->displayName && cvar->displayName[0]) {
			
			changed = ImGui::DragInt(cvar->name, &cvar->integer, cvar->dragspeed, cvar->min, cvar->max);
			if (cvar->displayInfoSet && cvar->description && cvar->description[0]) {
				ImGui::SetTooltip(cvar->description);
			}
		} else {
			changed = ImGui::DragInt(cvar->name, &cvar->integer, cvar->dragspeed, cvar->min, cvar->max);
			if (cvar->displayInfoSet && cvar->description && cvar->description[0]) {
				ImGui::SetTooltip(cvar->description);
			}
		}
		if (changed)
			Cvar_SetInt(cvar, cvar->integer);
		return changed;
	}

	bool Cvar(cvar_t *cvar) {
		bool changed = false;
		switch (cvar->typeinfo) {
			case CvarType_Bool:
				changed = ImGui::CvarBool(cvar);
				break;

			case CvarType_Int:
				changed = ImGui::CvarInt(cvar);
				break;

			default:
				ImGui::Text("Don't know how to show cvar->typeinfo=%d, please implement it for ImGui::Cvar()", cvar->typeinfo);
		}
		return changed;
	}
}