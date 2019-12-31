#pragma once

#include "definitions.h"

class Partition;
class KernelFile;
class FileHandle;
class MemoryManager;
class DirectoryManager;
class FCB;

class KernelFS {
	bool formatting = false;
	Partition* mounted_partition = nullptr;
	MemoryManager* memory_manager = nullptr;
	DirectoryManager* directory_manager = nullptr;
	std::unordered_map<std::string, FileHandle*> open_files;
	unsigned int root_dir_index0;
public:
    char mount(Partition* partition);
    char unmount(); 
    char format();
    FileCnt readRootDir(); 
    char doesExist(char* fname);
    KernelFile* open_file(char* fname, char mode);
	void close_file(KernelFile* file);
    char deleteFile(char* fname);
};