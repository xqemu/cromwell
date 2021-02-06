/*

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-04-27 hamtitampti
	
 */

#include "2bload.h"
#include "sha1.h"

extern int decompress_kernel(char *out, char *data, int len);

u32 PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, unsigned int dw) 
{
	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
	base_addr |= ((reg_off & 0xff));

	IoOutputDword(0xcf8, base_addr );	
	IoOutputDword(0xcfc ,dw);

	return 0;    
}

u32 PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
	base_addr |= ((reg_off & 0xff));

	IoOutputDword(0xcf8, base_addr);
	return IoInputDword(0xcfc);
}

void BootSystemInitialization(void)
{
    register u32 res;

    /* translated to C from Xcodes.h */
    PciWriteDword(BUS_0, DEV_1,  FUNC_0, 0x84, 0x00008001);
    PciWriteDword(BUS_0, DEV_1,  FUNC_0, 0x10, 0x00008001);
    PciWriteDword(BUS_0, DEV_1,  FUNC_0, 0x04, 0x00000003);
    IoOutputByte(0x8049, 0x08);
    IoOutputByte(0x80d9, 0x00);
    IoOutputByte(0x8026, 0x01);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x4c, 0x00000001);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x18, 0x00010100);
    PciWriteDword(BUS_0, DEV_0,  FUNC_0, 0x84, 0x07ffffff);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x20, 0x0ff00f00);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x24, 0xf7f0f000);
    PciWriteDword(BUS_1, DEV_0,  FUNC_0, 0x10, 0x0f000000);
    PciWriteDword(BUS_1, DEV_0,  FUNC_0, 0x14, 0xf0000000);
    PciWriteDword(BUS_1, DEV_0,  FUNC_0, 0x04, 0x00000007);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x04, 0x00000007);
#ifndef MCPXREVD5
    writel(0x07633461, 0x0f0010b0);
#else
    writel(0x01000010, 0x0f0010b0);
