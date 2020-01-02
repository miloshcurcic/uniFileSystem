#include "file_handle.h"
#include "win_srw_lock.h"
#include "win_mutex.h"

FileHandle::FileHandle(const FCB& fcb)
{
	this->fcb = fcb;
	this->handle_srw_lock = new WinSlimReaderWriterLock();
	this->handle_mutex = new WinMutex();
}

void FileHandle::acquire_read_access(WinMutex* mutex)
{
	handle_mutex->wait();
	waiting_count++;
	mutex->signal();
	handle_srw_lock->acquire_srw_shared();
	mutex->wait();
	read_count++;
	waiting_count--;
	handle_mutex->signal();
	mutex->signal();
}

void FileHandle::acquire_write_access(WinMutex* mutex)
{
	handle_mutex->wait();
	waiting_count++;
	mutex->signal();
	handle_srw_lock->acquire_srw_exclusive();
	mutex->wait();
	waiting_count--;
	handle_mutex->signal();
	mutex->signal();
}

void FileHandle::release_read_access()
{
	handle_mutex->wait();
	handle_srw_lock->release_srw_shared();
	read_count--;
	handle_mutex->signal();
}

void FileHandle::release_write_access()
{
	handle_srw_lock->release_srw_exclusive();
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

unsigned int FileHandle::get_read_count()
{
	return read_count;
}

unsigned int FileHandle::get_waiting_count()
{
	return waiting_count;
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
