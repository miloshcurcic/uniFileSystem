#include "kernel_fs.h"
#include "kernel_file.h"
#include "file_handle.h"


ClusterNo KernelFile::read_local_index0(ClusterNo cluster_no, unsigned int entry)
{
	if (cluster_no != last_table0_cluster) {
		if (table0_written) {
			file_system->write_cluster(last_table0_cluster, 0, ClusterSize * sizeof(unsigned char), (char*)last_table0);
			table0_written = false;
		}
		last_table0_cluster = cluster_no;
		file_system->read_cluster(cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_table0);
	}
	return last_table0[entry];
}

void KernelFile::write_local_index0(ClusterNo cluster_no, unsigned int entry, ClusterNo value)
{
	if (cluster_no != last_table0_cluster) {
		if (table0_written) {
			file_system->write_cluster(last_table0_cluster, 0, ClusterSize * sizeof(unsigned char), (char*)last_table0);
			table0_written = false;
		}
		last_table0_cluster = cluster_no;
		file_system->read_cluster(cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_table0);
	}
	last_table0[entry] = value;
	table0_written = true;
}

ClusterNo KernelFile::read_local_index1(ClusterNo cluster_no, unsigned int entry)
{
	if (cluster_no != last_table1_cluster) {
		if (table1_written) {
			file_system->write_cluster(last_table1_cluster, 0, ClusterSize * sizeof(unsigned char), (char*)last_table1);
			table1_written = false;
		}
		last_table1_cluster = cluster_no;
		file_system->read_cluster(cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_table1);
	}
	return last_table1[entry];
}

void KernelFile::write_local_index1(ClusterNo cluster_no, unsigned int entry, ClusterNo value)
{
	if (cluster_no != last_table1_cluster) {
		if (table1_written) {
			file_system->write_cluster(last_table1_cluster, 0, ClusterSize * sizeof(unsigned char), (char*)last_table1);
			table1_written = false;
		}
		last_table1_cluster = cluster_no;
		file_system->read_cluster(cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_table1);
	}
	last_table1[entry] = value;
	table1_written = true;
}

void KernelFile::read_local_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, char* buffer)
{
	if (cluster_no != last_accessed_cluster_no) {
		if (last_written) {
			file_system->write_cluster(last_accessed_cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_accessed_cluster);
			last_written = false;
		}
		last_accessed_cluster_no = cluster_no;
		file_system->read_cluster(cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_accessed_cluster);
	}
	memcpy(buffer, last_accessed_cluster + start_pos, bytes * sizeof(unsigned char));
}

void KernelFile::write_local_cluster(ClusterNo cluster_no, BytesCnt start_pos, BytesCnt bytes, const char* buffer)
{
	if (cluster_no != last_accessed_cluster_no) {
		if (last_written) {
			file_system->write_cluster(last_accessed_cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_accessed_cluster);
			last_written = false;
		}
		last_accessed_cluster_no = cluster_no;
		file_system->read_cluster(cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_accessed_cluster);
	}
	memcpy(last_accessed_cluster + start_pos, buffer, bytes * sizeof(unsigned char));
	last_written = true;
}

KernelFile::KernelFile(KernelFS* file_system, std::string path, FileHandle* file_handle, char mode)
{
	this->path = path;
	this->file_system = file_system;
	this->file_handle = file_handle;
	this->mode = mode;
	if (this->mode == 'a') {
		this->pos = this->file_handle->get_size();
	}
}

void KernelFile::close()
{
	if (this->file_handle != nullptr) {
		if (last_written) {
			file_system->write_cluster(last_accessed_cluster_no, 0, ClusterSize * sizeof(unsigned char), (char*)last_accessed_cluster);
		}
		if (table0_written) {
			file_system->write_cluster(last_table0_cluster, 0, ClusterSize * sizeof(unsigned char), (char*)last_table0);
		}
		if (table1_written) {
			file_system->write_cluster(last_table1_cluster, 0, ClusterSize * sizeof(unsigned char), (char*)last_table1);
		}
		file_system->close_file(this);
	}
}

