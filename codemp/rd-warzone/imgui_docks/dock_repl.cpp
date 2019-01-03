#include "dock_repl.h"
#include "../imgui/include_imgui.h"

DockREPL::DockREPL(std::string filename_) {
	filename = filename_;
}

const char *DockREPL::label() {
	return filename.c_str();
}
void DockREPL::imgui() {
}