#ifndef DOCK_MDXA
#define DOCK_MDXA

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockMDXA : public Dock {
public:
	model_t *mod = NULL;
	mdxaHeader_t *header = NULL;
	//char lookupSurfNames[256][128]; // max MAX_BONES_RAG=256 bones, each can be 128 chars long... todo: look which defines point to max sizes
	DockMDXA(model_t *mod_);
	virtual const char *label();
	virtual void imgui();
	//void imgui_surface_vertices(mdxmSurface_t *surf);
	//void imgui_surface(mdxmSurface_t *surf);
	//void imgui_list_lods();
	//void imgui_list_surfhierarchy();
	void imgui_skeleton();
};

#endif
