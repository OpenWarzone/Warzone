#include "imgui_dock.h"
#include "imgui_c_utils.h"
#include "MagicFile.cpp/magicfile.h"

//#include <kung/opsys/opsystem_repl_julia.h>
//#include <kung/include_utils.h>
//#include <kung/imgui/dock/dock_repl.h>
//#include <kung/include_console.h>
//#include <kung/include_ccall.h>
//CCALL long	FS_ReadFile(const char *qpath, void **buffer);

DockContext g_dock;

#define ASSERT(x) IM_ASSERT(x)


char *SlotToString(Slot_ slot) {
	switch (slot) {
		case Slot_Left:	   return "Slot_Left";
		case Slot_Right:   return "Slot_Right";
		case Slot_Top:	   return "Slot_Top";
		case Slot_Bottom:  return "Slot_Bottom";
		case Slot_Tab:	   return "Slot_Tab";
		case Slot_Float:   return "Slot_Float";
		case Slot_None:	   return "Slot_None";
	}
	return "unknown slot enum";
}

CCALL int imgui_log(char *format, ...);

//namespace ImGui
//{

#if 0
	CDock::Dock()
		: id(0)
		, next_tab(nullptr)
		, prev_tab(nullptr)
		, parent(nullptr)
		, pos(0, 0)
		, size(-1, -1)
		, active(true)
		, status(Status_Float)
		, opened(false)
	{
		location[0] = 0;
		children[0] = children[1] = nullptr;
		last_frame = 0;
		invalid_frames = 0;
	}


	CDock::~Dock() { /*MemFree(label);*/ /*static buffer now*/ }

