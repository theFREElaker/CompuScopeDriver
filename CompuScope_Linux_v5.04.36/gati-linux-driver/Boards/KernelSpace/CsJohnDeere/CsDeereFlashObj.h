#ifndef __DEERE_FLASHOBJ_H__
#define __DEERE_FLASHOBJ_H__

#include "CsTypes.h"
#include "CsDefines.h"
#include "CsDeereFlash.h"
#include "GageDrv.h"

//----- STRUCTURE
typedef struct _FlashInfo {
	int8 *name;				// "Am29LV033C" for the 1st version of Combine on the base and add-on boards
	uInt32	addr;			// physical address, once translated 
	int32 areg;				// Can be set to zero for all parts 
	int32 nsect;			// # of sectors  
	int32 bank1start;		// first sector # in bank 1
	int32 bank2start;		// first sector # in bank 2
	uInt32	u32FlashSize;
	uInt32	u32SectorSize;

} DeereFlashInfo;

uInt32  DeereFlashStatus(PFDO_DATA	FdoData, uInt32 Addr);
uInt8	FlashReadByte(PFDO_DATA	FdoData, uInt32 Addr);
int32   FlashRead(PFDO_DATA	FdoData, uInt32 u32Addr, void *pData, uInt32 u32Size);
int32	FlashWrite( PFDO_DATA	FdoData, uInt32 Sector, uInt32 Offset, uInt8 *Buf, uInt32 nBytes);


#endif
