// Functions for processing FATX partitions
// (c) 2001 Andrew de Quincey

#include "boot.h"
#include "BootFATX.h"
#include <sys/types.h>


#undef FATX_DEBUG

//#define FATX_INFO


// Temporary struct used to load XBPartitioner data directly from disk.
typedef struct
{
	char name[16];
	u32  flags;
	u32  start;
	u32  size;
	u32  reserved;
} XBPartitionerTableEntry;

// Temporary struct used to load XBPartitioner data directly from disk.
typedef struct {
	char                 magic[16];
	char                 reserved[32];
	XBPartitionerTableEntry partitions[FATX_XBPARTITIONER_PARTITIONS_MAX];
} XBPartitionerTable;

int checkForLastDirectoryEntry(unsigned char* entry) {

	// if the filename length byte is 0 or 0xff,
	// this is the last entry
	if ((entry[0] == 0xff) || (entry[0] == 0)) {
		return 1;
	}

	// wasn't last entry
	return 0;
}

int LoadFATXFilefixed(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo,u8* Position) {

	if(partition == NULL) {
		VIDEO_ATTR=0xffe8e8e8;
	} else {
		if(FATXFindFile(partition,filename,FATX_ROOT_FAT_CLUSTER,fileinfo)) {
#ifdef FATX_DEBUG
			printk("ClusterID : %d\n",fileinfo->clusterId);
			printk("fileSize  : %d\n",fileinfo->fileSize);
#endif
			fileinfo->buffer = Position;
			memset(fileinfo->buffer,0xff,fileinfo->fileSize);
			
			if(FATXLoadFromDisk(partition, fileinfo)) {
				return true;
			} else {
#ifdef FATX_INFO
				printk("LoadFATXFile : error loading %s\n",filename);
#endif
				return false;
			}
		} else {
#ifdef FATX_INFO
			printk("LoadFATXFile : file %s not found\n",filename);
#endif
			return false;
		}
	}
	return false;
}

int LoadFATXFile(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo) {

	if(partition == NULL) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("LoadFATXFile : no open FATX partition\n");
#endif
	} else {
		if(FATXFindFile(partition,filename,FATX_ROOT_FAT_CLUSTER,fileinfo)) {
#ifdef FATX_DEBUG
			printk("ClusterID : %d\n",fileinfo->clusterId);
			printk("fileSize  : %d\n",fileinfo->fileSize);
#endif
			fileinfo->buffer = malloc(fileinfo->fileSize);
			memset(fileinfo->buffer,0,fileinfo->fileSize);
			if(FATXLoadFromDisk(partition, fileinfo)) {
				return true;
			} else {
#ifdef FATX_INFO
				printk("LoadFATXFile : error loading %s\n",filename);
#endif
				return false;
			}
		} else {
#ifdef FATX_INFO
			printk("LoadFATXFile : file %s not found\n",filename);
#endif
			return false;
		}
	}
	return false;
}

void PrintFAXPartitionTable(int nDriveIndex) {
	FATXPartition *partition = NULL;
	FATXFILEINFO fileinfo;

	VIDEO_ATTR=0xffe8e8e8;
	printk("FATX Partition Table:\n");
	memset(&fileinfo,0,sizeof(FATXFILEINFO));

	if(FATXSignature(nDriveIndex,SECTOR_SYSTEM)) {
		VIDEO_ATTR=0xffe8e8e8;
		printk("Partition SYSTEM\n");
		partition = OpenFATXPartition(nDriveIndex,SECTOR_SYSTEM,SYSTEM_SIZE);
		if(partition == NULL) {
			VIDEO_ATTR=0xffe8e8e8;
			printk("PrintFAXPartitionTable : error on opening STORE\n");
		} else {
			DumpFATXTree(partition);
		}
	}

	VIDEO_ATTR=0xffc8c8c8;
}

