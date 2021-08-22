#include "StdAfx.h"
#include "CsDeere.h"
#include "CsDeereFlashObj.h"


#define		CSBRANIAC_FLASH_TIMEOUT		 KTIME_SECOND ( 4 ) 

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DeereFlashInfo CsDeereFlash::IntelFlashDesc[] =
{
	//  StraataFlash J3 
	(int8*)"28F640J3", 0L, 0, 64, 1, 0, 0x1000000, 0x20000
};



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsDeereFlash::CsDeereFlash( CsPlxBase *pPlxBase )
{
	m_PlxBase = pPlxBase;
	FlashPtr = IntelFlashDesc;
}


//-----------------------------------------------------------------------------
// Function:  FlashCommand
// Description:  It performs every possible command available to AMD B revision flash parts.
// Note: for the Am29LV033C the address of the command "don't care", but it could mind for other type
// So if one day we add another type of AMD flash we may have to set the Address for the command, 
// but it will not affect the Am29LV033C
//-----------------------------------------------------------------------------
// IntelFlash

void  CsDeereFlash::FlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data)
{
	uInt16 Retry;
	uInt16 retrycount[FLASH_LASTCMD + 1] = {
							0,				// FLASH_SELECT    0				
							0,				// FLASH_RESET, FLASH_READ     1	
							0,				// FLASH_AUTOSEL   2				
							0,				// FLASH_PROG      3				
							COMMON_RETRY,	// FLASH_CERASE    4				
							COMMON_RETRY,	// FLASH_SERASE    5				
							0,				// FLASH_ESUSPEND  6				
							COMMON_RETRY,	// FLASH_ERESUME   7				
							COMMON_RETRY,	// FLASH_UB        8				
							COMMON_RETRY,	// FLASH_UBPROG    9				
							COMMON_RETRY,	// FLASH_UBRESET   10				
							};													

	Retry = retrycount[Command];
	uInt32	u32SectorAddr = Sector * FlashPtr->u32SectorSize;

	do
	{
		if (Command == FLASH_SELECT)
	 		return;
		else if (Command == FLASH_RESET || Command > FLASH_LASTCMD)
			ResetFlash();									//intel flash hard reset only
		else
		{
			switch (Command)
			{
				case FLASH_AUTOSEL:
					WriteFlashByte(u32SectorAddr, 0x90);
					break;
				case FLASH_PROG:
					WriteFlashByte(u32SectorAddr + Offset, 0x40);
					WriteFlashByte(u32SectorAddr + Offset, Data);
					break;
				case FLASH_SECTOR_ERASE:
					WriteFlashByte(u32SectorAddr, 0x20);
					WriteFlashByte(u32SectorAddr, 0xD0);
					break;
			}
		}

	 } while ((Retry-- > 0) && (FlashStatus(0) != STATUS_READY));
	 
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

uInt32  CsDeereFlash::FlashStatus(uInt32 Addr)
{
	uInt32	u32Status;

	UNREFERENCED_PARAMETER(Addr);
	u32Status = m_PlxBase->ReadFlashRegister(STATUS_REG_PLX);
	if ( ! (u32Status & INTEL_FLASH_BUSY) )
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
}

//-----------------------------------------------------------------------------
// Function   :  FlashSectorErase
// Description:  Erase the entire Chip
//-----------------------------------------------------------------------------
// IntelFlash

void  CsDeereFlash::FlashSectorErase( uInt32 Sector )
{
	uInt32		u32SectorAddr = Sector * FlashPtr->u32SectorSize;
	CSTIMER		CsTimeout;

	FlashCommand(FLASH_SECTOR_ERASE, Sector,0,0);

	CsTimeout.Set( CSBRANIAC_FLASH_TIMEOUT );
	while ((FlashStatus( u32SectorAddr )) == STATUS_FLASH_BUSY)
	{
		if ( CsTimeout.State() )
		{
			break;
		}
	}
}

//---------------
// Function   : ResetFlash
// Description:	Hard reset the flash without giving up host control  
//---------------
uInt32 CsDeereFlash::ResetFlash(void)
{
	uInt32	Status = 0;

	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD,  SEND_HOST_CMD);
	// Toggle reset bit
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD,  SEND_HOST_CMD | RESET_INT);
	// m_PlxBase->BBtiming(10);	// at least 500 ns as required
	Sleep(1);
	Status = m_PlxBase->ReadFlashRegister(STATUS_REG_PLX);				// Dummy read delay between 2 writes
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD,  SEND_HOST_CMD);	
	Status = m_PlxBase->ReadFlashRegister(STATUS_REG_PLX);				// Dummy read delay

	return CS_SUCCESS;
}


