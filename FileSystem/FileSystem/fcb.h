#pragma once

#include "definitions.h"

struct FCB {
	char name[FNAMELEN];
	char ext[FEXTLEN];
	//unsigned char : 0; // padding
	unsigned char padding = 0;
	unsigned int index0;
	unsigned int size;
	unsigned char free_to_use[12];
};