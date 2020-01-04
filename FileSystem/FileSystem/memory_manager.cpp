#include "memory_manager.h"
#include "part.h"
#include <math.h>

MemoryManager::MemoryManager(Partition* partition)
{
	this->partition = partition;

	bit_vector_size = partition->getNumOfClusters() / (ClusterSize * BYTE_LEN) + (partition->getNumOfClusters() % (ClusterSize * BYTE_LEN) != 0 ? 1 : 0);
	unsigned int upper_byte_limit = partition->getNumOfClusters() % (ClusterSize * BYTE_LEN) != 0 ? (partition->getNumOfClusters() % (ClusterSize * BYTE_LEN)) / BYTE_LEN : ClusterSize;
	unsigned int upper_bit_limit = (partition->getNumOfClusters() % BYTE_LEN) != 0 ? (1 << (partition->getNumOfClusters() % BYTE_LEN)) : 0;

	memset(empty_cluster, 0, ClusterSize * sizeof(unsigned char));

	bit_vector = new unsigned char* [bit_vector_size];
	for (unsigned int i = 0; i < bit_vector_size; i++) {
		bit_vector[i] = new unsigned char[ClusterSize];
		partition->readCluster(i, (char*)(bit_vector[i]));
		unsigned int i_limit = ClusterSize;
		if (i == (bit_vector_size - 1)) {
			i_limit = upper_byte_limit;
		}
		for (unsigned long j = 0; j < upper_byte_limit; j++) {
			unsigned int bit_limit = 0;
			if ((i == (bit_vector_size - 1)) && (j == (upper_byte_limit - 1))) {
				bit_limit = upper_bit_limit;
			}
			for (unsigned char k = 1; k != bit_limit; k <<= 1) {
				if ((bit_vector[i][j] & k) != 0) {
					available_clusters++;
				}
			}
		}
	}
}

ClusterNo MemoryManager::allocate_cluster_internal(ClusterNo near_to) {
	if (available_clusters == 0) {
		return 0;
	}

	available_clusters--;

	if (near_to == 0) {
		// Try to leave the first 10% of disk for the directory
		near_to = (ClusterNo)(partition->getNumOfClusters() * 0.1) + rand() % (ClusterNo)(0.8 * partition->getNumOfClusters());
	}

	unsigned int index = near_to / (ClusterSize * BYTE_LEN);
	unsigned int at = (near_to % (ClusterSize * BYTE_LEN)) / BYTE_LEN;

	unsigned int upper_byte_limit = partition->getNumOfClusters() % (ClusterSize * BYTE_LEN) != 0 ? (partition->getNumOfClusters() % (ClusterSize * BYTE_LEN)) / BYTE_LEN : ClusterSize;
	unsigned int upper_bit_limit = (partition->getNumOfClusters() % BYTE_LEN) != 0 ? (1 << (partition->getNumOfClusters() % BYTE_LEN)) : 0;
	
	ClusterNo cluster = 0;

	for (unsigned int cluster_id = index; cluster_id < bit_vector_size; cluster_id++) {
		unsigned int byte_limit = ClusterSize;
		if (cluster_id == bit_vector_size - 1) {
			byte_limit = upper_byte_limit;
		}
		for (unsigned int byte_id = (cluster_id == index ? at : 0); byte_id < upper_byte_limit; byte_id++) {
			unsigned int bit_limit = 0;
			if ((cluster_id == (bit_vector_size - 1)) && (byte_id == (upper_byte_limit - 1))) {
				bit_limit = upper_bit_limit;
			}
			for (unsigned char bit_id = (byte_id == at && cluster_id == index ? (unsigned char)1 << (near_to % BYTE_LEN) : 1); bit_id != bit_limit; bit_id <<= 1) {
				if ((bit_vector[cluster_id][byte_id] & bit_id) != 0) {
					cluster = cluster_id * ClusterSize * BYTE_LEN + byte_id * BYTE_LEN + (ClusterNo)log2(bit_id);
					bit_vector[cluster_id][byte_id] &= ~bit_id;
					break;
				}
			}
			if (cluster != 0) {
				break;
			}
		}
		if (cluster != 0) {
			break;
		}
	}
	if (cluster == 0) {
		for (unsigned int cluster_id = 0; cluster_id <= index; cluster_id++) {
			for (unsigned int byte_id = 0; byte_id < (cluster_id == index ? at : ClusterSize); byte_id++) {
				for (unsigned char bit_id = 1; bit_id != (cluster_id == index && byte_id == at ? (unsigned char)1 << (near_to % BYTE_LEN) : 0); bit_id <<= 1) {
					if ((bit_vector[cluster_id][byte_id] & bit_id) != 0) {
						cluster = cluster_id * ClusterSize * BYTE_LEN * byte_id * BYTE_LEN + (unsigned char)log2(bit_id);
						bit_vector[cluster_id][byte_id] &= ~bit_id;
						break;
					}
				}
				if (cluster != 0) {
					break;
				}
			}
			if (cluster != 0) {
				break;
			}
		}
	}
	return cluster;
}

ClusterNo MemoryManager::allocate_cluster(ClusterNo near_to)
{
	memory_mutex.wait();
	ClusterNo res = allocate_cluster_internal(near_to);
	memory_mutex.signal();
	return res;
}

ClusterNo MemoryManager::allocate_empty_cluster(ClusterNo near_to)
{
	memory_mutex.wait();
	ClusterNo allocated = allocate_cluster_internal(near_to);
	if (allocated == 0) {
		memory_mutex.signal();
		return 0;
	}
	partition->writeCluster(allocated, (char*)empty_cluster);
	memory_mutex.signal();
	return allocated;
	
}

