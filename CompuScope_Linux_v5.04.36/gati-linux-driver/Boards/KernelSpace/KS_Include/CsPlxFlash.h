#ifndef __PLX_FLASH_H__
#define __PLX_FLASH_H__

#include "CsTypes.h"
#include "CsDefines.h"
#include "GageDrv.h"


uInt32  FlashStatus(PFDO_DATA	FdoData, uInt32 Addr);
uInt8	FlashReadByte(PFDO_DATA	FdoData, uInt32 Addr);
int32   PlxFlashRead(PFDO_DATA	FdoData, uInt32 u32Addr, void *pData, uInt32 u32Size);
int32	PlxFlashWrite( PFDO_DATA	FdoData, uInt32 Sector, uInt32 Offset, void *Buf, uInt32 nBytes);


#endif
