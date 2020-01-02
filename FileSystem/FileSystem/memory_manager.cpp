#include "memory_manager.h"
#include "part.h"
#include "win_mutex.h"
#include <math.h>

MemoryManager::MemoryManager(Partition *partition)
{
	this->memory_mutex = new WinMutex();
	this->partition = partition;
	bit_vector_size = partition->getNumOfClusters() / (ClusterSize * BYTE_LEN) + (partition->getNumOfClusters() % (ClusterSize * BYTE_LEN) != 0 ? 1 : 0);

	memset(empty_cluster, 0, ClusterSize * sizeof(unsigned char));

	bit_vector = new unsigned char* [bit_vector_size];
	for (int i = 0; i < bit_vector_size; i++) {
		bit_vector[i] = new unsigned char[ClusterSize];
		partition->readCluster(i, (char*)bit_vector[i]);
	}
}

ClusterNo MemoryManager::allocate_cluster_internal(ClusterNo near_to) {

	if (near_to == 0) {
		// Try to leave the first 10% of disk for the directory
		near_to = (ClusterNo)(partition->getNumOfClusters() * 0.1) + rand() % (ClusterNo)(0.9 * partition->getNumOfClusters());
	}

	int index = near_to / (ClusterSize * BYTE_LEN);
	int at = (near_to % (ClusterSize * BYTE_LEN)) / BYTE_LEN;

	int min, max, min_at, max_at;
	ClusterNo min_cluster = 0, max_cluster = 0;

	for (min = index; min >= 0; min--) {
		for (min_at = (min == index ? at : ClusterSize - 1); min_at >= 0; min_at--) {
			if (bit_vector[min][min_at] != 0) {
				min_cluster = min * ClusterSize * BYTE_LEN + min_at * BYTE_LEN + (ClusterNo)log2(bit_vector[min][min_at]);
				break;
			}
		}
		if (min_cluster != 0) {
			break;
		}
	}

	for (max = index; max < bit_vector_size; max++) {
		for (max_at = (max == index ? at + 1 : 0); max_at < (max == bit_vector_size - 1 ? partition->getNumOfClusters() % (ClusterSize * BYTE_LEN) : ClusterSize); max_at++) {
			if (bit_vector[max][max_at] != 0) {
				max_cluster = max * ClusterSize * BYTE_LEN + max_at * BYTE_LEN + (ClusterNo)log2(bit_vector[max][max_at] & -(char)bit_vector[max][max_at]);
				break;
			}
		}
		if (max_cluster != 0) {
			break;
		}
	}

	if (min >= 0 && max < bit_vector_size) {
		if (near_to - min_cluster < max_cluster - near_to) {
			bit_vector[min][min_at] &= ~((unsigned char)1 << min_cluster % BYTE_LEN);
			return min_cluster;
		}
		else {
			bit_vector[max][max_at] &= ~((unsigned char)1 << max_cluster % BYTE_LEN);
			return max_cluster;
		}
	}
	else if (min >= 0) {
		bit_vector[min][min_at] &= ~((unsigned char)1 << min_cluster % BYTE_LEN);
		return min_cluster;
	}
	else if (max < bit_vector_size) {
		bit_vector[max][max_at] &= ~((unsigned char)1 << max_cluster % BYTE_LEN);
		return max_cluster;
	}
}

ClusterNo MemoryManager::allocate_cluster(ClusterNo near_to)
{
	memory_mutex->wait();
	ClusterNo res = allocate_cluster_internal(near_to);
	memory_mutex->signal();
	return res;
}

ClusterNo MemoryManager::allocate_empty_cluster(ClusterNo near_to)
{

	memory_mutex->wait();
	ClusterNo allocated = allocate_cluster_internal(near_to);
	// Check if allocation failed?
	partition->writeCluster(allocated, (char*)empty_cluster);
	memory_mutex->signal();
	return allocated;
	
}

std::list<ClusterNo> MemoryManager::allocate_n_clusters(ClusterNo near_to, unsigned int count)
{

	memory_mutex->wait();
	std::list<ClusterNo> res;
	for (unsigned int i = 0; i < count; i++) {
		// error check
		ClusterNo cluster = allocate_cluster_internal(near_to);
		res.push_back(cluster);
		near_to = cluster;
	}
	memory_mutex->signal();
	return res;
}

void MemoryManager::deallocate_cluster(ClusterNo cluster)
{
	memory_mutex->wait();
	unsigned int bit_vector_index = cluster / (ClusterSize * BYTE_LEN);
	unsigned int inner_index = cluster % (ClusterSize * BYTE_LEN);
	unsigned int bit_pos = cluster % BYTE_LEN;
	
	bit_vector[bit_vector_index][inner_index] |= (char)(1 << bit_pos);
	memory_mutex->signal();
}

void MemoryManager::format()
{
	memory_mutex->wait();
	for (int i = 0; i < bit_vector_size; i++) {
		memset(bit_vector[i], -1, ClusterSize * sizeof(unsigned char));
	}
	for (int i = 0; i < bit_vector_size; i++) {
		bit_vector[i / (ClusterSize * BYTE_LEN)][i % (ClusterSize * BYTE_LEN)] &= ~((unsigned char)1 << i % BYTE_LEN);
	}
	memory_mutex->signal();
}

MemoryManager::~MemoryManager()
{
	memory_mutex->wait();
	// Return in-mem bit-vector to disc and deallocate it
	for (int i = 0; i < bit_vector_size; i++) {
		partition->writeCluster(i, (char*)bit_vector[i]);
		delete bit_vector[i];
	}
	delete bit_vector;
	bit_vector = nullptr;
	memory_mutex->signal();
}
