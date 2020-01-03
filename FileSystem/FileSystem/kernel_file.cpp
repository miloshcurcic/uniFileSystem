#include "kernel_fs.h"
#include "kernel_file.h"
#include "file_handle.h"

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
	// handle options
	if (this->file_handle != nullptr) {
		file_system->close_file(this);
	}
}

char KernelFile::write(BytesCnt count, char* buffer)
{
	BytesCnt max_size = file_handle->get_max_size();

	std::list<ClusterNo> clusters;
	if (pos + count > max_size) {
		unsigned int num_clusters = (pos + count - max_size) / ClusterSize + (((pos + count - max_size) % ClusterSize) > 0 ? 1 : 0);
		clusters = file_system->allocate_n_nearby_clusters(0, num_clusters);
	}

	if (pos + count > file_handle->get_size()) {
		file_handle->set_size(pos + count);
	}

	unsigned int left = count;
	unsigned int cur_cluster_index = 0;
	unsigned int buffer_pos = 0;
	
	while (left > 0) {
		cur_cluster_index = pos / ClusterSize;

		if (cur_cluster_index == last_accessed_cluster_index) {
			unsigned int pos_within_cluster = pos % ClusterSize;
			BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

			write_local_cluster(last_accessed_cluster_no, pos_within_cluster, count, buffer + buffer_pos);

			buffer_pos += count;
			pos += count;
			left -= count;
		}
		else {
			if (cur_cluster_index == 0) {
				ClusterNo cluster_no = file_handle->get_data0_cluster();

				if (cluster_no == 0) {
					file_handle->set_data0_cluster(clusters.front());
					clusters.pop_front();
					cluster_no = file_handle->get_data0_cluster();
				}

				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);

				buffer_pos += count;
				pos += count;
				left -= count;
			}
			else if (cur_cluster_index == 1) {
				ClusterNo cluster_no = file_handle->get_data1_cluster();

				if (cluster_no == 0) {
					file_handle->set_data1_cluster(clusters.front());
					clusters.pop_front();
					cluster_no = file_handle->get_data1_cluster();
				}

				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);

				buffer_pos += count;
				pos += count;
				left -= count;
			}
			else if (cur_cluster_index > 1 && cur_cluster_index < NUM_INDEX_ENTRIES + 2) {
				unsigned int index_within_table = cur_cluster_index - 2;

				ClusterNo table_cluster = file_handle->get_index1_cluster();

				if (table_cluster == 0) {
					table_cluster = file_system->allocate_nearby_empty_cluster(file_handle->get_data1_cluster());
					file_handle->set_index1_cluster(table_cluster);
				}

				IndexEntry cluster_table[NUM_INDEX_ENTRIES];
				file_system->read_cluster(table_cluster, 0, ClusterSize, (char*)cluster_table);

				ClusterNo cluster_no = cluster_table[index_within_table];

				if (cluster_no == 0) {
					cluster_table[index_within_table] = clusters.front();
					clusters.pop_front();
					cluster_no = cluster_table[index_within_table];
					file_system->write_cluster(table_cluster, 0, ClusterSize, (char*)cluster_table);
				}

				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);

				buffer_pos += count;
				pos += count;
				left -= count;
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

				IndexEntry cluster_table[NUM_INDEX_ENTRIES];
				file_system->read_cluster(table0_cluster, 0, ClusterSize, (char*)cluster_table);

				ClusterNo table1_cluster = cluster_table[index_within_table0];

				if (table1_cluster == 0) {
					if (index_within_table0 != 0) {
						table1_cluster = file_system->allocate_nearby_empty_cluster(index_within_table0 - 1);
					}
					else {
						table1_cluster = file_system->allocate_nearby_empty_cluster(file_handle->get_index2_cluster());
					}
					cluster_table[index_within_table0] = table1_cluster;
					file_system->write_cluster(table0_cluster, 0, ClusterSize, (char*)cluster_table);
				}

				file_system->read_cluster(table1_cluster, 0, ClusterSize, (char*)cluster_table);

				ClusterNo cluster_no = cluster_table[index_within_table1];

				if (cluster_no == 0) {
					cluster_table[index_within_table1] = clusters.front();
					clusters.pop_front();
					cluster_no = cluster_table[index_within_table1];
					file_system->write_cluster(table1_cluster, 0, ClusterSize, (char*)cluster_table);
				}

				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);
				write_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);
				buffer_pos += count;
				pos += count;
				left -= count;
			}
		}
		last_accessed_cluster_index = cur_cluster_index;
	}
	return 0;
}

BytesCnt KernelFile::read(BytesCnt count, char* buffer)
{
	BytesCnt size = file_handle->get_size();
	BytesCnt res = (size - pos < count ? size - pos : count);
	BytesCnt left = res;
	unsigned int cur_cluster_index = 0;
	unsigned int buffer_pos = 0;

	while (left > 0) {
		cur_cluster_index = pos / ClusterSize;

		if (cur_cluster_index == last_accessed_cluster_index) {
			unsigned int pos_within_cluster = pos % ClusterSize;
			BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

			read_local_cluster(last_accessed_cluster_no, pos_within_cluster, count, buffer + buffer_pos);

			buffer_pos += count;
			pos += count;
			left -= count;
		}
		else {
			if (cur_cluster_index == 0) {
				ClusterNo cluster_no = file_handle->get_data0_cluster();

				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);

				buffer_pos += count;
				pos += count;
				left -= count;
			}
			else if (cur_cluster_index == 1) {
				ClusterNo cluster_no = file_handle->get_data1_cluster();

				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);

				buffer_pos += count;
				pos += count;
				left -= count;
			}
			else if (cur_cluster_index > 1 && cur_cluster_index < NUM_INDEX_ENTRIES + 2) {
				unsigned int index_within_table = cur_cluster_index - 2;

				ClusterNo table_cluster = file_handle->get_index1_cluster();
				IndexEntry cluster_table[NUM_INDEX_ENTRIES];
				file_system->read_cluster(table_cluster, 0, ClusterSize, (char*)cluster_table);

				ClusterNo cluster_no = cluster_table[index_within_table];
				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);

				buffer_pos += count;
				pos += count;
				left -= count;
			}
			else {
				unsigned int pos_in_table = cur_cluster_index - 2 - NUM_INDEX_ENTRIES;
				unsigned int index_within_table0 = pos_in_table / NUM_INDEX_ENTRIES;
				unsigned int index_within_table1 = pos_in_table % NUM_INDEX_ENTRIES;

				ClusterNo table0_cluster = file_handle->get_index2_cluster();
				IndexEntry cluster_table[NUM_INDEX_ENTRIES];
				file_system->read_cluster(table0_cluster, 0, ClusterSize, (char*)cluster_table);

				ClusterNo table1_cluster = cluster_table[index_within_table0];
				file_system->read_cluster(table1_cluster, 0, ClusterSize, (char*)cluster_table);

				ClusterNo cluster_no = cluster_table[index_within_table1];

				unsigned int pos_within_cluster = pos % ClusterSize;
				BytesCnt count = (ClusterSize - pos_within_cluster > left ? left : ClusterSize - pos_within_cluster);

				read_local_cluster(cluster_no, pos_within_cluster, count, buffer + buffer_pos);

				buffer_pos += count;
				pos += count;
				left -= count;
			}
		}
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
	return 0;
}
