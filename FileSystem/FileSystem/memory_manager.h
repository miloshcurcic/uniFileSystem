#pragma once

#include "definitions.h"

class Partition;

class MemoryManager {
	int bit_vector_size;
	unsigned char** bit_vector;
	Partition* partition;

	unsigned char empty_cluster[ClusterSize];
public:
	MemoryManager(Partition* partition);
	ClusterNo allocate_cluster(ClusterNo near_to = 0);
	ClusterNo allocate_empty_cluster(ClusterNo near_to = 0);
	void deallocate_cluster(ClusterNo cluster);
	~MemoryManager();
};