#endif

	ImVec2 CDock::getMinSize() const
	{
		if (!children[0]) return ImVec2(16, 16 + ImGui::GetTextLineHeightWithSpacing());

		ImVec2 s0 = children[0]->getMinSize();
		ImVec2 s1 = children[1]->getMinSize();
		return isHorizontal() ? ImVec2(s0.x + s1.x, ImMax(s0.y, s1.y))
								: ImVec2(ImMax(s0.x, s1.x), s0.y + s1.y);
	}


	bool CDock::isHorizontal() const { return children[0]->pos.x < children[1]->pos.x; }


	void CDock::setParent(CDock* dock)
	{
		parent = dock;
		for (CDock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->parent = dock;
		for (CDock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->parent = dock;
	}


	CDock& CDock::getSibling()
	{
		IM_ASSERT(parent);
		if (parent->children[0] == &getFirstTab()) return *parent->children[1];
		return *parent->children[0];
	}


	CDock& CDock::getFirstTab()
	{
		CDock* tmp = this;
		while (tmp->prev_tab) tmp = tmp->prev_tab;
		return *tmp;
	}


	void RemoveEmptyDocks() {

	}

	void CDock::setActive()
	{
		//log("Make Active: %ld last frame=%d invalid frames=%d\n", this, this->last_frame, this->invalid_frames);
		active = true;
		for (CDock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->active = false;
		for (CDock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->active = false;
	}


	bool CDock::isContainer() const { return children[0] != nullptr; }


	void CDock::setChildrenPosSize(const ImVec2& _pos, const ImVec2& _size)
	{
		ImVec2 s = children[0]->size;
		if (isHorizontal())
		{
			s.y = _size.y;
			s.x = (float)int(
				_size.x * children[0]->size.x / (children[0]->size.x + children[1]->size.x));
			if (s.x < children[0]->getMinSize().x)
			{
				s.x = children[0]->getMinSize().x;
			}
			else if (_size.x - s.x < children[1]->getMinSize().x)
			{
				s.x = _size.x - children[1]->getMinSize().x;
			}
			children[0]->setPosSize(_pos, s);

			s.x = _size.x - children[0]->size.x;
			ImVec2 p = _pos;
			p.x += children[0]->size.x;
			children[1]->setPosSize(p, s);
		}
		else
		{
			s.x = _size.x;
			s.y = (float)int(
				_size.y * children[0]->size.y / (children[0]->size.y + children[1]->size.y));
			if (s.y < children[0]->getMinSize().y)
			{
				s.y = children[0]->getMinSize().y;
			}
			else if (_size.y - s.y < children[1]->getMinSize().y)
			{
				s.y = _size.y - children[1]->getMinSize().y;
			}
			children[0]->setPosSize(_pos, s);

			s.y = _size.y - children[0]->size.y;
			ImVec2 p = _pos;
			p.y += children[0]->size.y;
			children[1]->setPosSize(p, s);
		}
	}


	void CDock::setPosSize(const ImVec2& _pos, const ImVec2& _size)
	{

		//imgui_log("setPosSize(pos=%d,%d size=%d,%d)\n", (int)_pos.x, (int)_pos.y, (int)_size.x, (int)_size.y);

		size = _size;
		pos = _pos;
		for (CDock* tmp = prev_tab; tmp; tmp = tmp->prev_tab)
		{
			tmp->size = _size;
			tmp->pos = _pos;
		}
		for (CDock* tmp = next_tab; tmp; tmp = tmp->next_tab)
		{
			tmp->size = _size;
			tmp->pos = _pos;
		}

		if (!isContainer()) return;
		setChildrenPosSize(_pos, _size);
	}





DockContext::~DockContext() {}

CDock& DockContext::getDock(const char* label, bool opened)
{
	ImU32 id = ImHash(label, 0);
	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (m_docks[i]->id == id) return *m_docks[i];
		//if (m_docks[i]->renamed) return *m_docks[i];
	}

	CDock* new_dock = (CDock *) ImGui::MemAlloc(sizeof(CDock));
	memset(new_dock, 0, sizeof(CDock));
	IM_PLACEMENT_NEW(new_dock) CDock();
	m_docks.push_back(new_dock);
	strlcpy(new_dock->label, label, sizeof new_dock->label);
	//new_dock->label = ImStrdup(label);
	//IM_ASSERT(new_dock->label);
	new_dock->id = id;
	new_dock->setActive();
	new_dock->status = Status_Float;
	new_dock->pos = ImVec2(0, 0);
	new_dock->size = ImGui::GetIO().DisplaySize;
	new_dock->opened = opened;
	new_dock->first = 1;
	new_dock->last_frame = 0;
	new_dock->invalid_frames = 0;
	new_dock->location[0] = 0;
	return *new_dock;
}


CCALL CDock *imgui_new_dock(const char *label, bool opened, float pos_x, float pos_y, float size_x, float size_y, char *location, int status) {
	DockContext *dockcontext = &g_dock;
	CDock* new_dock = (CDock *) ImGui::MemAlloc(sizeof(CDock));
	memset(new_dock, 0, sizeof(CDock));
	//IM_PLACEMENT_NEW(new_dock) Dock();
	// once I support Desktop's this needs to be specific ofc
	dockcontext->m_docks.push_back(new_dock);
	strlcpy(new_dock->label, label, sizeof new_dock->label);
	new_dock->id = (ImU32)new_dock; // we dont care about ImHash(label) == dock.id...
	new_dock->setActive();
	new_dock->opened = opened;
	new_dock->first = 1;
	new_dock->last_frame = 0;
	new_dock->invalid_frames = 0;
	new_dock->pos = ImVec2(pos_x, pos_y);
	new_dock->size = ImVec2(size_x, size_y);
	new_dock->status = Status_::Status_Float; // tryDockToStoredLocation() returns instantly on status==Status_Docked
	memcpy(new_dock->location, location, sizeof new_dock->location);
	if (status == Status_Docked)
		dockcontext->tryDockToStoredLocation(*new_dock);
	return new_dock;
}

CCALL int imgui_get_dock_count(int id) {
	return g_dock.m_docks.size();
}

// temporary filthy shit code, which is going to be replaced at some point with full julia ...
CCALL char *imgui_get_dock(int id) {
	char *buf = (char *)malloc(4096); // free in Julia
	char *startbuf = buf;
	DockContext *dockcontext = &g_dock;

	CDock *dock = dockcontext->getDockByIndex(id);

	if (! dock)
		return strdup("free me");
	//sprintf(buf, "docks %d\n\n", dockcontext->m_docks.size());
	
	dockcontext->fillLocation(*dock);

	int nextpos = 0;

	buf += sprintf(buf, "Dict(\n", id);
	buf += sprintf(buf, "	\"index\"    => %d,\n", id);
	buf += sprintf(buf, "	\"label\"    => \"%s\",\n", dock->parent ? (dock->label[0] == '\0' ? "DOCK" : dock->label) : "ROOT"),
	buf += sprintf(buf, "	\"x\"        => %d,\n", (int)dock->pos.x);
	buf += sprintf(buf, "	\"y\"        => %d,\n", (int)dock->pos.y);
	buf += sprintf(buf, "	\"size_x\"   => %d,\n", (int)dock->size.x);
	buf += sprintf(buf, "	\"size_y\"   => %d,\n", (int)dock->size.y);
	buf += sprintf(buf, "	\"status\"   => %d,\n", (int)dock->status);
	buf += sprintf(buf, "	\"active\"   => %d,\n", dock->active ? 1 : 0);
	buf += sprintf(buf, "	\"opened\"   => %d,\n", dock->opened ? 1 : 0);
	buf += sprintf(buf, "	\"location\" => \"%s\",\n", strlen(dock->location) ? dock->location : "-1");
	buf += sprintf(buf, "	\"child0\"   => %d,\n", dockcontext->getDockIndex(dock->children[0]));
	buf += sprintf(buf, "	\"child1\"   => %d,\n", dockcontext->getDockIndex(dock->children[1]));
	buf += sprintf(buf, "	\"prev_tab\" => %d,\n", dockcontext->getDockIndex(dock->prev_tab));
	buf += sprintf(buf, "	\"next_tab\" => %d,\n", dockcontext->getDockIndex(dock->next_tab));
	buf += sprintf(buf, "	\"parent\"   => %d \n", dockcontext->getDockIndex(dock->parent));
	buf += sprintf(buf, ")\n");

	return startbuf;
}

void DockContext::putInBackground()
{
	ImGuiWindow* win = ImGui::GetCurrentWindow();
	ImGuiContext& g = *GImGui;
	if (g.Windows[0] == win) return;

	for (int i = 0; i < g.Windows.Size; i++)
	{
		if (g.Windows[i] == win)
		{
			for (int j = i - 1; j >= 0; --j)
			{
				g.Windows[j + 1] = g.Windows[j];
			}
			g.Windows[0] = win;
			break;
		}
	}
}


void DockContext::splits()
{
	if (ImGui::GetFrameCount() == m_last_frame) return;
	m_last_frame = ImGui::GetFrameCount();

	putInBackground();

	ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
	ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGuiIO& io = ImGui::GetIO();
	for (int i = 0; i < m_docks.size(); ++i)
	{
		CDock& dock = *m_docks[i];
		if (!dock.isContainer()) continue;

		ImGui::PushID(i);
		if (!ImGui::IsMouseDown(0)) dock.status = Status_Docked;

		//ImVec2 size = dock.children[0]->size;
		ImVec2 dsize(0, 0);
		ImGui::SetCursorScreenPos(dock.children[1]->pos);
		ImVec2 min_size0 = dock.children[0]->getMinSize();
		ImVec2 min_size1 = dock.children[1]->getMinSize();
		if (dock.isHorizontal())
		{
			ImGui::InvisibleButton("split", ImVec2(3, dock.size.y));
			if (dock.status == Status_Dragged) dsize.x = io.MouseDelta.x;
			dsize.x = -ImMin(-dsize.x, dock.children[0]->size.x - min_size0.x);
			dsize.x = ImMin(dsize.x, dock.children[1]->size.x - min_size1.x);
		}
		else
		{
			ImGui::InvisibleButton("split", ImVec2(dock.size.x, 3));
			if (dock.status == Status_Dragged) dsize.y = io.MouseDelta.y;
			dsize.y = -ImMin(-dsize.y, dock.children[0]->size.y - min_size0.y);
			dsize.y = ImMin(dsize.y, dock.children[1]->size.y - min_size1.y);
		}
		ImVec2 new_size0 = dock.children[0]->size + dsize;
		ImVec2 new_size1 = dock.children[1]->size - dsize;
		ImVec2 new_pos1 = dock.children[1]->pos + dsize;
		dock.children[0]->setPosSize(dock.children[0]->pos, new_size0);
		dock.children[1]->setPosSize(new_pos1, new_size1);

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
		{
			dock.status = Status_Dragged;
		}

		draw_list->AddRectFilled(
			ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemHovered() ? color_hovered : color);
		ImGui::PopID();
	}
}



std::list<CDock *>invalidDocks;

#define Dock CDock

void EliminateEmptyDocks() {
	auto ite = g_dock.m_docks.begin();
	//for (int i=0; i<g_dock.m_docks.size(); i++) {
	int i=0;
	while (ite != g_dock.m_docks.end()) {
		//auto dock = g_dock.m_docks[i];
		auto dock = *ite;
		// delete element while iteration over vector
		if (dock->invalid_frames == 3 /*&& strcmp(dock->label, "ROOT")*/) {
			ite = g_dock.m_docks.erase(ite);
			continue;
		}
		//if (dock->invalid_frames == 3 && strcmp(dock->label, "ROOT")) {
		//
		//	ImGui::PushID(dock);
		//	if (ImGui::Button("Create Dock")) {
		//		auto repl = new REPLJulia();
		//		strcpy(repl->name, dock->label);
		//		repl->dock = dock;
		//		//g_dock.tryDockToStoredLocation(*dock);
		//	}
		//	ImGui::PopID();
		//}
		ite++;
		i++;
	}
}
void DockContext::checkNonexistent()
{
	invalidDocks.clear();
	
	EliminateEmptyDocks();

	int frame_limit = ImMax(0, ImGui::GetFrameCount() - 2);
	for (Dock* dock : m_docks)
	{

		//if (dock->renamed) continue;

		if (dock->isContainer()) continue;
		//if (dock->status == Status_Float) continue;
		if (dock->last_frame < frame_limit)
		{
			++dock->invalid_frames;
			if (dock->invalid_frames > 2)
			{
				invalidDocks.push_back(dock);


#if 0
				const char *extension = get_filename_ext(dock->label);
				if (strcmp(extension, "jl") == 0) {
					auto repl = new REPLJulia();
					strcpy(repl->name, dock->label);
					repl->dock = dock;
					continue;
				}
#endif
				doUndock(*dock);
				dock->status = Status_Float;
			}
			return;
		}
		dock->invalid_frames = 0;
	}
}


void DockContext::beginPanel()
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
								ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
								ImGuiWindowFlags_NoScrollWithMouse | /*ImGuiWindowFlags_ShowBorders | */
								ImGuiWindowFlags_NoBringToFrontOnFocus;
	Dock* root = getRootDock();
	if (root)
	{
		ImGui::SetNextWindowPos(root->pos);
		ImGui::SetNextWindowSize(root->size);
	}
	else
	{
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::Begin("###DockPanel", nullptr, flags);
	splits();

	// this causes a loop somehow when creating new juliarepl windows the moment we check for existence
	//checkNonexistent();
}


void DockContext::endPanel()
{
	ImGui::End();
	ImGui::PopStyleVar();
}

// Doesn't use input??
Dock* DockContext::getDockAt(const ImVec2& /*pos*/) const
{
	for (int i = 0; i < m_docks.size(); ++i)
	{
		Dock& dock = *m_docks[i];
		if (dock.isContainer()) continue;
		if (dock.status != Status_Docked) continue;
		if (ImGui::IsMouseHoveringRect(dock.pos, dock.pos + dock.size, false))
		{
			return &dock;
		}
	}

	return nullptr;
}


ImRect DockContext::getDockedRect(const ImRect& rect, Slot_ dock_slot)
{
	ImVec2 half_size = rect.GetSize() * 0.5f;
	switch (dock_slot)
	{
		default: return rect;
		case Slot_Top: return ImRect(rect.Min, rect.Min + ImVec2(rect.Max.x, half_size.y));
		case Slot_Right: return ImRect(rect.Min + ImVec2(half_size.x, 0), rect.Max);
		case Slot_Bottom: return ImRect(rect.Min + ImVec2(0, half_size.y), rect.Max);
		case Slot_Left: return ImRect(rect.Min, rect.Min + ImVec2(half_size.x, rect.Max.y));
	}
}


ImRect DockContext::getSlotRect(ImRect parent_rect, Slot_ dock_slot)
{
	ImVec2 size = parent_rect.Max - parent_rect.Min;
	ImVec2 center = parent_rect.Min + size * 0.5f;
	switch (dock_slot)
	{
		default: return ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));
		case Slot_Top: return ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));
		case Slot_Right: return ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));
		case Slot_Bottom: return ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));
		case Slot_Left: return ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
	}
}


