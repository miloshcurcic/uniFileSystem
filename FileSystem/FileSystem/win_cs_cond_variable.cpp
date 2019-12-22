#include "win_cs_cond_variable.h"
#include "win_crit_section.h"

WinCSConditionVariable::~WinCSConditionVariable()
{
}

WinCSConditionVariable::WinCSConditionVariable(WinCriticalSection* critical_section)
{
	InitializeConditionVariable(&condition_variable);
	this->critical_section = critical_section;
}

void WinCSConditionVariable::wait()
{
	SleepConditionVariableCS(&condition_variable, &(critical_section->critical_section), INFINITE);
}

void WinCSConditionVariable::signal()
{
	WakeConditionVariable(&condition_variable);
}

void WinCSConditionVariable::signalAll()
{
	WakeAllConditionVariable(&condition_variable);
}