#endif
    writel(0x66660000, 0x0f0010cc);
    res  = readl(0x0f101000);
    res &= 0x000c0000;
    if (res == 0x00000000) {
        res  = readl(0x0f101000);
        res &= 0xe1f3ffff;
        res |= 0x80000000;
        writel(res, 0x0f101000);
        writel(0xeeee0000, 0x0f0010b8);
    } else if (res == 0x000c0000) {
        res  = readl(0x0f101000);
        res &= 0xe1f3ffff;
        res |= 0x860c0000;
        writel(res, 0x0f101000);
        writel(0xffff0000, 0x0f0010b8);
    } else {
        res  = readl(0x0f101000);
        res &= 0xe1f3ffff;
        res |= 0x820c0000;
        writel(res, 0x0f101000);
        writel(0x11110000, 0x0f0010b8);
    }
    writel(0x00000009, 0x0f0010d4);
    writel(0x00000000, 0x0f0010b4);
    writel(0x00005866, 0x0f0010bc);
    writel(0x0351c858, 0x0f0010c4);
    writel(0x30007d67, 0x0f0010c8);
    writel(0x00000000, 0x0f0010d8);
    writel(0xa0423635, 0x0f0010dc);
    writel(0x0c6558c6, 0x0f0010e8);
    writel(0x03070103, 0x0f100200);
    writel(0x11000016, 0x0f100410);
    writel(0x84848888, 0x0f100330);
    writel(0xffffcfff, 0x0f10032c);
    writel(0x00000001, 0x0f100328);
    writel(0x000000df, 0x0f100338);

    /* initialize SMBus controller */
    PciWriteDword(BUS_0, DEV_1, FUNC_1, 0x04, 0x00000001);
    PciWriteDword(BUS_0, DEV_1, FUNC_1, 0x14, 0x0000c001);
    PciWriteDword(BUS_0, DEV_1, FUNC_1, 0x18, 0x0000c201);
    IoOutputByte(SMBUS+0x200, 0x70);

    /* initialize video encoder */
    /*
     * It is necessary to write to the video encoder, as the PIC
     * snoops the I2C traffic and will reset us if it doesn't see what
     * it judges as 'appropriate' traffic. Conexant is the most urgent,
     * as on v1.0 Xboxes, the PIC was very strict and reset us earlier
     * than later models.
     */
    do {
        /* Conexant video encoder */
        IoOutputByte(SMBUS+4, 0x8a); /* set Conexant address */
        IoOutputByte(SMBUS+8, 0xba);
        IoOutputByte(SMBUS+6, 0x3f);
        IoOutputByte(SMBUS+2, 0x0a);
        do {
            res = IoInputByte(SMBUS);
            if (res == 0x10) {
                IoOutputByte(SMBUS, 0x10);
                IoOutputByte(SMBUS+8, 0x6c);
                IoOutputByte(SMBUS+6, 0x46);
                IoOutputByte(SMBUS+2, 0x0a);
                while (IoInputByte(SMBUS) != 0x10);
                IoOutputByte(SMBUS, 0x10);
                IoOutputByte(SMBUS+8, 0xb8);
                IoOutputByte(SMBUS+6, 0x00);
                IoOutputByte(SMBUS+2, 0x0a);
                while (IoInputByte(SMBUS) != 0x10);
                IoOutputByte(SMBUS, 0x10);
                IoOutputByte(SMBUS+8, 0xce);
                IoOutputByte(SMBUS+6, 0x19);
                IoOutputByte(SMBUS+2, 0x0a);
                while (IoInputByte(SMBUS) != 0x10);
                IoOutputByte(SMBUS, 0x10);
                IoOutputByte(SMBUS+8, 0xc6);
                IoOutputByte(SMBUS+6, 0x9c);
                IoOutputByte(SMBUS+2, 0x0a);
                while (IoInputByte(SMBUS) != 0x10);
                IoOutputByte(SMBUS, 0x10);
                IoOutputByte(SMBUS+8, 0x32);
                IoOutputByte(SMBUS+6, 0x08);
                IoOutputByte(SMBUS+2, 0x0a);
                while (IoInputByte(SMBUS) != 0x10);
                IoOutputByte(SMBUS, 0x10);
                IoOutputByte(SMBUS+8, 0xc4);
                IoOutputByte(SMBUS+6, 0x01);
                IoOutputByte(SMBUS+2, 0x0a);
                while (IoInputByte(SMBUS) != 0x10);
                IoOutputByte(SMBUS, 0x10);
                break;
            }
        } while (res & 0x08);

        if (res == 0x10) break;

        /* Focus video encoder */
        IoOutputByte(SMBUS, 0xff); /* clear any error */
        IoOutputByte(SMBUS, 0x10);
        IoOutputByte(SMBUS+4, 0xd4); /* set Focus address */
        IoOutputByte(SMBUS+8, 0x0c);
        IoOutputByte(SMBUS+6, 0x00);
        IoOutputByte(SMBUS+2, 0x0a);
        do {
            res = IoInputByte(SMBUS);
            if (res == 0x10) {
                IoOutputByte(SMBUS, 0x10);
                IoOutputByte(SMBUS+8, 0x0d);
                IoOutputByte(SMBUS+6, 0x20);
                IoOutputByte(SMBUS+2, 0x0a);
                while (IoInputByte(SMBUS) != 0x10);
                IoOutputByte(SMBUS, 0x10);
                break;
            }
        } while (res & 0x08);

        if (res == 0x10) break;

        /* Xcalibur video encoder */
        /*
         * We don't check to see if these writes fail, as
         * we've already tried Conexant and Focus - Oh dear,
         * not another encoder...  :(
         */
        IoOutputByte(SMBUS, 0xff); /* clear any error */
        IoOutputByte(SMBUS, 0x10);
        IoOutputByte(SMBUS+4, 0xe0); /* set Xcalibur address */
        IoOutputByte(SMBUS+8, 0x00);
        IoOutputByte(SMBUS+6, 0x00);
        IoOutputByte(SMBUS+2, 0x0a);
        while (IoInputByte(SMBUS) != 0x10);
        IoOutputByte(SMBUS, 0x10);
        IoOutputByte(SMBUS+8, 0xb8);
        IoOutputByte(SMBUS+6, 0x00);
        IoOutputByte(SMBUS+2, 0x0a);
        while (IoInputByte(SMBUS) != 0x10);
        IoOutputByte(SMBUS, 0x10);
    } while (0);

    IoOutputByte(SMBUS+4, 0x20); /* set PIC write address */
    IoOutputByte(SMBUS+8, 0x01);
    IoOutputByte(SMBUS+6, 0x00);
    IoOutputByte(SMBUS+2, 0x0a);
    while (IoInputByte(SMBUS) != 0x10);
    IoOutputByte(SMBUS, 0x10);
    IoOutputByte(SMBUS+4, 0x21); /* set PIC read address */
    IoOutputByte(SMBUS+8, 0x01);
    IoOutputByte(SMBUS+2, 0x0a);
    while (IoInputByte(SMBUS) != 0x10);
    IoOutputByte(SMBUS, 0x10);
    res = IoInputByte(SMBUS+6); /* if SMC version does not match ... ????? */
    writel(0x00011c01, 0x0f680500);
    writel(0x000a0400, 0x0f68050c);
    writel(0x00000000, 0x0f001220);
    writel(0x00000000, 0x0f001228);
    writel(0x00000000, 0x0f001264);
    writel(0x00000010, 0x0f001210);
    res  = readl(0x0f101000);
    res &= 0x06000000;
    if (res == 0x00000000) {
        writel(0x48480848, 0x0f001214);
        writel(0x88888888, 0x0f00122c);
    } else {
        writel(0x09090909, 0x0f001214);
        writel(0xaaaaaaaa, 0x0f00122c);
    }
    writel(0xffffffff, 0x0f001230);
    writel(0xaaaaaaaa, 0x0f001234);
    writel(0xaaaaaaaa, 0x0f001238);
    writel(0x8b8b8b8b, 0x0f00123c);
    writel(0xffffffff, 0x0f001240);
    writel(0x8b8b8b8b, 0x0f001244);
    writel(0x8b8b8b8b, 0x0f001248);
    writel(0x00000001, 0x0f1002d4);
    writel(0x00100042, 0x0f1002c4);
    writel(0x00100042, 0x0f1002cc);
    writel(0x00000011, 0x0f1002c0);
    writel(0x00000011, 0x0f1002c8);
    writel(0x00000032, 0x0f1002c0);
    writel(0x00000032, 0x0f1002c8);
    writel(0x00000132, 0x0f1002c0);
    writel(0x00000132, 0x0f1002c8);
    writel(0x00000001, 0x0f1002d0);
    writel(0x00000001, 0x0f1002d0);
    writel(0x80000000, 0x0f100210);
    writel(0xaa8baa8b, 0x0f00124c);
    writel(0x0000aa8b, 0x0f001250);
    writel(0x081205ff, 0x0f100228);
    writel(0x00010000, 0x0f001218);
    res  = PciReadDword(BUS_0, DEV_1, FUNC_0, 0x60);
    res |= 0x00000400;
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x60, res);
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x4c, 0x0000fdde);
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x9c, 0x871cc707);
    res  = PciReadDword(BUS_0, DEV_1, FUNC_0, 0xb4);
    res |= 0x00000f00;
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0xb4, res);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x40, 0xf0f0c0c0);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x44, 0x00c00000);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x5c, 0x04070000);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x6c, 0x00230801);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x6c, 0x01230801);
    writel(0x03070103, 0x0f100200);
    writel(0x11448000, 0x0f100204);
    PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x3c, 0x00000000);
    IoOutputByte(SMBUS, 0x10);

    /* report memory size to PIC scratch register */
    IoOutputByte(SMBUS+4, 0x20); /* set PIC write address */
    IoOutputByte(SMBUS+8, 0x13);
    IoOutputByte(SMBUS+6, 0x0f);
    IoOutputByte(SMBUS+2, 0x0a);
    while (IoInputByte(SMBUS) != 0x10);
    IoOutputByte(SMBUS, 0x10);
    IoOutputByte(SMBUS+8, 0x12);
    IoOutputByte(SMBUS+6, 0xf0);
    IoOutputByte(SMBUS+2, 0x0a);
    while (IoInputByte(SMBUS) != 0x10);
    IoOutputByte(SMBUS, 0x10);

    /* reload NV2A registers */
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x20, 0xfdf0fd00);
    PciWriteDword(BUS_1, DEV_0,  FUNC_0, 0x10, 0xfd000000);

    IoOutputByte(0x0061, 0x08);

    /* enable IDE and NIC */
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x8c, 0x40000000);

    /* CPU whoami ? sesless ? */
    PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x80, 0x00000100);
}

