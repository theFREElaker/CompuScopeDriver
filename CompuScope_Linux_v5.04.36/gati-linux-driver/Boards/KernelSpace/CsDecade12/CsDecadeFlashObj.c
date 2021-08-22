#include "CsTypes.h"
#include "DecadeDrv.h"
#include "DecadeGageDrv.h"
#include "DecadeFlashObj.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr)
{
	uInt32 u32Data = 0;

	WriteGIO(FdoData, CSRB_ADDON_FLASH_ADD, Addr);
	WriteGIO(FdoData, CSRB_ADDON_FLASH_CMD, CSRB_ADDON_FCE | CSRB_ADDON_FOE);

	u32Data = ReadGIO(FdoData, CSRB_ADDON_FLASH_DATA);

	WriteGIO(FdoData, CSRB_ADDON_FLASH_CMD, 0);
	return (u32Data);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data)
{
	WriteGIO(FdoData, CSRB_ADDON_FLASH_ADD, Addr);
	WriteGIO(FdoData, CSRB_ADDON_FLASH_DATA, Data);

	WriteGIO(FdoData, CSRB_ADDON_FLASH_CMD, CSRB_ADDON_FCE | CSRB_ADDON_FWE);
	WriteGIO(FdoData, CSRB_ADDON_FLASH_CMD, 0);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	AddonResetFlash(PFDO_DATA FdoData)
{
	WriteGIO(FdoData, CSRB_ADDON_FLASH_CMD, CSRB_ADDON_RESET);
	WriteGIO(FdoData, CSRB_ADDON_FLASH_CMD, 0);
}


#define INTEL_FLASH_BUSY 0x20

uInt32	RabbitFlashStatus(PFDO_DATA FdoData)
{
	uInt32	u32Status;

	u32Status = ReadGIO( FdoData, CSRB_ADDON_STATUS );

	if ( ! (u32Status & INTEL_FLASH_BUSY) )
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  AddonWriteFlash(PFDO_DATA FdoData, uInt32 Sector, uInt32 u32Offset, void *pBuffer, uInt32 nBytes)
{
	uInt8	*pSrc = (uInt8 *) pBuffer;
	uInt32	u32Dst = u32Offset;
	uInt32	u32Addr = Sector*RB_INTEL_FLASH_SECTOR_SIZE;
	int32	i32Status = 0;

	while (nBytes > 0)
	{
		AddonWriteFlashByte( FdoData, u32Addr, 0x40 );
		AddonWriteFlashByte( FdoData, u32Addr + u32Dst, *pSrc );

		while ((i32Status = RabbitFlashStatus(FdoData)) == STATUS_FLASH_BUSY);

		if (i32Status != STATUS_READY)
			//---- try again
		{
		}
		else
		{
			nBytes--;
			u32Dst++;
			pSrc++;
		}
	}
	return CS_SUCCESS;
}

