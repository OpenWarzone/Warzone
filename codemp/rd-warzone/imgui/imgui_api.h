#ifndef IMGUI_API_H
#define IMGUI_API_H

#include "ccall/ccall.h"
#include "imgui_default_docks.h"

struct imgui_globals_s {
	int mouse_left;
	int mouse_top;
	int screen_width;
	int screen_height;
	unsigned int global_ticks;
};

extern struct imgui_globals_s imguidata;
EXTERNC int imgui_ready;
typedef int kungbool; // just for having type safety among all compilers

CCALL void imgui_set_mousepos(int left, int top);
CCALL void imgui_set_widthheight(int width, int height);
CCALL void imgui_mouse_set_button(int button, kungbool state);
CCALL void imgui_mouse_wheel(float wheelDelta);
CCALL void imgui_on_key(int key, kungbool state);
CCALL void imgui_on_shift(kungbool state);
CCALL void imgui_on_ctrl (kungbool state);
CCALL void imgui_on_alt  (kungbool state);
CCALL void imgui_set_ticks(unsigned int ticks);
CCALL void imgui_on_key_text(int key);
CCALL void imgui_on_text(char *text);

CCALL void imgui_init();
CCALL void imgui_new_frame();
CCALL void imgui_end_frame();
CCALL void imgui_render();

#endif