ImRect DockContext::getSlotRectOnBorder(ImRect parent_rect, Slot_ dock_slot)
{
	ImVec2 size = parent_rect.Max - parent_rect.Min;
	ImVec2 center = parent_rect.Min + size * 0.5f;
	switch (dock_slot)
	{
		case Slot_Top:
			return ImRect(ImVec2(center.x - 20, parent_rect.Min.y + 10),
				ImVec2(center.x + 20, parent_rect.Min.y + 30));
		case Slot_Left:
			return ImRect(ImVec2(parent_rect.Min.x + 10, center.y - 20),
				ImVec2(parent_rect.Min.x + 30, center.y + 20));
		case Slot_Bottom:
			return ImRect(ImVec2(center.x - 20, parent_rect.Max.y - 30),
				ImVec2(center.x + 20, parent_rect.Max.y - 10));
		case Slot_Right:
			return ImRect(ImVec2(parent_rect.Max.x - 30, center.y - 20),
				ImVec2(parent_rect.Max.x - 10, center.y + 20));
		default: ASSERT(false);
	}
	IM_ASSERT(false);
	return ImRect();
}


Dock* DockContext::getRootDock()
{
	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (!m_docks[i]->parent &&
			(m_docks[i]->status == Status_Docked || m_docks[i]->children[0]))
		{
			return m_docks[i];
		}
	}
	return nullptr;
}


