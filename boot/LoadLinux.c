/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"
#include "video.h"
#include "memory_layout.h"
#include <shared.h>
#include <filesys.h>
#include "rc4.h"
#include "sha1.h"
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"
#include "BootParser.h"
#include "config.h"
#include "iso_fs.h"

//Grub bits
unsigned long saved_drive;
grub_error_t errnum;
unsigned long saved_partition;
unsigned long boot_drive;
	
static int nRet;
static u32 dwKernelSize = 0, dwInitrdSize = 0;


void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine, unsigned int entry);
void setup(void* KernelPos, void* PhysInitrdPos, unsigned long InitrdSize, const char* kernel_cmdline);
void I2CRebootSlow(void);

void try_elf_boot(char *data, int len);


void BootPrintConfig(const OPTLINUX *optLinux) {
	int CharsProcessed=0, CharsSinceNewline=0, Length=0;
	char c;
	printk("  Bootconfig : Kernel  %s \n", optLinux->szKernel);
	VIDEO_ATTR=0xffa8a8a8;
	if (strlen(optLinux->szInitrd)) {
		printk("  Bootconfig : Initrd  %s \n", optLinux->szInitrd);
	}
	printk("  Bootconfig : Kernel commandline :\n");
	Length = strlen(optLinux->szAppend);
	while (CharsProcessed<Length) {
		c = optLinux->szAppend[CharsProcessed];
		CharsProcessed++;
		CharsSinceNewline++;
		if ((CharsSinceNewline>50 && c==' ') || CharsSinceNewline>65) {
			printk("\n");
			if (CharsSinceNewline>25) printk("%c",c);
			CharsSinceNewline = 0;
		} 
		else printk("%c",c);
	}
	printk("\n");
	VIDEO_ATTR=0xffa8a8a8;
}


void memPlaceKernel(const u8* kernelOrg, u32 kernelSize)
{
	unsigned int nSizeHeader=((*(kernelOrg + 0x01f1))+1)*512;
	memcpy((u8 *)KERNEL_SETUP, kernelOrg, nSizeHeader);
	memcpy((u8 *)KERNEL_PM_CODE, kernelOrg + nSizeHeader, kernelSize - nSizeHeader);

	/* Try to execute a pure ELF binary here, using the etherboot
	 * code. This is required for ELF kernels, such as FreeBSD */
	try_elf_boot((char *)kernelOrg, kernelSize);
}



CONFIGENTRY *DetectLinuxNative(char *szGrub) {
	CONFIGENTRY *config;
	unsigned int nLen;
	u32 dwConfigSize = 0;

	memset((u8 *)KERNEL_SETUP, 0, KERNEL_HDR_SIZE);

	//Try for /boot/linuxboot.cfg first
	strcpy(&szGrub[4], "/boot/linuxboot.cfg");
	nRet=grub_open(szGrub);

	if (nRet != 1 || errnum) {
		//Not found - try /linuxboot.cfg
		errnum = 0;
		strcpy(&szGrub[4], "/linuxboot.cfg");
		nRet=grub_open(szGrub);
	}
	
	dwConfigSize=filemax;
	if (nRet != 1 || errnum) {
		//File not found
		errnum = 0;
		return NULL;
	}
	
	nLen=grub_read((void *)KERNEL_SETUP, filemax);

	if (nLen > MAX_CONFIG_FILESIZE) nLen = MAX_CONFIG_FILESIZE;
	
	config = ParseConfig((char *)KERNEL_SETUP, nLen, NULL);

	grub_close();
	return config;
}

int LoadLinuxNative(char *szGrub, const OPTLINUX *optLinux) {
	u8* tempBuf;

	memset((u8 *)KERNEL_SETUP, 0, KERNEL_HDR_SIZE);

	VIDEO_ATTR=0xffd8d8d8;
	printk("  Loading %s ", optLinux->szKernel);
	VIDEO_ATTR=0xffa8a8a8;
	strncpy(&szGrub[4], optLinux->szKernel, strlen(optLinux->szKernel));

	nRet=grub_open(szGrub);

	if (nRet != 1) {
		printk("Unable to load kernel, Grub error %d\n", errnum);
		errnum = 0;
		wait_ms(2000);
		return false;
	}
        
	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (u8*)INITRD_START;
	dwKernelSize=grub_read(tempBuf, MAX_KERNEL_SIZE);
	memPlaceKernel(tempBuf, dwKernelSize);
	grub_close();
	printk(" - %d bytes\n", dwKernelSize);

	if (strlen(optLinux->szInitrd)) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s ", optLinux->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
		strncpy(&szGrub[4], optLinux->szInitrd, sizeof(optLinux->szInitrd));
		nRet=grub_open(szGrub);
		if (filemax == 0) {
			printf("Error: initrd file is empty!\n");
			wait_ms(2000);
			return false;
		}
		if ((nRet != 1) || errnum) {
			printk("Unable to load initrd, Grub error %d\n", errnum);
			errnum = 0;
			wait_ms(2000);
			return false;
		}
		printk(" - %d bytes\n", filemax);
		dwInitrdSize=grub_read((void*)INITRD_START, MAX_INITRD_SIZE);
		grub_close();
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
	}
	return true;
}

