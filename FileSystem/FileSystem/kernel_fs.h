#pragma once

#include "definitions.h"
#include <regex>

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
	static const std::regex file_regex;
public:
    char mount(Partition* partition);
    char unmount(); 
    char format();
    FileCnt readRootDir(); 
    char doesExist(const char* fname);
    KernelFile* open_file(const char* fname, char mode);
	void close_file(const KernelFile* file);
    char deleteFile(const char* fname);
};