bool DockContext::dockSlots(Dock& dock, Dock* dest_dock, const ImRect& rect, bool on_border)
{
	ImDrawList* canvas = ImGui::GetWindowDrawList();
	ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
	ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
	ImVec2 mouse_pos = ImGui::GetIO().MousePos;
	for (int i = 0; i < (on_border ? 4 : 5); ++i)
	{
		ImRect r =
			on_border ? getSlotRectOnBorder(rect, (Slot_)i) : getSlotRect(rect, (Slot_)i);
		bool hovered = r.Contains(mouse_pos);
			
		canvas->AddRectFilled(r.Min, r.Max, hovered ? color_hovered : color);
		if (!hovered) continue;

		if (!ImGui::IsMouseDown(0))
		{
			doDock(dock, dest_dock ? dest_dock : getRootDock(), (Slot_)i);
			return true;
		}
		ImRect docked_rect = getDockedRect(rect, (Slot_)i);
		canvas->AddRectFilled(docked_rect.Min, docked_rect.Max, ImGui::GetColorU32(ImGuiCol_Button));
	}
	return false;
}


void DockContext::handleDrag(Dock& dock)
{
	Dock* dest_dock = getDockAt(ImGui::GetIO().MousePos);

	ImGui::Begin("##Overlay",
		NULL,
		ImVec2(0, 0),
		0.f,
		ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_AlwaysAutoResize);
	ImDrawList* canvas = ImGui::GetWindowDrawList();

	canvas->PushClipRectFullScreen();

	ImU32 docked_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
	docked_color = (docked_color & 0x00ffFFFF) | 0x80000000;
	dock.pos = ImGui::GetIO().MousePos - m_drag_offset;
	if (dest_dock)
	{
		if (dockSlots(dock,
				dest_dock,
				ImRect(dest_dock->pos, dest_dock->pos + dest_dock->size),
				false))
		{
			canvas->PopClipRect();
			ImGui::End();
			return;
		}
	}
	if (dockSlots(dock, nullptr, ImRect(ImVec2(0, 0), ImGui::GetIO().DisplaySize), true))
	{
		canvas->PopClipRect();
		ImGui::End();
		return;
	}
	canvas->AddRectFilled(dock.pos, dock.pos + dock.size, docked_color);
	canvas->PopClipRect();

	if (!ImGui::IsMouseDown(0))
	{
		dock.status = Status_Float;
		dock.location[0] = 0;
		dock.setActive();
	}

	ImGui::End();
}


void DockContext::fillLocation(Dock& dock)
{
	if (dock.status == Status_Float) return;
	char* c = dock.location;
	Dock* tmp = &dock;
	while (tmp->parent)
	{
		*c = getLocationCode(tmp);
		tmp = tmp->parent;
		++c;
	}
	*c = 0;
}


void DockContext::doUndock(Dock& dock)
{
	if (dock.prev_tab)
		dock.prev_tab->setActive();
	else if (dock.next_tab)
		dock.next_tab->setActive();
	else
		dock.active = false;
	Dock* container = dock.parent;

	if (container)
	{
		Dock& sibling = dock.getSibling();
		if (container->children[0] == &dock)
		{
			container->children[0] = dock.next_tab;
		}
		else if (container->children[1] == &dock)
		{
			container->children[1] = dock.next_tab;
		}

		bool remove_container = !container->children[0] || !container->children[1];
		if (remove_container)
		{
			if (container->parent)
			{
				Dock*& child = container->parent->children[0] == container
									? container->parent->children[0]
									: container->parent->children[1];
				child = &sibling;
				child->setPosSize(container->pos, container->size);
				child->setParent(container->parent);
			}
			else
			{
				if (container->children[0])
				{
					container->children[0]->setParent(nullptr);
					container->children[0]->setPosSize(container->pos, container->size);
				}
				if (container->children[1])
				{
					container->children[1]->setParent(nullptr);
					container->children[1]->setPosSize(container->pos, container->size);
				}
			}
			for (int i = 0; i < m_docks.size(); ++i)
			{
				if (m_docks[i] == container)
				{
					m_docks.erase(m_docks.begin() + i);
					break;
				}
			}
			//container->~Dock();
			ImGui::MemFree(container);
		}
	}
	if (dock.prev_tab) dock.prev_tab->next_tab = dock.next_tab;
	if (dock.next_tab) dock.next_tab->prev_tab = dock.prev_tab;
	dock.parent = nullptr;
	dock.prev_tab = dock.next_tab = nullptr;
}


void DockContext::drawTabbarListButton(Dock& dock)
{
	if (!dock.next_tab) return;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	if (ImGui::InvisibleButton("list", ImVec2(16, 16)))
	{
		ImGui::OpenPopup("tab_list_popup");
	}
	if (ImGui::BeginPopup("tab_list_popup"))
	{
		Dock* tmp = &dock;
		while (tmp)
		{
			bool dummy = false;
			ImGui::PushID(tmp);
			if (ImGui::Selectable(tmp->label, &dummy))
			{
				tmp->setActive();
			}
			ImGui::PopID();
			tmp = tmp->next_tab;
		}
		ImGui::EndPopup();
	}

	bool hovered = ImGui::IsItemHovered();
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	ImVec2 center = (min + max) * 0.5f;
	ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
	ImU32 color_active = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
	draw_list->AddRectFilled(ImVec2(center.x - 4, min.y + 3),
		ImVec2(center.x + 4, min.y + 5),
		hovered ? color_active : text_color);
	draw_list->AddTriangleFilled(ImVec2(center.x - 4, min.y + 7),
		ImVec2(center.x + 4, min.y + 7),
		ImVec2(center.x, min.y + 12),
		hovered ? color_active : text_color);
}


