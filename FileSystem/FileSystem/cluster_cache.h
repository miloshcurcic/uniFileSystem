#pragma once

#include "definitions.h"

class Partition;

class ClusterCache {
	std::list<std::tuple<ClusterNo, bool, char[ClusterSize]>*> queue;
	std::unordered_map<ClusterNo, std::list<std::tuple<ClusterNo, bool, char[ClusterSize]>*>::iterator> cluster_map;
	std::unordered_set<ClusterNo> clusters;
	
	WinMutex cache_mutex;

	Partition *partition;

	std::tuple<ClusterNo, bool, char[ClusterSize]>* find_cluster(ClusterNo cluster_no);
public:
	ClusterCache(Partition* partition);
	void write_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer);
	void read_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer);
	void flush_cache();

	~ClusterCache();
};