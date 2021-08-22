
#include "StdAfx.h"
#include "CsTypes.h"
#include "CsRabbit.h"
#include "CsPlxDefines.h"
#include "CsPrivateStruct.h"
#include "CsRabbitFlash.h"


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
//
//-----------------------------------------------------------------------------

uInt8 CsRabbitDev::GetFlashManufactoryCode()
{
	uInt8 u8FlashId;

	RabbitInitFlash(0);

	RabbitFlashCommand( FLASH_AUTOSEL,0,0,0 );
	u8FlashId = (uInt8) AddonReadFlashByte(0);

	return( u8FlashId );
}

//-----------------------------------------------------------------------------
// Function   :  init_flash
// Description:  initializes setting of parameters and the appropriate sector table
//-----------------------------------------------------------------------------
// IntelFlash
void  CsRabbitDev:: RabbitInitFlash(uInt32 FlashIndex)
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

void  CsRabbitDev::RabbitFlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data)
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

	 } while ((Retry-- > 0) && (RabbitFlashStatus() != STATUS_READY));

}

//-----------------------------------------------------------------------------
// Function   :   FlashWrite
// Description:
//	Faster way to program multiple data words, without
//	needing the function overhead of looping algorithms which
//	program word by word.
//-----------------------------------------------------------------------------
// IntelFlash

