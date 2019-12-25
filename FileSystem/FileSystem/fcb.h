#pragma once

#include "definitions.h"

struct FCB {
	char name[FNAMELEN] = { 0 };
	char ext[FEXTLEN] = { 0 };
	//unsigned char : 0; // padding
	unsigned char padding = 0;
	unsigned int index0 = 0;
	unsigned int size = 0;
	unsigned char free_to_use[12] = { 0 };
};