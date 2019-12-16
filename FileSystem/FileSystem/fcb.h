#pragma once

struct FCB {
	char name[8];
	char ext[3];
	//unsigned char : 0; // padding
	unsigned char padding = 0;
	unsigned int index0;
	unsigned int size;
	unsigned char free_to_use[12];
};