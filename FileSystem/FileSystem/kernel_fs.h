#pragma once

#include "definitions.h"
#include "map"
#include <string>

class Partition;
class KernelFile;
class FCB;

class KernelFS {

    char mount(Partition* partition);
    char unmount(); 
    char format();
    FileCnt readRootDir(); 
    char doesExist(char* fname);
    KernelFile* open(char* fname, char mode);
    char deleteFile(char* fname);

private:

    Partition* mounted_partition = nullptr;
    std::map<std::string, FCB> open_files;
    unsigned int bit_vector_size;
    unsigned int root_dir_index0;
    
};