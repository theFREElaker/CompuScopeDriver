#ifdef _WIN32
#include "gagedrv.h"
#include "CsTypes.h"
#endif
#include "CsPrivateStruct.h"
#include "CsDeereFlashObj.h"
#include "CsPlxDefines.h"
#include "CsTimer.h"
#include "NiosApi.h"
#include "PlxSupport.h"

#define		CSDEERE_FLASH_TIMEOUT		 KTIME_SECOND ( 4 ) 

//---- Retry count for command, could be specific for each command
#define COMMON_RETRY	15//----- CONSTANTS

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DeereFlashInfo IntelFlashDesc[] =
{
	//  StraataFlash J3 
	{ "28F640J3", 0L, 0, 64, 1, 0, 0x1000000, 0x20000 }
};


//-----------------------------------------------------------------------------
// Function   :   FlashWrite
// Description:  
//	Faster way to program multiple data words, without  
//	needing the function overhead of looping algorithms which         
//	program word by word.                               
//-----------------------------------------------------------------------------
// IntelFlash

int32  FlashWrite(PFDO_DATA FdoData, uInt32 Sector, uInt32 Offset, uInt8 *Buf, uInt32 nBytes)
{								
	uInt8	*Src;
	uInt32	Dst;
	uInt32	Status;
	DeereFlashInfo	*FlashPtr = IntelFlashDesc;
	uInt32	u32SectorAddr = Sector * FlashPtr->u32SectorSize;
	CSTIMER		CsTimeout;


	Dst = Offset;
	Src = Buf;

	CsSetTimer(&CsTimeout, CSDEERE_FLASH_TIMEOUT);
	while ((Status = DeereFlashStatus( FdoData, u32SectorAddr )) == STATUS_FLASH_BUSY)
	{
		if ( CsStateTimer(&CsTimeout) )
			break;
	}

	if (Status != STATUS_READY) 
		return CS_FLASH_TIMEOUT;

	while (nBytes > 0)
	{
		WriteFlashByte( FdoData, u32SectorAddr, 0x40 ); 
		WriteFlashByte( FdoData, u32SectorAddr + Dst, *Src ); 

		while ((Status = DeereFlashStatus(FdoData, u32SectorAddr)) == STATUS_FLASH_BUSY);

		if (Status != STATUS_READY)
			//---- try again
		{
		}
		else
		{
			nBytes--;
			Dst++;
			Src++;
		}
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Function   : FlashStatus(Addr) 
// Description:  utilizes the DQ6, DQ5, and DQ3 polling algorithms    
// described in the flash data book.  It can quickly ascertain the   
// operational status of the flash device, and return an             
// appropriate status code
//-----------------------------------------------------------------------------
// IntelFlash

#define INTEL_FLASH_BUSY 0x20

uInt32  DeereFlashStatus(PFDO_DATA FdoData, uInt32 Addr)
{
	uInt32	u32Status;

	UNREFERENCED_PARAMETER(Addr);
	u32Status = ReadFlashRegister(FdoData, STATUS_REG_PLX);
	if ( ! (u32Status & INTEL_FLASH_BUSY) )
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
}

//---------------
// Function   : FlashReadByte 
// Description:  
// Flash read custom implementation						
// Flash access is 32 bits aligned						
// the LSByte is data, upper bytes don't care			    
//---------------

uInt8 FlashReadByte(PFDO_DATA FdoData, uInt32 Addr)
{
	uInt32 u32Data;

	//---- write host control bit
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, HOST_CTRL);

	//---- write flash location address[7..0]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_1,Addr & 0xFF);
	//---- write flash location address[15..8]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[24..16]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_3, (Addr >> 16) & 0xFF);

	//---- send the read command to the PLD
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, HOST_CTRL|FCE_HOST|FOE_HOST);

	u32Data = (uInt8)((ReadFlashRegister(FdoData, FLASH_DATA_REGISTER))& 0xFF);

	//---- write host control bit
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, HOST_CTRL);

	return ((uInt8)(u32Data));
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int32   FlashRead(PFDO_DATA FdoData, uInt32 u32Addr, void *pData, uInt32 u32Size)
{
	int32	i32Status = CS_SUCCESS;
	uInt8	*ptr = (uInt8 *)(pData);
	uInt32  i = 0;

	if ( u32Addr >= DEERE_FLASH_SIZE )
		return CS_FAIL;

	if ( (u32Addr + u32Size) >= DEERE_FLASH_SIZE )
		u32Size = DEERE_FLASH_SIZE - u32Addr - 1;


	for (i = u32Addr ; i < u32Size + u32Addr  ; i++)
	{
		*ptr = FlashReadByte( FdoData, i );
		ptr++;
	}

	return i32Status;
}

