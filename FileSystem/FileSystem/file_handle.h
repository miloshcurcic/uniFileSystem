#pragma once

#include "fcb.h"

class WinSlimReaderWriterLock;
class WinMutex;

class FileHandle {
	FCB fcb;
	unsigned int read_count = 0;
	unsigned int waiting_count = 0;
	WinSlimReaderWriterLock* handle_srw_lock;
	WinMutex* handle_mutex;
public:
	FileHandle(const FCB& fcb);
	void acquire_read_access(WinMutex* mutex);
	void acquire_write_access(WinMutex* mutex);
	void release_read_access();
	void release_write_access();
	ClusterNo get_data0_cluster();
	ClusterNo get_data1_cluster();
	ClusterNo get_index1_cluster();
	ClusterNo get_index2_cluster();
	FCB get_fcb();
	unsigned int get_read_count();
	unsigned int get_waiting_count();

	void set_data0_cluster(ClusterNo cluster_no);
	void set_data1_cluster(ClusterNo cluster_no);
	void set_index1_cluster(ClusterNo cluster_no);
	void set_index2_cluster(ClusterNo cluster_no);
	BytesCnt get_size();
	void set_size(BytesCnt new_size);
	BytesCnt get_max_size();
};