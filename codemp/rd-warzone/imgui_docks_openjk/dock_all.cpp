#include "dock_all.h"
#include "../imgui_docks/dock_console.h"
#include "../imgui_openjk/gluecode.h"

DockAll::DockAll() {}

#include "../tr_debug.h"

extern bool show_all_window;
extern bool show_node_and_console_windows;

const char *DockAll::label() {
	return "All";
}

CDock *findDock(char *name) {
	for (CDock *dock : g_dock.m_docks) {
		if (strcmp(dock->label, name) == 0)
			return dock;
	}
	return NULL;
}
CDock *rootDock() {
	return g_dock.getRootDock();
}

bool undock(CDock *dock) {
	if (dock == NULL)
		return false;
	//if (dock->status != Status_::Status_Docked)
	//	return false;
	g_dock.doUndock(*dock);
	dock->status = Status_Float;
	return true;
}

bool dockLeft(CDock *from, CDock *to) {
	if (from == NULL || to == NULL)
		return false;
	g_dock.doUndock(*from);
	from->status = Status_Float;
	g_dock.doDock(*from, to, Slot_::Slot_Left);
	from->status = Status_Docked;
	return true;
}
bool dockTop(CDock *from, CDock *to) {
	//if (from == NULL || to == NULL)
	//	return false;
	g_dock.doUndock(*from);
	from->status = Status_Float;
	g_dock.doDock(*from, to, Slot_::Slot_Top);
	from->status = Status_Docked;
	return true;
}
bool dockBottom(CDock *from, CDock *to) {
	if (from == NULL || to == NULL)
		return false;
	g_dock.doUndock(*from);
	from->status = Status_Float;
	g_dock.doDock(*from, to, Slot_::Slot_Bottom);
	from->status = Status_Docked;
	return true;
}
bool dockRight(CDock *from, CDock *to) {
	if (from == NULL || to == NULL)
		return false;
	g_dock.doUndock(*from);
	from->status = Status_Float;
	g_dock.doDock(*from, to, Slot_::Slot_Right);
	from->status = Status_Docked;
	return true;
}
bool dockTab(CDock *from, CDock *to) {
	if (from == NULL || to == NULL)
		return false;
	if (to->status != Status_::Status_Docked)
		return false;
	g_dock.doUndock(*from);
	from->status = Status_Float;
	g_dock.doDock(*from, to, Slot_::Slot_Tab);
	from->status = Status_::Status_Docked;
	return true;
}

namespace ImGui {
	////bool Enum(char *name, int enumCount, ...) {
	//bool Enum(char *name, char **names, int *values) {
	//	//va_list valist;
	//	//va_start(valist, enumCount);
	//	//
	//	//for(int i=0; i<enumCount; i++) {
	//	//	char *enumName = va_arg(valist, char *);
	//	//	int enumValue = va_arg(valist, int);
	//	//	ImGui::Text("enumName=%s enumValue=%d", enumName, enumValue);
	//	//}
	//	//va_end(valist);
	//
	//	return false;
	//}

	bool Enum(Status_ *status) {
        //return ImGui::Combo("Status", (int *)&status, "Status_Docked\0Status_Float\0Status_Drag\0\0");
		//int current_item = 1;
		char *names[] = {"Docked", "Float", "Drag"};
		int bla = *status;
		bool ret = ImGui::Combo("Status",  (int *)&bla, names, 3);
		if (ret) {
			*status = (Status_)bla;
		}
		//ImGui::SameLine();
		//ImGui::Text("aaaaaaaaaaaaaaaaaaaaa ret=%d bla=%d", (int)ret, bla);
		return ret;
	}

}

