#include <stdio.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_dock.h"
#include "../imgui_docks/dock.h"
#include "imgui_openjk_default_docks.h"

// custom docks
#include "../imgui_docks_openjk/dock_shadereditor.h"
#include "../imgui_docks_openjk/dock_mapinfo.h"
#include "../imgui_docks_openjk/dock_postprocess.h"
#include "../imgui_docks_openjk/dock_perf.h"
#include "../imgui_docks_openjk/dock_controlflow.h"
#include "../imgui_docks_openjk/dock_models.h"
#include "../imgui_docks_openjk/dock_charactereditor.h"
#include "../imgui_docks_openjk/dock_all.h"
#include "imgui/imgui_api.h"
#include "../tr_local.h"

#include <list>

std::list<Dock *> imgui_quake_docks;

extern bool show_all_window;

void alignTabsDefault();

CCALL int add_dock(Dock *dock) {
	imgui_quake_docks.push_back( dock );
	return 1; // UQ1: Was missing - return 1???
}

CCALL void *imgui_get_current_dock();

CCALL int imgui_openjk_default_docks() {
	if (imgui_quake_docks.size() == 0) {
		// the last dock is seen first when pressing F2, so lets make it an useful one
#ifdef __EXPERIMETNAL_CHARACTER_EDITOR__
		imgui_quake_docks.push_back(new DockCharacterEditor());
#endif //__EXPERIMETNAL_CHARACTER_EDITOR__
		imgui_quake_docks.push_back(new DockPostProcess());
		imgui_quake_docks.push_back(new DockMapInfo());
		imgui_quake_docks.push_back(new DockPerf());
		//imgui_quake_docks.push_back(new DockControlFlow());		// postprocess handles this, but with tooltips
		//imgui_quake_docks.push_back(new DockModels());			// Don't need this for now...
		imgui_quake_docks.push_back(new DockShaders());
		//imgui_quake_docks.push_back(new DockExplorer());
		//imgui_quake_docks.push_back(new DockAnims());
		//imgui_quake_docks.push_back(new DockSound());
		//imgui_quake_docks.push_back(new DockVars());
		//imgui_quake_docks.push_back(new DockQuakeShaders());
		//imgui_quake_docks.push_back(new DockHuds());
		//imgui_quake_docks.push_back(new DockImages());
		//imgui_quake_docks.push_back(new DockEntity());
		//imgui_quake_docks.push_back(new DockConfigstrings());
		#ifndef EMSCRIPTEN
		//imgui_quake_docks.push_back(new DockDuktape());
		//imgui_quake_docks.push_back(new DockOpsys( "opsystems/matmulpoint.opsys" ));
		#endif

		if (show_all_window)
		{
			imgui_quake_docks.push_back(new DockAll());
		}
	}

	for (Dock *dock : imgui_quake_docks) {
		bool closed = true;
		if (BeginDock(dock->label(), &closed, 0, dock->cdock)) {
			if (dock->cdock == NULL)
				dock->cdock = (CDock *)imgui_get_current_dock();
			dock->imgui();
		}
		EndDock();
	}
	
	// gotta make sure that the docks were at least drawn once, otherwise cant find them via string label
	static int first = 1;
	if (first) {
		alignTabsDefault();
		first = 0;
	}

	return 1;
}