int FATXSignature(int nDriveIndex, unsigned int partitionOffset) {

	unsigned char partitionInfo[FATX_PARTITION_MAGIC_LEN];
	unsigned int readSize;
	#ifdef FATX_DEBUG
		//printk("FATXSignature: Read partition header\n");
	#endif
	// load the partition header
	readSize = FATXRawRead(nDriveIndex, partitionOffset, 0, FATX_PARTITION_MAGIC_LEN, (char *)&partitionInfo);

	if (readSize != FATX_PARTITION_MAGIC_LEN) {
		VIDEO_ATTR=0xffe8e8e8;
	#ifdef FATX_INFO
			printk("FATXSignature: Out of data reading partition header drv %d offset 0x%X\n", nDriveIndex, partitionOffset);
	#endif
		return false;
	}
	if (strncmp(partitionInfo, "FATX", FATX_PARTITION_MAGIC_LEN)) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("FATXSignature: No FATX partition magic found at drv %d offset 0x%X\n", nDriveIndex, partitionOffset);
#endif
		return false;
	}
	return true;
}

void LoadStockPartitionTable(int driveId, FATXPartitionTable *partitionTable) {
	#ifdef FATX_INFO
	//printk("LoadStockPartitionTable\n");
	#endif
	partitionTable->partitions[0] = OpenFATXPartition(driveId, SECTOR_STORE, STORE_SIZE);
	partitionTable->partitions[1] = OpenFATXPartition(driveId, SECTOR_SYSTEM, SYSTEM_SIZE);
	partitionTable->partitions[2] = OpenFATXPartition(driveId, SECTOR_CACHE1, CACHE1_SIZE);
	partitionTable->partitions[3] = OpenFATXPartition(driveId, SECTOR_CACHE2, CACHE2_SIZE);
	partitionTable->partitions[4] = OpenFATXPartition(driveId, SECTOR_CACHE3, CACHE3_SIZE);
	for (int partIdx = 0; partIdx < FATX_STOCK_PARTITIONS_MAX; partIdx++) {
		if (partitionTable->partitions[partIdx] != NULL) {
			partitionTable->nPartitions++;
		}
	}
}

void LoadXBPartitionerTable(int driveId, FATXPartitionTable *partitionTable) {
	XBPartitionerTable xbPartTable;
	XBPartitionerTableEntry xbPartEntry;
	memset(&xbPartTable,0,sizeof(XBPartitionerTable));

	BootIdeReadSector(driveId, &xbPartTable, SECTOR_CONFIG, 0, sizeof(XBPartitionerTable));
	if (strncmp("****PARTINFO****", xbPartTable.magic, 16)) {
		#ifdef FATX_INFO
		printk("No XBPartitioner partition table detected\n");
		#endif
	}
	#ifdef FATX_INFO
	printk("LoadXBPartitionerTable: XBPartitioner partition table detected on drive %d\n", driveId);
	#endif
	for (int partIdx = 0; partIdx < FATX_XBPARTITIONER_PARTITIONS_MAX; partIdx++) {
		xbPartEntry = xbPartTable.partitions[partIdx];
		if (!(xbPartEntry.flags & FATX_XBPARTITIONER_PARTITION_IN_USE)) {
			continue;
		}
		#ifdef FATX_INFO
		// printk("Found XBPartitioner entry start 0x%X size 0x%X\n", xbPartEntry.start, xbPartEntry.size);
		#endif
		partitionTable->partitions[partIdx] = OpenFATXPartition(driveId, xbPartEntry.start, xbPartEntry.size);
		if (partitionTable->partitions[partIdx] == NULL) {
			#ifdef FATX_INFO
			printk("Could not open XBPartitioner drive %d partindex %d start %X size %X\n", driveId, partIdx, xbPartEntry.start, xbPartEntry.size);
			#endif
		}
		else {
			partitionTable->nPartitions+=1;
		}
	}
	if (partitionTable->nPartitions == 0) {
		#ifdef FATX_DBEBUG
		printk("Could not load any XBPartitioner partitions, but table magic found?\n");
		#endif
	}
}

