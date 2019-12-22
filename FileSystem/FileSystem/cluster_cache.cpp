#include "cluster_cache.h"
#include "part.h"

#include <tuple>

ClusterCache::ClusterCache(Partition* partition, unsigned int max_length)
{
	this->partition = partition;
	this->max_length = max_length;
}

std::tuple<ClusterNo, bool, char[ClusterSize]>* ClusterCache::find_cluster(ClusterNo cluster_no)
{
	std::tuple<ClusterNo, bool, char[ClusterSize]> *cluster_ptr;
	if (cluster_map.find(cluster_no) == cluster_map.end()) {
		if (cluster_map.size() != max_length) {
			cluster_ptr = new std::tuple<ClusterNo, bool, char[ClusterSize]>();

			partition->readCluster(cluster_no, std::get<2>(*cluster_ptr));
			std::get<0>(*cluster_ptr) = cluster_no;
			std::get<1>(*cluster_ptr) = false;
		}
		else {
			cluster_ptr = queue.back();
			queue.pop_back();
			cluster_map.erase(std::get<0>(*cluster_ptr));

			if (std::get<1>(*cluster_ptr)) {
				//Cluster was edited
				partition->writeCluster(std::get<0>(*cluster_ptr), std::get<2>(*cluster_ptr));
			}

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
	// check if start_pos + bytes > cluster_length
	auto cluster_info = find_cluster(cluster_no);
	auto cluster_data = std::get<2>(*cluster_info);
	for (int i = start_pos; i < start_pos + bytes; i++) {
		cluster_data[i] = buffer[i - start_pos];
	}
	std::get<1>(*cluster_info) = true;
}

void ClusterCache::read_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer)
{
	// check if start_pos + bytes > cluster_length
	auto cluster_info = find_cluster(cluster_no);
	auto cluster_data = std::get<2>(*cluster_info);
	for (int i = start_pos; i < start_pos + bytes; i++) {
		buffer[i-start_pos] = cluster_data[i];
	}
	std::get<1>(*cluster_info) = true;

}

void ClusterCache::flush_cache()
{
	for (auto tuple : queue) {
		if (std::get<1>(*tuple)) {
			partition->writeCluster(std::get<0>(*tuple), std::get<2>(*tuple));
		}
	}
	queue.clear();
	cluster_map.clear();
}

ClusterCache::~ClusterCache()
{
	flush_cache();
}

