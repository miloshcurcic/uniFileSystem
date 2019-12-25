#pragma once
#include "definitions.h"
#include "cluster_cache.h"
#include "fcb.h"

class DirectoryCache {
	ClusterCache directory_index_cache;
	ClusterCache directory_cache;
	IndexEntry root_dir_0[NUM_INDEX_ENTRIES];
	const FCB zero_fcb;
public:
	DirectoryCache(Partition *partition);
	FCB find_file(const char* file_name);
	void delete_file(const char* file_name);
	unsigned int calculate_hash(const char *file_name);
	unsigned int get_index_0(unsigned int hash);
	unsigned int get_index_1(unsigned int hash);
};