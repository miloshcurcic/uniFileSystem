#include "kernel_fs.h"
#include "kernel_file.h"
#include "file_handle.h"
#include "memory_manager.h"
#include "directory_manager.h"
#include "cluster_cache.h"
#include "part.h"

const std::regex KernelFS::file_regex("(?:\\\\|/)([[:w:]_]{1,8})\\.([[:w:]_]{1,3})");

KernelFS::KernelFS() : open_file_sem(0)
{
}

char KernelFS::mount(Partition* partition)
{
	fs_mutex.wait();
    if (partition == nullptr) {
		// Error
		fs_mutex.signal();
		return 0;
    }

	do {
		fs_mutex.signal();
		mounted_mutex.wait();
		fs_mutex.wait();
	} while (mounted_partition != nullptr);

	memory_manager = new MemoryManager(partition);
	directory_manager = new DirectoryManager(partition, memory_manager);
	cluster_cache = new ClusterCache(partition);
	index0_cache = new ClusterCache(partition, INDEX0_CACHE_SIZE);
	index1_cache = new ClusterCache(partition, INDEX1_CACHE_SIZE);
	mounted_partition = partition;

	fs_mutex.signal();
	return 1;
}

char KernelFS::unmount()
{
	fs_mutex.wait();
	if (mounted_partition == nullptr) {
		fs_mutex.signal();
		return -1; // error
	}

	if (unmounting) {
		fs_mutex.signal();
		return -2; // error
	}

	unmounting = true;

	do {
		fs_mutex.signal();
		open_file_sem.wait();
		fs_mutex.wait();
	} while (open_files.size() != 0);

	delete memory_manager;
	delete directory_manager;
	delete cluster_cache;
	delete index0_cache;
	delete index1_cache;

	memory_manager = nullptr;
	directory_manager = nullptr;
    mounted_partition = nullptr;
	cluster_cache = nullptr;
	index0_cache = nullptr;
	index1_cache = nullptr;

	unmounting = false;

	mounted_mutex.signal();
	fs_mutex.signal();
	return 0;
}

char KernelFS::format()
{
	fs_mutex.wait();

	if (formatting) {
		fs_mutex.signal();
		return -1;
	}

	formatting = true;

	do {
		fs_mutex.signal();
		open_file_sem.wait();
		fs_mutex.wait();
	} while(open_files.size() != 0);

	memory_manager->format();
	directory_manager->format();
	formatting = false;

	fs_mutex.signal();
	return 0;
}

FileCnt KernelFS::readRootDir()
{
	fs_mutex.wait();
	FileCnt res;
	if (formatting || unmounting) {
		res = -1;
	}
	else {
		res = directory_manager->get_file_count();
	}
	fs_mutex.signal();
	return res;
}

char KernelFS::doesExist(const char* fname)
{
	fs_mutex.wait();

	std::smatch match;
	std::string sname(fname);

	if (!std::regex_search(sname, match, file_regex)) {
		char res = directory_manager->does_file_exist(match.str(1).c_str(), match.str(2).c_str()) ? 1 : 0;
		fs_mutex.signal();
		return res;
	}
	else {
		fs_mutex.signal();
		return 0;
	}
}

KernelFile* KernelFS::open_file(const char* fname, char mode) // not synchronized
{
	fs_mutex.wait();

	if (fname == nullptr) {
		fs_mutex.signal();
		return nullptr;
	}

	if (mode != 'r' && mode != 'w' && mode != 'a') {
		fs_mutex.signal();
		return nullptr;
	}

	if (formatting || unmounting) {
		fs_mutex.signal();
		return nullptr;
	}

	std::smatch match;
	std::string sname(fname);

	if (!std::regex_search(sname, match, file_regex)) {
		fs_mutex.signal();
		return nullptr;
	}

	FileHandle* handle = nullptr;

	auto located = open_files.find(sname);
	if (located != open_files.end()) {
		handle = located->second;
		if (mode == 'r') {
			handle->wait_acquire_read_access(fs_mutex);
		}
	}
	else {
		handle = directory_manager->create_file_handle(match.str(1).c_str(), match.str(2).c_str());
		if (mode != 'w' && handle == nullptr) {
			fs_mutex.signal();
			return nullptr; // error
		}
		else {
			open_files[sname] = handle;
			if (mode == 'r') {
				handle->acquire_read_access();
			}
		}
	}

	if (mode == 'w' && handle == nullptr) {
		FCB new_file(match.str(1).c_str(), match.str(2).c_str());
		handle = new FileHandle(new_file);
		open_files[sname] = handle;
		handle->acquire_write_access();
	}
	else if (mode != 'r') {
		handle->wait_acquire_write_access(fs_mutex);
		if (mode == 'w') {
			free_handle(handle);
		}
	}
	
	KernelFile* res = new KernelFile(this, sname, handle, mode);
	
	fs_mutex.signal();

	return res;
}

void KernelFS::close_file(KernelFile* file)
{
	fs_mutex.wait();
	if (file->mode == 'r') {
		file->file_handle->release_read_access();
	}
	else {
		file->file_handle->release_write_access();
	}

	if (file->file_handle->get_read_count() == 0 && file->file_handle->get_waiting_count() == 0) {
		directory_manager->update_or_add_entry(file->file_handle->get_fcb());
		open_files.erase(file->path);
		delete file->file_handle;
		file->file_handle = nullptr;
	}

	if ((unmounting || formatting) && (open_files.size() == 0)) {
		open_file_sem.signal();
	}
	fs_mutex.signal();
}

