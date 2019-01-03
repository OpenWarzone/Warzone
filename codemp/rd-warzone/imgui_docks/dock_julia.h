#include "dock.h"
#include <string>

class DockJulia : public Dock {
public:
	DockJulia();
	virtual const char *label();
	virtual void imgui();
};