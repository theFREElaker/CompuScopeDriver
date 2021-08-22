
#include "StdAfx.h"
#include "CsTypes.h"
#include "CsPlxBase.h"
#include "CsPlxDefines.h"
#include "CsPrivateStruct.h"
#include "GageWrapper.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

//-----------------------------------------------------------------------------
//---------------- CODE FOR FLASH AM29LV033C -----------------
//-----------------------------------------------------------------------------
FlashInfo FlashDesc[] =
{	//---- Am29LV033C description
	{(int8*)"Am29LV033C", 0L, 0, 64, 1, 0 , 0x400000, 0x10000,
		{
		{65536,     0x000000,    1},
		{65536,     0x010000,    1},
		{65536,     0x020000,    1},
		{65536,     0x030000,    1},
		{65536,     0x040000,    1},
		{65536,     0x050000,    1},
		{65536,     0x060000,    1},
		{65536,     0x070000,    1},
		{65536,     0x080000,    1},
		{65536,     0x090000,    1},
		{65536,     0x0A0000,    1},
		{65536,     0x0B0000,    1},
		{65536,     0x0C0000,    1},
		{65536,     0x0D0000,    1},
		{65536,     0x0E0000,    1},
		{65536,     0x0F0000,    1},
		{65536,     0x100000,    1},
		{65536,     0x110000,    1},
		{65536,     0x120000,    1},
		{65536,     0x130000,    1},
		{65536,     0x140000,    1},
		{65536,     0x150000,    1},
		{65536,     0x160000,    1},
		{65536,     0x170000,    1},
		{65536,     0x180000,    1},
		{65536,     0x190000,    1},
		{65536,     0x1A0000,    1},
		{65536,     0x1B0000,    1},
		{65536,     0x1C0000,    1},
		{65536,     0x1D0000,    1},
		{65536,     0x1E0000,    1},
		{65536,     0x1F0000,    1},
		{65536,     0x200000,    1},
		{65536,     0x210000,    1},
		{65536,     0x220000,    1},
		{65536,     0x230000,    1},
		{65536,     0x240000,    1},
		{65536,     0x250000,    1},
		{65536,     0x260000,    1},
		{65536,     0x270000,    1},
		{65536,     0x280000,    1},
		{65536,     0x290000,    1},
		{65536,     0x2A0000,    1},
		{65536,     0x2B0000,    1},
		{65536,     0x2C0000,    1},
		{65536,     0x2D0000,    1},
		{65536,     0x2E0000,    1},
		{65536,     0x2F0000,    1},
		{65536,     0x300000,    1},
		{65536,     0x310000,    1},
		{65536,     0x320000,    1},
		{65536,     0x330000,    1},
		{65536,     0x340000,    1},
		{65536,     0x350000,    1},
		{65536,     0x360000,    1},
		{65536,     0x370000,    1},
		{65536,     0x380000,    1},
		{65536,     0x390000,    1},
		{65536,     0x3A0000,    1},
		{65536,     0x3B0000,    1},
		{65536,     0x3C0000,    1},
		{65536,     0x3D0000,    1},
		{65536,     0x3E0000,    1},
		{65536,     0x3F0000,    1}
		}
	}
};

//-----------------------------------------------------------------------------
// Function   :  init_flash
// Description:  initializes setting of parameters and the appropriate sector table
//-----------------------------------------------------------------------------
void  CsPlxBase:: InitFlash(uInt32 FlashIndex)
{
	FlashPtr = &FlashDesc[FlashIndex];
}

//-----------------------------------------------------------------------------
// Function:  FlashCommand
// Description:  It performs every possible command available to AMD B revision flash parts.
// Note: for the Am29LV033C the address of the command "don't care", but it could mind for other type
// So if one day we add another type of AMD flash we may have to set the Address for the command,
// but it will not affect the Am29LV033C
//-----------------------------------------------------------------------------

