#ifdef _WIN32
#include <ntddk.h>
#include "gagedrv.h"
#endif
#include "CsTypes.h"
#include "CsSpiderDrv.h"
#include "CsSpiderFlashObj.h"


#define INTEL_FLASH_BUSY 0x20

uInt32	SpiderFlashStatus(PFDO_DATA FdoData)
{
	uInt32	u32Status;

	u32Status = ReadGIO( FdoData, CSPDR_ADDON_STATUS );

	if ( ! (u32Status & INTEL_FLASH_BUSY) )
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr)
{
	uInt32 u32Data = 0;

	WriteGIO(FdoData, CSPDR_ADDON_FLASH_ADD, Addr);
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE | CSPDR_ADDON_FOE);

	u32Data = ReadGIO(FdoData, CSPDR_ADDON_FLASH_DATA);
	
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, 0);
	return (u32Data);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data)
{
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_ADD, Addr);
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_DATA, Data);

	WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE | CSPDR_ADDON_FWE);
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE);
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, 0);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	AddonResetFlash(PFDO_DATA FdoData)
{
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_RESET);
	WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, 0);
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int32   AddonReadFlash(PFDO_DATA FdoData, uInt32 u32Addr, void *pData, uInt32 u32Size)
{
	uInt8	*f_ptr = (uInt8 *)(pData);
	uInt32  i;

	AddonResetFlash(FdoData);

	for (i = u32Addr ; i < u32Size + u32Addr ; i++)
	{
		*(f_ptr) = (uInt8) AddonReadFlashByte(FdoData, i);
		f_ptr++;
	}

	return CS_SUCCESS;
}

int32  AddonWriteFlash(PFDO_DATA FdoData, uInt32 u32Sector, uInt32 u32Offset, void *pBuffer, uInt32 nBytes)
{
	uInt8	*pSrc = (uInt8 *) pBuffer;
	uInt32	u32Addr = u32Sector*SPDR_INTEL_FLASH_SECTOR_SIZE;
	uInt32	u32Dst = u32Offset + u32Addr;

	int32	i32GroupLen = 0;
	int32	i32Count = 0; 
	int32 	retry = 1000;	

	while (nBytes > 0)
	{
		i32GroupLen = (nBytes > 0x20) ? 0x20 : nBytes;
		i32Count = i32GroupLen - 1;

		AddonWriteFlashByte(FdoData, 0, 0xE8);

		while ((SpiderFlashStatus(FdoData)) != STATUS_READY)
		{
			retry--;
			if (retry<0) break;
		}

		AddonWriteFlashByte(FdoData, u32Dst, i32Count++);
		
		while(i32Count-- > 0)
		{
			AddonWriteFlashByte(FdoData, u32Dst++, *pSrc++);
		}

		AddonWriteFlashByte(FdoData, u32Dst, 0xD0);

		while ((SpiderFlashStatus(FdoData)) != STATUS_READY)
		{
			retry--;
			if (retry<0) break;
		}

		nBytes -= i32GroupLen;
		retry = 1000;	
	}

	return CS_SUCCESS;
}

	