// This needs to be freed using CloseFATXPartitionTable, which also closes its
// component partitions.
FATXPartitionTable *OpenFatXPartitionTable(int driveId) {
	FATXPartitionTable* partitionTable;
	u8 ba[4];

	// Check for filesystem magic number at sector 3
	memset(ba,0x00,FATX_DISK_BRFR_MAGIC_LEN);
	BootIdeReadSector(driveId, ba, FATX_DISK_BRFR_MAGIC_SECTOR, 0, FATX_DISK_BRFR_MAGIC_LEN);

	if (strncmp("BRFR",ba,FATX_DISK_BRFR_MAGIC_LEN)) {
		#ifdef FATX_DEBUG
			printk("OpenFatXPartitionTable drive %d: Couldn't find FATX magic number\n", driveId);
		#endif
		return NULL;
	}

	partitionTable = malloc(sizeof(FATXPartitionTable));
	if (partitionTable == NULL) {
		#ifdef FATX_DEBUG
				printk("OpenFatXPartitionTable: Out of memory\n");
		#endif
		return NULL;
	}
	memset(partitionTable,0,sizeof(FATXPartitionTable));

	LoadXBPartitionerTable(driveId, partitionTable);
	if (partitionTable->nPartitions == 0) {
		#ifdef FATX_INFO
		printk("No XBPartitioner partitions found; checking for standard partition table\n");
		#endif
		LoadStockPartitionTable(driveId, partitionTable);
	}

	if (partitionTable->nPartitions == 0) {
		#ifdef FATX_INFO
		printk("No FATX partitions found\n");
		#endif
		CloseFATXPartitionTable(partitionTable);
		return NULL;
	}

	#ifdef FATX_INFO
	printk("Found %d partitions\n", partitionTable->nPartitions);
	#endif
	return partitionTable;

}

FATXPartition *OpenFATXPartition(int nDriveIndex,
	unsigned int partitionOffset,
	unsigned int partitionSize) {

	FATXPartition *partition;
	unsigned int clusterSizeKB;
	unsigned int chainTableSize;

	// Checks for "FATX" magic
	if (!FATXSignature(nDriveIndex, partitionOffset)) {
		return NULL;
	}

	partition = malloc(sizeof(FATXPartition));
	if (partition == NULL) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_DEBUG
		printk("OpenFATXPartition : Out of memory\n");
#endif
		return NULL;
	}
	memset(partition,0,sizeof(FATXPartition));

	// setup the easy bits
	partition->nDriveIndex = nDriveIndex;
	partition->partitionStart = partitionOffset;
	partition->partitionSize = partitionSize;

	// calculate cluster size
	clusterSizeKB = 16;
	u_int64_t compare = 0x20000000;
	while (partitionSize > compare)
	{
		compare *= 2;
		clusterSizeKB *= 2;
	}
	partition->clusterSize = clusterSizeKB * 0x400;
	partition->clusterCount = (partitionSize / (partition->clusterSize / FATX_SECTOR_SIZE)) + 1;

	// The chain map entry size is 2 if there are < 65525 clusters, and the
	// filesystem is "FATX16".  Usually, the chain map entry size is 4.
	partition->chainMapEntrySize = (partition->clusterCount >= 0xfff4) ? 4 : 2; // 65525

	// Calculate the size of the chain map; it gets rounded up even if
	// the chainTableSize % 4096 == 0
	chainTableSize = (partition->clusterCount * partition->chainMapEntrySize);
	chainTableSize += 4096 - chainTableSize % 4096;

	partition->cluster1Address =  FATX_CHAINTABLE_BLOCKSIZE + chainTableSize;
	partition->nCachedChainMapBlock = -1;

	#ifdef FATX_INFO
	printk("OpenFATXPartition: Drv: %d, start: 0x%X, nSec: 0x%X ", nDriveIndex, partition->partitionStart, partition->partitionSize);
	printk("nClus: 0x%X, cSiz: %dKB\n", partition->clusterCount, clusterSizeKB, partition->clusterSize);
	printk("OpenFATXPartition: chaintableSize: 0x%X chaintableEntrySize: %d cl1 addr: 0x%X\n",
				 chainTableSize, partition->chainMapEntrySize, partition->cluster1Address);
	#endif

	return partition;
}

