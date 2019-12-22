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
		cluster_map.erase(cluster_no);
	}

	queue.push_front(cluster_ptr);
	cluster_map[cluster_no] = queue.begin();

	return cluster_ptr;
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