CONFIGENTRY *DetectLinuxFATX(FATXPartition *partition) {
	FATXFILEINFO fileinfo;
	CONFIGENTRY *config=NULL, *currentConfigItem=NULL;
	
	if (LoadFATXFile(partition, "/linuxboot.cfg", &fileinfo)) {
		//Root of partition has a linuxboot.cfg in it
		config = (CONFIGENTRY *)malloc(sizeof(CONFIGENTRY));
		config = ParseConfig(fileinfo.buffer, fileinfo.fileSize, NULL);
		free(fileinfo.buffer);
	}
	else if (LoadFATXFile(partition, "/debian/linuxboot.cfg", &fileinfo)) {
		//Try in /debian
		config = (CONFIGENTRY *)malloc(sizeof(CONFIGENTRY));
		config = ParseConfig(fileinfo.buffer, fileinfo.fileSize, "/debian");
		free(fileinfo.buffer);
	}

	return config;
}

int LoadLinuxFATX(FATXPartition *partition, const OPTLINUX *optLinux) {

	static FATXFILEINFO fileinfo;
	static FATXFILEINFO infokernel;
	static FATXFILEINFO infoinitrd;
	u8* tempBuf;

	memset((u8 *)KERNEL_SETUP, 0, KERNEL_HDR_SIZE);
	memset(&fileinfo, 0x00, sizeof(fileinfo));
	memset(&infokernel, 0x00, sizeof(infokernel));
	memset(&infoinitrd, 0x00, sizeof(infoinitrd));

	VIDEO_ATTR=0xffd8d8d8;
	printk("  Loading %s from FATX", optLinux->szKernel);
	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (u8*)INITRD_START;
	if (!LoadFATXFilefixed(partition, optLinux->szKernel, &infokernel, tempBuf)) {
		printk("Error loading kernel %s\n", optLinux->szKernel);
		wait_ms(5000);
		return false;
	} else {
		dwKernelSize = infokernel.fileSize;
		// moving the kernel to its final location
		memPlaceKernel(tempBuf, dwKernelSize);
		
		printk(" - %d bytes\n", infokernel.fileRead);
	}

	if (strlen(optLinux->szInitrd)) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from FATX", optLinux->szInitrd);
		wait_ms(50);
		if (!LoadFATXFilefixed(partition, optLinux->szInitrd, &infoinitrd, (void *)INITRD_START)) {
			printk("Error loading initrd %s\n", optLinux->szInitrd);
			wait_ms(2000);
			return false;
		}
		
		dwInitrdSize = infoinitrd.fileSize;
		printk(" - %d %d bytes\n", dwInitrdSize,infoinitrd.fileRead);
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
		printk("");
	}
	return true;
}


CONFIGENTRY *DetectLinuxCD(int cdromId) {
	long dwConfigSize=0;
	CONFIGENTRY *config;

	memset((u8 *)KERNEL_SETUP, 0, KERNEL_HDR_SIZE);

	dwConfigSize = BootIso9660GetFile(cdromId, "/linuxboo.cfg", (u8 *)KERNEL_SETUP, 0x800);

	if (dwConfigSize <= 0) {
		dwConfigSize = BootIso9660GetFile(cdromId, "/linuxboot.cfg", (u8 *)KERNEL_SETUP, 0x800);
	}

	//Failed to load the config file
	if (dwConfigSize <= 0) return NULL;
        
	// LinuxBoot.cfg File Loaded
	config = ParseConfig((char *)KERNEL_SETUP, dwConfigSize, NULL);
	
	return config;
}

int LoadLinuxCD(CONFIGENTRY *config) {
	const OPTLINUX *optLinux = &config->opt.Linux;
	u8* tempBuf;

	memset((u8 *)KERNEL_SETUP, 0, KERNEL_HDR_SIZE);

	VIDEO_ATTR=0xffd8d8d8;
	printk("  Loading %s from CD", optLinux->szKernel);
	VIDEO_ATTR=0xffa8a8a8;
	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (u8*)INITRD_START;
	dwKernelSize = BootIso9660GetFile(config->drive, optLinux->szKernel, tempBuf, MAX_KERNEL_SIZE);

	if (dwKernelSize < 0) {
		printk("Not Found, error %d\nHalting\n", dwKernelSize);
		wait_ms(2000);
		return false;
	} else {
		memPlaceKernel(tempBuf, dwKernelSize);
		printk(" - %d bytes\n", dwKernelSize);
	}

	if (strlen(optLinux->szInitrd)) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from CD", optLinux->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
		
		dwInitrdSize = BootIso9660GetFile(config->drive, optLinux->szInitrd, (void *)INITRD_START, MAX_INITRD_SIZE);
		if (dwInitrdSize < 0) {
			printk("Not Found, error %d\nHalting\n", dwInitrdSize);
			wait_ms(2000);
			return false;
		}
		printk(" - %d bytes\n", dwInitrdSize);
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
		printk("");
	}
	return true;
}

