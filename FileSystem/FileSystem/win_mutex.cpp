#include "win_mutex.h"

WinMutex::WinMutex()
{
	mutex = CreateMutex(NULL, FALSE, NULL);
}

void WinMutex::signal()
{
	ReleaseMutex(mutex);
}

void WinMutex::wait()
{
	WaitForSingleObject(mutex, INFINITE);
}

WinMutex::~WinMutex()
{
	CloseHandle(mutex);
}
