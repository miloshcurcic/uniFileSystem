#pragma once

#include "definitions.h"

class FileHandle;
class KernelFS;

class KernelFile {
	FileHandle* file_handle;
	std::string path;
	BytesCnt pos;
	char mode;

	int last_accessed_cluster_index = -1;
	unsigned int last_accessed_cluster_no = 0;
	unsigned char last_accessed_cluster[ClusterSize];
	bool last_written = false;

	void read_local_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer);
	void write_local_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer);

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