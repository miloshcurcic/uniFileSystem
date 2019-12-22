#include "win_srw_cond_variable.h"
#include "win_srw_lock.h"

WinSRWConditionVariable::~WinSRWConditionVariable()
{
}

WinSRWConditionVariable::WinSRWConditionVariable(WinSlimReaderWriterLock* srw_lock)
{
	InitializeConditionVariable(&condition_variable);
	this->srw_lock = srw_lock;
}

void WinSRWConditionVariable::wait_shared()
{
	SleepConditionVariableSRW(&condition_variable, &(srw_lock->srw_lock), INFINITE, CONDITION_VARIABLE_LOCKMODE_SHARED);
}

void WinSRWConditionVariable::wait_exclusive()
{
	SleepConditionVariableSRW(&condition_variable, &(srw_lock->srw_lock), INFINITE, NULL);
}

void WinSRWConditionVariable::signal()
{
	WakeConditionVariable(&condition_variable);
}

void WinSRWConditionVariable::signal_all()
{
	WakeAllConditionVariable(&condition_variable);
}