bool DockContext::tabbar(Dock& dock, bool close_button)
{
	float tabbar_height = 2 * ImGui::GetTextLineHeightWithSpacing();
	ImVec2 size(dock.size.x, tabbar_height);
	bool tab_closed = false;

	ImGui::SetCursorScreenPos(dock.pos);
	char tmp[20];
	ImFormatString(tmp, IM_ARRAYSIZE(tmp), "tabs%d", (int)dock.id);
	if (ImGui::BeginChild(tmp, size, true))
	{
		Dock* dock_tab = &dock;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImU32 color               = ImGui::GetColorU32(ImGuiCol_FrameBg);
		ImU32 color_active        = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
		ImU32 color_hovered       = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
		ImU32 text_color          = ImGui::GetColorU32(ImGuiCol_Text);
		ImU32 text_color_disabled = ImGui::GetColorU32(ImGuiCol_TextDisabled);
		float line_height = ImGui::GetTextLineHeightWithSpacing();
		float tab_base;

		drawTabbarListButton(dock);
		char tabtext[256];

		while (dock_tab)
		{
			ImGui::SameLine(0, 15);
			
			ImGui::PushID(dock_tab);

			const char* text_end = ImGui::FindRenderedTextEnd(dock_tab->label);
			ImVec2 size(ImGui::CalcTextSize(dock_tab->label, text_end).x, line_height);
			if (ImGui::InvisibleButton(dock_tab->label, size))
			{
				dock_tab->setActive();
			}

			if (ImGui::IsItemActive() && ImGui::IsMouseDragging())
			{
				m_drag_offset = ImGui::GetMousePos() - dock_tab->pos;
				doUndock(*dock_tab);
				dock_tab->status = Status_Dragged;
			}

			bool hovered = ImGui::IsItemHovered();
			ImVec2 pos = ImGui::GetItemRectMin();
			size.x += 20 + ImGui::GetStyle().ItemSpacing.x;
				
			tab_base = pos.y;

			draw_list->AddRectFilled(pos+ImVec2(-8.0f, 0.0),
										pos+size,
										hovered ? color_hovered : (dock_tab->active ? color_active : color));



			if (dock_tab->isDockChanged)
				snprintf(tabtext, sizeof tabtext, "%s*", dock_tab->label);
			else
				strlcpy(tabtext, dock_tab->label, sizeof tabtext);
				
			draw_list->AddText(pos, text_color, tabtext, NULL);
			//draw_list->AddText(pos, text_color, dock_tab->label, text_end);


			close_button = 1; // force close button on all
			// I want close buttons to work on non-active ones aswell lol
			if (close_button) {
                ImGui::SameLine();
				ImGui::PushID(dock_tab);
                tab_closed = ImGui::InvisibleButton("close", ImVec2(16, 16));
				ImGui::PopID();
				if (tab_closed) {
					//log("close button pressed for %s\n", dock_tab->label);
					dock_tab->closeButtonPressed = 1;
				}
				// fuckin shit, for some reason when i have six tabs: OPS, /julia/a.jl, /julia/b.jl and 3 from julia, "im a", "im b" and "im c"
				// and i click close on "im c", then its disable /julia.jl.... hence just ignore the builtin "close mechanism" and simply rely on my ->closeButtonPressed
				tab_closed = false;
				// Only draw close button as long we didn't request a close, as UI indicator helper kinda
				//if ( ! dock_tab->closeButtonPressed)
				{
					ImVec2 center = ((ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f);
					draw_list->AddLine( center + ImVec2(-3.5f, -3.5f), center + ImVec2(3.5f, 3.5f), dock_tab->active ? text_color : text_color_disabled);
					draw_list->AddLine( center + ImVec2(3.5f, -3.5f), center + ImVec2(-3.5f, 3.5f), dock_tab->active ? text_color : text_color_disabled);
				}
            }
			
			ImGui::PopID();
			dock_tab = dock_tab->next_tab;
		}
		ImVec2 cp(dock.pos.x, tab_base + line_height);
		draw_list->AddLine(cp, cp + ImVec2(dock.size.x, 0), color);
	}
	ImGui::EndChild();
	return tab_closed;
}


void DockContext::setDockPosSize(Dock& dest, Dock& dock, Slot_ dock_slot, Dock& container)
{
	IM_ASSERT(!dock.prev_tab);
	IM_ASSERT(!dock.next_tab);
	IM_ASSERT(!dock.children[0]);
	IM_ASSERT(!dock.children[1]);

	dest.pos = container.pos;
	dest.size = container.size;
	dock.pos = container.pos;
	dock.size = container.size;

	switch (dock_slot)
	{
		case Slot_Bottom:
			dest.size.y *= 0.5f;
			dock.size.y *= 0.5f;
			dock.pos.y += dest.size.y;
			break;
		case Slot_Right:
			dest.size.x *= 0.5f;
			dock.size.x *= 0.5f;
			dock.pos.x += dest.size.x;
			break;
		case Slot_Left:
			dest.size.x *= 0.5f;
			dock.size.x *= 0.5f;
			dest.pos.x += dock.size.x;
			break;
		case Slot_Top:
			dest.size.y *= 0.5f;
			dock.size.y *= 0.5f;
			dest.pos.y += dock.size.y;
			break;
		default: IM_ASSERT(false); break;
	}
	dest.setPosSize(dest.pos, dest.size);

	if (container.children[1]->pos.x < container.children[0]->pos.x ||
		container.children[1]->pos.y < container.children[0]->pos.y)
	{
		Dock* tmp = container.children[0];
		container.children[0] = container.children[1];
		container.children[1] = tmp;
	}
}


void DockContext::doDock(Dock& dock, Dock* dest, Slot_ dock_slot)
{

	//imgui_log("doDock(dock=%s, dest=%s, dock_slot=%s)\n", dock.label, dest==NULL?"NULL":dest->label, SlotToString(dock_slot));

	IM_ASSERT(!dock.parent);
	if (!dest)
	{
		dock.status = Status_Docked;
		dock.setPosSize(ImVec2(0, 0), ImGui::GetIO().DisplaySize);
	}
	else if (dock_slot == Slot_Tab)
	{
		Dock* tmp = dest;
		while (tmp->next_tab)
		{
			tmp = tmp->next_tab;
		}

		tmp->next_tab = &dock;
		dock.prev_tab = tmp;
		dock.size = tmp->size;
		dock.pos = tmp->pos;
		dock.parent = dest->parent;
		dock.status = Status_Docked;
	}
	else if (dock_slot == Slot_None)
	{
		dock.status = Status_Float;
	}
	else
	{
		Dock* container = (Dock*)ImGui::MemAlloc(sizeof(Dock));
		memset(container, 0, sizeof(Dock));
		IM_PLACEMENT_NEW(container) Dock();
		m_docks.push_back(container);
		container->children[0] = &dest->getFirstTab();
		container->children[1] = &dock;
		container->next_tab = nullptr;
		container->prev_tab = nullptr;
		container->parent = dest->parent;
		container->size = dest->size;
		container->pos = dest->pos;
		container->status = Status_Docked;
		strlcpy(container->label, "", sizeof container->label);
		//container->label = ImStrdup("");

		if (!dest->parent)
		{
		}
		else if (&dest->getFirstTab() == dest->parent->children[0])
		{
			dest->parent->children[0] = container;
		}
		else
		{
			dest->parent->children[1] = container;
		}

		dest->setParent(container);
		dock.parent = container;
		dock.status = Status_Docked;

		setDockPosSize(*dest, dock, dock_slot, *container);
	}
	dock.setActive();
}


void DockContext::rootDock(const ImVec2& pos, const ImVec2& size)
{
	Dock* root = getRootDock();
	if (!root) return;

	ImVec2 min_size = root->getMinSize();
	ImVec2 requested_size = size;
	root->setPosSize(pos, ImMax(min_size, requested_size));
}


void DockContext::setDockActive()
{
	IM_ASSERT(m_current);
	if (m_current) m_current->setActive();
}


Slot_ DockContext::getSlotFromLocationCode(char code)
{
	switch (code)
	{
		case '1': return Slot_Left;
		case '2': return Slot_Top;
		case '3': return Slot_Bottom;
		default: return Slot_Right;
	}
}


char DockContext::getLocationCode(Dock* dock)
{
	if (!dock) return '0';

	if (dock->parent->isHorizontal())
	{
		if (dock->pos.x < dock->parent->children[0]->pos.x) return '1';
		if (dock->pos.x < dock->parent->children[1]->pos.x) return '1';
		return '0';
	}
	else
	{
		if (dock->pos.y < dock->parent->children[0]->pos.y) return '2';
		if (dock->pos.y < dock->parent->children[1]->pos.y) return '2';
		return '3';
	}
}


void DockContext::tryDockToStoredLocation(Dock& dock)
{
	if (dock.status == Status_Docked) return;
	if (dock.location[0] == 0) return;
		
	Dock* tmp = getRootDock();
	if (!tmp) return;

	Dock* prev = nullptr;
	char* c = dock.location + strlen(dock.location) - 1;
	while (c >= dock.location && tmp)
	{
		prev = tmp;
		tmp = *c == getLocationCode(tmp->children[0]) ? tmp->children[0] : tmp->children[1];
		if(tmp) --c;
	}
	if (tmp && tmp->children[0]) tmp = tmp->parent;
	doDock(dock, tmp ? tmp : prev, tmp && !tmp->children[0] ? Slot_Tab : getSlotFromLocationCode(*c));
}


bool DockContext::begin(const char* label, bool* opened, ImGuiWindowFlags extra_flags, Dock *useThisDock)
{
	Dock *dock = NULL;
	
	char tmp[256];
	char tmp_floatwindow[256];
	


	if (useThisDock) {
		dock = useThisDock;
		// kung foo man: to allow docks with the same name, I set the dock->id to the pointer address of the new dock
		// that makes sure that a new dock with useThisDock==NULL won't find our useThisDock instance, because the id is calculated from name (normally)
		dock->id = (int)dock;
		// dock->setActive(); // fucks up everything in on top of each other kinda lol
	} else {
		dock = &getDock(label, !opened || *opened);
	}


	// kung foo man: I need multiple panels with the same name, which also don't loose focus when I rename the window label
	// Hence I use the dock pointer as ID, so ImGui has a good "ID" to keep track of the state of the windows, even after renaming the label dynamically
	// Since the "label" is "unstable", ONLY use the pointer as ID
	snprintf(tmp, sizeof tmp, "%zx", (size_t)dock);
	snprintf(tmp_floatwindow, sizeof tmp_floatwindow, "%s###%zx", label, (size_t)dock); // ### = only last part as id, ## = label and ptr as id

	if (!dock->opened && (!opened || *opened)) tryDockToStoredLocation(*dock);
	dock->last_frame = ImGui::GetFrameCount();
	if (strcmp(dock->label, label) != 0)
	{
		//MemFree(dock->label);
		//dock->label = ImStrdup(label);
		strlcpy(dock->label, label, sizeof dock->label);
	}

	m_end_action = EndAction_None;

	if (dock->first && opened) *opened = dock->opened;
	dock->first = 0;
	if (opened && !*opened)
	{
		if (dock->status != Status_Float)
		{
			fillLocation(*dock);
			doUndock(*dock);
			dock->status = Status_Float;
		}
		dock->opened = false;
		return false;
	}
	dock->opened = true;

	m_end_action = EndAction_Panel;
	beginPanel();

	m_current = dock;
	if (dock->status == Status_Dragged) handleDrag(*dock);

	bool is_float = dock->status == Status_Float;

	if (is_float)
	{
		ImGui::SetNextWindowPos(dock->pos);
		ImGui::SetNextWindowSize(dock->size);
		bool ret = ImGui::Begin(tmp_floatwindow,
			opened,
			dock->size,
			-1.0f,
			ImGuiWindowFlags_NoCollapse /*| ImGuiWindowFlags_ShowBorders*/ | extra_flags);

		// imgui sets to false when user pressed CloseButton()
		if (*opened == false)
			dock->closeButtonPressed = true;

		m_end_action = EndAction_End;
		dock->pos = ImGui::GetWindowPos();
		dock->size = ImGui::GetWindowSize();

		ImGuiContext& g = *GImGui;

		if (g.ActiveId == ImGui::GetCurrentWindow()->MoveId && g.IO.MouseDown[0])
		{
			m_drag_offset = ImGui::GetMousePos() - dock->pos;
			doUndock(*dock);
			dock->status = Status_Dragged;
		}
		return ret;
	}

	if (!dock->active && dock->status != Status_Dragged) return false;

	m_end_action = EndAction_EndChild;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
	float tabbar_height = ImGui::GetTextLineHeightWithSpacing();
	if (tabbar(dock->getFirstTab(), opened != nullptr))
	{
		fillLocation(*dock);
		*opened = false;
	}
	ImVec2 pos = dock->pos;
	ImVec2 size = dock->size;
	pos.y += tabbar_height + ImGui::GetStyle().WindowPadding.y;
	size.y -= tabbar_height + ImGui::GetStyle().WindowPadding.y;

	ImGui::SetCursorScreenPos(pos);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
								ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
								extra_flags;




	

	//strcpy(tmp, label);
	//strcat(tmp, "_docked"); // to avoid https://github.com/ocornut/imgui/issues/713
	bool ret = ImGui::BeginChild(tmp, size, true, flags);
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	return ret;
}


void DockContext::end()
{
	if (m_end_action == EndAction_End)
	{
		ImGui::End();
	}
	else if (m_end_action == EndAction_EndChild)
	{
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	}
	m_current = nullptr;
	if (m_end_action > EndAction_None) endPanel();
}


int DockContext::getDockIndex(Dock* dock)
{
	if (!dock) return -1;

	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (dock == m_docks[i]) return i;
	}

	IM_ASSERT(false);
	return -1;
}

