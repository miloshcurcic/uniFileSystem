#pragma once
#include <Windows.h>

class WinCriticalSection;

class WinCSConditionVariable {
	CONDITION_VARIABLE condition_variable;
	WinCriticalSection* critical_section = nullptr;

	~WinCSConditionVariable();
public:
	WinCSConditionVariable(WinCriticalSection *critical_section);
	void wait();
	void signal();
	void signalAll();
};