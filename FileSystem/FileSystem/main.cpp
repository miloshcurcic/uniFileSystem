#include "part.h"
#include "cluster_cache.h"
#include <iostream>

void write5chars(char* arr, unsigned int start_pos) {
	for (int i = 0; i < 5; i++) {
		std::cout << arr[start_pos + i];
	}
}

int main() {
	auto part = new Partition((char*)"p1.ini");
	auto cluster_cache = new ClusterCache(part);

	char* r_buffer = new char[5];
	char* w_buffer = new char[5]{ "test" };
	char* cluster = std::get<2>(*cluster_cache->find_cluster(5));

	write5chars(cluster, 10);

	cluster_cache->read_cluster(5, 10, 5, r_buffer);

	write5chars(r_buffer, 0);

	cluster_cache->write_cluster(5, 10, 5, w_buffer);
	
	write5chars(cluster, 10);
}