void CsDeereFlash::WriteFlashByte( uInt32 Addr, uInt32 Data )
{
	//---- write host control bit
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD, SEND_HOST_CMD);

	//---- write flash location address[7..0]
	m_PlxBase->WriteFlashRegister(FLASH_ADD_REG_1, Addr & 0xFF);
	//---- write flash location address[15..8]
	m_PlxBase->WriteFlashRegister(FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[21..16]
	m_PlxBase->WriteFlashRegister(FLASH_ADD_REG_3, (Addr >> 16) & 0xFF);

	//---- Write Data register
	m_PlxBase->WriteFlashRegister(FLASH_DATA_REGISTER, (uInt8)(Data));

	//---- send the write command to the PLD  ---  need a rising edge on FWE_REG
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD,SEND_WRITE_CMD);

	int32 Status = m_PlxBase->ReadFlashRegister(STATUS_REG_PLX);
	Status = m_PlxBase->ReadFlashRegister(STATUS_REG_PLX);
	Status = m_PlxBase->ReadFlashRegister(STATUS_REG_PLX);

	//---- write host control bit
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD, SEND_HOST_CMD);

}


//---------------
// Function   : FlashReadByte 
// Description:  
// Flash read custom implementation						
// Flash access is 32 bits aligned						
// the LSByte is data, upper bytes don't care			    
//---------------

uInt8 CsDeereFlash::FlashReadByte(uInt32 Addr)
{
	uInt32 u32Data;

	//---- write host control bit
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD, HOST_CTRL);

	//---- write flash location address[7..0]
	m_PlxBase->WriteFlashRegister(FLASH_ADD_REG_1,Addr & 0xFF);
	//---- write flash location address[15..8]
	m_PlxBase->WriteFlashRegister(FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[24..16]
	m_PlxBase->WriteFlashRegister(FLASH_ADD_REG_3, (Addr >> 16) & 0xFF);

	//---- send the read command to the PLD
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD, HOST_CTRL|FCE_HOST|FOE_HOST);

	u32Data = (uInt8)((m_PlxBase->ReadFlashRegister(FLASH_DATA_REGISTER))& 0xFF);

	//---- write host control bit
	m_PlxBase->WriteFlashRegister(COMMAND_REG_PLD, HOST_CTRL);

	return ((uInt8)(u32Data));
}

//-----------------------------------------------------------------------------
// Function   : WriteEepromEx
// Description: 
//-----------------------------------------------------------------------------

