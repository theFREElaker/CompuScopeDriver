
#include "StdAfx.h"
#include "CsSpider.h"
#include "CsSpiderFlash.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

FlashInfo IntelFlashDesc[] =
{
	//  StraataFlash J3
	{(int8*)"28F640J3", 0L, 0, 64, 1, 0 , 0x800000, 0x20000,
		{
		{0x20000,     0x000000,    1},
		{0x20000,     0x020000,    1},
		{0x20000,     0x040000,    1},
		{0x20000,     0x060000,    1},
		{0x20000,     0x080000,    1},
		{0x20000,     0x0A0000,    1},
		{0x20000,     0x0C0000,    1},
		{0x20000,     0x0E0000,    1},

		{0x20000,     0x100000,    1},
		{0x20000,     0x120000,    1},
		{0x20000,     0x140000,    1},
		{0x20000,     0x160000,    1},
		{0x20000,     0x180000,    1},
		{0x20000,     0x1A0000,    1},
		{0x20000,     0x1C0000,    1},
		{0x20000,     0x1E0000,    1},

		{0x20000,     0x200000,    1},
		{0x20000,     0x220000,    1},
		{0x20000,     0x240000,    1},
		{0x20000,     0x260000,    1},
		{0x20000,     0x280000,    1},
		{0x20000,     0x2A0000,    1},
		{0x20000,     0x2C0000,    1},
		{0x20000,     0x2E0000,    1},

		{0x20000,     0x300000,    1},
		{0x20000,     0x320000,    1},
		{0x20000,     0x340000,    1},
		{0x20000,     0x360000,    1},
		{0x20000,     0x380000,    1},
		{0x20000,     0x3A0000,    1},
		{0x20000,     0x3C0000,    1},
		{0x20000,     0x3E0000,    1},

		{0x20000,     0x400000,    1},
		{0x20000,     0x420000,    1},
		{0x20000,     0x440000,    1},
		{0x20000,     0x460000,    1},
		{0x20000,     0x480000,    1},
		{0x20000,     0x4A0000,    1},
		{0x20000,     0x4C0000,    1},
		{0x20000,     0x4E0000,    1},

		{0x20000,     0x500000,    1},
		{0x20000,     0x520000,    1},
		{0x20000,     0x540000,    1},
		{0x20000,     0x560000,    1},
		{0x20000,     0x580000,    1},
		{0x20000,     0x5A0000,    1},
		{0x20000,     0x5C0000,    1},
		{0x20000,     0x5E0000,    1},

		{0x20000,     0x600000,    1},
		{0x20000,     0x620000,    1},
		{0x20000,     0x640000,    1},
		{0x20000,     0x660000,    1},
		{0x20000,     0x680000,    1},
		{0x20000,     0x6A0000,    1},
		{0x20000,     0x6C0000,    1},
		{0x20000,     0x6E0000,    1},

		{0x20000,     0x700000,    1},
		{0x20000,     0x720000,    1},
		{0x20000,     0x740000,    1},
		{0x20000,     0x760000,    1},
		{0x20000,     0x780000,    1},
		{0x20000,     0x7A0000,    1},
		{0x20000,     0x7C0000,    1},
		{0x20000,     0x7E0000,    1}
		}
	}
};

//-----------------------------------------------------------------------------
// Function   :  init_flash
// Description:  initializes setting of parameters and the appropriate sector table
//-----------------------------------------------------------------------------
// IntelFlash
void  CsSpiderDev:: SpiderInitFlash(uInt32 FlashIndex)
{
	FlashPtr = &IntelFlashDesc[FlashIndex];
}

//-----------------------------------------------------------------------------
// Function:  FlashCommand
// Description:  It performs every possible command available to AMD B revision flash parts.
// Note: for the Am29LV033C the address of the command "don't care", but it could mind for other type
// So if one day we add another type of AMD flash we may have to set the Address for the command,
// but it will not affect the Am29LV033C
//-----------------------------------------------------------------------------
// IntelFlash

