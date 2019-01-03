#include "dock.h"
#include <string>

#include "../imgui/imgui_dock.h"

class DockREPL : public Dock {
public:
	std::string filename;
	CDock *imguidock;
	DockREPL(std::string filename_);
	virtual const char *label();
	virtual void imgui();
};