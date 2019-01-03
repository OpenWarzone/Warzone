#include "tr_local.h"

#include <iostream>
using namespace std;
#include <cstdlib>
#include <sys/timeb.h>

#include "tr_debug.h"

#ifdef __PERFORMANCE_DEBUG__
#include <chrono>
std::chrono::time_point<std::chrono::steady_clock> 	DEBUG_PERFORMANCE_TIME;
char												DEBUG_PERFORMANCE_NAME[128] = { 0 };
#endif //__PERFORMANCE_DEBUG__

std::chrono::time_point<std::chrono::steady_clock> getNanoSecondTime()
{
	return std::chrono::high_resolution_clock::now();
}

int getNanoSecondSpan(std::chrono::time_point<std::chrono::steady_clock> nTimeStart) {
	int nSpan = std::chrono::duration_cast<std::chrono::nanoseconds>(getNanoSecondTime() - nTimeStart).count();
	//if (nSpan < 0)
	//	nSpan += 0x100000;// *1000;
	return nSpan;
}

// this allows for nested performance tracking
std::map<std::string, perfdata_t> performancelog;
std::list<std::string> perfNameStack;

void DEBUG_StartTimer(char *name, qboolean usePerfCvar)
{
#ifndef __PERFORMANCE_DEBUG_STARTUP__
	if (!usePerfCvar) return;
#endif //__PERFORMANCE_DEBUG_STARTUP__

#ifdef __PERFORMANCE_DEBUG__
	if (!usePerfCvar || r_perf->integer)
	{
		if (usePerfCvar)
			qglFinish();

		// then log the start time
		performancelog[name].starttime = getNanoSecondTime();
		perfNameStack.push_back(name);

		memset(DEBUG_PERFORMANCE_NAME, 0, sizeof(char) * 128);
		strcpy(DEBUG_PERFORMANCE_NAME, name);
		DEBUG_PERFORMANCE_TIME = getNanoSecondTime();

		/*
		if (DEBUG_PERFORMANCE_NAME[0] != '\0' && strlen(DEBUG_PERFORMANCE_NAME) > 0)
		{
			ri->Printf(PRINT_WARNING, "%s begins.\n", DEBUG_PERFORMANCE_NAME);
		}
		*/
	}
#endif //__PERFORMANCE_DEBUG__
}

void DEBUG_EndTimer(qboolean usePerfCvar)
{
#ifndef __PERFORMANCE_DEBUG_STARTUP__
	if (!usePerfCvar) return;
#endif //__PERFORMANCE_DEBUG_STARTUP__

#ifdef __PERFORMANCE_DEBUG__
	if (!usePerfCvar || r_perf->integer)
	{
		if (usePerfCvar)
			qglFinish();
			
		// ...and log the end time, so we can calculate a real delta
		auto lastPerfName = perfNameStack.back();
		performancelog[lastPerfName].stoptime = getNanoSecondTime();
		perfNameStack.pop_back();

#ifdef __PERFORMANCE_DEBUG_TEXT__
		//DEBUG_PERFORMANCE_TIME = getNanoSecondSpan(DEBUG_PERFORMANCE_TIME);

		// you can see the results in Perf dock now --  UQ1: Gonna keep the messages for now so I can track down startup errors easier...
		if (DEBUG_PERFORMANCE_NAME[0] != '\0' && strlen(DEBUG_PERFORMANCE_NAME) > 0)
		{
			ri->Printf(PRINT_WARNING, "%s took %i ns to complete.\n", DEBUG_PERFORMANCE_NAME, getNanoSecondSpan(DEBUG_PERFORMANCE_TIME));
		}
		else
		{
			ri->Printf(PRINT_WARNING, "%s took %i ns to complete.\n", "unknown", getNanoSecondSpan(DEBUG_PERFORMANCE_TIME));
		}
#endif //__PERFORMANCE_DEBUG_TEXT__

		DEBUG_PERFORMANCE_TIME = getNanoSecondTime();
	}
#endif //__PERFORMANCE_DEBUG__
}
