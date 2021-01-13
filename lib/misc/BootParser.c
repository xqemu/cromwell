#include "boot.h"

CONFIGENTRY *ParseConfig(char *szBuffer, unsigned int fileLen, char *szPath) {
	char *linePtr;
	char *currentPos = szBuffer;
	char *defaultTitle=NULL;

	CONFIGENTRY *rootEntry = (CONFIGENTRY*)malloc(sizeof(CONFIGENTRY));
	CONFIGENTRY *currentEntry = rootEntry;
	
	memset(rootEntry,0x00,sizeof(CONFIGENTRY));
	//Null terminate the input data
	//Caller should have allocated an extra byte at the end of the array to enable us to do this.
	*(szBuffer+fileLen)=0;
	
	for (linePtr = (char *)strsep(&currentPos, "\n"); linePtr != NULL; linePtr = (char *)strsep(&currentPos, "\n")) {
		char *param, *paramdata;
		char *p;
		
		param = (char *)strsep(&linePtr," \t");
	
		p=param+strlen(param)+1;
		//Strip off leading whitespace
		while (isspace(*p)) p++;
		
		paramdata = p;
	
		//Strip off trailing whitespace
		p=paramdata+strlen(paramdata)-1;
		while (p!=paramdata && isspace(*p)) p--;
		*(p+1)=0;
	
		if (!strncmp(param,"title",5)) {
			//Do we already have a title in this entry?
			//If so, this is the start of a new 'bootitem'.  
			//Otherwise, it begins a whole new entry
			if (strlen(currentEntry->title)==0) {
				strncpy(currentEntry->title, paramdata, strlen(paramdata));
			}
			else {	
				currentEntry->nextConfigEntry = malloc(sizeof(CONFIGENTRY));
				memset(currentEntry->nextConfigEntry, 0x00, sizeof(CONFIGENTRY));
				currentEntry->nextConfigEntry->previousConfigEntry = currentEntry;
				currentEntry = (CONFIGENTRY*)currentEntry->nextConfigEntry;
				strncpy(currentEntry->title, paramdata, strlen(paramdata));
			}
			currentEntry->bootSystem = SYS_LINUX;
		}
		else if (!strncmp(param,"kernel",6)) {
			//Handle 'xbox-os' naming conventions
			chrreplace(paramdata, '\\', '/');

			if (szPath!=NULL) { 
				sprintf(currentEntry->opt.Linux.szKernel, "%s/%s", szPath, paramdata);
			}
			else {
				//Add a leading slash, if the path name does not already contain one
				if (*paramdata=='/') sprintf(currentEntry->opt.Linux.szKernel, "%s", paramdata);
				else sprintf(currentEntry->opt.Linux.szKernel, "/%s", paramdata);
			}
		}
		else if (!strncmp(param,"initrd",6)) {
			//Handle 'xbox-os' naming conventions
			chrreplace(paramdata, '\\', '/');

			if (!strncmp(paramdata,"no",2)) {
				//Ugly, but backwards compatible
				//This means no initrd
				continue;
			}

			if (szPath!=NULL) { 
				sprintf(currentEntry->opt.Linux.szInitrd, "%s/%s", szPath, paramdata);
			}
			else {
				//Add a leading slash, if the path name does not already contain one
				if (*paramdata=='/') sprintf(currentEntry->opt.Linux.szInitrd, "%s", paramdata);
				else sprintf(currentEntry->opt.Linux.szInitrd, "/%s", paramdata);
			}
		}
		else if (!strncmp(param,"append",6)) {
			strncpy(currentEntry->opt.Linux.szAppend, paramdata, strlen(paramdata));
		}
		else if (!strncmp(param,"default",7)) {
			defaultTitle = paramdata;
		}
	}
	
	if (defaultTitle!=NULL) {
		//A default entry was specified. Find it and mark it default.
		for (currentEntry = rootEntry; currentEntry!=NULL; currentEntry = currentEntry->nextConfigEntry) {
			if (!strncmp(currentEntry->title, defaultTitle, strlen(defaultTitle))) {
				currentEntry->isDefault = 1;
				break;
			}
		}
	}
	
	return rootEntry;
}