#include "MagicFile.cpp/magicfile.h"
void DockContext::save()
{

	HTML_unlink("imgui_dock.layout");
	RAMFILE *fp = HTML_fopen("imgui_dock.layout", "w");
	HTML_fprintf(fp, "docks %d\n\n", m_docks.size());
	for (int i = 0; i < m_docks.size(); ++i) {
		CDock& dock = *m_docks[i];

		HTML_fprintf(fp, "index    %d\n", i);
		HTML_fprintf(fp, "label    %s\n", dock.parent ? (dock.label[0] == '\0' ? "DOCK" : dock.label) : "ROOT"),
		HTML_fprintf(fp, "x        %d\n", (int)dock.pos.x);
		HTML_fprintf(fp, "y        %d\n", (int)dock.pos.y);
		HTML_fprintf(fp, "size_x   %d\n", (int)dock.size.x);
		HTML_fprintf(fp, "size_y   %d\n", (int)dock.size.y);
		HTML_fprintf(fp, "status   %d\n", (int)dock.status);
		HTML_fprintf(fp, "active   %d\n", dock.active ? 1 : 0);
		HTML_fprintf(fp, "opened   %d\n", dock.opened ? 1 : 0);
		fillLocation(dock);
		HTML_fprintf(fp, "location %s\n", strlen(dock.location) ? dock.location : "-1");
		HTML_fprintf(fp, "child0   %d\n", getDockIndex(dock.children[0]));
		HTML_fprintf(fp, "child1   %d\n", getDockIndex(dock.children[1]));
		HTML_fprintf(fp, "prev_tab %d\n", getDockIndex(dock.prev_tab));
		HTML_fprintf(fp, "next_tab %d\n", getDockIndex(dock.next_tab));
		HTML_fprintf(fp, "parent   %d\n\n", getDockIndex(dock.parent));
	}
	HTML_fclose(fp);

}
	


