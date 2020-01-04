#pragma once

#include "definitions.h"
#include <regex>

class Partition;
class KernelFile;
class FileHandle;
class MemoryManager;
class DirectoryManager;
class ClusterCache;
class FCB;

class KernelFS {
	bool formatting = false;
	bool unmounting = false;
	Partition* mounted_partition = nullptr;
	MemoryManager* memory_manager = nullptr;
	DirectoryManager* directory_manager = nullptr;
	ClusterCache* cluster_cache = nullptr;
	ClusterCache* index0_cache = nullptr;
	ClusterCache* index1_cache = nullptr;
	std::unordered_map<std::string, FileHandle*> open_files;
	unsigned int root_dir_index0 = 0;
	static const std::regex file_regex;

	WinMutex mounted_mutex;
	WinMutex fs_mutex;
	WinSemaphore open_file_sem;

	void free_handle(FileHandle* handle);
public:
	KernelFS();
    char mount(Partition* partition);
    char unmount(); 
    char format();
    FileCnt readRootDir(); 
    char doesExist(const char* fname);
    KernelFile* open_file(const char* fname, char mode);
	void close_file(KernelFile* file);
    char deleteFile(const char* fname);

	void write_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer);
	void read_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer);
	void write_index0(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer);
	void read_index0(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer);
	void write_index1(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer);
	void read_index1(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer);

	std::list<ClusterNo> allocate_n_nearby_clusters(ClusterNo near_to, unsigned int count);
	ClusterNo allocate_nearby_cluster(ClusterNo near_to);
	ClusterNo allocate_nearby_empty_cluster(ClusterNo near_to);
	void deallocate_n_clusters(std::list<ClusterNo>& clusters);
	void deallocate_cluster(ClusterNo cluster_no);
	void deallocate_data_cluster(ClusterNo cluster_no);
	void deallocate_index0_cluster(ClusterNo cluster_no);
	void deallocate_index1_cluster(ClusterNo cluster_no);
	~KernelFS();
};