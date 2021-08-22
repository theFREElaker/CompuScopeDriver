
#include "StdAfx.h"
#include "CsTypes.h"
#include "CsHexagon.h"
#include "CsPlxDefines.h"
#include "CsPrivateStruct.h"
#include "CsHexagonFlash.h"

#define HEXAGON_FLASH_SECTOR_SIZE_BYTE	HEXAGON_FLASH_SECTOR_SIZE
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon::WriteBaseBoardFlash(uInt32 u32StartAddr, void *pBuffer, uInt32 u32Size, bool bBackup )
{	
	int32	i32Status = CS_SUCCESS;
	uInt8	*pu8Data = (uInt8 *) pBuffer;
	uInt8	*pBackupMemory = NULL;			// Hold the contents of eeprom sectors
	uInt32	u32EndAddr = u32StartAddr + u32Size - 1;
	uInt32	u32BackupSizeFisrt = 0;
	uInt32	u32BackupSizeLast = 0;
	uInt8	*pFirstSectorBackup = NULL;		// Point to the backup memory of the first sector
	uInt8	*pLastSectorBackup = NULL;		// Point to the backup memory of the lastt sector

	uInt32	u32OffsetInSector = 0;
	uInt32	u32SectorSize = HEXAGON_FLASH_SECTOR_SIZE_BYTE;
	uInt32	u32StartSectorAddr = 0;
	uInt32	u32EndSectorAddr = 0;
	
	// Write to Flash will erase the whole sector even if we write only a couple of bytes of data.
	// We have to backup the current content of the sector, update them with the new data then write to
	// the flash the updated sector.

	// Calculate the backup size
	u32OffsetInSector	= (u32StartAddr % u32SectorSize);
	u32StartSectorAddr		= u32StartAddr - u32OffsetInSector;
	if ( bBackup && (0 != u32OffsetInSector) )
		u32BackupSizeFisrt = u32OffsetInSector;

	u32OffsetInSector	= (u32EndAddr % u32SectorSize);
	u32EndSectorAddr		= u32EndAddr - u32OffsetInSector;
	if ( bBackup && (0 != u32OffsetInSector) )
		u32BackupSizeLast = u32SectorSize - u32OffsetInSector;

	bool	bMultipleSectors = (u32StartSectorAddr != u32EndSectorAddr);

	// Allocate memory for backup the contents of first and last sectors
	if ( u32BackupSizeFisrt > 0  || u32BackupSizeLast > 0 )
	{
		pBackupMemory = (uInt8 * ) ::GageAllocateMemoryX( 2*u32SectorSize );
		if ( NULL == pBackupMemory )
			return CS_INSUFFICIENT_RESOURCES;

		pFirstSectorBackup	= pBackupMemory;
		pLastSectorBackup	= pFirstSectorBackup + u32SectorSize;
	}

	if ( bMultipleSectors )
	{
		// Copy the new data into the fisrt and last backup buffers
		if ( u32BackupSizeFisrt > 0 )
		{
			// Backup the contents of the sector before StartAddr
			IoctlReadFlash( u32StartSectorAddr, pFirstSectorBackup, u32BackupSizeFisrt );
			// Copy new data to buffer
			GageCopyMemoryX( pFirstSectorBackup + u32BackupSizeFisrt, pu8Data, (u32SectorSize - u32BackupSizeFisrt));
		}

		if ( u32BackupSizeLast > 0 )
		{
			// Backup the contents of the sector after the EndAddr ( StartAddr + Size )
			IoctlReadFlash( u32EndAddr , pLastSectorBackup + (u32EndAddr % u32SectorSize),  u32BackupSizeLast );
			// Copy new data to buffer
			GageCopyMemoryX( pLastSectorBackup, pu8Data + u32Size - (u32SectorSize - u32BackupSizeLast), (u32SectorSize - u32BackupSizeLast));
		}

		uInt32	u32Remain = u32Size;
		uInt32	u32Addr = u32StartSectorAddr;

		// Write the first sector
		if ( u32BackupSizeFisrt > 0 )
		{
			u32Addr		= u32StartSectorAddr;
			
			i32Status	= IoctlWriteFlash( u32Addr, 0, pFirstSectorBackup, u32SectorSize );
			pu8Data		+= u32SectorSize - (u32StartAddr % u32SectorSize);
			u32Addr		+= u32SectorSize;
			u32Remain	-= (u32SectorSize-u32BackupSizeFisrt);
		}
		else
		{
			uInt32			u32WriteSize = u32SectorSize - (u32StartAddr % u32SectorSize);
			u32Addr			= u32StartAddr;
			
			i32Status	= IoctlWriteFlash( u32Addr, 0, pu8Data, u32WriteSize );
			pu8Data		+= u32WriteSize;
			u32Addr		+= u32WriteSize;
			u32Remain	-= u32WriteSize;
		}

		// Write the middle sectors
		for ( ; u32Addr < u32EndSectorAddr; u32Addr += u32SectorSize )
		{
			i32Status	= IoctlWriteFlash( u32Addr, 0, pu8Data, u32SectorSize );
			pu8Data		+= u32SectorSize;
			u32Remain	-= u32SectorSize;
		}

		// Write the last sector
		if ( u32BackupSizeLast > 0 )
		{
			i32Status = IoctlWriteFlash( u32EndSectorAddr, 0, pLastSectorBackup, u32SectorSize );
		}
		else
		{
			i32Status = IoctlWriteFlash( u32EndSectorAddr, 0, pu8Data, u32Remain );
		}
	}
	else
	{
		if ( u32BackupSizeFisrt > 0  || u32BackupSizeLast > 0 )
		{
			// Backup the contents of the sector
			IoctlReadFlash( u32StartSectorAddr, pFirstSectorBackup, u32SectorSize );
			// Copy new data to buffer
			GageCopyMemoryX( pFirstSectorBackup + u32BackupSizeFisrt, pu8Data, u32Size);

			// Write the whole sector	
			i32Status = IoctlWriteFlash( u32EndSectorAddr, 0, pFirstSectorBackup, u32SectorSize );
		}
		else
		{
			// Write into the flash wihtout backup	
			i32Status = IoctlWriteFlash( u32StartAddr, 0, pu8Data, u32Size );
		}
	}

	GageFreeMemoryX(pBackupMemory);
	return i32Status;
}

