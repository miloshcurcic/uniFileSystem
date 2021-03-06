#include "directory_manager.h"
#include "memory_manager.h"
#include "fcb.h"
#include "file_handle.h"
#include "cluster_cache.h"
#include "part.h"

DirectoryManager::DirectoryManager(Partition* partition, MemoryManager* memory_manager)
{

	root_dir_cluster_0 =  partition->getNumOfClusters() / (ClusterSize * BYTE_LEN) +  (partition->getNumOfClusters() % (ClusterSize * BYTE_LEN) != 0 ? 1 : 0);
	this->memory_manager = memory_manager;
	this->partition = partition;

	this->directory_cache = new ClusterCache(partition, DIRECTORY_CACHE_SIZE);
	
	/* Read root directory index cluster 0, this will be buffered until DirectoryManager is destroyed */
	partition->readCluster(root_dir_cluster_0, (char*)root_dir_index_0);

	/* This is used for keeping track of total number of files and deleting files */
	/* From last_index we can get index0_entry and index1_entry which can help us */
	/* with managing directory size */
	last_index = last_index_count = 0;
	while ((last_index / NUM_INDEX_ENTRIES) + 1 != NUM_INDEX_ENTRIES && root_dir_index_0[(last_index / NUM_INDEX_ENTRIES) + 1] != 0) {
		last_index += NUM_INDEX_ENTRIES;
	}

	if (root_dir_index_0[(last_index / NUM_INDEX_ENTRIES)] != 0) {
		IndexEntry temp[NUM_INDEX_ENTRIES];
		directory_cache->read_cluster(root_dir_index_0[(last_index / NUM_INDEX_ENTRIES)], 0, ClusterSize * sizeof(unsigned char), (char*)temp);
		while ((last_index % NUM_INDEX_ENTRIES) + 1 != NUM_INDEX_ENTRIES && (temp[(last_index % NUM_INDEX_ENTRIES) + 1] != 0)) {
			last_index++;
		}

		if (temp[last_index % NUM_INDEX_ENTRIES] != 0) {
			FCB data_temp[NUM_DIRECTORY_ENTRIES];
			directory_cache->read_cluster(temp[last_index % NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)data_temp);
			for (last_index_count = 0; last_index_count < NUM_DIRECTORY_ENTRIES; last_index_count++) {
				if (memcmp(&data_temp[last_index_count], &ZERO_FCB, sizeof(FCB)) == 0) {
					break;
				}
			}
		}
	}

}

