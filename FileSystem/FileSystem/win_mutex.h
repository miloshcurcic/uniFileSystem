#pragma once
#include <Windows.h>

class WinMutex {
	HANDLE mutex;
public:
	WinMutex();
	void signal();
	void wait();
	~WinMutex();
};