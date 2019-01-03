#include "dock_perf.h"
#include "../imgui_docks/dock_console.h"
#include "../imgui_openjk/gluecode.h"

DockPerf::DockPerf() {}

#include "../tr_debug.h"
#include <chrono>

const char *DockPerf::label() {
	return "Perf";
}

#include <iomanip>
#include <locale>
#include <sstream>

template<class T>
std::string FormatWithCommas(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

void DockPerf::imgui() {
	if (r_perf->integer) {
		if (ImGui::Button("Disable Performance Measuring"))
			Cvar_SetInt(r_perf, 0);
	} else {
		if (ImGui::Button("Enable Performance Measuring"))
			Cvar_SetInt(r_perf, 1);
	}

	for (auto perfdata : performancelog) {
		const char *name = perfdata.first.c_str();
		std::chrono::time_point<std::chrono::steady_clock> starttime = perfdata.second.starttime;
		std::chrono::time_point<std::chrono::steady_clock> stoptime = perfdata.second.stoptime;
		//int delta = stoptime - starttime;
		int delta = std::chrono::duration_cast<std::chrono::nanoseconds>(stoptime - starttime).count();
		float nanoseconds = (float)delta;
		const float maxTime = (1000.0 * 1000.0 * 1000.0) / 60.0; // 16.66ms is the absolute maximum, if a frame takes like 10ms its already in the very evil range...
		float maxPercent = (nanoseconds / maxTime) * 100.0;
		//ImGui::Text("%s: start=%10d stop=%10d nanoseconds=%10.2f maxPercent=%10.2f", name, starttime, stoptime, milliseconds, maxPercent);
		//ImGui::Text("%s: nanoseconds=%10.2f maxPercent=%10.2f", name, nanoseconds, maxPercent);
		//ImGui::Text("%s: nanoseconds=%10d maxPercent=%10.2f", name, delta, maxPercent);
		ImGui::Text("%30s: nanoseconds=%16s maxPercent=%10.2f", name, FormatWithCommas(delta).c_str(), maxPercent);
	}
}