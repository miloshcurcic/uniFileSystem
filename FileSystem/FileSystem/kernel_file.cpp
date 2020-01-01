#include "kernel_fs.h"
#include "kernel_file.h"
#include "file_handle.h"

KernelFile::KernelFile(KernelFS* file_system, std::string path, FileHandle* file_handle, char mode)
{
	this->path = path;
	this->file_system = file_system;
	this->file_handle = file_handle;
	this->mode = mode;
}

void KernelFile::close()
{
	// handle options
	file_system->close_file(this);
}

BytesCnt KernelFile::filePos()
{
	return pos;
}

BytesCnt KernelFile::getFileSize()
{
	return 0;
}