void BootAGPBUSInitialization(void)
{
	u32 temp;
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x54,   PciReadDword(BUS_0, DEV_1, FUNC_0, 0x54) | 0x88000000 );
	
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x64,   (PciReadDword(BUS_0, DEV_0, FUNC_0, 0x64))| 0x88000000 );
	
	temp =  PciReadDword(BUS_0, DEV_0, FUNC_0, 0x6C);
	IoOutputDword(0xcfc , temp & 0xFFFFFFFE);
	IoOutputDword(0xcfc , temp );
	
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x80, 0x00000100);
}

/* -------------------------  Main Entry for after the ASM sequences ------------------------ */

extern void BootStartBiosLoader ( void ) {

	// do not change this, this is linked to many many scipts
	unsigned int PROGRAMM_Memory_2bl 	= 0x00100000;
	unsigned int CROMWELL_Memory_pos 	= 0x03A00000;
	unsigned int CROMWELL_compress_temploc 	= 0x02000000;

	unsigned int Buildinflash_Flash[4]	= { 0xfff00000,0xfff40000,0xfff80000,0xfffc0000};
        unsigned int cromwellidentify	 	=  1;
        unsigned int flashbank		 	=  3;  // Default Bank
        unsigned int cromloadtry		=  0;
        	
	struct SHA1Context context;
      	unsigned char SHA1_result[20];
        
        unsigned char bootloaderChecksum[20];
        unsigned int bootloadersize;
        unsigned int loadretry;
	unsigned int compressed_image_start;
	unsigned int compressed_image_size;
	unsigned int Biossize_type;
	
        int validimage;

	// Perform basic system initialization (formerly from X-codes)
	BootSystemInitialization();

	memcpy(&bootloaderChecksum[0],(void*)PROGRAMM_Memory_2bl,20);
	memcpy(&bootloadersize,(void*)(PROGRAMM_Memory_2bl+20),4);
	memcpy(&compressed_image_start,(void*)(PROGRAMM_Memory_2bl+24),4);
	memcpy(&compressed_image_size,(void*)(PROGRAMM_Memory_2bl+28),4);
	memcpy(&Biossize_type,(void*)(PROGRAMM_Memory_2bl+32),4);
	        
      	SHA1Reset(&context);
	SHA1Input(&context,(void*)(PROGRAMM_Memory_2bl+20),bootloadersize-20);
	SHA1Result(&context,SHA1_result);
	        
        if (memcmp(&bootloaderChecksum[0],&SHA1_result[0],20)==0) {
		// HEHE, the Image we copy'd into ram is SHA-1 hash identical, this is Optimum
		BootPerformPicChallengeResponseAction();
		                                      
	} else {
		// Bad, the checksum does not match, but we can nothing do now, we wait until PIC kills us
		while(1);
	}
       
        // Sets the Graphics Card to 60 MB start address
        (*(unsigned int*)0xFD600800) = (0xf0000000 | ((64*0x100000) - 0x00400000));
        
	BootAGPBUSInitialization();

	(*(unsigned int*)(0xFD000000 + 0x100200)) = 0x03070103 ;
	(*(unsigned int*)(0xFD000000 + 0x100204)) = 0x11448000 ;
        
        PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  // 128 MB
	
	// Lets go, we have finished, the Most important Startup, we have now a valid Micro-loder im Ram
	// we are quite happy now
	
        validimage=0;
        flashbank=3;
	for (loadretry=0;loadretry<100;loadretry++) {
		cromloadtry=0;
		if (Biossize_type==0) {
                	// Means we have a 256 kbyte image
                	 flashbank=3;
                }                                 
               	else if (Biossize_type==1) {
                	// Means we have a 1MB image
                	// If 25 load attempts failed, we switch to the next bank
			switch (loadretry) {
				case 0:
					flashbank=1;
					break;	
				case 25:
     	          			flashbank=2;
      					break;
        	        	case 50:
					flashbank=0;
                			break;
	                	case 75:
        	        		flashbank=3;
                			break;	
			}
                }
		cromloadtry++;	
                
        	// Copy From Flash To RAM
      		memcpy(&bootloaderChecksum[0],(void*)(Buildinflash_Flash[flashbank]+compressed_image_start),20);

                memcpy((void*)CROMWELL_compress_temploc,(void*)(Buildinflash_Flash[flashbank]+compressed_image_start+20),compressed_image_size);
		memset((void*)(CROMWELL_compress_temploc+compressed_image_size),0x00,20*1024);
		
		// Lets Look, if we have got a Valid thing from Flash        	
      		SHA1Reset(&context);
		SHA1Input(&context,(void*)(CROMWELL_compress_temploc),compressed_image_size);
		SHA1Result(&context,SHA1_result);
		
		if (memcmp(&bootloaderChecksum[0],SHA1_result,20)==0) {
			// The Checksum is good                          
			// We start the Cromwell immediatly
                        
			setLED("rrrr");
		
			BufferIN = (unsigned char*)(CROMWELL_compress_temploc);
			BufferINlen=compressed_image_size;
			BufferOUT = (unsigned char*)CROMWELL_Memory_pos;
			decompress_kernel(BufferOUT, BufferIN, BufferINlen);
			
			// This is a config bit in Cromwell, telling the Cromwell, that it is a Cromwell and not a Xromwell
			flashbank++; // As counting starts with 0, we increase +1
			memcpy((void*)(CROMWELL_Memory_pos+0x20),&cromwellidentify,4);
			memcpy((void*)(CROMWELL_Memory_pos+0x24),&cromloadtry,4);
		 	memcpy((void*)(CROMWELL_Memory_pos+0x28),&flashbank,4);
		 	memcpy((void*)(CROMWELL_Memory_pos+0x2C),&Biossize_type,4);
		 	validimage=1;
		 	
		 	break;
		}
	}
        
	if (validimage==1) {
		setLED("oooo");

		// We now jump to the cromwell, Good bye 2bl loader
		// This means: jmp CROMWELL_Memory_pos == 0x03A00000
		__asm __volatile__ (
		"wbinvd\n"
		"cld\n"
		"ljmp $0x10, $0x03A00000\n"   
		);
		// We are not Longer here
	}
	
	// Bad, we did not get a valid im age to RAM, we stop and display a error
	//setLED("rrrr");	

	setLED("oooo");
        
//	I2CTransmitWord(0x10, 0x1901); // no reset on eject
//	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray        

        while(1);
}