void ExittoLinux(const OPTLINUX *optLinux) {
	VIDEO_ATTR=0xff8888a8;
	BootPrintConfig(optLinux);
	printk("     Kernel:  %s\n", (char *)(0x00090200+(*((u16 *)0x9020e)) ));
	printk("\n");
	{
		char *sz="\2Starting Linux\2";
		VIDEO_CURSOR_POSX=((vmode.width-BootVideoGetStringTotalWidth(sz))/2)*4;
		VIDEO_CURSOR_POSY=vmode.height-64;

		VIDEO_ATTR=0xff9f9fbf;
		printk(sz);
	}
	setLED("rrrr");
	startLinux((void*)INITRD_START, dwInitrdSize, optLinux->szAppend, 0x100000);
}
	

void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine, unsigned int entry)
{
	int nAta=0;
	// turn off USB
	BootStopUSB();
	setup( (void *)KERNEL_SETUP, initrdStart, initrdSize, appendLine);
        
	if(tsaHarddiskInfo[0].m_bCableConductors == 80) {
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&2) nAta=1;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&4) nAta=2;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&8) nAta=3;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&16) nAta=4;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&32) nAta=5;
	} else {
		// force the HDD into a good mode 0x40 ==UDMA | 2 == UDMA2
		nAta=2; // best transfer mode without 80-pin cable
	}
	// nAta=1;
	BootIdeSetTransferMode(0, 0x40 | nAta);
	BootIdeSetTransferMode(1, 0x40 | nAta);

	// Set the LED to auto-mode
	resetLED();
	         
	// Set framebuffer address to final location (for vesafb driver)
	(*(unsigned int*)0xFD600800) = (0xf0000000 | ((xbox_ram*0x100000) - FB_SIZE));
	
	// disable interrupts
	asm volatile ("cli\n");
	
	// clear idt area
	memset((void*)IDT_LOC,0x0,1024*8);
	
	__asm__ ("movl %0,%%ebx" : : "a" (entry));	/* ebx = entry */
	
	__asm __volatile__ (
	"wbinvd\n"
	
	// Flush the TLB
	"xor %eax, %eax \n"
	"mov %eax, %cr3 \n"
	
	// Load IDT table (0xB0000 = IDT_LOC)
	"lidt 	0xB0000\n"
	
	// DR6/DR7: Clear the debug registers
	"xor %eax, %eax \n"
	"mov %eax, %dr6 \n"
	"mov %eax, %dr7 \n"
	"mov %eax, %dr0 \n"
	"mov %eax, %dr1 \n"
	"mov %eax, %dr2 \n"
	"mov %eax, %dr3 \n"
	
	// Kill the LDT, if any
	"xor	%eax, %eax \n"
	"lldt %ax \n"
        
	// Reload CS as 0010 from the new GDT using a far jump
	".byte 0xEA       \n"   // jmp far 0010:reload_cs
	".long reload_cs_exit  \n"
	".word 0x0010  \n"
	
	".align 16  \n"
	"reload_cs_exit: \n"

	// CS is now a valid entry in the GDT.  Set SS, DS, and ES to valid
	// descriptors, but clear FS and GS as they are not necessary.

	// Set SS, DS, and ES to a data32 segment with maximum limit.
	"movw $0x0018, %ax \n"
	"mov %eax, %ss \n"
	"mov %eax, %ds \n"
	"mov %eax, %es \n"

	// Clear FS and GS
	"xor %eax, %eax \n"
	"mov %eax, %fs \n"
	"mov %eax, %gs \n"

	// Set the stack pointer to give us a valid stack
	"movl $0x03BFFFFC, %esp \n"
	
	"xor 	%eax, %eax \n"
	"xor 	%ecx, %ecx \n"
	"xor 	%edx, %edx \n"
	"xor 	%edi, %edi \n"
	"movl 	$0x90000, %esi\n"       // kernel setup area

	// This is for FreeBSD; i386/i386/locore.s expects certain stack
	// values for bootload info - Linux doesn't care
	"pushl	$0x0\n"
	"pushl	$0x0\n"
	"pushl	$0x0\n"
	"pushl	$0x0\n"
	"pushl	$0x0\n"

	"pushl	$0x10\n"
	"pushl	%ebx\n"			// 0x10:ebx is the entry point
	"xor	%ebx,%ebx\n"		// clean leftover ebx (held entry point)
	".byte	0xcb\n	"		// retf
	);
	
	// We are not longer here, we are already in the Linux loader, we never come back here
	
	// See you again in Linux then	
	while(1);
}

