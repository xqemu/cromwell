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
 
	//The bytecode interpreter begins here
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000810, 0x00008001);	
	xcode_pciout(0x80000804, 0x00000003);
	xcode_outb(0x00008049, 0x00000008);
	xcode_outb(0x000080d9, 0x00000000);
	xcode_outb(0x00008026, 0x00000001);
	xcode_pciout(0x8000f04c, 0x00000001);
	xcode_pciout(0x8000f018, 0x00010100);
	xcode_pciout(0x80000084, 0x07ffffff);
	
	xcode_pciout(0x8000f020, 0x0ff00f00);
	xcode_pciout(0x8000f024, 0xf7f0f000);
	xcode_pciout(0x80010010, 0x0f000000);
	xcode_pciout(0x80010014, 0xf0000000);
	xcode_pciout(0x80010004, 0x00000007);
	xcode_pciout(0x8000f004, 0x00000007);

#ifndef MCPXREVD5
	xcode_poke(0x0f0010b0, 0x07633461);
#else
	xcode_poke(0x0f0010b0, 0x01000010);
#endif

	xcode_poke(0x0f0010cc, 0x66660000);

	xcode_peek(0x0f101000);
	
	xcode_bittoggle(0x000c0000,0x00000000);

	xcode_ifgoto(0x00000000,6);
	
	xcode_peek(0x0f101000);
	
	xcode_bittoggle(0xe1f3ffff,0x80000000);

	
	xcode_poke_a(0x0f101000);
	xcode_poke(0x0f0010b8, 0xeeee0000);
	xcode_goto(11);
	
	xcode_ifgoto(0x000c0000,6);

	xcode_peek(0x0f101000);
	xcode_bittoggle(0xe1f3ffff,0x860c0000);
	xcode_poke_a(0x0f101000);
	
	xcode_poke(0x0f0010b8, 0xffff0000);
        xcode_goto(5);
	xcode_peek(0x0f101000);
	
	xcode_bittoggle(0xe1f3ffff,0x820c0000);
	xcode_poke_a(0x0f101000);
	xcode_poke(0x0f0010b8, 0x11110000);
	xcode_poke(0x0f0010d4, 0x00000009);
	xcode_poke(0x0f0010b4, 0x00000000);
	xcode_poke(0x0f0010bc, 0x00005866);
	xcode_poke(0x0f0010c4, 0x0351c858);
	xcode_poke(0x0f0010c8, 0x30007d67);
	xcode_poke(0x0f0010d8, 0x00000000);
	xcode_poke(0x0f0010dc, 0xa0423635);
	xcode_poke(0x0f0010e8, 0x0c6558c6);
	xcode_poke(0x0f100200, 0x03070103);
	xcode_poke(0x0f100410, 0x11000016);
	xcode_poke(0x0f100330, 0x84848888);
	xcode_poke(0x0f10032c, 0xffffcfff);
	xcode_poke(0x0f100328, 0x00000001);
	xcode_poke(0x0f100338, 0x000000df);
	
	// Set up the SM - bus controller
	xcode_pciout(0x80000904, 0x00000001);
	xcode_pciout(0x80000914, 0x0000c001);
	xcode_pciout(0x80000918, 0x0000c201);
	xcode_outb(0x0000c200, 0x00000070);
	
	
	//VIDEO INIT CODE
	/* It is necessary to write to the video encoder, as the PIC
	snoops the I2C traffic and will reset us if it doesn't see what 
	it judges as 'appropriate' traffic.  Conexant is the most urgent,
	as on 1.0 Xboxes, the PIC was very strict and reset us earlier
	than later models 
	*/

	//CONEXANT START
	//Set conexant address
	xcode_outb(0x0000c004, 0x0000008a);
	xcode_outb(SMBUS+8, 0x000000ba);
	xcode_outb(SMBUS+6, 0x0000003f);
	xcode_outb(SMBUS+2, 0x0000000a);
				
	xcode_inb(SMBUS);
	xcode_ifgoto(0x00000010,2); //Write failed, forward 2
	xcode_goto(4); //Success, skip to next conexant write.
	xcode_bittoggle(0x00000008, 0x00000000); //Check fail status
	xcode_ifgoto(0x00000000,-4); //No result yet, read again.
	xcode_goto(39);	//Write failed, on to Focus
	xcode_outb(SMBUS, 0x00000010); //Next conexant write.
	//I2CTransmitWord(0x45,0x6c46);
	SMB_xcode_Write(0x6c,0x46);		// +6

	//Are these REALLY needed to keep the PIC happy?
	//I2CTransmitWord(0x45,0xb800);
	SMB_xcode_Write(0xb8,0x00);		// +6
	//I2CTransmitWord(0x45,0xce19);
	SMB_xcode_Write(0xce,0x19);		// +6
	//I2CTransmitWord(0x45,0xc69c);
	SMB_xcode_Write(0xc6,0x9c);		// +6
	//I2CTransmitWord(0x45,0x3208);
	SMB_xcode_Write(0x32,0x08);		// +6
	//I2CTransmitWord(0x45,0xc401);
	SMB_xcode_Write(0xc4,0x01);		// +6

	xcode_goto(36); //Video complete -> VIDEND 


	//FOCUS START
	//Clear the error from the previous attempts
	xcode_outb(SMBUS, 0x000000ff);
	xcode_outb(SMBUS, 0x00000010);
	
	//Set focus address
	xcode_outb(0x0000c004, 0x000000d4);
	//I2CTransmitWord(0x6a,0x0c00);
	xcode_outb(SMBUS+8, 0x0000000c);
	xcode_outb(SMBUS+6, 0x00000000);
	xcode_outb(SMBUS+2, 0x0000000a);
        xcode_inb(SMBUS);
	xcode_ifgoto(0x00000010,2); //Write failed, forward 2
	xcode_goto(4); //Success, skip to next focus write.
	xcode_bittoggle(0x00000008, 0x00000000); //Check fail status
	xcode_ifgoto(0x00000000,-4); //No result yet, read again.
	xcode_goto(9);	//Write failed, on to Xcalibur
	xcode_outb(SMBUS, 0x00000010); //Next focus write.
	//I2CTransmitWord(0x6a,0x0d20);
	SMB_xcode_Write(0x0d,0x20);	 
	xcode_goto(16); //video complete -> VIDEND

	//XCALIBUR START	
	/* We don't check to see if these writes fail, as
	we've already tried Conexant and Focus - Oh dear,
	not another encoder...  :(    
	*/

	//Clear the error from the previous attempts
	xcode_outb(SMBUS, 0x000000ff);
	xcode_outb(SMBUS, 0x00000010);
	
	xcode_outb(0x0000c004, 0x000000E0);
	//I2CTransmitWord(0x70,0x00);
	SMB_xcode_Write(0x0,0x0);		// +6
	//I2CTransmitWord(0x70,0x00);
	SMB_xcode_Write(0xb8,0x00);		// +6	

	
	//VIDEND - Encoder 'init' complete.

        // PIC SLAVE Address (Write)
	xcode_outb(SMBUS+4, 0x00000020);
        // I2Ctransmit(0x20,0x1,0x0);
	SMB_xcode_Write(0x01,0x00);		// +6

	// PIC SLAVE Address (Read)
	xcode_outb(SMBUS+4, 0x00000021);

	// I2Cgetbyte(0x8a,0x1);
	xcode_outb(SMBUS+8, 0x00000001);
	xcode_outb(SMBUS+2, 0x0000000a);
        xcode_inb(SMBUS);
        xcode_ifgoto(0x00000010,-1)
        xcode_outb(SMBUS, 0x00000010);


	// If SMC version does not match ... ?????
	xcode_inb(SMBUS+6);
	
	xcode_poke(0x0f680500, 0x00011c01);
	xcode_poke(0x0f68050c, 0x000a0400);
	xcode_poke(0x0f001220, 0x00000000);
	xcode_poke(0x0f001228, 0x00000000);
	xcode_poke(0x0f001264, 0x00000000);
	xcode_poke(0x0f001210, 0x00000010);
	xcode_peek(0x0f101000);
	xcode_bittoggle(0x06000000,0x00000000);
	xcode_ifgoto(0x00000000,4);
	xcode_poke(0x0f001214, 0x48480848);
	xcode_poke(0x0f00122c, 0x88888888);
	xcode_goto(7);
	xcode_ifgoto(0x06000000,4);
	xcode_poke(0x0f001214, 0x09090909);
	xcode_poke(0x0f00122c, 0xaaaaaaaa);
	xcode_goto(3);
     
	xcode_poke(0x0f001214, 0x09090909);
	xcode_poke(0x0f00122c, 0xaaaaaaaa);
	xcode_poke(0x0f001230, 0xffffffff);
	xcode_poke(0x0f001234, 0xaaaaaaaa);
	xcode_poke(0x0f001238, 0xaaaaaaaa);
	xcode_poke(0x0f00123c, 0x8b8b8b8b);
	xcode_poke(0x0f001240, 0xffffffff);
	xcode_poke(0x0f001244, 0x8b8b8b8b);
	xcode_poke(0x0f001248, 0x8b8b8b8b);
	xcode_poke(0x0f1002d4, 0x00000001);
	xcode_poke(0x0f1002c4, 0x00100042);
	xcode_poke(0x0f1002cc, 0x00100042);
	xcode_poke(0x0f1002c0, 0x00000011);
	xcode_poke(0x0f1002c8, 0x00000011);
	xcode_poke(0x0f1002c0, 0x00000032);
	xcode_poke(0x0f1002c8, 0x00000032);
	xcode_poke(0x0f1002c0, 0x00000132);
	xcode_poke(0x0f1002c8, 0x00000132);
	xcode_poke(0x0f1002d0, 0x00000001);
	xcode_poke(0x0f1002d0, 0x00000001);
	xcode_poke(0x0f100210, 0x80000000);
	xcode_poke(0x0f00124c, 0xaa8baa8b);
	xcode_poke(0x0f001250, 0x0000aa8b);
	xcode_poke(0x0f100228, 0x081205ff);

	xcode_poke(0x0f001218, 0x00010000);


	xcode_pciin_a(0x80000860);
	xcode_bittoggle(0xffffffff,0x00000400);   
	xcode_pciout_a(0x80000860);

	xcode_pciout(0x8000084c, 0x0000fdde);
	xcode_pciout(0x8000089c, 0x871cc707);
	xcode_pciin_a(0x800008b4);
	xcode_bittoggle(0xfffff0ff,0x00000f00);
	xcode_pciout_a(0x800008b4);
	xcode_pciout(0x80000340, 0xf0f0c0c0);
	xcode_pciout(0x80000344, 0x00c00000);
	xcode_pciout(0x8000035c, 0x04070000);
	xcode_pciout(0x8000036c, 0x00230801);
	xcode_pciout(0x8000036c, 0x01230801);
	xcode_goto(1);
	xcode_goto(1);
	xcode_poke(0x0f100200, 0x03070103);
	xcode_poke(0x0f100204, 0x11448000);
	xcode_pciout(0x8000103c, 0x00000000);


	xcode_outb(SMBUS, 0x00000010);
	
	/* ----  Report Memory Size to PIC scratch register ---- */
        
        // We emulate Good memory result to PIC

	//xcode_pciin_a(0x8000183c);
	//xcode_bittoggle(0x000000ff,0x00000000);
	//xcode_outb_a(SMBUS+6);


	xcode_outb(SMBUS+4, 0x00000020);

	SMB_xcode_Write(0x13,0x0f);		// +6
	SMB_xcode_Write(0x12,0xf0);		// +6		

	/* ---- Reload Nvidia Registers  ------------------------*/
	
	xcode_pciout(0x8000f020, 0xfdf0fd00);
	xcode_pciout(0x80010010, 0xfd000000);
	
	
	// overflow trick (orig)
	// jmp    0x8:0xfffc1000
	//xcode_poke(0x00000000, 0xfc1000ea);
	//xcode_poke(0x00000004, 0x000008ff);
	
	// overflow trick (xemu-friendly method)
	// mov eax, 0xfffc1000
	// jmp eax
	// nop
	xcode_poke(0x00000000, 0xfc1000B8);
	xcode_poke(0x00000004, 0x90e0ffff);
	

       	xcode_END(0x806);
