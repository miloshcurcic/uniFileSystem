#pragma once
#include <Windows.h>
#include <list>
#include <memory>

class WinCSConditionVariable;

class WinCriticalSection {
	CRITICAL_SECTION critical_section;
	std::list<WinCSConditionVariable*> cond_list;
public:
	WinCriticalSection();
	void enter();
	void exit();
	WinCSConditionVariable* make_condition_variable();
	~WinCriticalSection();

	friend class WinCSConditionVariable;
};