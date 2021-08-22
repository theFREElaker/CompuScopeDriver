#ifdef _WIN32
#include <ntddk.h>
#include "gagedrv.h"
#endif
#include "CsTypes.h"
#include "CsPrivateStruct.h"
#include "CsPlxDefines.h"
#include "NiosApi.h"

#define		FASTBALL_FLASH_TIMEOUT			KTIME_MILISEC(500)

//----- CONSTANTS
//---- maximum number of sectors supported
#define MAXSECTORS  64
#define SECTOR_SIZE	0x10000

//---- Command codes for the flash_command routine
#define FLASH_SELECT			0       //---- no command; just perform the mapping
#define FLASH_RESET				1       //---- reset to read mode
#define FLASH_READ				1       //---- reset to read mode, by any other name
#define FLASH_AUTOSEL			2       //---- autoselect (fake Vid on pin 9)
#define FLASH_PROG				3       //---- program a word
#define FLASH_CHIP_ERASE		4       //---- chip erase
#define FLASH_SECTOR_ERASE		5       //---- sector erase
#define FLASH_ERASE_SUSPEND		6       //---- suspend sector erase
#define FLASH_ERASE_RESUME		7       //---- resume sector erase
#define FLASH_UNLOCK_BYP		8       //---- go into unlock bypass mode
#define FLASH_UNLOCK_BYP_PROG	9       //---- program a word using unlock bypass
#define FLASH_UNLOCK_BYP_RESET	10      //---- reset to read mode from unlock bypass mode
#define FLASH_LASTCMD			10      //---- used for parameter checking

//---- Return codes from flash_status
#define STATUS_READY			0       //---- ready for action
#define STATUS_FLASH_BUSY		1       //---- operation in progress
#define STATUS_ERASE_SUSP		2		//---- erase suspended
#define STATUS_FLASH_TIMEOUT	3       //---- operation timed out
#define STATUS_ERROR			4       //---- unclassified but unhappy status

//---- AMD's manufacturer ID
#define AMDPART   	0x01

//---- AMD device ID's
#define ID_AM29LV033C   0xA3

//---- Index for a given type in the table FlashInfo
#define AM29LV033C	0

//---- Retry count for command, could be specific for each command
#define COMMON_RETRY	15//----- CONSTANTS
//---- maximum number of sectors supported
#define PLXBASEFLASH_MAXSECTORS		64
#define PLXBASEFLASH_SECTORSIZE		0x10000
#define	PLXBASEFLASHSIZE			0x800000


#define MAX_WRITE_RETRY 500


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt32 ReadFlashByte( PFDO_DATA	FdoData, uInt32 Addr )
{
	uInt32 u32Data;

	//---- write host control bit
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, HOST_CTRL);

	//---- write flash location address[7..0]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_1,Addr & 0xFF);
	//---- write flash location address[15..8]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[21..16]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_3, (Addr >> 16) & 0xFF);

	//---- send the read command to the PLD
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, HOST_CTRL|FCE_HOST|FOE_HOST);

	u32Data = (uInt8)((ReadFlashRegister(FdoData, FLASH_DATA_REGISTER))& 0xFF);

	//---- write host control bit
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, HOST_CTRL);

	return ((uInt8)(u32Data));
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void WriteFlashByte(PFDO_DATA	FdoData, uInt32 Addr, uInt32 Data )
{
	//---- write host control bit
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, SEND_HOST_CMD);

	//---- write flash location address[7..0]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_1, Addr & 0xFF);
	//---- write flash location address[15..8]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[21..16]
	WriteFlashRegister(FdoData, FLASH_ADD_REG_3, (Addr >> 16) & 0xFF);

	//---- Write Data register
	WriteFlashRegister(FdoData, FLASH_DATA_REGISTER, (uInt8)(Data));

	//---- send the write command to the PLD  ---  need a rising edge on FWE_REG
	WriteFlashRegister(FdoData, COMMAND_REG_PLD,SEND_WRITE_CMD);

	//---- write host control bit
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, SEND_HOST_CMD);

}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt32 ResetFlash( PFDO_DATA	FdoData )
{
	uInt32 Status = 0;

	WriteFlashRegister(FdoData, COMMAND_REG_PLD, RESET_INT);
	WriteFlashRegister(FdoData, COMMAND_REG_PLD, RESET_INT);
	WriteFlashRegister(FdoData, COMMAND_REG_PLD,  0);

//	m_ShPlxData->bFlashReset = FALSE;

	return Status;
}


//-----------------------------------------------------------------------------
// Function   : FlashStatus(Addr)
// Description:  utilizes the DQ6, DQ5, and DQ3 polling algorithms
// described in the flash data book.  It can quickly ascertain the
// operational status of the flash device, and return an
// appropriate status code
//-----------------------------------------------------------------------------
uInt32  FlashStatus(PFDO_DATA FdoData, uInt32 Addr)
{
	uInt8 d, t;
	int8	Retry = 1;

	do
	{
		d = (uInt8) ReadFlashByte(FdoData, Addr);		  // read data
		t = (uInt8)(d ^ ReadFlashByte(FdoData, Addr));    // read it again and see what toggled

		if (t == 0)            // no toggles, nothing's happening, actually done
			return STATUS_READY;
		
		else if (t == 0x04)  // erase-suspend
			return STATUS_ERASE_SUSP;
		else if (t & 0x40)
		{
			if (d & 0x20)    // timeout
				return STATUS_FLASH_TIMEOUT;
			else
			return STATUS_FLASH_BUSY;
		}
	}while (Retry-- >= 0);

	if (Retry < 0)
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
}