bool FATXCacheChainMapBlock(FATXPartition *partition, int blockNum) {
	unsigned int bytesRead = 0;
	if (partition->nCachedChainMapBlock < 0) {
			partition->cachedChainMapBlock = (u_int32_t *)malloc(FATX_CHAINTABLE_BLOCKSIZE);
	}
#ifdef FATX_INFO
	// printk("FATXCacheChainMapBlock: caching block num %d\n", blockNum);
#endif
	bytesRead = FATXRawRead(partition->nDriveIndex, partition->partitionStart,
													FATX_CHAINTABLE_BLOCKSIZE + FATX_CHAINTABLE_BLOCKSIZE * blockNum,
													FATX_CHAINTABLE_BLOCKSIZE, (u8*)partition->cachedChainMapBlock);
	if (bytesRead != FATX_CHAINTABLE_BLOCKSIZE) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_DEBUG
		printk("FATXCacheChainMapBlock : out of data??\n");
#endif
		partition->nCachedChainMapBlock = -1;
		return false;
	}
	partition->nCachedChainMapBlock = blockNum;

	return true;
}

void DumpFATXTree(FATXPartition *partition) {
	// OK, start off the recursion at the root FAT
	_DumpFATXTree(partition, FATX_ROOT_FAT_CLUSTER, 0);
}

void _DumpFATXTree(FATXPartition* partition, int clusterId, int nesting) {

	int endOfDirectory;
	unsigned char* curEntry;
	unsigned char clusterData[partition->clusterSize];
	int i,j;
	char writeBuf[512];
	char filename[50];
	u_int32_t filenameSize;
	u_int32_t fileSize;
	u_int32_t entryClusterId;
	unsigned char flags;
	char flagsStr[5];

	// OK, output all the directory entries
	endOfDirectory = 0;
	while(clusterId != -1) {
		LoadFATXCluster(partition, clusterId, clusterData);

		// loop through it, outputing entries
		for(i=0; i< partition->clusterSize / FATX_DIRECTORYENTRY_SIZE; i++) {

			// work out the currentEntry
			curEntry = clusterData + (i * FATX_DIRECTORYENTRY_SIZE);

			// first of all, check that it isn't an end of directory marker
			if (checkForLastDirectoryEntry(curEntry)) {
				endOfDirectory = 1;
				break;
			}

			// get the filename size
			filenameSize = curEntry[0];

			// check if file is deleted
			if (filenameSize == 0xE5) {
				continue;
			}

			// check size is OK
			if ((filenameSize < 1) || (filenameSize > FATX_FILENAME_MAX)) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("_DumpFATXTree : Invalid filename size: %i\n", filenameSize);
			}

			// extract the filename
			memset(filename, 0, 50);
			memcpy(filename, curEntry+2, filenameSize);
			filename[filenameSize] = 0;

			// get rest of data
			flags = curEntry[1];
			entryClusterId = *((u_int32_t*) (curEntry + 0x2c));
			fileSize = *((u_int32_t*) (curEntry + 0x30));

			// wipe fileSize
			if (flags & FATX_FILEATTR_DIRECTORY) {
				fileSize = 0;
			}

			// zap flagsStr
			strcpy(flagsStr, "    ");

			// work out other flags
			if (flags & FATX_FILEATTR_READONLY) {
	              		flagsStr[0] = 'R';
			}
			if (flags & FATX_FILEATTR_HIDDEN) {
				flagsStr[1] = 'H';
			}
			if (flags & FATX_FILEATTR_SYSTEM) {
				flagsStr[2] = 'S';
			}
			if (flags & FATX_FILEATTR_ARCHIVE) {
				flagsStr[3] = 'A';
			}

			// check we don't have any unknown flags
/*
			if (flags & 0xc8) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("WARNING: file %s has unknown flags %x\n", filename, flags);
			}
*/
			
			// Output it
			for(j=0; j< nesting; j++) {
				writeBuf[j] = ' ';
			}

			VIDEO_ATTR=0xffe8e8e8;
			printk("/%s  [%s] (SZ:%i CL%x))\n",filename, flagsStr,
					fileSize, entryClusterId);

			// If it is a sub-directory, recurse
			/*
			if (flags & FATX_FILEATTR_DIRECTORY) {
				_DumpFATXTree(partition, entryClusterId, nesting+1);
			}
			*/
			// have we hit the end of the directory yet?
		}		
		if (endOfDirectory) {
			break;
		}	
		clusterId = getNextClusterInChain(partition, clusterId);
	}
}

