#include "directory_manager.h"
#include "memory_manager.h"
#include "fcb.h"
#include "file_handle.h"
#include "part.h"

DirectoryManager::DirectoryManager(Partition* partition, MemoryManager* memory_manager)
{
	root_dir_cluster_0 = ClusterSize * BYTE_LEN / partition->getNumOfClusters() + (ClusterSize * BYTE_LEN % partition->getNumOfClusters() != 0 ? 1 : 0);
	this->memory_manager = memory_manager;
	this->partition = partition;

	partition->readCluster(root_dir_cluster_0, (char*)root_dir_index_0);

	last_index = last_index_count = 0;
	while ((last_index / NUM_INDEX_ENTRIES) + 1 != NUM_INDEX_ENTRIES && root_dir_index_0[(last_index / NUM_INDEX_ENTRIES) + 1] != 0) {
		last_index += NUM_INDEX_ENTRIES;
	}

	if (root_dir_index_0[(last_index / NUM_INDEX_ENTRIES)] != 0) {
		IndexEntry temp[NUM_INDEX_ENTRIES];
		partition->readCluster(root_dir_index_0[(last_index / NUM_INDEX_ENTRIES)], (char*)temp);
		while ((last_index % NUM_INDEX_ENTRIES) + 1 != NUM_INDEX_ENTRIES && (temp[(last_index % NUM_INDEX_ENTRIES) + 1] != 0)) {
			last_index++;
		}

		if (temp[last_index % NUM_INDEX_ENTRIES] != 0) {
			FCB data_temp[NUM_DIRECTORY_ENTRIES];
			partition->readCluster(temp[last_index % NUM_INDEX_ENTRIES], (char*)data_temp);
			for (last_index_count = 0; last_index_count < NUM_DIRECTORY_ENTRIES; last_index_count++) {
				if (memcmp(&data_temp[last_index_count], &ZERO_FCB, sizeof(FCB)) == 0) {
					break;
				}
			}
		}
	}

}

std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> DirectoryManager::find_file(const char* file_name, const char* file_ext)
{
	IndexEntry temp0[NUM_INDEX_ENTRIES];
	FCB temp1[NUM_DIRECTORY_ENTRIES];

	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> res;
	memset(&res, 0, sizeof(std::tuple<IndexEntry, IndexEntry, unsigned int, FCB>));

	for (auto cluster_index : root_dir_index_0) {
		partition->readCluster(cluster_index, (char*)temp0);

		for (int i = 0; i < NUM_INDEX_ENTRIES; i++) {
			partition->readCluster(temp0[i], (char*)temp1);

			for (int j = 0; j < NUM_DIRECTORY_ENTRIES; j++) {
				if (memcmp(&temp1[j], &ZERO_FCB, sizeof(FCB))) {
					return res;
				}
				if ((memcmp(&temp1[j].name, &file_name, FNAMELEN * sizeof(char)) == 0) && (memcmp(&temp1[j].ext, &file_ext, FEXTLEN * sizeof(char)) == 0)) {
					std::get<3>(res) = temp1[j];
					return res;
				}
				std::get<2>(res)++;
			}
			std::get<1>(res)++;
			std::get<2>(res) = 0;
		}
		std::get<0>(res)++;
		std::get<1>(res) = 0;
	}
	return res;
}

FileHandle* DirectoryManager::create_file_handle(const char* file_name, const char* file_ext)
{
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_name, file_ext);
	if (std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) == 0) {
		return nullptr;
	}

	return new FileHandle(std::get<3>(file_data));	
}

FileHandle* DirectoryManager::add_and_get_file_handle(const FCB& fcb)
{
	if (add_file(fcb) == false) {
		return nullptr;
	}

	return new FileHandle(fcb);
}

bool DirectoryManager::add_file(const FCB& file_info)
{
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_info.name, file_info.ext);
	if (std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) != 0) {
		return false;
	}

	if (last_index_count == NUM_DIRECTORY_ENTRIES) {
		last_index++;
		last_index_count = 0;
	}

	if (root_dir_index_0[last_index / NUM_INDEX_ENTRIES] == 0) {
		// Check if allocation failed
		unsigned int near_to = 0;
		if (last_index == 0) {
			near_to = root_dir_cluster_0;
		}
		else {
			near_to = root_dir_index_0[((last_index / NUM_INDEX_ENTRIES) + NUM_INDEX_ENTRIES - 1) % NUM_INDEX_ENTRIES];
		}
		root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = memory_manager->allocate_empty_cluster(near_to);
	}

	IndexEntry temp0[NUM_INDEX_ENTRIES];
	partition->readCluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], (char*)temp0);

	if (temp0[last_index % NUM_INDEX_ENTRIES] == 0) {
		// What if alloc failed
		unsigned int near_to = 0;
		if (last_index % NUM_INDEX_ENTRIES != 0) {
			near_to = temp0[((last_index % NUM_INDEX_ENTRIES) + NUM_INDEX_ENTRIES - 1) % NUM_INDEX_ENTRIES];
		}
		else {
			near_to = root_dir_index_0[last_index / NUM_INDEX_ENTRIES];
		}
		temp0[last_index % NUM_INDEX_ENTRIES] = memory_manager->allocate_empty_cluster(near_to);
		partition->writeCluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], (char*)temp0);
	}

	FCB temp1[NUM_DIRECTORY_ENTRIES];
	partition->readCluster(temp0[last_index % NUM_INDEX_ENTRIES], (char*)temp1);

	temp1[last_index_count] = file_info;
	last_index_count++;

	partition->writeCluster(temp0[last_index % NUM_INDEX_ENTRIES], (char*)temp1);
	return true;
}

