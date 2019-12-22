#include "win_crit_section.h"
#include "win_cs_cond_variable.h"

WinCriticalSection::WinCriticalSection()
{
	InitializeCriticalSection(&critical_section);
}

void WinCriticalSection::enter()
{
	EnterCriticalSection(&critical_section);
}

void WinCriticalSection::exit()
{
	LeaveCriticalSection(&critical_section);
}

WinCSConditionVariable* WinCriticalSection::make_condition_variable()
{
	auto cond_variable = new WinCSConditionVariable(this);
	cond_list.push_back(cond_variable);
	return cond_variable;
}

WinCriticalSection::~WinCriticalSection()
{
	DeleteCriticalSection(&critical_section);
	for(auto cond : cond_list) {
		delete cond;
	}
	cond_list.clear();
}