//-----------------------------------------------------------------------------
// Function:  FlashCommand
// Description:  It performs every possible command available to AMD B revision flash parts.
// Note: for the Am29LV033C the address of the command "don't care", but it could mind for other type
// So if one day we add another type of AMD flash we may have to set the Address for the command,
// but it will not affect the Am29LV033C
//-----------------------------------------------------------------------------

void  FlashCommand(PFDO_DATA FdoData, uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data)
{
//---------------
// On systems where bus glitching is prevalent, some long command
// strings may be interrupted and cause the command to fail (this
// is most probable on six cycle commands such as chip erase). In
// order to ensure that flash_command executes the command
// properly, it may be necessary to issue the command more than
// once in order for it to be accepted by the flash device.  In
// these cases it is recommended that the retry number be made
// positive (such as 1 or 2), so that flash_command will try
// to issue the command more than once.  Keep in mind that this
// will only be attempted if the command fails in the first
// attempt.
//-----------------------------------------------------------------------------
	uInt32	u32Addr = Sector*PLXBASEFLASH_SECTORSIZE;
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
		if (Command == FLASH_SELECT) {
			return;
		} else if (Command == FLASH_RESET || Command > FLASH_LASTCMD) {
			WriteFlashByte(FdoData, u32Addr, 0xF0);   // assume reset device to read mode
		} else if (Command == FLASH_ERASE_SUSPEND) {
			WriteFlashByte(FdoData, u32Addr, 0xB0);   // suspend sector erase
		} else if (Command == FLASH_ERASE_RESUME) {
			WriteFlashByte(FdoData, u32Addr, 0x30);   // resume suspended sector erase
		} else if (Command == FLASH_UNLOCK_BYP_PROG) {
			WriteFlashByte(FdoData, u32Addr, 0xA0);
			WriteFlashByte(FdoData, u32Addr + Offset, (uInt8)Data);
		} else if (Command == FLASH_UNLOCK_BYP_RESET) {
			WriteFlashByte(FdoData, u32Addr, 0x90);
			WriteFlashByte(FdoData, u32Addr, 0x0);
		}
		else {

			WriteFlashByte(FdoData, u32Addr, 0xAA);	// unlock 1
			WriteFlashByte(FdoData, u32Addr, 0x55);   // unlock 2
			switch (Command) {
				case FLASH_AUTOSEL:
	 				WriteFlashByte(FdoData, u32Addr, 0x90);
					break;
				case FLASH_PROG:
	 				WriteFlashByte(FdoData, u32Addr, 0xA0);
					WriteFlashByte(FdoData, u32Addr + Offset, (uInt8)Data);
					break;
				case FLASH_UNLOCK_BYP:
	 				WriteFlashByte(FdoData, u32Addr, 0x20);
					break;
				case FLASH_CHIP_ERASE:
	 				WriteFlashByte(FdoData, u32Addr, 0x80);
	 				WriteFlashByte(FdoData, u32Addr, 0xAA);
	 				WriteFlashByte(FdoData, u32Addr, 0x55);
	 				WriteFlashByte(FdoData, u32Addr, 0x10);
					break;
				case FLASH_SECTOR_ERASE:
	 				WriteFlashByte(FdoData, u32Addr, 0x80);
	 				WriteFlashByte(FdoData, u32Addr, 0xAA);
	 				WriteFlashByte(FdoData, u32Addr, 0x55);
	 				WriteFlashByte(FdoData, u32Addr, 0x30);
					break;
			}
		}

	} while(Retry-- > 0 && (FlashStatus(FdoData, u32Addr) == STATUS_READY));
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   PlxFlashRead( PFDO_DATA FdoData,  uInt32 u32Addr, void *pBuffer,  uInt32 u32BufferSize)
{
	uInt8	*f_ptr = (uInt8 *)(pBuffer);
	uInt32  i;

//	if ( FdoData->CardState.bFlashReset )
//		ResetFlash( FdoData );

	for (i = u32Addr ; i < u32BufferSize + u32Addr ; i++)
	{
		*(f_ptr) = (uInt8)( ReadFlashByte(FdoData, i));
		f_ptr++;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  PlxFlashWrite(PFDO_DATA FdoData, uInt32 u32Sector, uInt32 u32Offset, void *pData, uInt32 u32Size)
{
	uInt32	i = 0;
	uInt8	*ptr = (uInt8 *)(pData);
	uInt32	u32StartAddr;
	uInt8	u8Test;
	uInt32	u32RetryCount = 0;

	u32StartAddr = u32Sector*PLXBASEFLASH_SECTORSIZE + u32Offset;


	for (i = 0; i < u32Size; )	//----- Byte aligned
	{
		u32Sector = (i+u32StartAddr)/PLXBASEFLASH_SECTORSIZE;
		u32Offset = (i+u32StartAddr)%PLXBASEFLASH_SECTORSIZE;

		FlashCommand(FdoData, FLASH_PROG, u32Sector, u32Offset, ptr[i]);
		FlashCommand(FdoData, FLASH_RESET,0,0,0);
		u8Test = (uInt8) ReadFlashByte(FdoData, i+u32StartAddr);
		if ( u8Test != ptr[i] )
		{
			if ( u32RetryCount++ > MAX_WRITE_RETRY )
				return CS_MISC_ERROR;
		}
		else
		{
			u32RetryCount = 0;
			i++;
		}
	}

	return CS_SUCCESS;
}
