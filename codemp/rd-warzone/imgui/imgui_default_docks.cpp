#include "imgui.h"
#include "imgui_dock.h"
#include <stdio.h>
#include "ccall/ccall.h"

#include "../imgui_docks/dock.h"
#include "../imgui_docks/dock_console.h"
#include "../imgui_docks/dock_repl.h"
#include "../imgui_docks/dock_node.h"
#include "../imgui_docks/dock_julia.h"

#include <list>
std::list<Dock *> docks;

bool show_all_window = false;
bool show_another_window = true;
bool show_node_and_console_windows = false;
bool show_demo_window = false;

ImVec4 clear_color_2 = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

int main_menu_GUIasd() {
    //////////////////////
    // Placeholder menu //
    //////////////////////
    int menu_height = 0;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::BeginMenu("Open Recent"))
            {
                ImGui::MenuItem("fish_hat.c");
                ImGui::MenuItem("fish_hat.inl");
                ImGui::MenuItem("fish_hat.h");
                if (ImGui::BeginMenu("More.."))
                {
                    ImGui::MenuItem("Hello");
                    ImGui::MenuItem("Sailor");
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Disabled", false)) // Disabled
            {
                IM_ASSERT(0);
            }
           // if (ImGui::MenuItem("Fullscreen", NULL, fullscreen)) {fullscreen = !fullscreen;}
            if (ImGui::MenuItem("Quit", "Alt+F4")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("New"))
        {
            //if (ImGui::MenuItem("Julia REPL", "")) {
			//	auto repl = new REPLJulia();
			//	GetUniqueFilename(repl->name, sizeof repl->name, "julia", "jl");
			//}
            //if (ImGui::MenuItem("Javascript REPL", "")) {
			//	auto repl = new REPLJulia();
			//	GetUniqueFilename(repl->name, sizeof repl->name, "javascript", "js");
			//}
            //if (ImGui::MenuItem("Python REPL", "")) {
			//	auto repl = new REPLJulia();
			//	GetUniqueFilename(repl->name, sizeof repl->name, "python", "py");
			//}
            if (ImGui::MenuItem("Save Layout", "")) {
				SaveDock();
			}
            if (ImGui::MenuItem("Load Layout", "")) {
				LoadDock();
			}

			
            ImGui::EndMenu();
           
        }

        menu_height = ImGui::GetWindowSize().y;

        ImGui::EndMainMenuBar();
    }
    
    return menu_height;
}

CCALL int imgui_default_docks() {
    ImGuiIO& io = ImGui::GetIO();

	if (ImGui::GetIO().DisplaySize.y > 0) {
		////////////////////////////////////////////////////
		// Setup root docking window                      //
		// taking into account menu height and status bar //
		////////////////////////////////////////////////////
		float menu_height = 200;
		auto pos = ImVec2(0, main_menu_GUIasd());
		auto size = ImGui::GetIO().DisplaySize;
		size.y -= pos.y;

		int statusbar = 1;

		if (statusbar)
			RootDock(pos, ImVec2(size.x, size.y - 25.0f));
		else
			RootDock(pos, ImVec2(size.x, size.y));

		if (statusbar) {
			// Draw status bar (no docking)
			ImGui::SetNextWindowSize(ImVec2(size.x, 25.0f), ImGuiSetCond_Always);
			ImGui::SetNextWindowPos(ImVec2(0, size.y - 6.0f), ImGuiSetCond_Always);
			ImGui::Begin("statusbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize);
			ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
			//ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
			//ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
			ImGui::End();
		}
	}
	
	
	g_dock.checkNonexistent();

	if (show_node_and_console_windows)
	{
		static int first = 1;
		if (first) {
			first = 0;
			//docks.push_back(new DockREPL("testone"));
			docks.push_back(new DockConsole());
			docks.push_back(new DockNode());
			//docks.push_back(new DockJulia());
		}
	}

	for (Dock *dock : docks) {
		bool closed = true;
		if (BeginDock(dock->label(), &closed)) {
			dock->imgui();
		}
		EndDock();
	}

	if (show_demo_window)
	{
		static bool closed = true;
		if (BeginDock("Demo Stuff", &closed)) {
			ImGui::ShowDemoWindow(&show_demo_window);
		}
		EndDock();
	}


	return 1;
}