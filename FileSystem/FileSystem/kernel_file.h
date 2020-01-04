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

	ClusterNo last_cluster = 0;

	IndexEntry last_table0[NUM_INDEX_ENTRIES];
	IndexEntry last_table1[NUM_INDEX_ENTRIES];
	ClusterNo last_table0_cluster = 0;
	ClusterNo last_table1_cluster = 0;
	bool table0_written = false;
	bool table1_written = false;

	ClusterNo read_local_index0(ClusterNo cluster_no, unsigned int entry);
	void write_local_index0(ClusterNo cluster_no, unsigned int entry, ClusterNo value);
	ClusterNo read_local_index1(ClusterNo cluster_no, unsigned int entry);
	void write_local_index1(ClusterNo cluster_no, unsigned int entry, ClusterNo value);
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