#ifndef __SPIDER_FLASHOBJ_H__
#define __SPIDER_FLASHOBJ_H__

#include "CsTypes.h"
#include "CsDefines.h"

//---- Return codes from flash_status
#define STATUS_READY			0       //---- ready for action
#define STATUS_FLASH_BUSY		1       //---- operation in progress
#define STATUS_ERASE_SUSP		2		//---- erase suspended
#define STATUS_FLASH_TIMEOUT	3       //---- operation timed out
#define STATUS_ERROR			4       //---- unclassified but unhappy status

#define SPDR_INTEL_FLASH_SECTOR_SIZE		0x20000		// 128 k

#define PLXBASEFLASH_MAXSECTORS	64

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

	struct	{
		uInt32 size;				// # of bytes in this sector
		uInt32 base;				// offset from beginning of device
		int32 bank;				// 1 or 2 for DL; 1 for LV
			} sec[PLXBASEFLASH_MAXSECTORS];  // per-sector info

} SpiderFlashInfo;

uInt32  SpiderFlashStatus(PFDO_DATA	FdoData);
//uInt8	FlashReadByte(PFDO_DATA	FdoData, uInt32 Addr);
int32   AddonReadFlash(PFDO_DATA FdoData, uInt32 u32Addr, void *pData, uInt32 u32Size);
int32   AddonWriteFlash(PFDO_DATA FdoData, uInt32 Sector, uInt32 u32Offset, void *pBuffer, uInt32 nBytes);

uInt32	AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr);
void	AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data);
void	AddonResetFlash(PFDO_DATA FdoData);

#endif