int32  CsRabbitDev::RabbitFlashWrite( uInt32 Sector, uInt32 Offset, uInt8 *Buf, uInt32 nBytes)
{
	uInt8 *Src;
	uInt32 Dst;
	uInt32 Status;
	Dst = Offset;
	Src = Buf;
	CSTIMER		CsTimer;

	CsTimer.Set(CS_WAIT_TIMEOUT);
	while ((Status = RabbitFlashStatus()) == STATUS_FLASH_BUSY)
	{
		if ( CsTimer.State() )
			break;
	}

	if (Status != STATUS_READY)
		return CS_MISC_ERROR;

	// Call DoAddonWriteFlash because IoctlAddonWriteFlash doesnot works on some of PCa
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

uInt32  CsRabbitDev::RabbitFlashStatus()
{
	uInt32	u32Status = ReadGIO( CSRB_ADDON_STATUS );

	if ( ! (u32Status & INTEL_FLASH_BUSY) )
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
}
//-----------------------------------------------------------------------------
// Function   :  FlashReset
// Description:  Send the command to reset the Flash - Ready to read
//-----------------------------------------------------------------------------
void  CsRabbitDev::RabbitFlashReset()
{
	RabbitFlashCommand( FLASH_RESET,0,0,0 );
}

//-----------------------------------------------------------------------------
// Function   :  FlashWriteByte
// Description:  Send the command to write One byte
//-----------------------------------------------------------------------------
void  CsRabbitDev::RabbitFlashWriteByte(  uInt32 Addr, uInt32 Data )
{
	uInt32 Sector, Offset;

	Sector = Addr / FlashPtr->sec[0].size; //---- True since all the sectors are the same size
	Offset = Addr % FlashPtr->sec[0].size;

	RabbitFlashCommand(FLASH_PROG, Sector, Offset, Data);
}



#define MAXSECTORS  64      // maximum number of sectors supported

//-----------------------------------------------------------------------------
// Function   :  FlashChipErase
// Description:  Erase the entire Chip
//-----------------------------------------------------------------------------
void  CsRabbitDev::RabbitFlashChipErase()
{
	uInt8		u8Sector = 0;

	while(u8Sector < MAXSECTORS)
		RabbitFlashSectorErase( u8Sector++ );

}

//-----------------------------------------------------------------------------
// Function   :  FlashSectorErase
// Description:  Erase the entire Chip
//-----------------------------------------------------------------------------
// IntelFlash

void  CsRabbitDev::RabbitFlashSectorErase( uInt32 Sector)
{
	CSTIMER		CsTimer;
	RabbitFlashCommand(FLASH_SECTOR_ERASE, Sector,0,0);

	CsTimer.Set(CS_ERASE_WAIT_TIMEOUT);
	while ((RabbitFlashStatus()) == STATUS_FLASH_BUSY)
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
int32  CsRabbitDev::WriteEntireEeprom(AddOnFlash32Mbit *Eeprom)
{
	uInt8	*f_ptr = (uInt8 *)(Eeprom);
	uInt32	i;
	uInt8 n;

	if ( ! GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! GetInitStatus()->Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	RabbitInitFlash(0);
	AddonResetFlash();

	RabbitFlashChipErase();

	for (i = 0 ; i < sizeof(AddOnFlash32Mbit) ; i++)//----- Byte aligned
	{
		RabbitFlashWriteByte(i, f_ptr[i]);

		RabbitFlashCommand(FLASH_RESET,0,0,0);
		n = (uInt8)( AddonReadFlashByte(i));
		if (n != f_ptr[i])
			i--;//retry
	}

	RabbitFlashReset();
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Function   : WriteSectorsOfEeprom
// Description: Erase the sectors then Write Contiguous Sectors of the  Eeprom
//-----------------------------------------------------------------------------

int32  CsRabbitDev::WriteSectorsOfEeprom(void *pData, uInt32 u32StartAddr, uInt32 u32Size)
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

	if ( ! GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! GetInitStatus()->Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	// Detecttion if we need to backup part of the fisrt and last sector
	if ( u32StartAddr % RABBIT_INTEL_FLASH_SECTOR_SIZE )
	{
		u32BackupSizeFisrt = u32StartAddr % RABBIT_INTEL_FLASH_SECTOR_SIZE;
	}

	if ( u32EndAddr % RABBIT_INTEL_FLASH_SECTOR_SIZE )
	{
		u32BackupSizeLast = RABBIT_INTEL_FLASH_SECTOR_SIZE - (u32EndAddr % RABBIT_INTEL_FLASH_SECTOR_SIZE);
	}

	// Allocate memory for backup the contents of first and last sectors
	if ( u32BackupSizeFisrt > 0  || u32BackupSizeLast > 0 )
	{
		pBackupMemory = (uInt8 * ) ::GageAllocateMemoryX( RABBIT_INTEL_FLASH_SECTOR_SIZE );
		if ( NULL == pBackupMemory )
			return CS_INSUFFICIENT_RESOURCES;

		pFirstSectorBackup = pBackupMemory;
		pLastSectorBackup = pFirstSectorBackup + u32BackupSizeFisrt;
	}

	// Find the sectors to be erased
	FirstSector = u32StartAddr / RABBIT_INTEL_FLASH_SECTOR_SIZE;
	LastSector = (u32EndAddr - 1) / RABBIT_INTEL_FLASH_SECTOR_SIZE;


	if ( FirstSector == LastSector )
	{
		if ( 0 == pBackupMemory )
		{
			// Only one sector of eeprom need to be erased and u32Size must be equal to the sector size
			AddonResetFlash();
			RabbitFlashSectorErase(FirstSector);
			RabbitFlashWrite( FirstSector, 0, pu8Data, RABBIT_INTEL_FLASH_SECTOR_SIZE );
		}
		else
		{
			// Only one sector of eeprom need to be erased and u32Size must be smaller than the sector size
			// In this case, we can write new data and backup data in one shot
			uInt8	*pBuffer = pBackupMemory;
			ReadEeprom( pBuffer, u32StartAddr - (u32StartAddr % RABBIT_INTEL_FLASH_SECTOR_SIZE), u32BackupSizeFisrt );
			pBuffer += u32BackupSizeFisrt;
			RtlCopyMemory( pBuffer, pu8Data, u32Size );
			pBuffer += u32Size;
			ReadEeprom( pBuffer, u32EndAddr, u32BackupSizeLast );

			AddonResetFlash();

			RabbitFlashSectorErase(FirstSector);
			RabbitFlashWrite( FirstSector, 0, pBackupMemory, RABBIT_INTEL_FLASH_SECTOR_SIZE );
		}
	}
	else
	{
		// Backup the contents of the sector before StartAddr
		if ( u32BackupSizeFisrt > 0 )
			ReadEeprom( pFirstSectorBackup, u32StartAddr - (u32StartAddr % RABBIT_INTEL_FLASH_SECTOR_SIZE), u32BackupSizeFisrt );

		// Backup the contents of the sector after the EndAddr ( StartAddr + Size )
		if ( u32BackupSizeLast > 0 )
			ReadEeprom( pLastSectorBackup, u32EndAddr, u32BackupSizeLast );

		AddonResetFlash();

		// Write the first backup contents
		if ( u32BackupSizeFisrt > 0 )
		{
			RabbitFlashSectorErase(FirstSector);
			RabbitFlashWrite( FirstSector, 0, pFirstSectorBackup, u32BackupSizeFisrt );
			RabbitFlashWrite( FirstSector, u32BackupSizeFisrt, pu8Data, (RABBIT_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeFisrt) );
			pu8Data += (RABBIT_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeFisrt);
			FirstSector += 1;
		}

		// Write the new contents
		for (i = FirstSector; i < LastSector ; i++)
		{
			RabbitFlashSectorErase(i);
			RabbitFlashWrite( i, 0, pu8Data, RABBIT_INTEL_FLASH_SECTOR_SIZE );
			pu8Data += RABBIT_INTEL_FLASH_SECTOR_SIZE;
		}

		// Write the last backup contents
		if ( u32BackupSizeLast > 0 )
		{
			RabbitFlashSectorErase(LastSector);
			RabbitFlashWrite( LastSector, 0, pu8Data, (RABBIT_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeLast) );
			RabbitFlashWrite( LastSector, (RABBIT_INTEL_FLASH_SECTOR_SIZE - u32BackupSizeLast), pLastSectorBackup, u32BackupSizeLast );
		}
	}

	if ( NULL != pBackupMemory )
		::GageFreeMemoryX( pBackupMemory );

	RabbitFlashReset();

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Function   : WriteEepromEx
// Description:
//-----------------------------------------------------------------------------
int32  CsRabbitDev::WriteEepromEx(void *pData, uInt32 Addr, uInt32 Size,  FlashAccess flSequence )
{
	int32	i32status = CS_SUCCESS;

	if ( ! GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! GetInitStatus()->Info.u32AddOnPresent )
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
			RabbitInitFlash(0);
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
int32   CsRabbitDev::ReadEeprom(void *Eeprom, uInt32 Addr, uInt32 Size)
{
	if ( ! GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! GetInitStatus()->Info.u32AddOnPresent )
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

uInt32 CsRabbitDev::AddonReadFlashByte(uInt32 Addr)
{
#if 1
	return CsDevIoCtl::AddonReadByteFromFlash( Addr );
#else
	WriteGIO(CSRB_ADDON_FLASH_ADD, Addr);
	WriteGIO(CSRB_ADDON_FLASH_CMD, CSRB_ADDON_FCE | CSRB_ADDON_FOE);

	uInt32 u32Data = ReadGIO(CSRB_ADDON_FLASH_DATA);

	WriteGIO(CSRB_ADDON_FLASH_CMD, 0);
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

void CsRabbitDev::AddonWriteFlashByte(uInt32 Addr, uInt32 Data)
{
#if 1
	CsDevIoCtl::AddonWriteByteToFlash( Addr, Data );
#else
	WriteGIO(CSRB_ADDON_FLASH_ADD, Addr);
	WriteGIO(CSRB_ADDON_FLASH_DATA, Data);

	WriteGIO(CSRB_ADDON_FLASH_CMD, CSRB_ADDON_FCE | CSRB_ADDON_FWE);
	WriteGIO(CSRB_ADDON_FLASH_CMD, 0);
#endif
}

//---------------
// Function   : AddonResetFlash
// Description:	Hard reset the flash without giving up host control
//---------------
uInt32 CsRabbitDev::AddonResetFlash(void)
{
	WriteGIO(CSRB_ADDON_FLASH_CMD, CSRB_ADDON_RESET);
	WriteGIO(CSRB_ADDON_FLASH_CMD, 0);

	return CS_SUCCESS;
}


//---------------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------------

int32	CsRabbitDev::DoAddonWriteFlash( uInt32 u32Sector, uInt32 u32Offset, PVOID pBuffer, uInt32 u32WriteSize )
{
	uInt8	*pSrc = (uInt8 *) pBuffer;
	uInt32	u32Addr = u32Sector*RABBIT_INTEL_FLASH_SECTOR_SIZE;
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

		while ((RabbitFlashStatus()) != STATUS_READY)
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

		while ((RabbitFlashStatus()) != STATUS_READY)
		{
			retry--;
			if (retry<0) break;
		}

		nBytes -= i32GroupLen;
		retry = 1000;	
	}

	return CS_SUCCESS;
}

