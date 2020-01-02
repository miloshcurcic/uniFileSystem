#pragma once

#include "fcb.h"

class FileHandle {
	FCB fcb;
	unsigned int read_count = 0;
public:
	FileHandle(const FCB& fcb);
	ClusterNo get_data0_cluster();
	ClusterNo get_data1_cluster();
	ClusterNo get_index1_cluster();
	ClusterNo get_index2_cluster();
	FCB get_fcb();

	void set_data0_cluster(ClusterNo cluster_no);
	void set_data1_cluster(ClusterNo cluster_no);
	void set_index1_cluster(ClusterNo cluster_no);
	void set_index2_cluster(ClusterNo cluster_no);
	BytesCnt get_size();
	void set_size(BytesCnt new_size);
	BytesCnt get_max_size();
};