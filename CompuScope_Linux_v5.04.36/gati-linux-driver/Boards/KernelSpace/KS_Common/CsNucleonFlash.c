#ifdef _WINDOWS_
#include <ntddk.h>
#else
#ifndef KdPrint
#define KdPrint(x) printk x
#endif
#endif

#include "GageDrv.h"
#include "CsTypes.h"

#define NUCLEONFLASH_SECTOR_SIZE0	0x8000
#define NUCLEONFLASH_SECTOR_SIZE1	0x20000


#define FLASH_ADDR0_OFFSET	0
#define FLASH_ADDR1_OFFSET	1
#define FLASH_ADDR2_OFFSET	2
#define FLASH_ADDR3_OFFSET	3
#define FLASH_DATA0_OFFSET	4
#define FLASH_DATA1_OFFSET	5
#define FLASH_STATUS_OFFSET	6
#define FLASH_CMD_OFFSET	6

#define nPROTECT_BOOTIMAGE
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#define IO_READ_ENABLE		0x2000
#define IO_WRITE_STROBE		0x1000
void	WriteGioCpld( PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Data )
{
	WriteFlashRegister ( FdoData, 0, (u32Data & 0xFF) | ((u32Addr & 0xF) << 8) );
	ReadFlashRegister ( FdoData, 0 );	// dummy read
	WriteFlashRegister ( FdoData, 0, (u32Data & 0xFF) | ((u32Addr & 0xF) << 8) | IO_WRITE_STROBE );
	ReadFlashRegister ( FdoData, 0 );	// dummy read
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	ReadGioCpld( PFDO_DATA FdoData, uInt32 u32Addr )
{
	WriteFlashRegister ( FdoData, 0, IO_READ_ENABLE | ((u32Addr & 0xF) << 8) );
	ReadFlashRegister ( FdoData, 0 );	// dummy read
	return ReadFlashRegister ( FdoData, 0 );
}


//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void	HardResetFlash(PFDO_DATA FdoData)
{
	WriteFlashRegister(FdoData, 0, 0x04000);
	WriteFlashRegister(FdoData, 0, 0);
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void WriteFlashData(PFDO_DATA FdoData, uInt32 u32Data)
{
	WriteGioCpld( FdoData, FLASH_DATA0_OFFSET, u32Data & 0xff);
	WriteGioCpld( FdoData, FLASH_DATA1_OFFSET, (u32Data & 0xff00)>>8);
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
uInt16	ReadFlashData(PFDO_DATA FdoData)
{
	uInt16 u16Data = 0xff & ReadGioCpld(FdoData, FLASH_DATA0_OFFSET);

	u16Data |= (ReadGioCpld(FdoData, FLASH_DATA1_OFFSET) << 8);

	return u16Data;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void WriteFlashAddress(PFDO_DATA FdoData, uInt32 u32Addr)
{
	WriteGioCpld( FdoData, FLASH_ADDR0_OFFSET, u32Addr & 0xff);
	WriteGioCpld( FdoData, FLASH_ADDR1_OFFSET, (u32Addr & 0xff00)>>8);
	WriteGioCpld( FdoData, FLASH_ADDR2_OFFSET, (u32Addr & 0xff0000)>>16);
	WriteGioCpld( FdoData, FLASH_ADDR3_OFFSET, (u32Addr & 0x01000000)>>24);

}

#define	FLASH_CMD_IDLE	0x0
#define	FLASH_CMD_READ	0x1
#define	FLASH_CMD_WRITE	0x2

#define	FLASH_STATE_READY	0x1
#define	FLASH_STATE_IDLE	0x2
#define	FLASH_STATE_DONE	0x4

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int32 FlashReadValue(PFDO_DATA FdoData, uInt32 u32Addr, uInt16 *pu16Data )
{
	uInt32	u32State = 0;
	int32	count = 0x10000;

#ifdef CHECK_FLASH_ADDRESS   
     // Make sure register offset is valid
	if (u32Addr >= 0x1000000)
    {
        KdPrint(("FlashReadValue ERROR - invalid Flash Address (0x%x)\n", u32Addr));

        *pu16Data = -1;

        return CS_FLASH_INVALID_SECTOR;
    }
#endif

	WriteFlashAddress(FdoData, u32Addr);

	u32State = ReadGioCpld( FdoData, FLASH_STATUS_OFFSET);
	while(!(u32State & FLASH_STATE_IDLE))
	{
		WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);
		u32State = ReadGioCpld(FdoData, FLASH_STATUS_OFFSET);
		if(count-- < 0){
			KdPrint(("ERROR - Read fails at Flash Address (0x%x)\n", u32Addr));
	        return CS_FLASH_DATAREAD_ERROR;
		}
	}

	WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_READ);	//write command READ

	count = 0x10000;

	u32State = ReadGioCpld( FdoData, FLASH_STATUS_OFFSET);
	while(!(u32State & FLASH_STATE_DONE))
	{
		u32State = ReadGioCpld( FdoData, FLASH_STATUS_OFFSET);
		if(u32State & FLASH_STATE_IDLE)
		{	//repet if idle only
			WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_READ);	//write command READ
			KdPrint(("ERROR - Should be DONE by now!\n"));
		}
		if(count-- < 0)
		{
			KdPrint(("ERROR - Read fails at Flash Address (0x%x)\n", u32Addr));
			return CS_FLASH_DATAREAD_ERROR;
		}
	}

	//If in DONE state, read data and send to IDLE
	*pu16Data = ReadFlashData(FdoData);

	WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);
	
//	KdPrint(("FLASH_READ - END - Flash Address (0x%x) - Data: (0x%x)\n", u32Addr, *pu16Data));

    return CS_SUCCESS;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int32 FlashWriteValue(PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Val )
{
	uInt32	u32State;
	int32	count = 0x10000;

#ifdef CHECK_FLASH_ADDRESS   
	// Make sure register offset is valid
    if (u32Addr >= 0x1000000)
    {
        KdPrint(("FlashWriteValue ERROR - invalid Flash Address (0x%x)\n", u32Addr));
        return CS_FLASH_INVALID_SECTOR;
    }
#endif    

	u32Val &= 0xffff;

	WriteFlashAddress(FdoData, u32Addr);

	u32State = ReadGioCpld( FdoData, FLASH_STATUS_OFFSET);
	while(!(u32State & FLASH_STATE_IDLE))
	{
		u32State = ReadGioCpld( FdoData, FLASH_STATUS_OFFSET);
		if(u32State & FLASH_STATE_DONE)
		{
			WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);
			KdPrint(("ERROR - Should be IDLE by now!\n"));
		}

		if(count-- < 0)
		{
			KdPrint(("ERROR - Write fails at Flash Address (0x%x)\n", u32Addr));
	        return CS_FLASH_DATAWRITE_ERROR;
		}
	}

	WriteFlashData(FdoData, u32Val);					  //write data

	WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_WRITE);

	count = 0x10000;

	u32State = ReadGioCpld( FdoData, FLASH_STATUS_OFFSET);
	while(!(u32State & FLASH_STATE_DONE))
	{
		u32State = ReadGioCpld( FdoData, FLASH_STATUS_OFFSET);
		if(u32State & FLASH_STATE_IDLE)
		{	//repeat if idle only
			WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_WRITE);
			KdPrint(("ERROR - Should be DONE by now!\n"));
		}
		if(count-- < 0)
		{
			KdPrint(("ERROR - Read fails at Flash Address (0x%x)\n", u32Addr));
			return CS_FLASH_DATAWRITE_ERROR;
		}
	}

	WriteGioCpld(FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);
