#include "../imgui/include_imgui.h"
#include <list>

#include <string>
#include <sstream>
#include <iostream>

//#include <kung/include_webgamegui.h>


#include "dock_console.h"

//#include <client/client.h>

DockConsole::DockConsole() {
}

const char *DockConsole::label() {
	return "Console";
}

void DockConsole::imgui() {
	imgui_console_content();
}

//#include <kung/include_gl.h>
//#include <kung/include_glfw.h>
// had to change this to a pointer, because otherwise it would crash... probably it wasn't initialized yet
std::list<std::string> *console_lines = NULL;

double lasttime;

// fix later so it works with sdl only and emscripten
double glfwGetTime() {
	return ImGui::GetTime();
	//return 0.00;
}

//int log(char *format, ...);
//// ye, just a wrapper..
//extern "C" int opsystem_log(char *format, ...) {
//	char buf[4096];
//	va_list args;
//	va_start (args, format);
//	vsprintf(buf, format, args);
//	va_end (args);
//
//	return imgui_log("%s", buf);
//}

CCALL int js_printf(char *msg, ...) {
	va_list argptr;
	va_start(argptr, msg);
	char buf[4096];
	vsnprintf(buf, 4096, msg, argptr);
	int ret = imgui_log("%s", buf);
	va_end(argptr);
	return ret;
}

EXTERNC int imgui_log_to_browser_console = 0;

// goddamn, this clashes with math.h log xD use opsystem_log then
int imgui_log(char *format, ...) {
	char *buf;
	va_list args;
	va_start (args, format);
	
	// the output can be enourmous, so instead of a static buffer, we first figure out the length needed and then just alloc that size
	size_t needed = vsnprintf(NULL, 0, format, args);
	needed += 1;
	buf = (char *)malloc(needed);

	if (buf == NULL) {
		imgui_log("failed to allocate %d bytes for imgui_log()\n", needed);
		return -1;
	}

	vsnprintf(buf, needed, format, args);
	va_end (args);

	// initialize, if not already done
	if (console_lines == NULL) {
		console_lines = new std::list<std::string>;
		lasttime = glfwGetTime();
	}

	double deltatime = glfwGetTime() - lasttime;

	if (console_lines->size()) {
		std::string lastmsg = console_lines->back();
		console_lines->pop_back();
		std::string merged = lastmsg + buf;
		char *cstr = (char *)merged.c_str();
		int len = strlen(cstr);
		char *from = cstr;
		for (int i=0; i<len; i++) {
			char c = cstr[i];
			if (c == '\n') {
				char *to = cstr + (i); // - 1, so we dont add the newline into the "new line"
				
				std::string tmp(from, to);
				char time[128];
				sprintf(time, "Delta=%f ", deltatime);

				tmp = time + tmp;

				//printf("newline part: \"%s\"\n", tmp.c_str());
				if (imgui_log_to_browser_console)
					puts(tmp.c_str());

				console_lines->push_back(tmp);
				from = cstr + i + 1;
			}
		}
		if (strlen(from)) {
			std::string tmp(from);
			//printf("from: \"%s\"\n", tmp.c_str());
			if (imgui_log_to_browser_console)
				puts(tmp.c_str());
			console_lines->push_back( tmp );
		} else {
			// we pop_back the last string to add "buf", but when the last string was already newlined, it would remove the already finished line
			console_lines->push_back(std::string());
		}
	} else {
		auto dup = std::string(buf);
		if (imgui_log_to_browser_console)
			puts(dup.c_str());
		console_lines->push_back(dup);
	}
	lasttime = glfwGetTime();
	free(buf);
	return 1;
}

void imgui_console_content() {
	static bool autoscroll = 1;
	static int forceoff = 0;
	//ImGui::Begin("Console", &show_console);


	// initialize, if not already done
	if (console_lines == NULL) {
		console_lines = new std::list<std::string>;
		lasttime = glfwGetTime();
	}



	//if (ImGui::GetIO().MouseWheel != 0.0f) {
	//	if (autoscroll)
	//		forceoff = 20;
	//}

	// re-enable autoscroll when the scrollbar goes to end of window
	if ( ! (ImGui::GetScrollY() < ImGui::GetScrollMaxY()))
	{
		//autoscroll = 1;
		//forceoff = 0;
	} else {
		autoscroll = 0;
	}
	// disable auto scroll when user scrolls in console
	
	// used scrolling upwards = disable autoscroll
	if (ImGui::IsMouseHoveringWindow() && ImGui::GetIO().MouseWheel > 0.0f) {
		//log("mousewheel: %.2f\n", ImGui::GetIO().MouseWheel);
		autoscroll = 0;
	}

	if (forceoff)
		autoscroll = 0;

	auto backup = ImGui::GetCursorPos();

	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 170, 25 + ImGui::GetScrollY()));
	if (ImGui::Button("Clean")) {
		console_lines->clear();
	}
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 110, 25 + ImGui::GetScrollY()));
	ImGui::Checkbox("Autoscroll", &autoscroll);
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 600, 25 + ImGui::GetScrollY()));
	//ImGui::Text("winh=%.2f maxscroll=%.2f scoll=%.2f autoscroll %d", ImGui::GetWindowHeight(), ImGui::GetScrollMaxY(), ImGui::GetScrollY(), autoscroll);


	ImGui::SetCursorPos(backup);
	for (auto i : *console_lines)
		ImGui::Text("%s", i.c_str());

	if (autoscroll)
		ImGui::SetScrollHere();


	if (forceoff)
		forceoff--;
	//ImGui::End();
}