CDock *DockContext::getDockByIndex(int idx) {
	if (idx < 0)
		return NULL;
	if (idx >= m_docks.size())
		return NULL;
	return m_docks[idx];
}


void trimnl(char *str) {
	int n = strlen(str);
	if (str[n-1] == '\n')
		str[n-1] = 0;
	n = strlen(str);
	if (str[n-1] == '\r')
		str[n-1] = 0;
	n = strlen(str);
	if (str[n-1] == '\n')
		str[n-1] = 0;
	n = strlen(str);
	if (str[n-1] == '\r')
		str[n-1] = 0;
}


struct MEM_FILE {
	char *data;
	int size;
	int position;
};
CCALL int MEM_fscanf(MEM_FILE *f, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = vsscanf((const char *)((size_t)f->data + f->position), fmt, args);
	va_end(args);
	f->position += ret;
	// meh no clue how to get %n shit via varargs... just jump to next line, works for my imgui .layout case
	// sooner or later is just wanna use JSON over JS-API nonetheless
	for (int i=f->position; i<f->size; i++) {
		if (((char *)f->data)[i] == '\n') {
			f->position = i + 1;
			break;
		}
	}
	return ret;
}

void DockContext::load()
{
#if 1
	RAMFILE *fp = HTML_fopen("imgui_dock.layout", "r");
	//int bytes = FS_ReadFile("imgui_dock.layout",

	if (fp == NULL) {
		printf("DockContext::load> cant open imgui_dock.layout\n");
		return;
	} else {

		for (int i = 0; i < m_docks.size(); ++i)
		{
			//m_docks[i]->~Dock();
			ImGui::MemFree(m_docks[i]);
		}
		m_docks.clear();

		int ival;
		char str2[64];
		HTML_fscanf(fp, "docks %d", &ival);
		printf("%d docks\n", ival);

		for (int i = 0; i < ival; i++) {
			CDock *new_dock = (CDock *) ImGui::MemAlloc(sizeof(CDock));
			memset(new_dock, 0, sizeof(CDock));
			assert(new_dock != NULL);
			m_docks.push_back(new_dock);
		}

		for (int i = 0; i < ival; i++) {
			int id, id1, id2, id3, id4, id5;
			int st;
			int b1, b2;
			char lab[512]; // this can cause stack corruption, i have to get rid of all the fscanf aids... was 32 bytes, now just upped to 512 to fix the "bug"
				
			// had problem with "Op List"... spaces are not allowed
			int debug = 0;

			if (debug) printf("new op %d\n\n", i);

			
			HTML_fscanf(fp, "%s %d", str2, &id);
			if (debug) printf("id: %s %d\n", str2, id);
			//fscanf(fp, "%s %[0-9a-zA-Z ]", str2, lab);
			HTML_fscanf(fp, "%s %[^\n]", str2, lab);
			if (debug) printf("lab[0]: %s %d\n", str2, lab[0]);
			HTML_fscanf(fp, "%s %f", str2, &m_docks[id]->pos.x);
			if (debug) printf("posx: %s %f\n", str2, m_docks[id]->pos.x);
			HTML_fscanf(fp, "%s %f", str2, &m_docks[id]->pos.y);
			if (debug) printf("posy: %s %f\n", str2, m_docks[id]->pos.y);
			HTML_fscanf(fp, "%s %f", str2, &m_docks[id]->size.x);
			if (debug) printf("sizex: %s %f\n", str2, m_docks[id]->size.x);
			HTML_fscanf(fp, "%s %f", str2, &m_docks[id]->size.y);
			if (debug) printf("sizey: %s %f\n", str2, m_docks[id]->size.y);
			HTML_fscanf(fp, "%s %d", str2, &st);
			if (debug) printf("st: %s %d\n", str2, st);
			HTML_fscanf(fp, "%s %d", str2, &b1);
			if (debug) printf("b1: %s %d\n", str2, b1);
			HTML_fscanf(fp, "%s %d", str2, &b2);
			if (debug) printf("b2: %s %d\n", str2, b2);
			HTML_fscanf(fp, "%s %s", str2, &m_docks[id]->location[0]);
			if (debug) printf("location[0]: %s %d\n", str2, m_docks[id]->location[0]);
			HTML_fscanf(fp, "%s %d", str2, &id1);
			if (debug) printf("id1: %s %d\n", str2, id1);
			HTML_fscanf(fp, "%s %d", str2, &id2);
			if (debug) printf("id2: %s %d\n", str2, id2);
			HTML_fscanf(fp, "%s %d", str2, &id3);
			if (debug) printf("id3: %s %d\n", str2, id3);
			HTML_fscanf(fp, "%s %d", str2, &id4);
			if (debug) printf("id4: %s %d\n", str2, id4);
			HTML_fscanf(fp, "%s %d", str2, &id5);
			if (debug) printf("id5: %s %d\n", str2, id5);

			//m_docks[id]->label = strdup(lab);
			trimnl(lab);
			trimnl(m_docks[id]->location);

			strlcpy(m_docks[id]->label, lab, sizeof m_docks[id]->label);
			m_docks[id]->id = ImHash(m_docks[id]->label,0);
				
			m_docks[id]->children[0] = getDockByIndex(id1);
			m_docks[id]->children[1] = getDockByIndex(id2);
			m_docks[id]->prev_tab = getDockByIndex(id3);
			m_docks[id]->next_tab = getDockByIndex(id4);
			m_docks[id]->parent = getDockByIndex(id5);
			m_docks[id]->status = (Status_)st;
			m_docks[id]->active = b1;
			m_docks[id]->opened = b2;
			m_docks[id]->last_frame = 0;
			m_docks[id]->invalid_frames = 0;
			m_docks[id]->first = 0;


			CDock *dock = m_docks[i];

				const char *extension = get_filename_extension(dock->label);
				if (
					strcmp(extension, "jl") == 0 ||
					strcmp(extension, "ic") == 0 ||
					strcmp(extension, "js") == 0 ||
					strcmp(extension, "py") == 0
				) {
					//auto repl = new DockREPL(dock->label);
					////strncpy(repl->name, dock->label, sizeof repl->name);
					////repl->LoadFile(repl->name);
					//repl->imguidock = dock;
					//continue;
				}
				
		}

		for (auto dock : m_docks) {

				
			tryDockToStoredLocation(*dock);

			//if (dock->label)
			//	printf("dock %s loc=%s status=%d\n", dock->label, dock->location, dock->status);
			//if (dock->children[0])
			//	printf("child 0: %s\n", dock->children[0]->label);
			//if (dock->children[1])
			//	printf("child 1: %s\n", dock->children[1]->label);
		}

		HTML_fclose(fp);
	}
#endif
}


