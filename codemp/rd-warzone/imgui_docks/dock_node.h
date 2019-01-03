#include "dock.h"
#include <string>

class DockNode : public Dock {
public:
	char repl_filename[128] = {"tmp.txt"};
	char replbuffer[4096] = {0};

	DockNode();
	virtual const char *label();
	virtual void imgui();
};