#include "directory_cache.h"
#include "part.h"

DirectoryCache::DirectoryCache(Partition* partition, ClusterNo root_dir_index_0_cluster)
{
	// Read root dir index 0 for quick access
	partition->readCluster(root_dir_index_0_cluster, (char*)root_dir_index_0);
}



void DirectoryCache::add_directory_cluster(ClusterNo cluster_no, char* buffer)
{
	std::tuple<ClusterNo, bool, char*> cluster_tuple { cluster_no, false, buffer };
	if (directory_queue.size() == DIRECTORY_TABLE_CACHE_SIZE) {
		auto returning = directory_queue.back();
		directory_queue.pop_back();
		directory_clusters.erase(std::get<0>(returning));
		
		if (std::get<1>(returning)) {
			//Cluster was edited
			partition->writeCluster(std::get<0>(returning), std::get<2>(returning));
		}
	}

	directory_clusters.insert(cluster_no);
	directory_queue.push_front(cluster_tuple);
}

FCB DirectoryCache::find_file(const char* file_name)
{
	for (auto cluster_tuple : directory_queue) {
		FCB* fcb_array = (FCB*)std::get<2>(cluster_tuple);
		for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
			if (memcmp(&fcb_array[i], &ZERO_FCB, sizeof(FCB))) {
				break;
			}
			if (memcmp(&fcb_array[i].name, &file_name, FNAMELEN * sizeof(char))) {
				return fcb_array[i];
			}
		}
	}

	IndexEntry* temp0 = new IndexEntry[NUM_INDEX_ENTRIES];
	FCB* temp1 = new FCB[NUM_DIRECTORY_ENTRIES];

	for (auto cluster_index : root_dir_index_0) {
		partition->readCluster(cluster_index, (char*)temp0);

		for (int i = 0; i < NUM_INDEX_ENTRIES; i++) {
			if (directory_clusters.find(temp0[i]) != directory_clusters.end()) {
				continue;
			}
			partition->readCluster(temp0[i], (char*)temp1);

			for (int j = 0; j < NUM_DIRECTORY_ENTRIES; j++) {
				if (memcmp(&temp1[j], &ZERO_FCB, sizeof(FCB))) {
					break;
				}
				if (memcmp(&temp1[j].name, &file_name, FNAMELEN * sizeof(char))) {
					add_directory_cluster(temp0[i], (char*)temp1);
					delete temp0;
					return temp1[j];
				}
			}
		}
	}

	delete temp0;
	delete temp1;
	return ZERO_FCB;
}

bool DirectoryCache::delete_file(const char* file_name) // not done
{
	for (auto cluster_tuple : directory_queue) {
		FCB* fcb_array = (FCB*)std::get<2>(cluster_tuple);
		for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
			if (memcmp(&fcb_array[i], &ZERO_FCB, sizeof(FCB))) {
				break;
			}
			if (memcmp(&fcb_array[i].name, &file_name, FNAMELEN * sizeof(char))) {
				int j = i;
				for (j = i; j < NUM_DIRECTORY_ENTRIES - 1; j++) { // check if this is buggy
					if (memcmp(&fcb_array[j + 1], &ZERO_FCB, sizeof(FCB))) {
						break;
					}
					fcb_array[j] = fcb_array[j + 1];
				}
				fcb_array[j] = ZERO_FCB;
				return;
			}
		}
	}

	IndexEntry* temp0 = new IndexEntry[NUM_INDEX_ENTRIES];
	FCB* temp1 = new FCB[NUM_DIRECTORY_ENTRIES];

	for (auto cluster_index : root_dir_index_0) {
		partition->readCluster(cluster_index, (char*)temp0);

		for (int i = 0; i < NUM_INDEX_ENTRIES; i++) {
			if (directory_clusters.find(temp0[i]) != directory_clusters.end()) {
				continue;
			}
			partition->readCluster(temp0[i], (char*)temp1);

			for (int j = 0; j < NUM_DIRECTORY_ENTRIES; j++) {
				if (memcmp(&temp1[j], &ZERO_FCB, sizeof(FCB))) {
					break;
				}
				if (memcmp(&temp1[j].name, &file_name, FNAMELEN * sizeof(char))) {
					add_directory_cluster(temp0[i], (char*)temp1);
					int j = i;
					for (j = i; j < NUM_DIRECTORY_ENTRIES - 1; j++) { // check if this is buggy
						if (memcmp(&temp1[j + 1], &ZERO_FCB, sizeof(FCB))) {
							break;
						}
						temp1[j] = temp1[j + 1];
					}
					temp1[j] = ZERO_FCB;
					delete temp0;
					return;
				}
			}
		}
	}

	delete temp0;
	delete temp1;
}