#include "file_handle.h"

FileHandle::FileHandle(const FCB& fcb)
{
	this->fcb = fcb;
}

ClusterNo FileHandle::get_data0_cluster()
{
	return fcb.data0;
}

ClusterNo FileHandle::get_data1_cluster()
{
	return fcb.data1;
}

ClusterNo FileHandle::get_index1_cluster()
{
	return fcb.index1_0;
}

ClusterNo FileHandle::get_index2_cluster()
{
	return fcb.index2_0;
}

FCB FileHandle::get_fcb()
{
	return fcb;
}

void FileHandle::set_data0_cluster(ClusterNo cluster_no)
{
	fcb.data0 = cluster_no;
}

void FileHandle::set_data1_cluster(ClusterNo cluster_no)
{
	fcb.data1 = cluster_no;
}

void FileHandle::set_index1_cluster(ClusterNo cluster_no)
{
	fcb.index1_0 = cluster_no;
}

void FileHandle::set_index2_cluster(ClusterNo cluster_no)
{
	fcb.index2_0 = cluster_no;
}

BytesCnt FileHandle::get_size()
{
	return fcb.size;
}

void FileHandle::set_size(BytesCnt new_size)
{
	fcb.size = new_size;
}

BytesCnt FileHandle::get_max_size()
{
	return fcb.size / ClusterSize * ClusterSize + (fcb.size % ClusterSize != 0 ? ClusterSize : 0);
}
