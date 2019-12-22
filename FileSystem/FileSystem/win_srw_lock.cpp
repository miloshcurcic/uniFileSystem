#include "win_srw_lock.h"
#include "win_srw_cond_variable.h"

WinSlimReaderWriterLock::WinSlimReaderWriterLock()
{
	InitializeSRWLock(&srw_lock);
}

void WinSlimReaderWriterLock::acquire_srw_exclusive()
{
	AcquireSRWLockExclusive(&srw_lock);
}

void WinSlimReaderWriterLock::acquire_srw_shared()
{
	AcquireSRWLockShared(&srw_lock);
}

void WinSlimReaderWriterLock::release_srw_exclusive()
{
	ReleaseSRWLockExclusive(&srw_lock);
}

void WinSlimReaderWriterLock::release_srw_shared()
{
	ReleaseSRWLockShared(&srw_lock);
}

WinSRWConditionVariable* WinSlimReaderWriterLock::make_condition_variable()
{
	auto cond_variable = new WinSRWConditionVariable(this);
	cond_list.push_back(cond_variable);
	return cond_variable;
}

WinSlimReaderWriterLock::~WinSlimReaderWriterLock()
{
	for (auto cond : cond_list) {
		delete cond;
	}
	cond_list.clear();
}
