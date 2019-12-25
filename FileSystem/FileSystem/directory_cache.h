#pragma once
#include "definitions.h"
#include "cluster_cache.h"
#include "fcb.h"

class DirectoryCache {
	IndexEntry root_dir_index_0[NUM_INDEX_ENTRIES];

	std::list<std::tuple<ClusterNo, bool, char*>> directory_queue;
	std::unordered_set<ClusterNo> directory_clusters;

	Partition* partition;
public:
	DirectoryCache(Partition* partition, ClusterNo root_dir_index_0_cluster);
	FCB find_file(const char* file_name);
	bool delete_file(const char* file_name);

	void add_directory_cluster(ClusterNo cluster_no, char* buffer);
};