#pragma once
#include "definitions.h"

class FCB;
class MemoryManager;
class Partition;

class DirectoryManager {
	MemoryManager* memory_manager;

	ClusterNo root_dir_cluster_0;
	IndexEntry root_dir_index_0[NUM_INDEX_ENTRIES];

	unsigned int last_index;
	unsigned int last_index_count;

	Partition* partition;
public:
	DirectoryManager(Partition* partition, MemoryManager* memory_manager);
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> find_file(const char* file_name);
	bool add_file(const FCB& file_info);
	bool delete_file(const char* file_name);
};