void  CsSpiderDev::SpiderFlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data)
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

	do
	{
		if (Command == FLASH_SELECT)
	 		return;
		else if (Command == FLASH_RESET || Command > FLASH_LASTCMD)
			AddonResetFlash();									//intel flash hard reset only
		else
		{
			switch (Command)
			{
				case FLASH_AUTOSEL:
					AddonWriteFlashByte(FlashPtr->sec[Sector].base, 0x90);
					break;
				case FLASH_PROG:
					AddonWriteFlashByte(FlashPtr->sec[Sector].base + Offset, 0x40);
					AddonWriteFlashByte(FlashPtr->sec[Sector].base + Offset, Data);
					break;
				case FLASH_SECTOR_ERASE:
					AddonWriteFlashByte(FlashPtr->sec[Sector].base, 0x20);
					AddonWriteFlashByte(FlashPtr->sec[Sector].base, 0xD0);
					break;
			}
		}

	 } while ((Retry-- > 0) && (SpiderFlashStatus(0) != STATUS_READY));

}

//-----------------------------------------------------------------------------
// Function   :   FlashWrite
// Description:
//	Faster way to program multiple data words, without
//	needing the function overhead of looping algorithms which
//	program word by word.
//-----------------------------------------------------------------------------
// IntelFlash

