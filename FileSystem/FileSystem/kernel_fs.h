#pragma once

#include "definitions.h"

class Partition;
class KernelFile;
class FCB;

class KernelFS {
	bool formatting = false;
	Partition* mounted_partition = nullptr;
	std::unordered_map<std::string, FileHandle*> open_files;
	unsigned int bit_vector_size;
	unsigned int root_dir_index0;
	unsigned char** bit_vector;
	IndexEntry root_dir_0[NUM_INDEX_ENTRIES];
public:
    char mount(Partition* partition);
    char unmount(); 
    char format();
    FileCnt readRootDir(); 
    char doesExist(char* fname);
    KernelFile* open(char* fname, char mode);
	void close_file(KernelFile* file);
    char deleteFile(char* fname);
};