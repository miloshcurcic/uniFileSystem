#include "win_semaphore.h"

WinSemaphore::WinSemaphore(unsigned count)
{
	semaphore = CreateSemaphore(NULL, count, INFINITE, NULL);
}

void WinSemaphore::signal()
{
	ReleaseSemaphore(semaphore, 1, NULL);
}

void WinSemaphore::wait()
{
	WaitForSingleObject(semaphore, INFINITE);
}

WinSemaphore::~WinSemaphore()
{
	CloseHandle(semaphore);
}
