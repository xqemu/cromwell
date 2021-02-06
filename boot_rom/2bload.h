/***************************************************************************
      Includes used by XBox boot code
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "consts.h"
#include "stdint.h"
#include "cromwell_types.h"


/////////////////////////////////
// LED-flashing codes
// or these together as argument to I2cSetFrontpanelLed

enum {
	I2C_LED_RED0 = 0x80,
	I2C_LED_RED1 = 0x40,
	I2C_LED_RED2 = 0x20,
	I2C_LED_RED3 = 0x10,
	I2C_LED_GREEN0 = 0x08,
	I2C_LED_GREEN1 = 0x04,
	I2C_LED_GREEN2 = 0x02,
	I2C_LED_GREEN3 = 0x01
};

///////////////////////////////
/* BIOS-wide error codes		all have b31 set  */

enum {
	ERR_SUCCESS = 0,  // completed without error

	ERR_I2C_ERROR_TIMEOUT = 0x80000001,  // I2C action failed because it did not complete in a reasonable time
	ERR_I2C_ERROR_BUS = 0x80000002, // I2C action failed due to non retryable bus error

	ERR_BOOT_PIC_ALG_BROKEN = 0x80000101 // PIC algorithm did not pass its self-test
};

//////// BootPerformPicChallengeResponseAction.c

/* ----------------------------  IO primitives -----------------------------------------------------------
*/

static __inline void IoOutputByte(u16 wAds, u8 bValue) {
    __asm__ __volatile__ ("outb %b0,%w1": :"a" (bValue), "Nd" (wAds));
}

static __inline void IoOutputWord(u16 wAds, u16 wValue) {
    __asm__ __volatile__ ("outw %0,%w1": :"a" (wValue), "Nd" (wAds));
	}

static __inline void IoOutputDword(u16 wAds, u32 dwValue) {
    __asm__ __volatile__ ("outl %0,%w1": :"a" (dwValue), "Nd" (wAds));
}

static __inline u8 IoInputByte(u16 wAds) {
  unsigned char _v;
  __asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

static __inline u16 IoInputWord(u16 wAds) {
  u16 _v;
  __asm__ __volatile__ ("inw %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

static __inline u32 IoInputDword(u16 wAds) {
  u32 _v;
  __asm__ __volatile__ ("inl %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

// boot process
int BootPerformPicChallengeResponseAction(void);
// LED control (see associated enum above)
int I2cSetFrontpanelLed(u8 b);
int I2cResetFrontpanelLed(void);

////////// BootResetActions.c

#define readl(addr) (*(volatile unsigned int *) (addr))
#define writel(b,addr) ((*(volatile unsigned int *) (addr)) = (b))

void BootSystemInitialization(void);
void BootStartBiosLoader(void);

///////// BootPerformPicChallengeResponseAction.c

int I2CTransmitWord(u8 bPicAddressI2cFormat, u16 wDataToWrite);
int I2CTransmitByteGetReturn(u8 bPicAddressI2cFormat, u8 bDataToWrite);

void *memcpy(void *dest, const void *src,  size_t size);
void *memset(void *dest, int data,  size_t size);
int memcmp(const void *pb, const void *pb1, size_t n);

unsigned char *BufferIN;
int BufferINlen;
unsigned char *BufferOUT;
int BufferOUTPos;
