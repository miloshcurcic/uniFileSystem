#include "fs.h"
#include "kernel_fs.h"
#include "file.h"

FS::FS()
{
	myImpl = new KernelFS();
}

FS::~FS()
{
	delete myImpl;
}

char FS::mount(Partition* partition)
{
	return myImpl->mount(partition);
}

char FS::unmount()
{
	return myImpl->unmount();
}

char FS::format()
{
	return myImpl->format();
}

FileCnt FS::readRootDir()
{
	return myImpl->readRootDir();
}

char FS::doesExist(char* fname)
{
	return myImpl->doesExist(fname);
}

File* FS::open(char* fname, char mode)
{
	File* new_file = new File();
	new_file->myImpl = myImpl->open_file(fname, mode);
	if (new_file->myImpl == nullptr) {
		delete new_file;
		return nullptr;
	}
	return new_file;
}

char FS::deleteFile(char* fname)
{
	return myImpl->deleteFile(fname);
}
