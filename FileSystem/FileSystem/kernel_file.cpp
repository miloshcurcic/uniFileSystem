#include "kernel_fs.h"
#include "kernel_file.h"



KernelFile::KernelFile(std::string file_path, FileHandle* file_handle, char mode)
{
	this->file_path = file_path;
	this->file_handle = file_handle;
	this->mode = mode;
}

void KernelFile::close()
{
	file_system->close_file(this);
}