void Print() {
	for (int i = 0; i < g_dock.m_docks.size(); ++i)
	{
		ImGui::Text("i=%d this=0x%.8p state=(%d %d) pos=(%.0f %.0f) size=(%.0f %.0f) children=(%s %s) tabs=(%s %s) parent=%s status=%d  location='%s' label='%s'\n", i, 
					(void*)g_dock.m_docks[i],
					g_dock.m_docks[i]->active,
					g_dock.m_docks[i]->opened,
					g_dock.m_docks[i]->pos.x,
					g_dock.m_docks[i]->pos.y,
					g_dock.m_docks[i]->size.x,
					g_dock.m_docks[i]->size.y,
					g_dock.m_docks[i]->children[0] ? g_dock.m_docks[i]->children[0]->label : "None",
					g_dock.m_docks[i]->children[1] ? g_dock.m_docks[i]->children[1]->label : "None",
					g_dock.m_docks[i]->prev_tab    ? g_dock.m_docks[i]->prev_tab->label    : "None",
					g_dock.m_docks[i]->next_tab    ? g_dock.m_docks[i]->next_tab->label    : "None",
					g_dock.m_docks[i]->parent      ? g_dock.m_docks[i]->parent->label      : "None",
					g_dock.m_docks[i]->status,
					g_dock.m_docks[i]->location,
					g_dock.m_docks[i]->label);

	}
}

void ShutdownDock()
{
	for (int i = 0; i < g_dock.m_docks.size(); ++i)
	{
		//g_dock.m_docks[i]->~Dock();
		ImGui::MemFree(g_dock.m_docks[i]);
	}
	g_dock.m_docks.clear();
}


void RootDock(const ImVec2& pos, const ImVec2& size)
{
	g_dock.rootDock(pos, size);
}


void SetDockActive()
{
	g_dock.setDockActive();
}


bool BeginDock(const char* label, bool* opened, ImGuiWindowFlags extra_flags, Dock *useThisDock)
{
	return g_dock.begin(label, opened, extra_flags, useThisDock);
}


void EndDock()
{
	g_dock.end();
}



void SaveDock()
{
	g_dock.save();
}




void LoadDock()
{
	g_dock.load();
}


//} // namespace ImGui


CCALL void dock_load() {
	g_dock.load();
}

CCALL void dock_save() {
	g_dock.save();
}