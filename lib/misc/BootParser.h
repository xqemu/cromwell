#ifndef _BootParser_H_
#define _BootParser_H_

#include "BootEEPROM.h"

#define MAX_CONFIG_FILESIZE 1024*256
#define MAX_LINE 2048

#define MAX_CONFIG_TITLE 24

struct CONFIGENTRY;

enum BootTypes {
	BOOT_CDROM,
	BOOT_FATX,
	BOOT_NATIVE,
};

enum BootSystems {
	SYS_LINUX,
	SYS_REACTOS,
};

typedef struct OPTLINUX {
	char szKernel[MAX_LINE];
	char szInitrd[MAX_LINE];
	char szAppend[MAX_LINE];
} OPTLINUX;

typedef struct OPTMULTIBOOT {
	u8* pBuffer;
	u32 uBufferSize;
	u32 uBootDevice;
} OPTMULTIBOOT;

typedef struct CONFIGENTRY {
	int  drive;
	int  partition;
	int  isDefault;
	enum BootTypes bootType;
	enum BootSystems bootSystem;
	char title[MAX_CONFIG_TITLE];
	char szPath[MAX_LINE];
	union {
		struct OPTLINUX Linux;
		struct OPTMULTIBOOT Multiboot;
	} opt;

	// Linked list of menu items
	struct CONFIGENTRY *previousConfigEntry;
	struct CONFIGENTRY *nextConfigEntry;

	struct CONFIGENTRY *nestedConfigEntry;

} CONFIGENTRY;

CONFIGENTRY* ParseConfig(char *szBuffer, unsigned int fileLen, char *szPath);
#endif // _BootParser_H_
