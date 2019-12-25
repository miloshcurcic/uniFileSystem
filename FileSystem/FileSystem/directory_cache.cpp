#include "directory_cache.h"

DirectoryCache::DirectoryCache(Partition* partition) :
	directory_index_cache(partition, DIRECTORY_INDEX_CACHE_SIZE),
	directory_cache(partition, DIRECTORY_TABLE_CACHE_SIZE)
{
}

FCB DirectoryCache::find_file(const char* file_name)
{
	unsigned int file_hash = calculate_hash(file_name);
	unsigned int index_0 = get_index_0(file_hash);
	unsigned int index_1 = get_index_1(file_hash);

	IndexEntry index_table_1[NUM_INDEX_ENTRIES];
	FCB fcb_table[NUM_DIRECTORY_ENTRIES];

	for(int i = 0; i < NUM_INDEX_ENTRIES; i++) {
		IndexEntry index_1_table_cluster = root_dir_0[index_0 + i];

		if (index_1_table_cluster == 0) {
			return zero_fcb;
		}

		directory_index_cache.read_cluster(index_1_table_cluster, 0, NUM_INDEX_ENTRIES * sizeof(IndexEntry), (char*)index_table_1);
		
		for (int j = 0; j < NUM_INDEX_ENTRIES; j++) {
			IndexEntry dir_table_cluster = index_table_1[index_1 + j];

			if (dir_table_cluster == 0) {
				return zero_fcb;
			}

			directory_cache.read_cluster(dir_table_cluster, 0, NUM_DIRECTORY_ENTRIES * sizeof(FCB), (char*)fcb_table);

			for (int k = 0; k < NUM_DIRECTORY_ENTRIES; k++) {
				if (std::memcmp(&fcb_table[k], &zero_fcb, sizeof(FCB))) {
					return zero_fcb;
				}

				if (std::strncmp(fcb_table[k].name, file_name, FNAMELEN) == 0) {
					return fcb_table[k];
				}
			}
		}
	}
	return zero_fcb;
}

void DirectoryCache::delete_file(const char* file_name) // not done, use temp cache buffer, prev/cur>
{
	unsigned int file_hash = calculate_hash(file_name);
	unsigned int index_0 = get_index_0(file_hash);
	unsigned int index_1 = get_index_1(file_hash);

	IndexEntry index_table_1[NUM_INDEX_ENTRIES];
	FCB fcb_table[NUM_DIRECTORY_ENTRIES];

	for (int i = 0; i < NUM_INDEX_ENTRIES; i++) {
		IndexEntry index_1_table_cluster = root_dir_0[index_0 + i];
		bool found = false;

		if (index_1_table_cluster == 0) {
			return;
		}

		directory_index_cache.read_cluster(index_1_table_cluster, 0, NUM_INDEX_ENTRIES * sizeof(IndexEntry), (char*)index_table_1);

		for (int j = 0; j < NUM_INDEX_ENTRIES; j++) {
			IndexEntry dir_table_cluster = index_table_1[index_1 + j];

			if (dir_table_cluster == 0) {
				return;
			}

			directory_cache.read_cluster(dir_table_cluster, 0, NUM_DIRECTORY_ENTRIES * sizeof(FCB), (char*)fcb_table);

			for (int k = 0; k < NUM_DIRECTORY_ENTRIES-1; k++) {
				if (std::memcmp(&fcb_table[k], &zero_fcb, sizeof(FCB))) {
					return;
				}

				if (std::strncmp(fcb_table[k].name, file_name, FNAMELEN) == 0) {
					found = true;
				}

				if (found) {
					fcb_table[k] = fcb_table[k + 1];
				}
			}
		}
	}
	return;
}

unsigned int DirectoryCache::calculate_hash(const char *file_name)
{
	return std::hash<const char*>{}(file_name) % (NUM_INDEX_ENTRIES * NUM_INDEX_ENTRIES);
}

unsigned int DirectoryCache::get_index_0(unsigned int hash)
{
	return hash / NUM_INDEX_ENTRIES;
}

unsigned int DirectoryCache::get_index_1(unsigned int hash)
{
	return hash % NUM_INDEX_ENTRIES;
}
