#include "cluster_cache.h"
#include "part.h"
#include "win_mutex.h"

#include <tuple>

ClusterCache::ClusterCache(Partition* partition)
{
	this->partition = partition;
	this->cache_mutex = new WinMutex();
}

std::tuple<ClusterNo, bool, char[ClusterSize]>* ClusterCache::find_cluster(ClusterNo cluster_no)
{
	std::tuple<ClusterNo, bool, char[ClusterSize]> *cluster_ptr;
	if (cluster_map.find(cluster_no) == cluster_map.end()) {
		if (cluster_map.size() != CLUSTER_CACHE_SIZE) {
			cluster_ptr = new std::tuple<ClusterNo, bool, char[ClusterSize]>();

			clusters.insert(cluster_no);
			partition->readCluster(cluster_no, std::get<2>(*cluster_ptr));
			std::get<0>(*cluster_ptr) = cluster_no;
			std::get<1>(*cluster_ptr) = false;
		}
		else {
			cluster_ptr = queue.back();
			queue.pop_back();
			cluster_map.erase(std::get<0>(*cluster_ptr));
			clusters.erase(std::get<0>(*cluster_ptr));

			if (std::get<1>(*cluster_ptr)) {
				//Cluster was edited
				partition->writeCluster(std::get<0>(*cluster_ptr), std::get<2>(*cluster_ptr));
			}

			clusters.insert(cluster_no);
			partition->readCluster(cluster_no, std::get<2>(*cluster_ptr));
			std::get<0>(*cluster_ptr) = cluster_no;
			std::get<1>(*cluster_ptr) = false;
		}
	}
	else {
		cluster_ptr = *cluster_map[cluster_no];
		queue.erase(cluster_map[cluster_no]);
	}

	queue.push_front(cluster_ptr);
	cluster_map[cluster_no] = queue.begin();

	return cluster_ptr;
}

void ClusterCache::write_cluster(ClusterNo cluster_no, BytesCnt start_pos,BytesCnt bytes, const char* buffer)
{
	cache_mutex->wait();
	// check if start_pos + bytes > cluster_length
	auto cluster_info = find_cluster(cluster_no);
	auto cluster_data = std::get<2>(*cluster_info);
	for (unsigned int i = start_pos; i < start_pos + bytes; i++) {
		cluster_data[i] = buffer[i - start_pos];
	}
	std::get<1>(*cluster_info) = true;
	cache_mutex->signal();
}

void ClusterCache::read_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer)
{
	cache_mutex->wait();
	// check if start_pos + bytes > cluster_length
	auto cluster_info = find_cluster(cluster_no);
	auto cluster_data = std::get<2>(*cluster_info);
	for (unsigned int i = start_pos; i < start_pos + bytes; i++) {
		buffer[i-start_pos] = cluster_data[i];
	}
	cache_mutex->signal();
}

void ClusterCache::flush_cache()
{
	cache_mutex->wait();
	for (auto tuple : queue) {
		if (std::get<1>(*tuple)) {
			partition->writeCluster(std::get<0>(*tuple), std::get<2>(*tuple));
		}
		delete tuple;
	}
	queue.clear();
	cluster_map.clear();
	cache_mutex->signal();
}

ClusterCache::~ClusterCache()
{
	flush_cache();
	delete cache_mutex;
}