#include "memory_manager.h"
#include "part.h"
#include <math.h>

MemoryManager::MemoryManager(Partition *partition)
{
	this->partition = partition;
	bit_vector_size = ClusterSize * BYTE_LEN / partition->getNumOfClusters() + (ClusterSize * BYTE_LEN % partition->getNumOfClusters() != 0 ? 1 : 0);

	memset(empty_cluster, 0, ClusterSize * sizeof(unsigned char));

	bit_vector = new unsigned char* [bit_vector_size];
	for (int i = 0; i < bit_vector_size; i++) {
		bit_vector[i] = new unsigned char[ClusterSize];
		partition->readCluster(i, (char*)bit_vector[i]);
	}
}

ClusterNo MemoryManager::allocate_cluster(ClusterNo near_to)
{
	int index = near_to / (ClusterSize * BYTE_LEN);
	int at = near_to % (ClusterSize * BYTE_LEN);
	
	int min, max, min_at, max_at;
	ClusterNo min_cluster = 0, max_cluster = 0;

	for (min = index; min >= 0; min--) {
		for (min_at = (min == index ? at : ClusterSize - 1); min_at >= 0; min_at--) {
			if (bit_vector[min][min_at] != 0) {
				 min_cluster = min * ClusterSize * BYTE_LEN + min_at * BYTE_LEN + (ClusterNo)log2(bit_vector[min][min_at]);
				 break;
			}
		}
	}
	
	for (max = index; max < bit_vector_size; min--) {
		for (max_at = (max == index ? at + 1 : 0); max_at < ClusterSize; max_at++) {
			if (bit_vector[max][max_at] != 0) {
				max_cluster = max * ClusterSize * BYTE_LEN + max_at * BYTE_LEN + (ClusterNo)log2(bit_vector[max][max_at] & -(char)bit_vector[max][max_at]);
				break;
			}
		}
	}

	if (min >= 0 && max < bit_vector_size) {
		return (near_to - min_cluster < max_cluster - near_to ? min_cluster : max_cluster);
	}
	else if (min >= 0) {
		return min_cluster;
	}
	else {
		return max_cluster;
	}
}

ClusterNo MemoryManager::allocate_empty_cluster(ClusterNo near_to)
{
	ClusterNo allocated = allocate_cluster(near_to);
	// Check if allocation failed?
	partition->writeCluster(allocated, (char*)empty_cluster);
	return allocated;
	
}

void MemoryManager::deallocate_cluster(ClusterNo cluster)
{
	unsigned int bit_vector_index = cluster / (ClusterSize * BYTE_LEN);
	unsigned int inner_index = cluster % (ClusterSize * BYTE_LEN);
	unsigned int bit_pos = cluster / BYTE_LEN;
	
	bit_vector[bit_vector_index][inner_index] &= ~(char)(1 << bit_pos);
}

MemoryManager::~MemoryManager()
{
	// Return in-mem bit-vector to disc and deallocate it
	for (int i = 0; i < bit_vector_size; i++) {
		partition->writeCluster(i, (char*)bit_vector[i]);
		delete bit_vector[i];
	}
	delete bit_vector;
	bit_vector = nullptr;
}
