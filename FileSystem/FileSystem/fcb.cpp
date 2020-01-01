#include "fcb.h"

FCB::FCB()
{
}

FCB::FCB(const char* name, const char* ext)
{
	int i = 0;
	while (name[i] != 0) {
		this->name[i] = name[i];
		i++;
	}
	while (i < FNAMELEN) {
		this->name[i] = ' ';
	}

	i = 0;
	while (ext[i] != 0) {
		this->ext[i] = ext[i];
		i++;
	}
	while (i < FEXTLEN) {
		this->ext[i] = ' ';
	}
}