void  CsPlxBase::FlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data)
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
			SendFlashCmd(FlashPtr->sec[Sector].base, 0xF0);   // assume reset device to read mode
		} else if (Command == FLASH_ERASE_SUSPEND) {
			SendFlashCmd(FlashPtr->sec[Sector].base, 0xB0);   // suspend sector erase
		} else if (Command == FLASH_ERASE_RESUME) {
			SendFlashCmd(FlashPtr->sec[Sector].base, 0x30);   // resume suspended sector erase
		} else if (Command == FLASH_UNLOCK_BYP_PROG) {
			SendFlashCmd(FlashPtr->sec[Sector].base, 0xA0);
			SendFlashCmd(FlashPtr->sec[Sector].base + Offset, (uInt8)Data);
		} else if (Command == FLASH_UNLOCK_BYP_RESET) {
			SendFlashCmd(FlashPtr->sec[Sector].base, 0x90);
			SendFlashCmd(FlashPtr->sec[Sector].base, 0x0);
		}
		else {
			SendFlashCmd(FlashPtr->sec[Sector].base, 0xAA);	// unlock 1
			SendFlashCmd(FlashPtr->sec[Sector].base, 0x55);   // unlock 2
			switch (Command) {
				case FLASH_AUTOSEL:
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x90);
					break;
				case FLASH_PROG:
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0xA0);
					SendFlashCmd(FlashPtr->sec[Sector].base + Offset, (uInt8)Data);
					break;
				case FLASH_UNLOCK_BYP:
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x20);
					break;
				case FLASH_CHIP_ERASE:
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x80);
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0xAA);
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x55);
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x10);
					break;
				case FLASH_SECTOR_ERASE:
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x80);
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0xAA);
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x55);
	 				SendFlashCmd(FlashPtr->sec[Sector].base, 0x30);
					break;
			}
		}

	} while(Retry-- > 0 && (FlashStatus(FlashPtr->sec[Sector].base) == STATUS_READY));
}


//-----------------------------------------------------------------------------
// Function   : FlashStatus(Addr)
// Description:  utilizes the DQ6, DQ5, and DQ3 polling algorithms
// described in the flash data book.  It can quickly ascertain the
// operational status of the flash device, and return an
// appropriate status code
//-----------------------------------------------------------------------------
uInt32  CsPlxBase::FlashStatus(uInt32 Addr) //----- ICI NAT to be reviewed
{
	uInt8 d, t;
	int8	Retry = 1;

	do
	{
		d = (uInt8) ReadFlashByte(Addr); // read data
		t = (uInt8)(d ^ ReadFlashByte(Addr));    // read it again and see what toggled

		if (t == 0) {           // no toggles, nothing's happening, actually done
			return STATUS_READY;
		}
		else if (t == 0x04) { // erase-suspend
			return STATUS_ERASE_SUSP;
		}
		else if (t & 0x40) {
			if (d & 0x20) {     // timeout
			return STATUS_FLASH_TIMEOUT;
			}
			else {
			return STATUS_FLASH_BUSY;
			}
		}
	}while (Retry-- >= 0);

	if (Retry < 0)
		return STATUS_FLASH_BUSY;

	return STATUS_READY;
}

//-----------------------------------------------------------------------------
// Function   :  FlashReset
// Description:  Send the command to reset the Flash - Ready to read
//-----------------------------------------------------------------------------
void  CsPlxBase::FlashReset()
{
	FlashCommand(FLASH_RESET,0,0,0);
}

//-----------------------------------------------------------------------------
// Function   :  FlashWriteByte
// Description:  Send the command to write One byte
//-----------------------------------------------------------------------------
void  CsPlxBase::FlashWriteByte(uInt32 Addr, uInt32 Data )
{
	uInt32 Sector, Offset;

	Sector = Addr / FlashPtr->sec[0].size; //---- True since all the sectors are the same size
	Offset = Addr % FlashPtr->sec[0].size;

	FlashCommand(FLASH_PROG, Sector, Offset, Data);
}

