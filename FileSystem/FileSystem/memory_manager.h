#pragma once

#include "definitions.h"

class Partition;

class MemoryManager {
	unsigned int bit_vector_size;
	unsigned char** bit_vector;
	Partition* partition;
	
	WinMutex memory_mutex;
	unsigned char empty_cluster[ClusterSize];
	unsigned long available_clusters;

	ClusterNo allocate_cluster_internal(ClusterNo near_to);
public:
	MemoryManager(Partition* partition);
	ClusterNo allocate_cluster(ClusterNo near_to = 0);
	ClusterNo allocate_empty_cluster(ClusterNo near_to = 0);
	std::list<ClusterNo> allocate_n_clusters(ClusterNo near_to, unsigned int count);
	std::list<ClusterNo> allocate_n_clusters_internal(ClusterNo near_to, unsigned int count);
	void deallocate_cluster(ClusterNo cluster);
	void deallocate_n_clusters(std::list<ClusterNo>& clusters);
	void format();
	~MemoryManager();
};