/* Linear search for file */
std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> DirectoryManager::find_file(const char* file_name, const char* file_ext)
{
	IndexEntry temp0[NUM_INDEX_ENTRIES];
	FCB temp1[NUM_DIRECTORY_ENTRIES];

	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> res;
	memset(&res, 0, sizeof(std::tuple<IndexEntry, IndexEntry, unsigned int, FCB>));

	char chk_name[FNAMELEN];
	int k = 0;
	while (k < FNAMELEN && file_name[k] != 0) {
		if (k >= FNAMELEN) {
			return res;
		}
		chk_name[k] = file_name[k];
		k++;
	}
	while (k < FNAMELEN) {
		chk_name[k] = ' ';
		k++;
	}

	char chk_ext[FEXTLEN];
	k = 0;
	while (k < FEXTLEN && file_ext[k] != 0) {
		if (k >= FEXTLEN) {
			return res;
		}
		chk_ext[k] = file_ext[k];
		k++;
	}
	while (k < FEXTLEN) {
		chk_ext[k] = ' ';
		k++;
	}

	for (auto cluster_index : root_dir_index_0) {
		if (cluster_index == 0) {
			return res;
		}
		directory_cache->read_cluster(cluster_index, 0, ClusterSize * sizeof(unsigned char), (char*)temp0);

		for (int i = 0; i < NUM_INDEX_ENTRIES; i++) {
			if (temp0[i] == 0) {
				return res;
			}
			directory_cache->read_cluster(temp0[i], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);

			for (int j = 0; j < NUM_DIRECTORY_ENTRIES; j++) {
				if (memcmp(&temp1[j], &ZERO_FCB, sizeof(FCB)) == 0) {
					return res;
				}
				if ((memcmp(&temp1[j].name, &chk_name, FNAMELEN * sizeof(char)) == 0) && (memcmp(&temp1[j].ext, &chk_ext, FEXTLEN * sizeof(char)) == 0)) {
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

std::tuple<IndexEntry, IndexEntry, unsigned int, FileHandle*> DirectoryManager::create_file_handle_and_get_pos(const char* file_name, const char* file_ext)
{
	directory_mutex.wait();
	
	std::tuple<IndexEntry, IndexEntry, unsigned int, FileHandle*> res;

	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_name, file_ext);

	if (std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) == 0) {
		std::get<3>(res) = nullptr;
		directory_mutex.signal();
		return res;
	}

	FileHandle* handle = new FileHandle(std::get<3>(file_data));

	std::get<0>(res) = std::get<0>(file_data);
	std::get<1>(res) = std::get<1>(file_data);
	std::get<2>(res) = std::get<2>(file_data);
	std::get<3>(res) = handle;
	
	directory_mutex.signal();
	return res;
}

/* If the file doesn't exist file handle cannot be created and we return nullptr, otherwise */
/* we create the file handle with the found FCB. */
FileHandle* DirectoryManager::create_file_handle(const char* file_name, const char* file_ext)
{
	directory_mutex.wait();
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_name, file_ext);

	if (std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) == 0) {
		directory_mutex.signal();
		return nullptr;
	}

	FileHandle* res = new FileHandle(std::get<3>(file_data));
	directory_mutex.signal();
	return res;	
}

void DirectoryManager::add_non_existing_file_internal(const FCB& file_info) {
	if (last_index_count == NUM_DIRECTORY_ENTRIES) {
		last_index++;
		last_index_count = 0;
	}

	if (root_dir_index_0[last_index / NUM_INDEX_ENTRIES] == 0) {
		unsigned int near_to = 0;
		if (last_index == 0) {
			near_to = root_dir_cluster_0;
		}
		else {
			near_to = root_dir_index_0[((last_index / NUM_INDEX_ENTRIES) + NUM_INDEX_ENTRIES - 1) % NUM_INDEX_ENTRIES];
		}

		root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = memory_manager->allocate_empty_cluster(near_to);

		if (root_dir_index_0[last_index / NUM_INDEX_ENTRIES] == 0) {
			/* Allocation failed, restore previous state */
			if (last_index_count == 0 && last_index != 0) {
				last_index--;
				last_index_count = NUM_DIRECTORY_ENTRIES;
			}
		}
	}

	IndexEntry temp0[NUM_INDEX_ENTRIES];
	directory_cache->read_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);

	if (temp0[last_index % NUM_INDEX_ENTRIES] == 0) {
		unsigned int near_to = 0;
		if (last_index % NUM_INDEX_ENTRIES != 0) {
			near_to = temp0[((last_index % NUM_INDEX_ENTRIES) + NUM_INDEX_ENTRIES - 1) % NUM_INDEX_ENTRIES];
		}
		else {
			near_to = root_dir_index_0[last_index / NUM_INDEX_ENTRIES];
		}
		temp0[last_index % NUM_INDEX_ENTRIES] = memory_manager->allocate_empty_cluster(near_to);

		if (temp0[last_index % NUM_INDEX_ENTRIES] == 0) {
			/* Allocation failed, restore previous state */
			if (last_index % NUM_INDEX_ENTRIES == 0) {
				deallocate_directory_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES]);
				root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = 0;
			}
			if (last_index_count == 0 && last_index != 0) {
				last_index--;
				last_index_count = NUM_DIRECTORY_ENTRIES;
			}
		}

		directory_cache->write_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);
	}

	FCB temp1[NUM_DIRECTORY_ENTRIES];
	directory_cache->read_cluster(temp0[last_index % NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);

	temp1[last_index_count] = file_info;
	last_index_count++;

	directory_cache->write_cluster(temp0[last_index % NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);
}

void DirectoryManager::deallocate_directory_cluster(ClusterNo cluster_no)
{
	directory_cache->flush_cluster_if_exists(cluster_no);
	memory_manager->deallocate_cluster(cluster_no);
}

/* Used internally by DirectoryManaged to add files to the file system. */
/* If a file exists within the file system or there's not enough memory */
/* function returns false. */
bool DirectoryManager::add_file_internal(const FCB& file_info) {
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_info.name, file_info.ext);
	if (std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) != 0) {
		return false;
	}

	if (last_index_count == NUM_DIRECTORY_ENTRIES) {
		last_index++;
		last_index_count = 0;
	}

	if (root_dir_index_0[last_index / NUM_INDEX_ENTRIES] == 0) {
		unsigned int near_to = 0;
		if (last_index == 0) {
			near_to = root_dir_cluster_0;
		}
		else {
			near_to = root_dir_index_0[((last_index / NUM_INDEX_ENTRIES) + NUM_INDEX_ENTRIES - 1) % NUM_INDEX_ENTRIES];
		}
		
		root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = memory_manager->allocate_empty_cluster(near_to);
		
		if (root_dir_index_0[last_index / NUM_INDEX_ENTRIES] == 0) {
			/* Allocation failed, restore previous state */
			if (last_index_count == 0 && last_index != 0) {
				last_index--;
				last_index_count = NUM_DIRECTORY_ENTRIES;
			}
			return false;
		}
	}

	IndexEntry temp0[NUM_INDEX_ENTRIES];
	directory_cache->read_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);

	if (temp0[last_index % NUM_INDEX_ENTRIES] == 0) {
		unsigned int near_to = 0;
		if (last_index % NUM_INDEX_ENTRIES != 0) {
			near_to = temp0[((last_index % NUM_INDEX_ENTRIES) + NUM_INDEX_ENTRIES - 1) % NUM_INDEX_ENTRIES];
		}
		else {
			near_to = root_dir_index_0[last_index / NUM_INDEX_ENTRIES];
		}
		temp0[last_index % NUM_INDEX_ENTRIES] = memory_manager->allocate_empty_cluster(near_to);

		if (temp0[last_index % NUM_INDEX_ENTRIES] == 0) {
			/* Allocation failed, restore previous state */
			if (last_index % NUM_INDEX_ENTRIES == 0) {
				deallocate_directory_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES]);
				root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = 0;
			}
			if (last_index_count == 0 && last_index != 0) {
				last_index--;
				last_index_count = NUM_DIRECTORY_ENTRIES;
			}
			return false;
		}

		directory_cache->write_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);
	}

	FCB temp1[NUM_DIRECTORY_ENTRIES];
	directory_cache->read_cluster(temp0[last_index % NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);

	temp1[last_index_count] = file_info;
	last_index_count++;

	directory_cache->write_cluster(temp0[last_index % NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);
	return true;
}

/* Used to update existing or add a new FCB to the file system. */
/* Returns false if there's not enough memory to create a new file entry. */
bool DirectoryManager::update_or_add_entry(const FCB& file_info)
{
	directory_mutex.wait();

	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_info.name, file_info.ext);
	if (std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) != 0) {
		IndexEntry temp0[NUM_INDEX_ENTRIES];
		FCB temp1[NUM_DIRECTORY_ENTRIES];

		directory_cache->read_cluster(root_dir_index_0[std::get<0>(file_data)], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);
		directory_cache->read_cluster(temp0[std::get<1>(file_data)], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);
		
		temp1[std::get<2>(file_data)] = file_info;

		directory_cache->write_cluster(temp0[std::get<1>(file_data)], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);
	}
	else {
		// what if alloc fails
		add_non_existing_file_internal(file_info);
	}

	directory_mutex.signal();
	
	return true;
}