//-----------------------------------------------------------------------------
// Function   :  FlashChipErase
// Description:  Erase the entire Chip
//-----------------------------------------------------------------------------
void  CsPlxBase::FlashChipErase()
{

	FlashCommand(FLASH_CHIP_ERASE,0,0,0);

	CSTIMER		CsTimer;
	CsTimer.Set(CS_ERASE_WAIT_TIMEOUT);
	while ((FlashStatus(FlashPtr->sec[0].base)) == STATUS_FLASH_BUSY)
	{
		if ( CsTimer.State() )
		{
			if( (FlashStatus(FlashPtr->sec[0].base)) == STATUS_FLASH_BUSY )
				m_PlxData->bFlashReset = TRUE;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Function   :  FlashSectorErase
// Description:  Erase the entire Chip
//-----------------------------------------------------------------------------
void  CsPlxBase::FlashSectorErase(uInt32 Sector)
{

	FlashCommand(FLASH_SECTOR_ERASE, Sector,0,0);

	CSTIMER		CsTimer;
	CsTimer.Set(CS_ERASE_WAIT_TIMEOUT);
	while ((FlashStatus(FlashPtr->sec[Sector].base)) == STATUS_FLASH_BUSY)
	{
		if ( CsTimer.State() )
		{
			if( (FlashStatus(FlashPtr->sec[Sector].base)) == STATUS_FLASH_BUSY )
				m_PlxData->bFlashReset = TRUE;
			break;
		}
	}
}

#define MAX_WRITE_RETRY 200

//----- INTERFACING THE ADD-ON AND FLASH ACCESS
//-----------------------------------------------------------------------------
// Function   : WriteEntireFlash
// Description: Erase the Flash, then Write the complete Flash
//-----------------------------------------------------------------------------
int32	CsPlxBase::WriteEntireFlash(BaseBoardFlash32Mbit *Flash)
{
	uInt8	*f_ptr = (uInt8 *)(Flash);
	uInt32	i;
	uInt8	n;
	uInt32	u32RetryCount = 0;

	FlashChipErase();

	if ( m_PlxData->bFlashReset )
 		ResetFlash();

	for (i = 0 ; i < sizeof(BaseBoardFlash32Mbit) ; i++)//----- Byte aligned
	{
		FlashWriteByte(i, f_ptr[i]);

		FlashCommand(FLASH_RESET,0,0,0);
		n = (uInt8)( ReadFlashByte(i));
		if (n != f_ptr[i])
		{
			i--;//retry
			if ( u32RetryCount++ > MAX_WRITE_RETRY )
			{
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "BaseBoard WriteFlash fatal error." );
				return CS_MISC_ERROR;
			}
		}
		else
			u32RetryCount = 0;
	}

	if ( m_PlxData->bFlashReset )
		ResetFlash();

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
// Function   : WriteFlashEx
// Description: Erase the sectors then Write Contiguous Sectors of the  Flash
//-----------------------------------------------------------------------------

int32	CsPlxBase::WriteFlashEx(void* Flash, uInt32 Addr, uInt32 Size, FlashAccess flSequence )
{
	int32	i32Status = CS_SUCCESS;
	uInt32	i;
	uInt32  FirstSector, LastSector;

	if ( AccessAuto == flSequence || AccessFirst == flSequence )
		InitFlash(0);

	if ((Addr == 0) && (Size == sizeof(BaseBoardFlash32Mbit)))
	{
		return WriteEntireFlash((BaseBoardFlash32Mbit *)Flash);
	}
	else
	{
		//----- erase the sectors
		FirstSector = Addr / SECTOR_SIZE;
		LastSector = (Addr + Size) / SECTOR_SIZE;

		uInt32	u32UpdateSize = (LastSector - FirstSector + 1) * SECTOR_SIZE;
		uInt32	u32Offset = Addr % SECTOR_SIZE;

		uInt8 *pu8Buffer = (uInt8 *) ::GageAllocateMemoryX( u32UpdateSize );
		if ( ! pu8Buffer )
			return CS_INSUFFICIENT_RESOURCES;

		IoctlReadFlash( FirstSector * SECTOR_SIZE, pu8Buffer,  u32UpdateSize);
		memcpy( &pu8Buffer[u32Offset], Flash, Size );

		uInt8 *pu8Temp = pu8Buffer;
		for(i = FirstSector; i <= LastSector; i++)
		{
			FlashSectorErase(i);
			Sleep(500);
			Sleep(500);
			Sleep(200);

			i32Status = IoctlWriteFlash(i, 0, pu8Temp, SECTOR_SIZE );
			pu8Temp += SECTOR_SIZE;
		}

		::GageFreeMemoryX(pu8Buffer);
	}

	if ( AccessAuto == flSequence || AccessLast == flSequence )
	{
		if ( m_PlxData->bFlashReset )
			ResetFlash();

		//----- Reset the pointer to the default image
		BaseBoardConfigReset(0);
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
// Function   : ReadFlash
// Description:
//-----------------------------------------------------------------------------
int32   CsPlxBase::ReadFlash(void *Flash, uInt32 Addr, uInt32 Size)
{
	uInt8	*f_ptr = (uInt8 *)(Flash);
	uInt32  i;

	InitFlash(0);
	if ( m_PlxData->bFlashReset )
		ResetFlash();

	for (i = Addr ; i < Size + Addr ; i++)
	{
		*(f_ptr) = (uInt8)( ReadFlashByte(i));
		f_ptr++;
	}

	return CS_SUCCESS;
}


//---------------
// Function   : ReadFlashByte
// Description:
// Flash read custom implementation
// Flash access is 32 bits aligned
// the LSByte is data, upper bytes don't care
//---------------

uInt32 CsPlxBase::ReadFlashByte(uInt32 Addr)
{
	uInt32 u32Data;

	//---- write host control bit
	WriteFlashRegister(COMMAND_REG_PLD, HOST_CTRL);

	//---- write flash location address[7..0]
	WriteFlashRegister(FLASH_ADD_REG_1,Addr & 0xFF);
	//---- write flash location address[15..8]
	WriteFlashRegister(FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[21..16]
	WriteFlashRegister(FLASH_ADD_REG_3, (Addr >> 16) & 0x3F);

	//---- send the read command to the PLD
	WriteFlashRegister(COMMAND_REG_PLD, HOST_CTRL|FCE_HOST|FOE_HOST);

	u32Data = (uInt8)((ReadFlashRegister(FLASH_DATA_REGISTER))& 0xFF);

	//---- write host control bit
	WriteFlashRegister(COMMAND_REG_PLD, HOST_CTRL);

	return ((uInt8)(u32Data));
}



//---------------
// Function   : SendFlashCmd
// Description:
// Flash write custom implementation
// Flash access is 32 bits aligned
// the LSByte is data, upper bytes don't care
//---------------

void CsPlxBase::SendFlashCmd(uInt32 Addr, uInt32 Data)
{
	//---- write host control bit
	WriteFlashRegister(COMMAND_REG_PLD, SEND_HOST_CMD);

	//---- write flash location address[7..0]
	WriteFlashRegister(FLASH_ADD_REG_1, Addr & 0xFF);
	//---- write flash location address[15..8]
	WriteFlashRegister(FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[21..16]
	WriteFlashRegister(FLASH_ADD_REG_3, (Addr >> 16) & 0x3F);

	//---- Write Data register
	WriteFlashRegister(FLASH_DATA_REGISTER, (uInt8)(Data));

	//---- send the write command to the PLD  ---  need a rising edge on FWE_REG
	WriteFlashRegister(COMMAND_REG_PLD,SEND_WRITE_CMD);

	//---- write host control bit
	WriteFlashRegister(COMMAND_REG_PLD, SEND_HOST_CMD);
}


//---------------
// Function   : ResetFlash
// Description:	Hard reset the flash without giving up host control
//---------------
uInt32 CsPlxBase::ResetFlash(void)
{
	uInt32 Status = 0;

	WriteFlashRegister(COMMAND_REG_PLD, RESET_INT);
	WriteFlashRegister(COMMAND_REG_PLD, RESET_INT);
	WriteFlashRegister(COMMAND_REG_PLD,  0);

	m_PlxData->bFlashReset = FALSE;

	return Status;
}
