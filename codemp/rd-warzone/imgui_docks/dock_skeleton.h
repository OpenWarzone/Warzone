#include "dock.h"
#include <string>

class DockSkeleton : public Dock {
public:
	DockSkeleton();
	virtual const char *label();
	virtual void imgui();
};