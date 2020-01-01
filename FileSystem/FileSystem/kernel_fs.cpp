#include "kernel_fs.h"
#include "kernel_file.h"
#include "file_handle.h"
#include "memory_manager.h"
#include "directory_manager.h"
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
