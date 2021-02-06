/*
 *
 *  BIOS ROM Startup Assembler
 *  (C)2002 Andy, Michael, Paul, Steve
 * Original top and bottom ROM code by Steve from an idea by Michael
 * -- NOTE: Comment removed, the top / bottom Code changed to turnaround code.
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* 
	Rewritten from Original .bin linking to compiler system by Lehner Franz (franz@caos.at) 
	Rewritten to Dual Boot concept for 2BL loading
	New written CPU Inits by Lehner Franz (franz@caos.at) 
	Written New Working Xcodes + Xcode compiler by Lehner Franz (franz@caos.at) 
	Focus support by Lehner Franz (franz@caos.at) 
	Xcalibur support by Lehner Franz (franz@caos.at) 
*/
 
#include "consts.h"
 
#define xcode_peek(val1) .byte 0x2; .long val1 ; .long 0x0 ;
#define xcode_poke(val1,val2) .byte 0x3; .long val1 ; .long val2 ;
#define xcode_pciout(val1,val2) .byte 0x4; .long val1 ; .long val2 ;
#define xcode_pciin_a(val1) .byte 0x5; .long val1 ; .long 0x0 ;

#define xcode_bittoggle(val1,val2) .byte 0x6; .long val1 ; .long val2 ;

#define xcode_ifgoto(val1,val2) .byte 0x8; .long val1 ; .long (9*(val2-1)) ;

#define xcode_outb(val1,val2) .byte 0x11; .long val1 ; .long val2 ;			    
#define xcode_inb(val1) .byte 0x12; .long val1 ; .long 0x0 ;			    

#define xcode_poke_a(val1) .byte 0x7; .long 0x3; .long val1 ;  
#define xcode_pciout_a(val1) .byte 0x7; .long 0x4; .long val1 ;  
#define xcode_outb_a(val1) .byte 0x7; .long 0x11; .long val1 ;  

#define xcode_goto(val1) .byte 0x9; .long 0x0; .long (9*(val1-1));

#define xcode_END(val1) .byte 0xEE; .long val1 ; .long 0x0;

#define SMB_xcode_Write(val1,val2); 	xcode_outb(SMBUS+8, val1); \
					xcode_outb(SMBUS+6, val2); \
					xcode_outb(SMBUS+2, 0x0000000a); \
        				xcode_inb(SMBUS); \
        				xcode_ifgoto(0x00000010,-1); \
					xcode_outb(SMBUS, 0x00000010); 

.code32

.section .low_rom, "ax"

.org 0x0
	/* MCPX Magic Values - clock timings*/
	
	.long 0xff000009
	.long 0xff000008
	.long 0x2b16d065
	.long 0x3346322d
	.long 0x01010101
	.long 0x08080808
	.long 0x00000801

	#ifndef MCPXREVD5
	.long 0xc8fc7c8a	// MCPX =< D5
	.long 0x44290213
	.long 0x90004998		
	.long 0x00000000
	#else
	.long 0xc8b4588a	// MCPX 1.6 > D5
	.long 0x00100000
	.long 0x00050aa7
	.long 0xf0000000
	#endif

	.long 0xffffffff
	.long 0xffffffff
	
	.org 0x40
	.long _start_checksum	// This number will be overwritten  
				// With imagebld, but we need a temp value
	
	.org 0x6c
	.long 0x00000107
	
	.org 0x70		// MCPX Config Area
	.long 0x0000000f
	.long 0x40004400
	
	#ifndef MCPXREVD5
	.long 0x12d10070
	.long 0x00000c90
	#else
	.long 0x16ce0090
	.long 0x00000dc0	
	#endif
	
	//The bytecode interpreter begins here
	.org 0x80

	// Note: X-codes are now translated to C code in BootSystemInitialization()

	// overflow trick
	xcode_poke(0x00000000, 0xfc1000ea);
	xcode_poke(0x00000004, 0x000008ff);

	xcode_END(0x806);

	// Note: never change this from offset 0x1000 ....
	// This is the Main Entry point ....        
        .org 0x1000
        
       	// Good Morning CPU

	// NOTE:
	/*
		We come here form the high rom section Jump
	*/
	
        // Clear Intel Interrupts in Processor Register
	// Everytime very good, specially when comming out of a running envoronment
		
	movl	$0x1b, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr
        
	// Interrupts now Dead

	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
	// kill the cache  = Disable bit 30 + 29 = CD + NW 
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
       	mov 	%cr0, %eax
	orl	$0x60000000, %eax
	mov	%eax, %cr0
	wbinvd

	// We clear the cr3 register
	mov	%eax, %eax
	mov	%eax, %cr3

	// Clear Memory Type register
	movl	$0x2ff, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr

