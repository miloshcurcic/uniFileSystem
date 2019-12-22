#include "win_crit_section.h"
#include "win_cond_variable.h"

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

WinConditionVariable* WinCriticalSection::make_condition_variable()
{
	auto cond_variable = new WinConditionVariable(this);
	cond_list.push_back(std::unique_ptr<WinConditionVariable>(cond_variable));
	return cond_variable;
}

WinCriticalSection::~WinCriticalSection()
{
	DeleteCriticalSection(&critical_section);
	cond_list.clear();
}
