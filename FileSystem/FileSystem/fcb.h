#pragma once

#include "definitions.h"

class FCB {
public:
	char name[FNAMELEN] = { 0 };
	char ext[FEXTLEN] = { 0 };
	//unsigned char : 0; // padding?
	unsigned char padding = 0;
	unsigned int data0 = 0;
	unsigned int data1 = 0;
	unsigned int index1_0 = 0;
	unsigned int index2_0 = 0;
	unsigned int size = 0;
};

const FCB ZERO_FCB;
