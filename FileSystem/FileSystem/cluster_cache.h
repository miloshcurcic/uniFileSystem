#pragma once

#include "definitions.h"

class Partition;

class ClusterCache {
	std::list<std::tuple<ClusterNo, bool, char[ClusterSize]>*> queue;
	std::unordered_map<ClusterNo, std::list<std::tuple<ClusterNo, bool, char[ClusterSize]>*>::iterator> cluster_map;
	
	unsigned int cache_size;

	WinMutex cache_mutex;

	Partition *partition;

	std::tuple<ClusterNo, bool, char[ClusterSize]>* find_cluster(ClusterNo cluster_no);
public:
	ClusterCache(Partition* partition, unsigned int size = CLUSTER_CACHE_SIZE);
	void write_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer);
	void read_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer);
	void flush_cache();
	void flush_cluster_if_exists(ClusterNo cluster_no);

	~ClusterCache();
};