int FATXLoadFromDisk(FATXPartition* partition, FATXFILEINFO *fileinfo) {

	unsigned char clusterData[partition->clusterSize];
	int fileSize = fileinfo->fileSize;
	int written;
	int clusterId = fileinfo->clusterId;
	u8 *ptr;

	fileinfo->fileRead = 0;
	ptr = fileinfo->buffer;

	// loop, outputting clusters
	while(clusterId != -1) {
		// Load the cluster data
		LoadFATXCluster(partition, clusterId, clusterData);

		// Now, output it
		written = (fileSize <= partition->clusterSize) ? fileSize : partition->clusterSize;
		memcpy(ptr,clusterData,written);
		fileSize -= written;
		fileinfo->fileRead+=written;
		ptr+=written;

		// Find next cluster
		clusterId = getNextClusterInChain(partition, clusterId);
	}

	// check we actually found enough data
	if (fileSize != 0) {
#ifdef FATX_INFO
   	printk("Hit end of cluster chain before file size was zero\n");
#endif
		//free(fileinfo->buffer);
		//fileinfo->buffer = NULL;
		return false;
	}
	return true;
}

int FATXFindFile(FATXPartition* partition,
                    char* filename,
                    int clusterId, FATXFILEINFO *fileinfo) {

	int i = 0;
#ifdef FATX_DEBUG
	VIDEO_ATTR=0xffc8c8c8;
	printk("FATXFindFile : %s\n",filename);
#endif

  	// convert any '\' to '/' characters
  	while(filename[i] != 0) {
	    	if (filename[i] == '\\') {
      			filename[i] = '/';
	    	}
	    	i++;
  	}
  	
	// skip over any leading / characters
  	i=0;
  	while((filename[i] != 0) && (filename[i] == '/')) {
	    	i++;
  	}

	return _FATXFindFile(partition,&filename[i],clusterId,fileinfo);
}

