#ifndef TR_DEBUG
#define TR_DEBUG

#include <map>
#include <list>
#include <chrono>

typedef struct perfdata_s {
	//int starttime;
	//int stoptime;
	std::chrono::time_point<std::chrono::steady_clock> starttime;
	std::chrono::time_point<std::chrono::steady_clock> stoptime;
} perfdata_t;

// this allows for nested performance tracking
extern std::map<std::string, perfdata_t> performancelog;
extern std::list<std::string> perfNameStack;

#endif