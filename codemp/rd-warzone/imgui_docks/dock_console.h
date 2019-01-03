#ifndef IMGUI_CONSOLE_H
#define IMGUI_CONSOLE_H

#include "../imgui/ccall/ccall.h"
#include "dock.h"
//#include <string>

CCALL int imgui_log(char *format, ...);
CCALL void imgui_console_content();

#ifdef __cplusplus

class DockConsole : public Dock {
public:
	DockConsole();
	virtual const char *label();
	virtual void imgui();
};

#endif

#endif