/*
  We are setting the Ram Init's now to set up the Regions in the Ram

*/ 

	// MTRR for RAM
	// from address 0, Writeback Caching, 128MB range
        movl	$0x200, %ecx
	movl	$0x00000000, %edx
	movl	$0x00000006, %eax 	// == WB_CACHE == 6
	//movl	$0x00000004, %eax 	// Temporary, as USB development
	wrmsr

	// MASK0 set to 0xf8000[000] == 128M
        movl	$0x201, %ecx
	movl	$0x0000000f, %edx
	movl	$0xf8000800, %eax
	wrmsr

	// MTRR for shadow RAM
	// from address 0xf0000000, Write-combining Caching, 128MB range
        movl	$0x202, %ecx
	movl	$0x00000000, %edx
	movl	$0xf0000001, %eax 	// Write-Combining == 1
	wrmsr

	// MASK0 set to 0xf8000[000] == 128M
        movl	$0x203, %ecx
	movl	$0x0000000f, %edx
	movl	$0xf8000800, %eax
	wrmsr

     // MTRR for FLASH
	movl	$0x204, %ecx
	movl	$0x00000000, %edx
	movl	$0xFff00000, %eax 	    // We set to Uncacheable
	wrmsr

        movl	$0x205, %ecx
	movl	$0x0000000f, %edx
	movl	$0xfff00800, %eax
	wrmsr
	

        xor	%eax, %eax
	xor	%edx, %edx

	movl	$0x206, %ecx		// IA32_MTRR_PHYS Base 3
        wrmsr
	movl	$0x207, %ecx		// IA32_MTRR_PHYS_MASK 3
        wrmsr

	movl	$0x208, %ecx		// IA32_MTRR_PHYS Base 4
        wrmsr                                                   
	movl	$0x209, %ecx		// IA32_MTRR_PHYS_MASK 4
        wrmsr                                                   

	movl	$0x20a, %ecx		// IA32_MTRR_PHYS Base 5
        wrmsr                                                   
	movl	$0x20b, %ecx		// IA32_MTRR_PHYS_MASK 5
        wrmsr                                                   

	movl	$0x20c, %ecx		// IA32_MTRR_PHYS Base 6
        wrmsr                                                   
	movl	$0x20d, %ecx		// IA32_MTRR_PHYS_MASK 6
        wrmsr

	movl	$0x20e, %ecx		// IA32_MTRR_PHYS Base 7
        wrmsr
	movl	$0x20f, %ecx		// IA32_MTRR_PHYS_MASK 7
        wrmsr                                                   
                                              

	// Define Memory in IA32_MTRR_DEF_TYPE
	movl	$0x2ff, %ecx
	xor	%edx, %edx
	movl	$0x800, %eax //Enable MTRRs
	wrmsr

	/* turn on normal cache */
	//  bit 30 + 29 = CD + NW 
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
	movl	%cr0, %eax
	mov 	%eax, %ebx
	andl	$0x9FFFFFFF,%eax
	movl	%eax, %cr0

        cld

	// copy everything into RAM 

	mov 	$_ram_location, %edi
	mov 	$_start_ramcopy, %esi
	mov 	$(_size_ramcopy + 100), %ecx
	shr	$2,%ecx    // We dividy / 4, as we copy Dword oriented
	rep 	movsl
	
        jmp   initaftermemcpy


/*
  We leave Flash at this point, we never come back to it anymore.
  the following ASM instructions below, already are linked to memory (look to the ldscript in this case)
*/

.section .text, "ax"
                         
       // Note: We are in Memory here, not in Flash anylonger, 
       // we have been copy'd here and linked for memory, as we reside in section .text
       // Look to LDscript
       
.global	MemoryChecksum
MemoryChecksum:      	
	// The SHA-1 Hashsum is dumped here after with ImageBLD
      	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00
	.long _size_sha1hash
	.long 0x00  // Will be dumped in by imagebld ->compressed_image_start;
	.long 0x00  // Will be dumped in by imagebld ->compressed_image_size;
	.long 0x00  // Will be dumped in by imagebld ->0 .. 256kb image, 1 .. 1MB image
	
	.org 0x40  // We have linkbase for this programm = 0x100000 + 0x40 = divisable /4 .. so CPU likes it
tableGdt:

	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x00 dummy
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x08 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x10 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00 // 0x18 data32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0x8f, 0x00 // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0x8f, 0x00 // 0x28 data16
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Dummy
	
tableGdtDescriptor:

	.word 0x30
	.long tableGdt
	.word 0x0       // fill Word, so we get alliged again

tableIdtDescriptor:

	.word 2048
	.long 0x400000
	.word 0x0	// fill Word, so we get alliged again
		
initaftermemcpy:
/*
  These Init Codes Can be found in the 2'nd Bootloader

*/ 
        // We set up a GDT now, this is not necessary needed, 
        // but it is preferred, as we have more ability with commands
        
        // We clear the IDT in RAM
        xor %eax,%eax
	mov $0x5000,%ecx
	mov $0x400000,%edi
	rep stosb

        lidt tableIdtDescriptor
	lgdt tableGdtDescriptor
	// set up selectors for everything


	xor	%eax, %eax
	lldt %ax

	// Reload CS as 0010 from the new GDT using a far jump
	.byte 0xEA          // jmp far 0010:reload_cs
	.long reload_cs
	.word 0x0010
.align 16
reload_cs:

	// CS is now a valid entry in the GDT.  Set SS, DS, and ES to valid
	// descriptors, but clear FS and GS as they are not necessary.

	// Set SS, DS, and ES to a data32 segment with maximum limit.
	movw $0x0018, %ax
	mov %eax, %ss
	mov %eax, %ds
	mov %eax, %es

	// Clear FS and GS
	xor %eax, %eax
	mov %eax, %fs
	mov %eax, %gs

	movl	$0x1ffff0, %esp

	// this can be found in BootResetAction.c
	jmp 	BootStartBiosLoader