bool DirectoryManager::delete_file(const char* file_name, const char* file_ext)
{
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_name, file_ext);
	if (std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) == 0) {
		return false;
	}

	// Remove clusters allocated by file, add cluster pool reusing logic?
	FCB file = std::get<3>(file_data);
	if (file.data0 != 0) {
		memory_manager->deallocate_cluster(file.data0);
		file.data0 = 0;
	}
	if (file.data1 != 0) {
		memory_manager->deallocate_cluster(file.data1);
		file.data1 = 0;
	}
	if (file.index1_0 != 0) {
		IndexEntry temp[NUM_INDEX_ENTRIES];
		partition->readCluster(file.index1_0, (char*)temp);
		for (IndexEntry i = 0; i < NUM_INDEX_ENTRIES; i++) {
			if (temp[i] != 0) {
				memory_manager->deallocate_cluster(temp[i]);
			}
			else {
				break;
			}
		}
		memory_manager->deallocate_cluster(file.index1_0);
		file.index1_0 = 0;
	}
	if (file.index2_0 != 0) {
		IndexEntry temp_l0[NUM_INDEX_ENTRIES];
		partition->readCluster(file.index2_0, (char*)temp_l0);
		for (IndexEntry i = 0; i < NUM_INDEX_ENTRIES; i++) {
			if (temp_l0[i] != 0) {
				IndexEntry temp_l1[NUM_INDEX_ENTRIES];
				partition->readCluster(temp_l0[i], (char*)temp_l1);
				for (IndexEntry j = 0; j < NUM_INDEX_ENTRIES; j++) {
					if (temp_l1[j] != 0) {
						memory_manager->deallocate_cluster(temp_l1[j]);
					}
					else {
						break;
					}
				}
				memory_manager->deallocate_cluster(temp_l0[i]);
			}
			else {
				break;
			}
		}
		memory_manager->deallocate_cluster(file.index2_0);
		file.index2_0 = 0;
	}

	IndexEntry temp0[NUM_INDEX_ENTRIES];
	FCB temp1[NUM_DIRECTORY_ENTRIES];

	partition->readCluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], (char*)temp0);
	partition->readCluster(temp0[last_index % NUM_INDEX_ENTRIES], (char*)temp1);
	last_index_count--;

	if (std::get<0>(file_data) * NUM_INDEX_ENTRIES + std::get<1>(file_data) == last_index) {
		if (last_index_count == 0) {
			if (last_index_count == 0) {
				memory_manager->deallocate_cluster(temp0[last_index % NUM_INDEX_ENTRIES]);
				if (last_index % NUM_INDEX_ENTRIES != 0) {
					temp0[last_index % NUM_INDEX_ENTRIES] = 0;
					partition->writeCluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], (char*)temp0);
				}
				else {
					memory_manager->deallocate_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES]);
					root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = 0;
				}

				if (last_index != 0) {
					last_index--;
					last_index_count = NUM_DIRECTORY_ENTRIES;
				}
			}
		}
		else {
			temp1[std::get<2>(file_data)] = temp1[last_index_count - 1];
			temp1[last_index_count - 1] = ZERO_FCB;
			partition->writeCluster(temp0[last_index % NUM_INDEX_ENTRIES], (char*)temp1);
		}
	}
	else {
		FCB moving = temp1[last_index_count];

		if (last_index_count == 0) {
			memory_manager->deallocate_cluster(temp0[last_index % NUM_INDEX_ENTRIES]);
			if (last_index % NUM_INDEX_ENTRIES != 0) {
				temp0[last_index % NUM_INDEX_ENTRIES] = 0;
				partition->writeCluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], (char*)temp0);
			}
			else {
				memory_manager->deallocate_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES]);
				root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = 0;
			}
			if (last_index != 0) {
				last_index--;
				last_index_count = NUM_DIRECTORY_ENTRIES;
			}
		}

		partition->readCluster(root_dir_index_0[std::get<0>(file_data)], (char*)temp0);
		partition->readCluster(temp0[std::get<1>(file_data)], (char*)temp1);

		temp1[std::get<2>(file_data)] = moving;

		partition->writeCluster(temp0[std::get<1>(file_data)], (char*)temp1);
	}

	return true;
}

FileCnt DirectoryManager::get_file_count()
{
	return last_index * NUM_DIRECTORY_ENTRIES + last_index_count;
}

bool DirectoryManager::does_file_exist(const char* file_name, const char* file_ext)
{
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_name, file_ext);

	return std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) == 0;
}