int32   CsDeereFlash::FlashWriteEx(uInt32 u32StartAddr, void *pData, uInt32 u32Size, bool bBackupContent )
{
	uInt8	*pu8Data = (uInt8 *) pData;
	uInt32	i;
	uInt32  FirstSector, LastSector;
	uInt8	*pBackupMemory = NULL;		// Hold the contents of eeprom sectors
	uInt32	u32EndAddr = u32StartAddr + u32Size;
	uInt32	u32BackupSizeFisrt = 0;
	uInt32	u32BackupSizeLast = 0;
	uInt32	u32StartOffset = 0;
	uInt32	u32EndOffset = 0;
	uInt8	*pFirstSectorBackup = 0;		// Point to the backup memory of the first sector
	uInt8	*pLastSectorBackup = 0;			// Point to the backup memory of the lastt sector
	int32	i32DataRemain = (int32) u32Size;


	// Offset relative to beginning of sector
	u32StartOffset	= u32StartAddr % FlashPtr->u32SectorSize;
	u32EndOffset	= (u32EndAddr % FlashPtr->u32SectorSize);

	// Detecttion if we need to backup part of the fisrt and last sector
	if ( bBackupContent )
	{
		u32BackupSizeFisrt	= u32StartOffset;
		if ( u32EndOffset )
			u32BackupSizeLast = FlashPtr->u32SectorSize - u32EndOffset;
	}

	// Allocate memory for backup the contents of first and last sectors
	if ( u32BackupSizeFisrt > 0 || u32BackupSizeLast > 0 )
	{
		pBackupMemory = (uInt8 * ) ::GageAllocateMemoryX( u32BackupSizeFisrt + u32BackupSizeLast );
		if ( NULL == pBackupMemory )
			return CS_INSUFFICIENT_RESOURCES;

		pFirstSectorBackup = pBackupMemory;
		pLastSectorBackup = pFirstSectorBackup + u32BackupSizeFisrt;
	}

	// Backup the contents of the sector before StartAddr
	if ( u32BackupSizeFisrt > 0 )
		m_PlxBase->IoctlReadFlash( u32StartAddr - (u32StartAddr % FlashPtr->u32SectorSize), pFirstSectorBackup, u32BackupSizeFisrt );

	// Backup the contents of the sector after the EndAddr ( StartAddr + Size )
	if ( u32BackupSizeLast > 0 )
		m_PlxBase->IoctlReadFlash( u32EndAddr, pLastSectorBackup, u32BackupSizeLast );

	// Erase the sectors 
	FirstSector = u32StartAddr / FlashPtr->u32SectorSize;
	LastSector = (u32EndAddr) / FlashPtr->u32SectorSize;

	ResetFlash();

	// Write the first sector
	FlashSectorErase(FirstSector);

	// Write the first backup contents
	if ( u32BackupSizeFisrt > 0 )
		m_PlxBase->IoctlWriteFlash( FirstSector, 0, pFirstSectorBackup, u32BackupSizeFisrt ); 

	if ( FirstSector == LastSector )
	{
		// The data is smaller than the sector size
		m_PlxBase->IoctlWriteFlash( FirstSector, u32StartOffset, pu8Data, u32Size ); 
		i32DataRemain -= u32Size;
	}
	else if ( u32StartOffset > 0 )
	{
		m_PlxBase->IoctlWriteFlash( FirstSector, u32StartOffset, pu8Data, (FlashPtr->u32SectorSize - u32StartOffset) ); 
		pu8Data += (FlashPtr->u32SectorSize - u32StartOffset);
		i32DataRemain -= (FlashPtr->u32SectorSize - u32StartOffset);
		FirstSector += 1;
	}

	// Write the middles sector if thre is any
	for (i = FirstSector; i < LastSector ; i++)
	{
		FlashSectorErase(i);
		m_PlxBase->IoctlWriteFlash( i, 0, pu8Data, FlashPtr->u32SectorSize ); 
		pu8Data += FlashPtr->u32SectorSize;
		i32DataRemain -= FlashPtr->u32SectorSize;
	}

	// Write the last sector
	if ( u32EndOffset > 0 && ( FirstSector != LastSector ) )
	{
		FlashSectorErase(LastSector);
		m_PlxBase->IoctlWriteFlash( LastSector, 0, pu8Data, u32EndOffset ); 
		i32DataRemain -= u32EndOffset;
	}

	// Write the last backup contents
	if ( u32BackupSizeLast > 0 )
		m_PlxBase->IoctlWriteFlash( LastSector, u32EndOffset, pLastSectorBackup, u32BackupSizeLast ); 

	ASSERT( 0 == i32DataRemain );

	if ( NULL != pBackupMemory )
		::GageFreeMemoryX( pBackupMemory );

	ResetFlash();
	m_PlxBase->BaseBoardConfigReset(0);

	return CS_SUCCESS;
}

