#pragma once

#include "fcb.h"

class FileHandle {
	FCB fcb;
	unsigned int read_count = 0;
public:
	FileHandle(const FCB& fcb);
};