#pragma once

#include <Windows.h>

class WinSlimReaderWriterLock;

class WinSRWConditionVariable {
	CONDITION_VARIABLE condition_variable;
	WinSlimReaderWriterLock* srw_lock = nullptr;

	~WinSRWConditionVariable();
public:
	WinSRWConditionVariable(WinSlimReaderWriterLock* srw_lock);
	void wait_shared();
	void wait_exclusive();
	void signal();
	void signal_all();

	friend class WinSlimReaderWriterLock;
};