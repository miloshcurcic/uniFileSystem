#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <deque>
#include <string>
#include <tuple>
#include <memory>

#include "win_crit_section.h"
#include "win_cs_cond_variable.h"
#include "win_mutex.h"
#include "win_semaphore.h"
#include "win_srw_cond_variable.h"
#include "win_srw_lock.h"

typedef long FileCnt;
typedef unsigned long BytesCnt;
typedef unsigned int IndexEntry;

const unsigned int FNAMELEN = 8;
const unsigned int FEXTLEN = 3;

const unsigned int BYTE_LEN = 8;

typedef unsigned long ClusterNo;
const unsigned long ClusterSize = 2048;

const unsigned int INDEX_ENTRY_WIDTH = 4;
const unsigned int DIRECTORY_ENTRY_WIDTH = 32;

const unsigned int NUM_INDEX_ENTRIES = ClusterSize / INDEX_ENTRY_WIDTH;
const unsigned int NUM_DIRECTORY_ENTRIES = ClusterSize / DIRECTORY_ENTRY_WIDTH;

const unsigned long MAX_FILE_SIZE = 2 * ClusterSize + NUM_INDEX_ENTRIES * ClusterSize + NUM_INDEX_ENTRIES * NUM_INDEX_ENTRIES * ClusterSize;

const unsigned int CLUSTER_CACHE_SIZE = 64;