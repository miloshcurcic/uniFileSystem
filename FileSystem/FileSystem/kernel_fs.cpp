#include "kernel_fs.h"
#include "part.h"

char KernelFS::mount(Partition* partition)
{
    if (partition == nullptr || mounted_partition != nullptr) {
        return 0;
    }
    mounted_partition = partition;
    bit_vector_size = ClusterSize * BYTE_LEN / partition->getNumOfClusters() + (ClusterSize * BYTE_LEN % partition->getNumOfClusters() != 0 ? 1 : 0);
    root_dir_index0 = bit_vector_size;
}

char KernelFS::unmount()
{
    mounted_partition = nullptr;
    return 0;
}

KernelFile* KernelFS::open(char* fname, char mode)
{
    return nullptr;
}