char KernelFS::deleteFile(const char* fname)
{
	fs_mutex.wait();
	std::smatch match;
	std::string sname(fname);

	if (!std::regex_search(sname, match, file_regex)) {
		fs_mutex.signal();
		return 0;
	}

	auto located = open_files.find(sname);
	if (located != open_files.end()) {
		fs_mutex.signal();
		return 0;
	}

	auto handle_pos_tuple = directory_manager->create_file_handle_and_get_pos(match.str(1).c_str(), match.str(2).c_str());

	/* File not found */
	if (std::get<3>(handle_pos_tuple) == nullptr) {
		fs_mutex.signal();
		return 0;
	}

	/* Splitting work that different modules do, directory entry will only be accessed through directory manager and */
	/* file data will only be accessed through KernelFS. */
	free_handle(std::get<3>(handle_pos_tuple));
	directory_manager->delete_existing_file_entry(std::get<3>(handle_pos_tuple)->get_fcb(), std::get<0>(handle_pos_tuple), std::get<1>(handle_pos_tuple), std::get<2>(handle_pos_tuple));
	delete std::get<3>(handle_pos_tuple);

	fs_mutex.signal();
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

void KernelFS::write_index0(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer)
{
	index0_cache->write_cluster(cluster_no, start_pos, bytes, buffer);
}

void KernelFS::read_index0(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer)
{
	index0_cache->read_cluster(cluster_no, start_pos, bytes, buffer);
}

void KernelFS::write_index1(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer)
{
	index1_cache->write_cluster(cluster_no, start_pos, bytes, buffer);
}

void KernelFS::read_index1(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer)
{
	index1_cache->read_cluster(cluster_no, start_pos, bytes, buffer);
}

std::list<ClusterNo> KernelFS::allocate_n_nearby_clusters(ClusterNo near_to, unsigned int count)
{
	return memory_manager->allocate_n_clusters(near_to, count);
}

void KernelFS::free_handle(FileHandle* handle)
{
	/* Deallocate all clusters used by the file, possible optimization */
	/* would be discarding those to a cluster pool that would be used */
	/* for quick allocation. */
	FCB file = handle->get_fcb();
	if (file.data0 != 0) {
		deallocate_data_cluster(file.data0);
		file.data0 = 0;
	}
	if (file.data1 != 0) {
		deallocate_data_cluster(file.data1);
		file.data1 = 0;
	}
	if (file.index1_0 != 0) {
		IndexEntry temp[NUM_INDEX_ENTRIES];
		index0_cache->read_cluster(file.index1_0, 0, ClusterSize * sizeof(unsigned int), (char*)temp);
		for (IndexEntry i = 0; i < NUM_INDEX_ENTRIES; i++) {
			if (temp[i] != 0) {
				deallocate_data_cluster(temp[i]);
			}
			else {
				break;
			}
		}
		deallocate_index0_cluster(file.index1_0);
		file.index1_0 = 0;
	}
	if (file.index2_0 != 0) {
		IndexEntry temp_l0[NUM_INDEX_ENTRIES];
		index0_cache->read_cluster(file.index2_0, 0, ClusterSize * sizeof(unsigned char), (char*)temp_l0);
		for (IndexEntry i = 0; i < NUM_INDEX_ENTRIES; i++) {
			if (temp_l0[i] != 0) {
				IndexEntry temp_l1[NUM_INDEX_ENTRIES];
				index1_cache->read_cluster(temp_l0[i], 0, ClusterSize * sizeof(unsigned char), (char*)temp_l1);
				for (IndexEntry j = 0; j < NUM_INDEX_ENTRIES; j++) {
					if (temp_l1[j] != 0) {
						deallocate_data_cluster(temp_l1[j]);
					}
					else {
						break;
					}
				}
				deallocate_index1_cluster(temp_l0[i]);
			}
			else {
				break;
			}
		}
		deallocate_index0_cluster(file.index2_0);
		file.index2_0 = 0;
	}
	handle->set_data0_cluster(0);
	handle->set_data1_cluster(0);
	handle->set_index1_cluster(0);
	handle->set_index2_cluster(0);
	handle->set_size(0);
}

void KernelFS::deallocate_n_clusters(std::list<ClusterNo>& clusters) {
	memory_manager->deallocate_n_clusters(clusters);
}

void KernelFS::deallocate_cluster(ClusterNo cluster_no) {
	memory_manager->deallocate_cluster(cluster_no);
}

void KernelFS::deallocate_data_cluster(ClusterNo cluster_no) {
	cluster_cache->flush_cluster_if_exists(cluster_no);
	memory_manager->deallocate_cluster(cluster_no);
}

void KernelFS::deallocate_index0_cluster(ClusterNo cluster_no) {
	index0_cache->flush_cluster_if_exists(cluster_no);
	memory_manager->deallocate_cluster(cluster_no);
}

void KernelFS::deallocate_index1_cluster(ClusterNo cluster_no) {
	index1_cache->flush_cluster_if_exists(cluster_no);
	memory_manager->deallocate_cluster(cluster_no);
}

KernelFS::~KernelFS()
{
	unmount();
}

ClusterNo KernelFS::allocate_nearby_cluster(ClusterNo near_to)
{
	return memory_manager->allocate_cluster(near_to);
}

ClusterNo KernelFS::allocate_nearby_empty_cluster(ClusterNo near_to)
{
	return memory_manager->allocate_empty_cluster(near_to);
}
