#ifndef DOCK_MDXM
#define DOCK_MDXM

#include "../imgui_docks/dock.h"
#include <string>
#include "../imgui/imgui_dock.h"
#include "../tr_local.h"

class DockMDXM : public Dock {
public:
	model_t *mod = NULL;
	mdxmHeader_t *header = NULL;
	char lookupSurfNames[256][128]; // max MAX_BONES_RAG=256 bones, each can be 128 chars long... todo: look which defines point to max sizes
	DockMDXM(model_t *mod_);
	virtual const char *label();
	virtual void imgui();
	void imgui_mdxm_surface_vertices(mdxmSurface_t *surf);
	void imgui_mdxm_surface(mdxmSurface_t *surf);
	void imgui_mdxm_list_lods();
	void imgui_mdxm_list_surfhierarchy();
};

#endif