bool DirectoryManager::delete_existing_file_entry(const FCB& file_info, IndexEntry entry0, IndexEntry entry1, unsigned int entry2)
{
	directory_mutex.wait();
	IndexEntry temp0[NUM_INDEX_ENTRIES];
	FCB temp1[NUM_DIRECTORY_ENTRIES];

	directory_cache->read_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);
	directory_cache->read_cluster(temp0[last_index % NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);
	last_index_count--;

	if (entry0 * NUM_INDEX_ENTRIES + entry1 == last_index) {
		if (last_index_count == 0) {
			deallocate_directory_cluster(temp0[last_index % NUM_INDEX_ENTRIES]);
			if (last_index % NUM_INDEX_ENTRIES != 0) {
				temp0[last_index % NUM_INDEX_ENTRIES] = 0;
				directory_cache->write_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);
			}
			else {
				deallocate_directory_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES]);
				root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = 0;
			}

			if (last_index != 0) {
				last_index--;
				last_index_count = NUM_DIRECTORY_ENTRIES;
			}
		}
		else {
			temp1[entry2] = temp1[last_index_count];
			temp1[last_index_count] = ZERO_FCB;
			directory_cache->write_cluster(temp0[last_index % NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);
		}
	}
	else {
		FCB moving = temp1[last_index_count];

		if (last_index_count == 0) {
			deallocate_directory_cluster(temp0[last_index % NUM_INDEX_ENTRIES]);
			if (last_index % NUM_INDEX_ENTRIES != 0) {
				temp0[last_index % NUM_INDEX_ENTRIES] = 0;
				directory_cache->write_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);
			}
			else {
				deallocate_directory_cluster(root_dir_index_0[last_index / NUM_INDEX_ENTRIES]);
				root_dir_index_0[last_index / NUM_INDEX_ENTRIES] = 0;
			}
			if (last_index != 0) {
				last_index--;
				last_index_count = NUM_DIRECTORY_ENTRIES;
			}
		}

		directory_cache->read_cluster(root_dir_index_0[entry0], 0, ClusterSize * sizeof(unsigned char), (char*)temp0);
		directory_cache->read_cluster(temp0[entry1], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);

		temp1[entry2] = moving;

		directory_cache->write_cluster(temp0[entry1], 0, ClusterSize * sizeof(unsigned char), (char*)temp1);
	}

	directory_mutex.signal();
	return true;
}

FileCnt DirectoryManager::get_file_count()
{
	directory_mutex.wait();
	FileCnt res = last_index * NUM_DIRECTORY_ENTRIES + last_index_count;
	directory_mutex.signal();
	return res;
}

void DirectoryManager::format()
{
	directory_mutex.wait();
	memset(root_dir_index_0, 0, ClusterSize * sizeof(unsigned char));
	last_index = last_index_count = 0;
	directory_mutex.signal();
}

bool DirectoryManager::does_file_exist(const char* file_name, const char* file_ext)
{
	directory_mutex.wait();
	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> file_data = find_file(file_name, file_ext);
	bool res = std::memcmp(&std::get<3>(file_data), &ZERO_FCB, sizeof(FCB)) != 0;
	directory_mutex.signal();
	return res;
}

DirectoryManager::~DirectoryManager()
{
	delete directory_cache;
	directory_cache = nullptr;
	partition->writeCluster(root_dir_cluster_0, (char*)root_dir_index_0);
}