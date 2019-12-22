#pragma once
#include <Windows.h>
#include <list>
#include <memory>

class WinConditionVariable;

class WinCriticalSection {
	CRITICAL_SECTION critical_section;
	std::list<std::unique_ptr<WinConditionVariable>> cond_list;
public:
	WinCriticalSection();
	void enter();
	void exit();
	WinConditionVariable* make_condition_variable();
	~WinCriticalSection();

	friend class WinCSConditionVariable;
};