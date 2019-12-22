#pragma once

#include "definitions.h"

class FileHandle;
class KernelFS;

class KernelFile {
	std::string file_path;
	FileHandle* file_handle;
	char mode;
	KernelFS* file_system;
public:
	KernelFile(std::string file_path, FileHandle* file_handle, char mode);
	void close();
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();

	friend class KernelFS;
};