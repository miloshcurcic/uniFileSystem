#pragma once
#include "definitions.h"

class FCB;
class MemoryManager;
class Partition;
class FileHandle;
class ClusterCache;

class DirectoryManager {
	MemoryManager* memory_manager;
	ClusterCache* directory_cache;

	ClusterNo root_dir_cluster_0;
	IndexEntry root_dir_index_0[NUM_INDEX_ENTRIES];

	unsigned int last_index;
	unsigned int last_index_count;

	WinMutex directory_mutex;

	Partition* partition;

	std::tuple<IndexEntry, IndexEntry, unsigned int, FCB> find_file(const char* file_name, const char* file_ext);
	bool add_file_internal(const FCB& file_info);
	void add_non_existing_file_internal(const FCB& file_info);

	void deallocate_directory_cluster(ClusterNo cluster_no);
public:
	DirectoryManager(Partition* partition, MemoryManager* memory_manager);

	std::tuple <IndexEntry, IndexEntry, unsigned int, FileHandle*> create_file_handle_and_get_pos(const char* file_name, const char* file_ext);
	FileHandle* create_file_handle(const char* file_name, const char* file_ext);

	bool update_or_add_entry(const FCB& file_info);
	bool delete_existing_file_entry(const FCB& file_info, IndexEntry entry0, IndexEntry entry1, unsigned int entry2);
	FileCnt get_file_count();
	void format();
	bool does_file_exist(const char* file_name, const char* file_ext);
	~DirectoryManager();
};