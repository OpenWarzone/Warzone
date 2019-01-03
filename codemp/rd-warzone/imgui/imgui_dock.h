#ifndef IMGUI_DOCK_H
#define IMGUI_DOCK_H

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include <list>
#include <vector>

#include "ccall/ccall.h"

//namespace ImGui {
	void RemoveEmptyDocks();



	enum Slot_ {
		Slot_Left,
		Slot_Right,
		Slot_Top,
		Slot_Bottom,
		Slot_Tab,

		Slot_Float,
		Slot_None
	};

	enum EndAction_ {
		EndAction_None,
		EndAction_Panel,
		EndAction_End,
		EndAction_EndChild
	};

	enum Status_ {
		Status_Docked,
		Status_Float,
		Status_Dragged
	};

	class CDock {
		public:

		ImU32 id;
		ImU32 next_new_id; // kung: generate a new id for pushid/popid automatically before each button in julia, reset this every frame, so they are the same
		bool isDockChanged = false; // ctrl+s/saving sets this back to false, while editing multiline is setting this to true
		char label[256];
		CDock *next_tab;
		CDock *prev_tab;
		CDock *parent;
		ImVec2 pos;
		ImVec2 size;
		int active = 0;
		Status_ status;
		bool opened;
		CDock *children[2];
		char location[16];
		int last_frame;
		int invalid_frames;
		int first = 0;
		int renamed = 0;
		int closeButtonPressed = 0;

		ImVec2 getMinSize() const;
		bool isHorizontal() const ;
		void setParent(CDock *dock);
		CDock &getSibling();
		CDock &getFirstTab();
		void setActive();
		bool isContainer() const ;
		void setChildrenPosSize(const ImVec2& _pos, const ImVec2& _size);
		void setPosSize(const ImVec2& _pos, const ImVec2& _size);
	};
	
	extern std::list<CDock *>invalidDocks;

	class DockContext {
	public:
		std::vector<CDock *> m_docks;
		ImVec2 m_drag_offset;
		CDock* m_current = nullptr;
		int m_last_frame = 0;
		EndAction_ m_end_action;

		~DockContext();
		CDock& getDock(const char* label, bool opened);
		void putInBackground();
		void splits();
		void checkNonexistent();
		void beginPanel();
		void endPanel();
		CDock* getDockAt(const ImVec2& /*pos*/) const; // Doesn't use input??
		ImRect getDockedRect(const ImRect& rect, Slot_ dock_slot);
		ImRect getSlotRect(ImRect parent_rect, Slot_ dock_slot);
		ImRect getSlotRectOnBorder(ImRect parent_rect, Slot_ dock_slot);
		CDock* getRootDock();
		bool dockSlots(CDock& dock, CDock* dest_dock, const ImRect& rect, bool on_border);
		void handleDrag(CDock& dock);
		void fillLocation(CDock& dock);
		void doUndock(CDock& dock);
		void drawTabbarListButton(CDock& dock);
		bool tabbar(CDock& dock, bool close_button);
		void setDockPosSize(CDock& dest, CDock& dock, Slot_ dock_slot, CDock& container);
		void doDock(CDock& dock, CDock* dest, Slot_ dock_slot);
		void rootDock(const ImVec2& pos, const ImVec2& size);
		void setDockActive();
		Slot_ getSlotFromLocationCode(char code);
		char getLocationCode(CDock* dock);
		void tryDockToStoredLocation(CDock& dock);
		bool begin(const char* label, bool* opened, ImGuiWindowFlags extra_flags, CDock *useThisDock=NULL);
		void end();
		int getDockIndex(CDock* dock);
		void save();
		CDock* getDockByIndex(int idx);
		void load();
	};

	IMGUI_API void ShutdownDock();
	IMGUI_API void RootDock(const ImVec2& pos, const ImVec2& size);
	IMGUI_API bool BeginDock(const char* label, bool* opened = nullptr, ImGuiWindowFlags extra_flags = 0, CDock *useThisDock=NULL);
	IMGUI_API void EndDock();
	IMGUI_API void SetDockActive();
	IMGUI_API void LoadDock();
	IMGUI_API void SaveDock();
	IMGUI_API void Print();

	

//} // namespace ImGui

extern DockContext g_dock;

#endif