char KernelFile::write(BytesCnt count, char* buffer)
{
	if (mode == 'r') {
		return 0;
	}

	if (file_handle->get_size() == MAX_FILE_SIZE) {
		return 0;
	}

	BytesCnt max_size = file_handle->get_max_size();

	std::list<ClusterNo> clusters;
	if (pos + count > max_size) {
		unsigned int num_clusters = (pos + count - max_size) / ClusterSize + (((pos + count - max_size) % ClusterSize) > 0 ? 1 : 0);
		clusters = file_system->allocate_n_nearby_clusters(last_cluster, num_clusters);
		if (clusters.size() != num_clusters) {
			file_system->deallocate_n_clusters(clusters);
			return 0;
		}
	}

	if (pos + count > file_handle->get_size()) {
		file_handle->set_size(pos + count);
	}

	unsigned int left = count;
	unsigned int cur_cluster_index = 0;
	unsigned int buffer_pos = 0;
	
	while (left > 0) {
		cur_cluster_index = pos / ClusterSize;
		unsigned int pos_within_cluster = pos % ClusterSize;
		BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

		if (cur_cluster_index == last_accessed_cluster_index) {
			
			write_local_cluster(last_accessed_cluster_no, pos_within_cluster, count, buffer + buffer_pos);
		}
		else {
			if (cur_cluster_index == 0) {
				ClusterNo cluster_no = file_handle->get_data0_cluster();

				if (cluster_no == 0) {
					cluster_no = clusters.front();
					last_cluster = cluster_no;
					clusters.pop_front();
					file_handle->set_data0_cluster(cluster_no);
				}

				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
			else if (cur_cluster_index == 1) {
				ClusterNo cluster_no = file_handle->get_data1_cluster();

				if (cluster_no == 0) {
					cluster_no = clusters.front();
					last_cluster = cluster_no;
					clusters.pop_front();
					file_handle->set_data1_cluster(cluster_no);
				}

				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
			else if (cur_cluster_index > 1 && cur_cluster_index < NUM_INDEX_ENTRIES + 2) {
				unsigned int index_within_table = cur_cluster_index - 2;

				ClusterNo table_cluster = file_handle->get_index1_cluster();

				if (table_cluster == 0) {
					table_cluster = file_system->allocate_nearby_empty_cluster(file_handle->get_data1_cluster());
					file_handle->set_index1_cluster(table_cluster);
				}

				ClusterNo cluster_no = read_local_index0(table_cluster, index_within_table);

				if (cluster_no == 0) {
					cluster_no = clusters.front();
					last_cluster = cluster_no;
					write_local_index0(table_cluster, index_within_table, cluster_no);
					clusters.pop_front();
				}

				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
			else {
				unsigned int pos_in_table = cur_cluster_index - 2 - NUM_INDEX_ENTRIES;
				unsigned int index_within_table0 = pos_in_table / NUM_INDEX_ENTRIES;
				unsigned int index_within_table1 = pos_in_table % NUM_INDEX_ENTRIES;

				ClusterNo table0_cluster = file_handle->get_index2_cluster();

				if (table0_cluster == 0) {
					table0_cluster = file_system->allocate_nearby_empty_cluster(file_handle->get_index1_cluster());
					file_handle->set_index2_cluster(table0_cluster);
				}
				
				ClusterNo table1_cluster = read_local_index0(table0_cluster, index_within_table0);

				if (table1_cluster == 0) {
					if (index_within_table0 != 0) {
						table1_cluster = file_system->allocate_nearby_empty_cluster(read_local_index0(table0_cluster, index_within_table0 - 1));
					}
					else {
						table1_cluster = file_system->allocate_nearby_empty_cluster(file_handle->get_index2_cluster());
					}
					write_local_index0(table0_cluster, index_within_table0, table1_cluster);
				}

				ClusterNo cluster_no = read_local_index1(table1_cluster, index_within_table1);

				if (cluster_no == 0) {
					cluster_no = clusters.front();
					last_cluster = cluster_no;
					write_local_index1(table1_cluster, index_within_table1, cluster_no);
					clusters.pop_front();
				}

				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
		}
		buffer_pos += count;
		pos += count;
		left -= count;

		last_accessed_cluster_index = cur_cluster_index;
	}
	return 0;
}

BytesCnt KernelFile::read(BytesCnt count, char* buffer)
{
	if (eof()) {
		return 0;
	}

	BytesCnt size = file_handle->get_size();
	BytesCnt res = (size - pos < count ? size - pos : count);
	BytesCnt left = res;
	unsigned int cur_cluster_index = 0;
	unsigned int buffer_pos = 0;

	while (left > 0) {
		cur_cluster_index = pos / ClusterSize;
		unsigned int pos_within_cluster = pos % ClusterSize;
		BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

		if (cur_cluster_index == last_accessed_cluster_index) {
			read_local_cluster(last_accessed_cluster_no, pos_within_cluster, count, buffer + buffer_pos);
		}
		else {
			if (cur_cluster_index == 0) {
				ClusterNo cluster_no = file_handle->get_data0_cluster();

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
			else if (cur_cluster_index == 1) {
				ClusterNo cluster_no = file_handle->get_data1_cluster();

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
			else if (cur_cluster_index > 1 && cur_cluster_index < NUM_INDEX_ENTRIES + 2) {
				unsigned int index_within_table = cur_cluster_index - 2;

				ClusterNo table_cluster = file_handle->get_index1_cluster();
				ClusterNo cluster_no = read_local_index0(table_cluster, index_within_table);

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
			else {
				unsigned int pos_in_table = cur_cluster_index - 2 - NUM_INDEX_ENTRIES;
				unsigned int index_within_table0 = pos_in_table / NUM_INDEX_ENTRIES;
				unsigned int index_within_table1 = pos_in_table % NUM_INDEX_ENTRIES;

				ClusterNo table0_cluster = file_handle->get_index2_cluster();
				ClusterNo table1_cluster = read_local_index0(table0_cluster, index_within_table0);
				ClusterNo cluster_no = read_local_index1(table1_cluster, index_within_table1);

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
			}
		}
		buffer_pos += count;
		pos += count;
		left -= count;

		last_accessed_cluster_index = cur_cluster_index;
	}
	return res;
}

char KernelFile::seek(BytesCnt pos)
{
	if (pos <= file_handle->get_size()) {
		this->pos = pos;
		return 1;
	}
	else {
		return 0;
	}
}

BytesCnt KernelFile::filePos()
{
	return pos;
}

char KernelFile::eof()
{
	// error?
	return (pos == file_handle->get_size() ? 2 : 0);
}

BytesCnt KernelFile::getFileSize()
{
	return file_handle->get_size();
}

char KernelFile::truncate()
{
	if (mode == 'r') {
		return 0;
	}
	return 0;
}
