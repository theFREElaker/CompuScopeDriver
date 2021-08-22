#ifndef __RABBIT_FLASHOBJ_H__
#define __RABBIT_FLASHOBJ_H__

#include "CsTypes.h"
#include "CsDefines.h"

//---- Return codes from flash_status
#define STATUS_READY			0       //---- ready for action
#define STATUS_FLASH_BUSY		1       //---- operation in progress
#define STATUS_ERASE_SUSP		2		//---- erase suspended
#define STATUS_FLASH_TIMEOUT	3       //---- operation timed out
#define STATUS_ERROR			4       //---- unclassified but unhappy status

#define RB_INTEL_FLASH_SECTOR_SIZE		0x20000		// 128 k

uInt32  RabbitFlashStatus(PFDO_DATA	FdoData);
int32   AddonReadFlash(PFDO_DATA FdoData, uInt32 u32Addr, void *pData, uInt32 u32Size);
int32   AddonWriteFlash(PFDO_DATA FdoData, uInt32 Sector, uInt32 u32Offset, void *pBuffer, uInt32 nBytes);

uInt32	AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr);
void	AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data);
void	AddonResetFlash(PFDO_DATA FdoData);

#endif