int _FATXFindFile(FATXPartition* partition,
                    char* filename,
                    int clusterId, FATXFILEINFO *fileinfo) {
	unsigned char* curEntry;
	unsigned char clusterData[partition->clusterSize];
	int i = 0;
	int endOfDirectory;
	u_int32_t filenameSize;
	u_int32_t flags;
	u_int32_t entryClusterId;
	u_int32_t fileSize;
	char seekFilename[50];
	char foundFilename[50];
	char* slashPos;
	int lookForDirectory = 0;
	int lookForFile = 0;


	// work out the filename we're looking for
	slashPos = strrchr0(filename, '/');
	if (slashPos == NULL) {
	// looking for file
		lookForFile = 1;

		// check seek filename size
		if (strlen(filename) > FATX_FILENAME_MAX) {
#ifdef FATX_INFO
			printk("Bad filename supplied (one leafname is too big)\n");
#endif
			return false;
		}

		// copy the filename to look for
		strcpy(seekFilename, filename);
	} else {
		// looking for directory
		lookForDirectory = 1;

		// check seek filename size
		if ((slashPos - filename) > FATX_FILENAME_MAX) {
#ifdef FATX_INFO
			printk("Bad filename supplied (one leafname is too big)\n");
#endif
			return false;
		}

		// copy the filename to look for
		memcpy(seekFilename, filename, slashPos - filename);
		seekFilename[slashPos - filename] = 0;
	}

#ifdef FATX_DEBUG
	VIDEO_ATTR=0xffc8c8c8;
	printk("_FATXFindFile : %s\n",filename);
#endif
	// OK, search through directory entries
	endOfDirectory = 0;
	while(clusterId != -1) {
    		// load cluster data
    		LoadFATXCluster(partition, clusterId, clusterData);

		// loop through it, outputing entries
		for(i=0; i< partition->clusterSize / FATX_DIRECTORYENTRY_SIZE; i++) {
			// work out the currentEntry
			curEntry = clusterData + (i * FATX_DIRECTORYENTRY_SIZE);

			// first of all, check that it isn't an end of directory marker
			if (checkForLastDirectoryEntry(curEntry)) {
				endOfDirectory = 1;
				break;
			}

			// get the filename size
			filenameSize = curEntry[0];

			// check if file is deleted
			if (filenameSize == 0xE5) {
				continue;
			}

			// check size is OK
			if ((filenameSize < 1) || (filenameSize > FATX_FILENAME_MAX)) {
#ifdef FATX_INFO
				printk("Invalid filename size: %i\n", filenameSize);
#endif
				return false;
			}

			// extract the filename
			memset(foundFilename, 0, 50);
			memcpy(foundFilename, curEntry+2, filenameSize);
			foundFilename[filenameSize] = 0;

			// get rest of data
			flags = curEntry[1];
			entryClusterId = *((u_int32_t*) (curEntry + 0x2c));
			fileSize = *((u_int32_t*) (curEntry + 0x30));

			// is it what we're looking for...
			if (strlen(seekFilename)==strlen(foundFilename) && strncmp(foundFilename, seekFilename,strlen(seekFilename)) == 0) {
				// if we're looking for a directory and found a directory
				if (lookForDirectory) {
					if (flags & FATX_FILEATTR_DIRECTORY) {
						return _FATXFindFile(partition, slashPos+1, entryClusterId,fileinfo);
					} else {
#ifdef FATX_INFO
						printk("File not found\n");
#endif
						return false;
					}
				}

				// if we're looking for a file and found a file
				if (lookForFile) {
					if (!(flags & FATX_FILEATTR_DIRECTORY)) {
						fileinfo->clusterId = entryClusterId;
						fileinfo->fileSize = fileSize;
						memset(fileinfo->filename,0,sizeof(fileinfo->filename));
						strcpy(fileinfo->filename,filename);
						return true;
					} else {
#ifdef FATX_INFO
						printk("File not found %s\n",filename);
#endif
						return false;
					}
				}
			}
		}

		// have we hit the end of the directory yet?
		if (endOfDirectory) {
			break;
		}

		// Find next cluster
		clusterId = getNextClusterInChain(partition, clusterId);
	}

	// not found it!
#ifdef FATX_INFO
	printk("File not found\n");
#endif
	return false;
}



