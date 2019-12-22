#pragma once

#include "definitions.h"

class Partition;

class ClusterCache {
	std::deque<std::tuple<ClusterNo, bool, char[ClusterSize]>*> queue;
	std::unordered_map<ClusterNo, std::deque<std::tuple<ClusterNo, bool, char[ClusterSize]>*>::iterator> cluster_map;

	Partition *partition;
	unsigned int max_length;
public:
	
	ClusterCache(Partition* partition, unsigned int max_length);
	std::tuple<ClusterNo, bool, char[ClusterSize]>* find_cluster(ClusterNo cluster_no);
	void flush_cache();

	~ClusterCache();
};