std::list<ClusterNo> MemoryManager::allocate_n_clusters(ClusterNo near_to, unsigned int count)
{
	memory_mutex.wait();
	std::list<ClusterNo> res = allocate_n_clusters_internal(near_to, count);
	memory_mutex.signal();
	return res;
}

std::list<ClusterNo> MemoryManager::allocate_n_clusters_internal(ClusterNo near_to, unsigned int count)
{
	std::list<ClusterNo> res;
	if (available_clusters < count) {
		return res;
	}

	available_clusters-= count;

	if (near_to == 0) {
		// Try to leave the first 10% of disk for the directory
		near_to = (ClusterNo)(partition->getNumOfClusters() * 0.1) + rand() % (ClusterNo)(0.8 * partition->getNumOfClusters());
	}

	unsigned int index = near_to / (ClusterSize * BYTE_LEN);
	unsigned int at = (near_to % (ClusterSize * BYTE_LEN)) / BYTE_LEN;
	unsigned int set_byte_limit = partition->getNumOfClusters() % (ClusterSize * BYTE_LEN);

	unsigned int upper_byte_limit = partition->getNumOfClusters() % (ClusterSize * BYTE_LEN) != 0 ? (partition->getNumOfClusters() % (ClusterSize * BYTE_LEN)) / BYTE_LEN : ClusterSize;
	unsigned int upper_bit_limit = (partition->getNumOfClusters() % BYTE_LEN) != 0 ? (1 << (partition->getNumOfClusters() % BYTE_LEN)) : 0;

	ClusterNo cluster = 0;

	for (unsigned int cluster_id = index; cluster_id < bit_vector_size; cluster_id++) {
		unsigned int byte_limit = ClusterSize;
		if (cluster_id == bit_vector_size - 1) {
			byte_limit = upper_byte_limit;
		}
		for (unsigned int byte_id = (cluster_id == index ? at : 0); byte_id < upper_byte_limit; byte_id++) {
			unsigned int bit_limit = 0;
			if ((cluster_id == (bit_vector_size - 1)) && (byte_id == (upper_byte_limit - 1))) {
				bit_limit = upper_bit_limit;
			}
			for (unsigned char bit_id = (byte_id == at && cluster_id == index ? (unsigned char)1 << (near_to % BYTE_LEN) : 1); bit_id != bit_limit; bit_id <<= 1) {
				if ((bit_vector[cluster_id][byte_id] & bit_id) != 0) {
					cluster = cluster_id * ClusterSize * BYTE_LEN + byte_id * BYTE_LEN + (unsigned char)log2(bit_id);
					near_to = cluster;
					bit_vector[cluster_id][byte_id] &= ~bit_id;
					res.push_back(cluster);
					if (res.size() == count) {
						break;
					}
				}
			}
			if (res.size() == count) {
				break;
			}
		}
		if (res.size() == count) {
			break;
		}
	}
	if (res.size() != count) {
		for (unsigned int cluster_id = 0; cluster_id <= index; cluster_id++) {
			for (unsigned int byte_id = 0; byte_id < (cluster_id == index ? at : ClusterSize); byte_id++) {
				for (unsigned char bit_id = 1; bit_id != (cluster_id == index && byte_id == at ? (unsigned char)1 << (near_to % BYTE_LEN) : 0); bit_id <<= 1) {
					if ((bit_vector[cluster_id][byte_id] & bit_id) != 0) {
						cluster = cluster_id * ClusterSize * BYTE_LEN * byte_id * BYTE_LEN + (unsigned char)log2(bit_id);
						bit_vector[cluster_id][byte_id] &= ~bit_id;
						near_to = cluster;
						res.push_back(cluster);
						if (res.size() == count) {
							break;
						}
					}
				}
				if (res.size() == count) {
					break;
				}
			}
			if (res.size() == count) {
				break;
			}
		}
	}
	return res;
}

void MemoryManager::deallocate_cluster(ClusterNo cluster)
{
	memory_mutex.wait();
	unsigned int bit_vector_index = cluster / (ClusterSize * BYTE_LEN);
	unsigned int inner_index = (cluster % (ClusterSize * BYTE_LEN)) / BYTE_LEN;
	unsigned int bit_pos = cluster % BYTE_LEN;
	
	bit_vector[bit_vector_index][inner_index] |= (char)(1 << bit_pos);
	available_clusters++;
	memory_mutex.signal();
}

void MemoryManager::deallocate_n_clusters(std::list<ClusterNo>& clusters)
{
	for (auto cluster : clusters) {
		deallocate_cluster(cluster);
	}
	clusters.clear();
}

void MemoryManager::format()
{
	memory_mutex.wait();
	available_clusters = partition->getNumOfClusters();
	for (unsigned int i = 0; i < bit_vector_size; i++) {
		memset(bit_vector[i], -1, ClusterSize * sizeof(unsigned char));
	}
	available_clusters -= bit_vector_size + 1;
	// <= root dir index 0
	for (unsigned int i = 0; i <= bit_vector_size; i++) {
		bit_vector[i / (ClusterSize * BYTE_LEN)][(i % (ClusterSize * BYTE_LEN)) / BYTE_LEN] &= ~((unsigned char)1 << i % BYTE_LEN);
	}
	memory_mutex.signal();
}

MemoryManager::~MemoryManager()
{
	memory_mutex.wait();
	for (unsigned int i = 0; i < bit_vector_size; i++) {
		partition->writeCluster(i, (char*)bit_vector[i]);
		delete bit_vector[i];
	}
	delete bit_vector;
	bit_vector = nullptr;
	memory_mutex.signal();
}