void alignTabsDefault() {
	// first make sure everything is undocked
	for (CDock *dock : g_dock.m_docks) {
	//	g_dock.doUndock(*dock);
	//	dock->status = Status_Float;
		if (dock->label[0] == 0)
			continue; // dont try to undock ghost docks
		undock(dock);
	}

	CDock *all = NULL;
	
	if (show_all_window)
	{
		all = findDock("All");
	}
	else
	{// Use PostProcess as default dock...
		all = findDock("PostProcess");
	}

	CDock *console = NULL;

	if (show_node_and_console_windows)
	{
		console = findDock("Console");
		CDock *node = findDock("Node");
		dockBottom(console, all);
		dockRight(node, console);
	}

	dockTop(all, NULL);

	// dock all the rest to top
	for (CDock *dock : g_dock.m_docks) {
		//	strcpy(dock->location, "2"); // 1=left, 2=top, 3=bottom, 4=right
		//	dock->status = Status_::Status_Docked;
		//	g_dock.doDock(*dock, g_dock.getRootDock(), Slot_::Slot_Top);
		if (dock->status == Status_::Status_Docked)
			continue;

		dockTab(dock, all);
	}

	if (show_node_and_console_windows)
	{
		// until i figure out how the dock code exactly works this must be good enough...
		// basically every step gets more successive to the aimed value (todo: rewrite dock system...)
		g_dock.getRootDock()->setPosSize(g_dock.getRootDock()->pos, g_dock.getRootDock()->size);
		console->parent->size.y = 180;
		g_dock.getRootDock()->setPosSize(g_dock.getRootDock()->pos, g_dock.getRootDock()->size);
		console->parent->size.y = 180;
		g_dock.getRootDock()->setPosSize(g_dock.getRootDock()->pos, g_dock.getRootDock()->size);
		console->parent->size.y = 180;
	}
}

extern int setConsoleHeight;
void DockAll::imgui() {
	int i = 0;
	for (CDock *dock : g_dock.m_docks) {
		ImGui::PushID(dock);
		

		ImGui::Text("i=%d on=%d open=%d childs=%s,%s tabs=%s,%s parent=%s\n", i, 
					dock->active,
					dock->opened,
					dock->children[0] ? g_dock.m_docks[i]->children[0]->label : "None",
					dock->children[1] ? g_dock.m_docks[i]->children[1]->label : "None",
					dock->prev_tab    ? g_dock.m_docks[i]->prev_tab->label    : "None",
					dock->next_tab    ? g_dock.m_docks[i]->next_tab->label    : "None",
					dock->parent      ? g_dock.m_docks[i]->parent->label      : "None"
		);
		
		ImGui::PushItemWidth(80);

		ImGui::Enum(&dock->status);

		ImGui::SameLine();
		ImGui::DragFloat2("pos", (float *)&dock->pos);

		ImGui::SameLine();
		ImGui::DragFloat2("size", (float *)&dock->size);
		
		ImGui::SameLine();
		ImGui::InputText("label", dock->label, sizeof(dock->label));
		ImGui::SameLine();
		ImGui::InputText("location", dock->location, sizeof(dock->location));
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Update Location")) {
			g_dock.tryDockToStoredLocation(*dock);
		}
		if (ImGui::Button("Undock")) {
			undock(dock);
		}
		if (ImGui::Button("dockLeft(dock, rootDock())")) {
			dockLeft(dock, rootDock());
		}

		ImGui::PopID();
		i++;
	}
	
	
	CDock *all = findDock("All");
	CDock *console = findDock("Console");
	CDock *node = findDock("Node");
		
	if (ImGui::Button("dockTop(all, rootDock());"))
		dockTop(all, rootDock());

	if (ImGui::Button("dockBottom(console, rootDock());"))
		dockBottom(console, rootDock());

	if (ImGui::Button("dockRight(node, console);"))
		dockRight(node, console);

	if (ImGui::Button("alignTabsDefault()")) {
		alignTabsDefault();
	}

	if (ImGui::Button("console->parent->size.y = 180")) {
		console->parent->size.y = 180;
	}
	
	if (ImGui::Button("dockTab(julia, all)")) {
		
		CDock *julia = findDock("Julia");
		CDock *all = findDock("All");
		dockTab(julia, all);
		

	}

}