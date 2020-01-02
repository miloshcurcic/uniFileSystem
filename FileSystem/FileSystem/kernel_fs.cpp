#include "kernel_fs.h"
#include "kernel_file.h"
#include "file_handle.h"
#include "memory_manager.h"
#include "directory_manager.h"
#include "cluster_cache.h"
#include "part.h"

const std::regex KernelFS::file_regex("(?:\\\\|/)([[:w:]_]{1,8})\\.([[:w:]_]{1,3})");

char KernelFS::mount(Partition* partition)
{
    if (partition == nullptr) {
		// Error
        return 0;
    }

	if (mounted_partition != nullptr) {
		// wait till unmounted
	}
	memory_manager = new MemoryManager(partition);
	directory_manager = new DirectoryManager(partition, memory_manager);
	cluster_cache = new ClusterCache(partition);
	mounted_partition = partition;
	return 1;
}

char KernelFS::unmount()
{
	if (mounted_partition == nullptr) {
		return -1; // error
	}

	if (open_files.size() != 0) {
		// wait
	}

	delete memory_manager;
	delete directory_manager;
	delete cluster_cache;

	memory_manager = nullptr;
	directory_manager = nullptr;
    mounted_partition = nullptr;
	cluster_cache = nullptr;

	// do extra clean-up
	return 0;
}

char KernelFS::format()
{
	formatting = true;
	if (open_files.size() != 0) {
		// wait
	}

	memory_manager->format();
	directory_manager->format();
	formatting = false;
	return 0;
}

FileCnt KernelFS::readRootDir()
{
	return directory_manager->get_file_count();
}

char KernelFS::doesExist(const char* fname)
{
	std::smatch match;
	std::string sname(fname);

	if (!std::regex_search(sname, match, file_regex)) {
		return directory_manager->does_file_exist(match.str(1).c_str(), match.str(2).c_str());
	}
	else {
		return 0;
	}
}

KernelFile* KernelFS::open_file(const char* fname, char mode) // not synchronized
{
	if (fname == nullptr) {
		return nullptr;
	}

	if (mode != 'r' && mode != 'w' && mode != 'a') {
		return nullptr;
	}

	if (formatting) {
		// Formatting, error
		return nullptr;
	}

	std::smatch match;
	std::string sname(fname);

	if (!std::regex_search(sname, match, file_regex)) {
		return nullptr;
	}

	FileHandle* handle = nullptr;

	
	if (mode == 'w') {
		directory_manager->delete_file(match.str(1).c_str(), match.str(2).c_str());
		FCB new_file(match.str(1).c_str(), match.str(2).c_str());
		handle = directory_manager->add_and_get_file_handle(new_file);
	}
	else {
		auto located = open_files.find(sname);
		if (located != open_files.end()) {
			handle = located->second;
		}
		else {
			handle = directory_manager->create_file_handle(match.str(1).c_str(), match.str(2).c_str());
			if (handle == nullptr) {
				return nullptr; // error
			}

			open_files[sname] = handle;
		}
	}

	return new KernelFile(this, sname, handle, mode);
}

void KernelFS::close_file(const KernelFile* file)
{
	directory_manager->update_or_add(file->file_handle->get_fcb());
	open_files.erase(file->path);
}

char KernelFS::deleteFile(const char* fname)
{
	std::smatch match;
	std::string sname(fname);

	if (!std::regex_search(sname, match, file_regex)) {
		return 0;
	}

	auto located = open_files.find(sname);
	if (located != open_files.end()) {
		return 0;
	}
	
	if (!directory_manager->delete_file(match.str(1).c_str(), match.str(2).c_str())) {
		return 0;
	}

	return 1;
}

void KernelFS::write_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer)
{
	cluster_cache->write_cluster(cluster_no, start_pos, bytes, buffer);
}

void KernelFS::read_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer)
{
	cluster_cache->read_cluster(cluster_no, start_pos, bytes, buffer);
}

std::list<ClusterNo> KernelFS::allocate_n_nearby_clusters(ClusterNo near_to, unsigned int count)
{
	std::list<ClusterNo> res;
	for (unsigned int i = 0; i < count; i++) {
		// error check
		ClusterNo cluster = memory_manager->allocate_cluster(near_to);
		res.push_back(cluster);
		near_to = cluster;
	}
	return res;
}

ClusterNo KernelFS::allocate_nearby_cluster(ClusterNo near_to)
{
	return memory_manager->allocate_cluster(near_to);
}

ClusterNo KernelFS::allocate_nearby_empty_cluster(ClusterNo near_to)
{
	return memory_manager->allocate_empty_cluster(near_to);
}
