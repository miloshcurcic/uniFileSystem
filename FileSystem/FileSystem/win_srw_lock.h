#pragma once

#include <memory>
#include <list>
#include <Windows.h>

class WinSRWConditionVariable;

class WinSlimReaderWriterLock {
	SRWLOCK srw_lock;
	std::list<std::unique_ptr<WinSRWConditionVariable>> cond_list;
public:
	WinSlimReaderWriterLock();
	void acquire_srw_exclusive();
	void acquire_srw_shared();
	void release_srw_exclusive();
	void release_srw_shared();
	WinSRWConditionVariable* make_condition_variable();
	~WinSlimReaderWriterLock();

	friend class WinSRWConditionVariable;
};