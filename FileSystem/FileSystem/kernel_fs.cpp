#include "kernel_fs.h"
#include "kernel_file.h"
#include "file_handle.h"
#include "memory_manager.h"
#include "directory_manager.h"
#include "part.h"

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

	memory_manager = nullptr;
	directory_manager = nullptr;
    mounted_partition = nullptr;
    
	// do extra clean-up
	return 0;
}

char KernelFS::format()
{
	formatting = true;
	if (open_files.size() != 0) {
		// wait
	}

	// initialize bit vector
	// initialize root dir

	return 0;
}

KernelFile* KernelFS::open_file(char* fname, char mode) // not synchronized
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

	FileHandle* handle = nullptr;

	auto located = open_files.find(fname);
	if (located != open_files.end()) {
		handle = located->second;
		
	}
	else {
		// find file and add it to map if it exists

	}

	if (handle == nullptr) {
		if (mode == 'w') {
			// Create a new file
		}
		else {
			// File doesn't exist error
			return nullptr;
		}
	}

	if (mode == 'r') {
		if (handle->write_access) {
			// wait
		}

		handle->num_readers++;
	}
	else {
		if (handle->write_access || handle->num_readers != 0) {
			// wait
		}

		if (handle != nullptr && mode == 'w') {
			// reuse handle for new file
		}

		handle->write_access = true;
	}

	return new KernelFile(fname, handle, mode);
}

void KernelFS::close_file(KernelFile* file) // not synchronized
{
	if (file->mode == 'r') {
		file->file_handle->num_readers--;
	}
	else {
		file->file_handle->write_access = false;
	}

	if (!file->file_handle->write_access && (file->file_handle->num_readers == 0)) {
		open_files.erase(file->file_path);
		delete file->file_handle; // add destructor for this, deallocate any used memory
		file->file_handle = nullptr;
	}
}