//	KdPrint(("FLASH_WRITE - END - Flash Address (0x%x) - Data: (0x%x)\n", u32Addr, u32Val));

    return CS_SUCCESS;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
uInt32 ReadFlashProgramStatus(PFDO_DATA FdoData, uInt32 u32Addr)
{
	int32	i32Status;
	uInt16	first_read,second_read;

	i32Status = FlashReadValue(FdoData, u32Addr, &first_read);//first read
	if ( CS_FAILED( i32Status ) )
		return 0;
	i32Status = FlashReadValue(FdoData, u32Addr, &second_read);//second read
	if ( CS_FAILED( i32Status ) )
		return 0;

	if((second_read & 0xffff) == (first_read & 0xffff))
	{	 //check toggle
		i32Status = FlashReadValue(FdoData, u32Addr, &first_read);//first read
		if ( CS_FAILED( i32Status ) )
			return 0;

		if((second_read & 0xffff) == (first_read & 0xffff))
			return 1;
	}

	return 0;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int32	FlashProgramWord(PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Val )
{
	int32	i32Status = CS_SUCCESS;
	uInt32	u32State;
	int32	count = 0x10000;
#ifdef PROTECT_BOOTIMAGE
	// Temporary do not allow to erase the boot image )
	if ( u32Addr < 0x2F0000 )
	{
        KdPrint(("FlashReadValue ERROR - invalid Flash Address (0x%x)\n", u32Addr));
		return CS_FLASH_INVALID_SECTOR;
	}
#endif

#ifdef CHECK_FLASH_ADDRESS
    // Make sure register offset is valid
    if (u32Addr >= 0x1000000)
    {
        KdPrint(("ERROR - invalid Flash Address (0x%x)\n", u32Addr));
        return CS_FLASH_INVALID_SECTOR;
    }
#endif

	u32Val &= 0xffff;

	count = 0x10000;

	// Write unlock cycles
	i32Status = FlashWriteValue(FdoData, 0x0555, 0x0aa);
	if ( CS_FAILED(i32Status) )
		return i32Status;
	i32Status = FlashWriteValue(FdoData, 0x02aa, 0x055);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Write program command
	i32Status = FlashWriteValue(FdoData, 0x0555, 0x0a0);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Write program data to address
	i32Status = FlashWriteValue(FdoData, u32Addr, u32Val);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	u32State = ReadFlashProgramStatus(FdoData, u32Addr);

	while(!(u32State))
	{	
		//could loop for ever!
		u32State = ReadFlashProgramStatus(FdoData, u32Addr);
		if(count-- < 0)
		{
			KdPrint(("ERROR - Program fails at Flash Address (0x%x) count: %d\n", u32Addr, count));
			return CS_MISC_ERROR;
		}
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int32	FlashProgramWord32(PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32SectorAddr, void *pBuffer, uInt32 u32BufferSize )
{
	int32	i32Status = CS_SUCCESS;
	uInt32	u32State;
	int32	count = 0x10000;
	uInt32	u32WordCount =  u32BufferSize >> 1;
	uInt32	u32WriteCount =  0;
	uInt16	*pu16Buffer = (uInt16 *) pBuffer;
	uInt32	u32Val = 0;


#ifdef PROTECT_BOOTIMAGE
	// Temporary do not allow to erase the boot image )
	if ( u32Addr < 0x2F0000 )
	{
        KdPrint(("FlashReadValue ERROR - invalid Flash Address (0x%x)\n", u32Addr));
		return CS_FLASH_INVALID_SECTOR;
	}
#endif

#ifdef CHECK_FLASH_ADDRESS
    // Make sure register offset is valid
    if (u32Addr >= 0x1000000)
    {
        KdPrint(("FlashProgramWord32 ERROR - invalid Flash Address (0x%x)\n", u32Addr));
        return CS_FLASH_INVALID_SECTOR;
    }
#endif

	// Write unlock cycles
	i32Status = FlashWriteValue(FdoData, 0x0555, 0x0aa);
	if ( CS_FAILED(i32Status) )
		return i32Status;
	i32Status = FlashWriteValue(FdoData, 0x02aa, 0x055);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Write buffer load command
	i32Status = FlashWriteValue(FdoData, u32Addr, 0x025);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Write word count
	i32Status = FlashWriteValue(FdoData, u32Addr, u32WordCount-1);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Load buffer
	while ( u32WriteCount < u32WordCount  )
	{
		u32Val = *pu16Buffer;
		i32Status = FlashWriteValue(FdoData, u32Addr, u32Val );
		if ( CS_FAILED(i32Status) )
			return i32Status;

		u32Addr++;
		pu16Buffer++;

		u32Val = *pu16Buffer;
		i32Status = FlashWriteValue(FdoData, u32Addr, u32Val );
		if ( CS_FAILED(i32Status) )
			return i32Status;

		u32Addr++;
		pu16Buffer++;

		u32WriteCount += 2;
	}

	// Write sector address
	i32Status = FlashWriteValue(FdoData, u32SectorAddr, 0x029);
	if ( CS_FAILED(i32Status) )
		return i32Status;
	
	// Back to the last written address
	u32Addr -= 1;

	u32State = ReadFlashProgramStatus(FdoData, u32Addr);
	while(!(u32State))
	{	
		//could loop for ever!
		u32State = ReadFlashProgramStatus(FdoData, u32Addr);
		if(count-- < 0)
		{
			KdPrint(("ERROR - FlashProgramWord32 fails at Flash Address (0x%x) count: %d\n", u32Addr, count));
			return CS_MISC_ERROR;
		}
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int32	FlashSectorErase(PFDO_DATA FdoData, uInt32 u32WordAddr)
{
	int32	i32Status = CS_SUCCESS;
	uInt16	u16Val = 0;
	uInt16	first_read = 0,second_read = 0xffff;
	int32	i32Count = 	0x50000;

#ifdef PROTECT_BOOTIMAGE
	// Temporary do not allow to erase the boot image )
	if ( u32WordAddr < 0x5E0000/2 )
	{
        KdPrint(("FlashSectorErase ERROR - Flash Sector Address (0x%x) is protected \n", u32WordAddr));
		return CS_FLASH_INVALID_SECTOR;
	}
#endif

	// Write unlock cycles
	i32Status = FlashWriteValue(FdoData, 0x0555, 0x0aa);
	if ( CS_FAILED(i32Status) )
		return i32Status;
	i32Status = FlashWriteValue(FdoData, 0x02aa, 0x055);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Write setup command
	i32Status = FlashWriteValue(FdoData, 0x0555, 0x080);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Write unlock cycles
	i32Status = FlashWriteValue(FdoData, 0x0555, 0x0aa);
	if ( CS_FAILED(i32Status) )
		return i32Status;
	i32Status = FlashWriteValue(FdoData, 0x02aa, 0x055);
	if ( CS_FAILED(i32Status) )
		return i32Status;
	
	// Write sector erase command
	i32Status = FlashWriteValue(FdoData, u32WordAddr, 0x030);
	if ( CS_FAILED(i32Status) )
		return i32Status;


	// Check for erase started
	i32Status = FlashReadValue(FdoData, u32WordAddr, &u16Val );
	if ( CS_FAILED(i32Status) )
		return i32Status;

	while(0==(0x08 & u16Val))
	{
		i32Status = FlashReadValue(FdoData, u32WordAddr, &u16Val );			 //DQ3 should be 1
		if ( CS_FAILED(i32Status) )
			return i32Status;

		if (i32Count-- < 0)
		{
			KdPrint(("FlashSectorErase ERROR - SectorAddr = 0x%x DQ3 stuck on 0\n", u32WordAddr));
			return CS_FLASH_SECTORERASE_ERROR;
		}
	}

	// Check for erase completed
	i32Count = 	0x50000;
	do
	{
		i32Status = FlashReadValue(FdoData, u32WordAddr, &first_read);
		if ( CS_FAILED(i32Status) )
			return i32Status;
		i32Status = FlashReadValue(FdoData, u32WordAddr, &second_read);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		if (i32Count-- < 0)
		{
			KdPrint(("FlashSectorErase ERROR - SectorAddr = 0x%x the content is not erased \n", u32WordAddr ));
			return CS_FLASH_SECTORERASE_ERROR;
		}
	}while((first_read != second_read) ||((first_read & 0xffff) != 0xffff)); //should read ffff no toggle

	return CS_SUCCESS;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int NucleonFlashRead( PFDO_DATA FdoData,  uInt32 u32Addr, void *pBuffer,  uInt32 u32BufferSize)
{
	int32	i32Status = CS_SUCCESS;
	uInt32	i;
	uInt16	u16Temp;
	uInt32	u32WordAddr = u32Addr/2;
	uInt32	u32ReadCount = u32BufferSize/2;
	uInt8	*pu8Buffer = (uInt8 *) pBuffer;

	if ( (u32Addr % 2) != 0 )
	{
		i32Status = FlashReadValue(FdoData, u32WordAddr, &u16Temp);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		*pu8Buffer = (u16Temp>> 8) & 0xFF;
		pu8Buffer++;
		u32BufferSize--;
		u32WordAddr++;
	}
	
	u32ReadCount = u32BufferSize/2;
	for ( i = 0; i < u32ReadCount; i++ )
	{
		i32Status = FlashReadValue(FdoData, u32WordAddr+i, (uInt16 *)pu8Buffer);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		pu8Buffer +=2;
	}

	if  ((u32BufferSize % 2) != 0 )
	{
		i32Status = FlashReadValue(FdoData, u32WordAddr+i, &u16Temp);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		*pu8Buffer = u16Temp & 0xFF;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
int NucleonFlashWrite( PFDO_DATA FdoData,  uInt32 u32Sector, uInt32 u32Offset, void *pBuffer,  uInt32 u32BufferSize)
{
	uInt32	i;
	uInt32	u32Temp;
	uInt32	u32Addr = 0;
	uInt32	u32WordAddr = 0;
	uInt32	u32WriteCount = u32BufferSize/2;
	uInt32	u32Remain = u32WriteCount;
	uInt16	*pu16Buffer = (uInt16 *) pBuffer;
	int32	i32Status = CS_SUCCESS;
	uInt32	u32SectorSize = 0;

	// Temporary do not allow to erase the boot image )
#ifdef PROTECT_BOOTIMAGE
	if ( u32Sector < 0x2F )
	{
		KdPrint(("NucleonFlashWrite ERROR - invalid Flash Sector (0x%x)\n", u32Sector));
		return CS_FLASH_INVALID_SECTOR;
	}
#endif

	// Calculate the address of the sector
	if ( u32Sector < 4 )
	{
		u32SectorSize	= NUCLEONFLASH_SECTOR_SIZE0;
		u32Addr			= u32Sector*u32SectorSize;
	}
	else 
	{
		u32SectorSize	= NUCLEONFLASH_SECTOR_SIZE1;
		u32Addr			= (u32Sector-4)*u32SectorSize + 4*NUCLEONFLASH_SECTOR_SIZE0;
	}

	u32WordAddr = u32Addr/2;

	if ( u32Offset + u32BufferSize > u32SectorSize )
	{
		KdPrint(("NucleonFlashWrite ERROR - invalid Offset (0x%x) or BufferSize (0x%x)\n", u32Offset, u32BufferSize ));
		return CS_FLASH_SECTORCROSS_ERROR;
	}

	i32Status = FlashSectorErase(FdoData, u32WordAddr);
	if ( CS_SUCCESS != i32Status )
	{
		HardResetFlash(FdoData);
		return i32Status;
	}

	// Offser in word
	u32Offset = u32Offset>>1;

	i = 0;
	while (u32Remain >= 32)
	{
		u32Temp = *pu16Buffer;
		i32Status = FlashProgramWord32(FdoData, u32WordAddr+u32Offset+i, u32Addr>>1, pu16Buffer, 32*sizeof(uInt16) );
		if ( CS_SUCCESS != i32Status )
		{
			HardResetFlash(FdoData);
			return i32Status;
		}

		pu16Buffer +=32;
		i += 32;
		u32Remain -=32;
	}

	while (u32Remain > 0)
	{
		u32Temp = *pu16Buffer;
		i32Status = FlashProgramWord(FdoData, u32WordAddr+u32Offset+i, u32Temp );
		if ( CS_SUCCESS != i32Status )
		{
			HardResetFlash(FdoData);
			return i32Status;
		}

		pu16Buffer++;
		i++;
		u32Remain --;
	}

	return CS_SUCCESS;
}