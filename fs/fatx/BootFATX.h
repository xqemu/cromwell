#ifndef _BootFATX_H_
#define _BootFATX_H_

#include <sys/types.h>

// Definitions for FATX on-disk structures
// (c) 2001 Andrew de Quincey

#define SECTOR_CONFIG	(0x00000000L)
#define SECTOR_CACHE1	(0x00000400L)
#define SECTOR_CACHE2	(0x00177400L)
#define SECTOR_CACHE3	(0x002EE400L)
#define SECTOR_SYSTEM	(0x00465400L)
#define SECTOR_STORE	(0x0055F400L)
#define SECTOR_EXTEND	(0x00EE8AB0L)

#define CONFIG_SIZE	(SECTOR_CACHE1 - SECTOR_CONFIG)
#define	CACHE1_SIZE	(SECTOR_CACHE2 - SECTOR_CACHE1)
#define	CACHE2_SIZE	(SECTOR_CACHE3 - SECTOR_CACHE2)
#define	CACHE3_SIZE	(SECTOR_SYSTEM - SECTOR_CACHE3)
#define SYSTEM_SIZE	(SECTOR_STORE  - SECTOR_SYSTEM)
#define STORE_SIZE	(SECTOR_EXTEND - SECTOR_STORE)

// The maximum number of partitions supported for a stock-formatted drive.
// Currently does not attempt to load an F and G which lack an XBPartitioner table.
// This legacy configuration may however be supported by the xbox-patched Linux kernel
#define FATX_STOCK_PARTITIONS_MAX 5

// The maximum number of partitions supported by the XBPartitioner table
#define FATX_XBPARTITIONER_PARTITIONS_MAX 14

// Flag that indicates whether a partition in the XBPartitioner table is active
#define FATX_XBPARTITIONER_PARTITION_IN_USE	0x80000000

// Size of a sector; 512 bytes
#define FATX_SECTOR_SIZE 0x200

// The disk sector containing the "BRFR" FATX magic number, which identifies
// the disk as being FATX-formatted.
#define FATX_DISK_BRFR_MAGIC_SECTOR 3

// The length in byes of the "BRFR" magic number
#define FATX_DISK_BRFR_MAGIC_LEN 4

// Size of FATX partition header
#define FATX_PARTITION_HEADERSIZE 0x1000

// The length in bytes of the "FATX" partition signature, which identifies a
// partition as a FATX partition.
#define FATX_PARTITION_MAGIC_LEN 4

// FATX partition magic
#define FATX_PARTITION_MAGIC 0x58544146

// FATX chain table block size
#define FATX_CHAINTABLE_BLOCKSIZE 4096

// ID of the root FAT cluster
#define FATX_ROOT_FAT_CLUSTER 1

// Size of FATX directory entries
#define FATX_DIRECTORYENTRY_SIZE 0x40

// File attribute: read only
#define FATX_FILEATTR_READONLY 0x01

// File attribute: hidden
#define FATX_FILEATTR_HIDDEN 0x02

// File attribute: system
#define FATX_FILEATTR_SYSTEM 0x04

// File attribute: archive
#define FATX_FILEATTR_ARCHIVE 0x20

// Directory entry flag indicating entry is a sub-directory
#define FATX_FILEATTR_DIRECTORY 0x10

// max filename size
#define FATX_FILENAME_MAX 42

// The drive letters, based on the partition indexes
const char FATX_DRIVE_LETTERS[FATX_XBPARTITIONER_PARTITIONS_MAX] =
  {'E', 'C', 'X', 'Y', 'Z', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N'};

// This structure describes a FATX partition
typedef struct {

  int nDriveIndex;

  // The starting sector of the partition
  u_int32_t partitionStart;

  // The size of the partition in sectors
  u_int32_t partitionSize;

  // The cluster size of the partition
  u_int32_t clusterSize;

  // Number of clusters in the partition
  u_int32_t clusterCount;

  // Size of entries in the cluster chain map
  u_int32_t chainMapEntrySize;

  // The currently cached block of the cluster chain map table.  It may be in
  // words or dwords, depending on the chain map entry size.
  u_int32_t *cachedChainMapBlock;

  u_int32_t nCachedChainMapBlock;

  // Address of cluster 1
  u_int64_t cluster1Address;

} FATXPartition;

typedef struct {
  FATXPartition* partitions[FATX_XBPARTITIONER_PARTITIONS_MAX];
  int nPartitions;
}  FATXPartitionTable;

typedef struct {
	char filename[FATX_FILENAME_MAX];
	int clusterId;
	u_int32_t fileSize;
	u_int32_t fileRead;
	u8 *buffer;
} FATXFILEINFO;

int LoadFATXFilefixed(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo,u8* Position);
int LoadFATXFile(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo);
void PrintFAXPartitionTable(int nDriveIndex);
char DriveLetterForPartitionIdx(int partitionIdx);
int FATXSignature(int nDriveIndex, unsigned int partitionOffset);
FATXPartitionTable *OpenFatXPartitionTable(int driveId);
// TODO:  Remove OpenFATXPartition from header once GentooX changes merged; should open the table instead.
// Removing it from the header requires changing its position in the source file which is not helpful to merging
FATXPartition *OpenFATXPartition(int nDriveIndex, unsigned int partitionOffset, unsigned int partitionSize);
int FATXRawRead(int drive, unsigned int sector, unsigned long long byte_offset, long byte_len, char *buf);
void DumpFATXTree(FATXPartition *partition);
void _DumpFATXTree(FATXPartition* partition, int clusterId, int nesting);
void LoadFATXCluster(FATXPartition* partition, int clusterId, unsigned char* clusterData);
u_int32_t getNextClusterInChain(FATXPartition* partition, int clusterId);
void CloseFATXPartitionTable(FATXPartitionTable* partitionTable);
int FATXFindFile(FATXPartition* partition,char* filename,int clusterId, FATXFILEINFO *fileinfo);
int _FATXFindFile(FATXPartition* partition,char* filename,int clusterId, FATXFILEINFO *fileinfo);
int FATXLoadFromDisk(FATXPartition* partition, FATXFILEINFO *fileinfo);

#endif //	_BootFATX_H_