u_int32_t getNextClusterInChain(FATXPartition* partition, int clusterId) {
	int nextClusterId = 0;
	u_int32_t eocMarker = 0;
	u_int32_t rootFatMarker = 0;
	u_int32_t maxCluster = 0;

	u_int32_t cachedChainMapBlockStartId;
	u_int32_t targetChainMapBlockNum = (clusterId * partition->chainMapEntrySize) / 4096;
	if (partition->nCachedChainMapBlock != targetChainMapBlockNum) {
		if (!FATXCacheChainMapBlock(partition, targetChainMapBlockNum)) {
			printk("Error caching chain map block %d\n", targetChainMapBlockNum);
			return -1;
		}
	}
	cachedChainMapBlockStartId = partition->nCachedChainMapBlock * (4096 / partition->chainMapEntrySize);
	if (clusterId < cachedChainMapBlockStartId || clusterId >= clusterId + 4096 / partition->chainMapEntrySize) {
		VIDEO_ATTR=0xffe8e8e8;
		printk("getNextClusterInChain : Wrong block cached; block start id %d, expected %d\n", cachedChainMapBlockStartId, clusterId);
		return -1;
	}

	if (clusterId < 1) {
		VIDEO_ATTR=0xffe8e8e8;
		printk("getNextClusterInChain : Attempt to access invalid cluster: %i\n", clusterId);
		return -1;
	}

	// get the next ID
	if (partition->chainMapEntrySize == 2) {
		nextClusterId = ((uint16_t *)partition->cachedChainMapBlock)[clusterId - cachedChainMapBlockStartId];
	  eocMarker = 0xffff;
		rootFatMarker = 0xfff8;
		maxCluster = 0xfff4;
	} else if (partition->chainMapEntrySize == 4) {
		nextClusterId = partition->cachedChainMapBlock[clusterId - cachedChainMapBlockStartId];
		eocMarker = 0xffffffff;
		rootFatMarker = 0xfffffff8;
		maxCluster = 0xfffffff4;
	} else {
		VIDEO_ATTR=0xffe8e8e8;
		printk("getNextClusterInChain : Unknown cluster chain map entry size: %i\n", partition->chainMapEntrySize);
	}

	// is it the end of chain?
  	if ((nextClusterId == eocMarker) || (nextClusterId == rootFatMarker)) {
		return -1;
	}
	
	// is it something else unknown?
	if (nextClusterId == 0) {
		VIDEO_ATTR=0xffe8e8e8;
		printk("getNextClusterInChain : Cluster chain problem: Next cluster after %i is unallocated!\n", clusterId);
        }
	if (nextClusterId > maxCluster) {
		printk("getNextClusterInChain : Next cluster after %i has invalid value: %i, 0x%X\n", clusterId, nextClusterId);
	}
	
	// OK!
	return nextClusterId;
}

void LoadFATXCluster(FATXPartition* partition, int clusterId, unsigned char* clusterData) {
	u_int64_t clusterAddress;
	u_int64_t readSize;
	
	// work out the address of the cluster
	clusterAddress = partition->cluster1Address + ((unsigned long long)(clusterId - 1) * partition->clusterSize);

	// Now, load it
	readSize = FATXRawRead(partition->nDriveIndex, partition->partitionStart,
			clusterAddress, partition->clusterSize, clusterData);

  if (readSize != partition->clusterSize) {
		printk("LoadFATXCluster : Out of data while reading cluster %i\n", clusterId);
	}
}
	    



int FATXRawRead(int drive, unsigned int sector, unsigned long long byte_offset, long byte_len, char *buf) {

	int byte_read;
	
	byte_read = 0;

//	printk("rawread: sector=0x%X, byte_offset=0x%X, len=%d\n", sector, byte_offset, byte_len);

        sector+=byte_offset/512;
        byte_offset%=512;

        while(byte_len) {
		int nThisTime=512;
		if(byte_len<512) nThisTime=byte_len;
                if(byte_offset) {
	                u8 ba[512];
			if(BootIdeReadSector(drive, buf, sector, 0, 512)) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("Unable to get first sector\n");
                                return false;
			}
			memcpy(buf, &ba[byte_offset], nThisTime-byte_offset);
			buf+=nThisTime-byte_offset;
			byte_len-=nThisTime-byte_offset;
			byte_read += nThisTime-byte_offset;
			byte_offset=0;
		} else {
			if(BootIdeReadSector(drive, buf, sector, 0, nThisTime)) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("Unable to get sector 0x%X\n", sector);
				return false;
			}
			buf+=nThisTime;
			byte_len-=nThisTime;
			byte_read += nThisTime;
		}
		sector++;
	}
	return byte_read;
}

void CloseFATXPartition(FATXPartition* partition) {
	if (partition != NULL) {
		if (partition->nCachedChainMapBlock >= 0) {
			free(partition->cachedChainMapBlock);
		}
		free(partition);
		partition = NULL;
	}
}

void CloseFATXPartitionTable(FATXPartitionTable* partitionTable) {
	if (partitionTable != NULL) {
		for (int partIdx = 0; partIdx < FATX_XBPARTITIONER_PARTITIONS_MAX; partIdx++) {
			CloseFATXPartition(partitionTable->partitions[partIdx]);
		}
		free(partitionTable);
	}
}
