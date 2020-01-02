#pragma once

#include "definitions.h"

class FileHandle;
class KernelFS;

class KernelFile {
	FileHandle* file_handle;
	std::string path;
	BytesCnt pos;
	char mode;
	KernelFS* file_system;
public:
	KernelFile(KernelFS* file_system, std::string path, FileHandle* file_handle, char mode);
	void close();
	char write(BytesCnt count, char* buffer);
	BytesCnt read(BytesCnt count, char* buffer);
	char seek(BytesCnt pos);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate(); //todo

	friend class KernelFS;
};