int32  CsSpiderDev::SpiderFlashWrite( uInt32 Sector, uInt32 Offset, uInt8 *Buf, uInt32 nBytes)
{
	uInt32 Status;
	CSTIMER		CsTimer;

	CsTimer.Set(CS_WAIT_TIMEOUT);
	while ((Status = SpiderFlashStatus( FlashPtr->sec[Sector].base)) == STATUS_FLASH_BUSY)
	{
		if ( CsTimer.State() )
			break;
	}

	if (Status != STATUS_READY)
		return CS_FLASHSTATE_ERROR;

	return DoAddonWriteFlash( Sector, Offset, Buf, nBytes );
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

uInt32  CsSpiderDev::SpiderFlashStatus(uInt32 Addr)
{
	uInt32	u32Status;

	UNREFERENCED_PARAMETER(Addr);

	u32Status = ReadGIO( CSPDR_ADDON_STATUS );

	if ( ! (u32Status & INTEL_FLASH_BUSY) )
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
}
//-----------------------------------------------------------------------------
// Function   :  FlashReset
// Description:  Send the command to reset the Flash - Ready to read
//-----------------------------------------------------------------------------
void  CsSpiderDev::SpiderFlashReset()
{
	SpiderFlashCommand( FLASH_RESET,0,0,0 );
}

//-----------------------------------------------------------------------------
// Function   :  FlashWriteByte
// Description:  Send the command to write One byte
//-----------------------------------------------------------------------------
void  CsSpiderDev::SpiderFlashWriteByte(  uInt32 Addr, uInt32 Data )
{
	uInt32 Sector, Offset;

	Sector = Addr / FlashPtr->sec[0].size; //---- True since all the sectors are the same size
	Offset = Addr % FlashPtr->sec[0].size;

	SpiderFlashCommand(FLASH_PROG, Sector, Offset, Data);
}



#define MAXSECTORS  64      // maximum number of sectors supported

//-----------------------------------------------------------------------------
// Function   :  FlashChipErase
// Description:  Erase the entire Chip
//-----------------------------------------------------------------------------
void  CsSpiderDev::SpiderFlashChipErase()
{
	uInt8		u8Sector = 0;

	while(u8Sector < MAXSECTORS)
		SpiderFlashSectorErase( u8Sector++ );

}

//-----------------------------------------------------------------------------
// Function   :  FlashSectorErase
// Description:  Erase the entire Chip
//-----------------------------------------------------------------------------
// IntelFlash

void  CsSpiderDev::SpiderFlashSectorErase( uInt32 Sector)
{
	CSTIMER		CsTimer;
	SpiderFlashCommand(FLASH_SECTOR_ERASE, Sector,0,0);

	CsTimer.Set(CS_ERASE_WAIT_TIMEOUT);
	while ((SpiderFlashStatus( FlashPtr->sec[Sector].base)) == STATUS_FLASH_BUSY)
	{
		if ( CsTimer.State() )
		{
			m_bFlashReset = TRUE;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Function   : WriteEntireEeprom
// Description: Erase the Add On Flash, then Write the complete Flash
//-----------------------------------------------------------------------------
int32  CsSpiderDev::WriteEntireEeprom(AddOnFlash32Mbit *Eeprom)
{
	uInt8	*f_ptr = (uInt8 *)(Eeprom);
	uInt32	i;
	uInt8 n;

	if ( ! m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! m_PlxData->InitStatus.Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	SpiderInitFlash(0);
	AddonResetFlash();

	SpiderFlashChipErase();

	for (i = 0 ; i < sizeof(AddOnFlash32Mbit) ; i++)//----- Byte aligned
	{
		SpiderFlashWriteByte(i, f_ptr[i]);

		SpiderFlashCommand(FLASH_RESET,0,0,0);
		n = (uInt8)( AddonReadFlashByte(i));
		if (n != f_ptr[i])
			i--;//retry
	}

	SpiderFlashReset();
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Function   : WriteSectorsOfEeprom
// Description: Erase the sectors then Write Contiguous Sectors of the  Eeprom
//-----------------------------------------------------------------------------

int32  CsSpiderDev::WriteSectorsOfEeprom(void *pData, uInt32 u32StartAddr, uInt32 u32Size)
{
	uInt8	*pu8Data = (uInt8 *) pData;
	uInt32	i;
	uInt32  FirstSector, LastSector;
	uInt8	*pBackupMemory = NULL;		// Hold the contents of eeprom sectors
	uInt32	u32EndAddr = u32StartAddr + u32Size;
	uInt32	u32BackupSizeFisrt = 0;
	uInt32	u32BackupSizeLast = 0;
	uInt8	*pFirstSectorBackup = NULL;		// Point to the backup memory of the first sector
	uInt8	*pLastSectorBackup = NULL;			// Point to the backup memory of the lastt sector

	if ( ! m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! m_PlxData->InitStatus.Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	// Detecttion if we need to backup part of the fisrt and last sector
	if ( u32StartAddr % SPDR_INTEL_FLASH_SECTOR_SIZE )
	{
		u32BackupSizeFisrt = u32StartAddr % SPDR_INTEL_FLASH_SECTOR_SIZE;
	}

	if ( u32EndAddr % SPDR_INTEL_FLASH_SECTOR_SIZE )
	{
		u32BackupSizeLast = SPDR_INTEL_FLASH_SECTOR_SIZE - (u32EndAddr % SPDR_INTEL_FLASH_SECTOR_SIZE);
	}

	// Allocate memory for backup the contents of first and last sectors
	if ( u32BackupSizeFisrt > 0  || u32BackupSizeLast > 0 )
	{
		pBackupMemory = (uInt8 * ) ::GageAllocateMemoryX( SPDR_INTEL_FLASH_SECTOR_SIZE );
		if ( NULL == pBackupMemory )
			return CS_INSUFFICIENT_RESOURCES;

		pFirstSectorBackup = pBackupMemory;
		pLastSectorBackup = pFirstSectorBackup + u32BackupSizeFisrt;
	}

	// Find the sectors to be erased
	FirstSector = u32StartAddr / SPDR_INTEL_FLASH_SECTOR_SIZE;
	LastSector = (u32EndAddr - 1) / SPDR_INTEL_FLASH_SECTOR_SIZE;

	if ( FirstSector == LastSector )
	{
		if ( 0 == pBackupMemory )
		{
			// Only one sector of eeprom need to be erased and u32Size must be equal to the sector size
			AddonResetFlash();
			SpiderFlashSectorErase(FirstSector);
			SpiderFlashWrite( FirstSector, 0, pu8Data, SPDR_INTEL_FLASH_SECTOR_SIZE );
		}
		else
		{
			// Only one sector of eeprom need to be erased and u32Size must be smaller than the sector size
			// In this case, we can write new data and backup data in one shot
			uInt8	*pBuffer = pBackupMemory;

			if ( u32BackupSizeFisrt > 0 )
			{
				ReadEeprom( pBuffer, u32StartAddr - (u32StartAddr % SPDR_INTEL_FLASH_SECTOR_SIZE), u32BackupSizeFisrt );
				pBuffer += u32BackupSizeFisrt;
			}

			RtlCopyMemory( pBuffer, pu8Data, u32Size );

			if ( u32BackupSizeLast > 0 )
			{
				pBuffer += u32Size;
				ReadEeprom( pBuffer, u32EndAddr, u32BackupSizeLast );
			}

			AddonResetFlash();

			SpiderFlashSectorErase(FirstSector);
			SpiderFlashWrite( FirstSector, 0, pBackupMemory, SPDR_INTEL_FLASH_SECTOR_SIZE );
		}
	}
	else
	{
		// Backup the contents of the sector before StartAddr
		if ( u32BackupSizeFisrt > 0 )
			ReadEeprom( pFirstSectorBackup, u32StartAddr - (u32StartAddr % SPDR_INTEL_FLASH_SECTOR_SIZE), u32BackupSizeFisrt );

		// Backup the contents of the sector after the EndAddr ( StartAddr + Size )
		if ( u32BackupSizeLast > 0 )
			ReadEeprom( pLastSectorBackup, u32EndAddr, u32BackupSizeLast );

		AddonResetFlash();

		// Write the first backup contents
		if ( u32BackupSizeFisrt > 0 )
		{
			SpiderFlashSectorErase(FirstSector);
			SpiderFlashWrite( FirstSector, 0, pFirstSectorBackup, u32BackupSizeFisrt );
			SpiderFlashWrite( FirstSector, u32BackupSizeFisrt, pu8Data, (SPDR_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeFisrt) );
			pu8Data += (SPDR_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeFisrt);
			FirstSector += 1;
		}

		// Write the new contents
		for (i = FirstSector; i < LastSector ; i++)
		{
			SpiderFlashSectorErase(i);
			SpiderFlashWrite( i, 0, pu8Data, SPDR_INTEL_FLASH_SECTOR_SIZE );
			pu8Data += SPDR_INTEL_FLASH_SECTOR_SIZE;
		}

		// Write the last backup contents
		if ( u32BackupSizeLast > 0 )
		{
			SpiderFlashSectorErase(LastSector);
			SpiderFlashWrite( LastSector, 0, pu8Data, (SPDR_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeLast) );
			SpiderFlashWrite( LastSector, (SPDR_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeLast), pLastSectorBackup, u32BackupSizeLast );
		}
	}

	if ( NULL != pBackupMemory )
		::GageFreeMemoryX( pBackupMemory );

	SpiderFlashReset();

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Function   : WriteEepromEx
// Description:
//-----------------------------------------------------------------------------
int32  CsSpiderDev::WriteEepromEx(void *pData, uInt32 Addr, uInt32 Size,  FlashAccess flSequence )
{
	int32	i32status = CS_SUCCESS;

	if ( ! m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! m_PlxData->InitStatus.Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	if ((Addr == 0) && (Size == sizeof(AddOnFlash32Mbit)))
	{
		i32status = WriteEntireEeprom((AddOnFlash32Mbit *)pData);
		AddonConfigReset();
	}
	else
	{
		if ( flSequence == AccessAuto || flSequence == AccessFirst )
		{
			SpiderInitFlash(0);
			AddonResetFlash();
		}

		i32status = WriteSectorsOfEeprom( pData, Addr, Size );

		if ( flSequence == AccessAuto || flSequence == AccessLast )
		{
			//----- Reset the pointer to the image
			AddonConfigReset();
		}
	}

	return i32status;
}

//-----------------------------------------------------------------------------
// Function   : ReadEeprom
// Description:
//-----------------------------------------------------------------------------
int32   CsSpiderDev::ReadEeprom(void *Eeprom, uInt32 Addr, uInt32 Size)
{
	if ( ! m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! m_PlxData->InitStatus.Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	return IoctlAddonReadFlash( Addr, Eeprom, Size );
}

//---------------
// Function   : ReadFlashByte
// Description:
// Flash read custom implementation
// Flash access is 32 bits aligned
// the LSByte is data, upper bytes don't care
//---------------

uInt32 CsSpiderDev::AddonReadFlashByte(uInt32 Addr)
{
#if 1
	return CsDevIoCtl::AddonReadByteFromFlash( Addr );
#else
	WriteGIO(CSPDR_ADDON_FLASH_ADD, Addr);
	WriteGIO(CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE | CSPDR_ADDON_FOE);

	uInt32 u32Data = ReadGIO(CSPDR_ADDON_FLASH_DATA);

	WriteGIO(CSPDR_ADDON_FLASH_CMD, 0);
	return (u32Data);
#endif
}

//---------------
// Function   : WriteFlashByte
// Description:
// Flash write custom implementation
// Flash access is 32 bits aligned
// the LSByte is data, upper bytes don't care
//---------------

void CsSpiderDev::AddonWriteFlashByte(uInt32 Addr, uInt32 Data)
{
#if 1
	CsDevIoCtl::AddonWriteByteToFlash( Addr, Data );
#else
	WriteGIO(CSPDR_ADDON_FLASH_ADD, Addr);
	WriteGIO(CSPDR_ADDON_FLASH_DATA, Data);

	WriteGIO(CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE | CSPDR_ADDON_FWE);
	WriteGIO(CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE);
	WriteGIO(CSPDR_ADDON_FLASH_CMD, 0);
#endif
}

//---------------
// Function   : AddonResetFlash
// Description:	Hard reset the flash without giving up host control
//---------------
uInt32 CsSpiderDev::AddonResetFlash(void)
{
	WriteGIO(CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_RESET);
	WriteGIO(CSPDR_ADDON_FLASH_CMD, 0);

	return CS_SUCCESS;
}

int32	CsSpiderDev::DoAddonWriteFlash( uInt32 u32Sector, uInt32 u32Offset, PVOID pBuffer, uInt32 u32WriteSize )
{
	uInt8	*pSrc = (uInt8 *) pBuffer;
	uInt32	u32Addr = u32Sector*SPDR_INTEL_FLASH_SECTOR_SIZE;
	uInt32	u32Dst = u32Offset + u32Addr;

	int32	i32GroupLen = 0;
	int32	i32Count = 0; 
	int32 	retry = 1000;	
	uInt32  nBytes	= u32WriteSize;

	while (nBytes > 0)
	{
		i32GroupLen = (nBytes > 0x20) ? 0x20 : nBytes;
		i32Count = i32GroupLen - 1;

		AddonWriteFlashByte(0, 0xE8);

		while ((SpiderFlashStatus( FlashPtr->sec[u32Sector].base)) != STATUS_READY)
		{
			retry--;
			if (retry<0) break;
		}

		AddonWriteFlashByte(u32Dst, i32Count++);
		
		while(i32Count-- > 0)
		{
			AddonWriteFlashByte(u32Dst++, *pSrc++);
		}

		AddonWriteFlashByte(u32Dst, 0xD0);

		while ((SpiderFlashStatus( FlashPtr->sec[u32Sector].base)) != STATUS_READY)
		{
			retry--;
			if (retry<0) break;
		}

		nBytes -= i32GroupLen;
		retry = 1000;	
	}

	return CS_SUCCESS;
}

