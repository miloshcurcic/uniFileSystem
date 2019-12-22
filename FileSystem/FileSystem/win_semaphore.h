#pragma once
#include <Windows.h>

class WinSemaphore {
	HANDLE semaphore;
public:
	WinSemaphore(unsigned count);
	void signal();
	void wait();
	~WinSemaphore();
};