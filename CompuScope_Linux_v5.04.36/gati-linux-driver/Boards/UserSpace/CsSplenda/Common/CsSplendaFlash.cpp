
#include "StdAfx.h"
#include "CsTypes.h"
#include "CsSplenda.h"
#include "CsPlxDefines.h"
#include "CsPrivateStruct.h"
#include "CsSplendaFlash.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

//-------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------

uInt32 CsSplendaDev::AddonResetFlash(void)
{
	// Althought we not not need this function for Splenda cards, the impelentation
	// is still needed because it is a pure virtual function in CsPlxbase class

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
// Function   : WriteEepromEx
// Description: 
//-----------------------------------------------------------------------------
int32  CsSplendaDev::WriteEepromEx(void *pData, uInt32 Addr, uInt32 Size,  bool bErrorLog )
{
	CSTIMER		CsTimer;

	if ( ! m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! m_PlxData->InitStatus.Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	// Allow access to ADDON thru the CPLD
	WriteGIO(CSPLNDA_ADDON_FLASH_CMD, CSPLNDA_ADDON_ACCESS_THRU_CPLD);
//	AddonFlashWriteEnable();

	unsigned char* pcData = (unsigned char*)pData;
	unsigned int delay = 0;
	uInt32 u32Status = 0;

	for (uInt32 i = 0; i < Size; i++)
	{
		AddonFlashWriteEnable();
		CsTimer.Set(CS_WAIT_TIMEOUT);
		// wait until it's safe to write
		do
		{
			u32Status = AddonFlashReadStatus();
			if ( CsTimer.State() )
			{
				u32Status = AddonFlashReadStatus();
				if((u32Status & CSPLNDA_ADDON_FLASH_WR_STATUS_MASK) != CSPLNDA_ADDON_FLASH_WR_ENABLED )
				{
					if ( bErrorLog )
					{
						CsEventLog EventLog;
						EventLog.Report( CS_ERROR_TEXT, "AddonFlash: CSPLNDA_ADDON_READ_STATUS Timeout" );
					}
					return CS_EEPROM_WRITE_TIMEOUT;
				}
			}
		}while ((u32Status & CSPLNDA_ADDON_FLASH_WR_STATUS_MASK) != CSPLNDA_ADDON_FLASH_WR_ENABLED);//not ready or not write enable
		
		uInt32 Data = (uInt32)(*pcData);
		AddonWriteFlashByte(Addr, Data);
		
		delay = 0;
		CsTimer.Set(CS_WAIT_TIMEOUT);
		// wait until write is done
		do{
			u32Status = AddonFlashReadStatus();
			if ( CsTimer.State() )
			{
				u32Status = AddonFlashReadStatus();
				if(u32Status & CSPLNDA_ADDON_FLASH_WRITE_BUSY)
				{
					CsEventLog EventLog;
					EventLog.Report( CS_ERROR_TEXT, "AddonFlash: CSPLNDA_ADDON_READ_STATUS Timeout" );	
					return CS_EEPROM_WRITE_TIMEOUT;
				}
			}
		}while (u32Status & CSPLNDA_ADDON_FLASH_WRITE_BUSY);	//write in progress
		
		pcData++;
		Addr++;
	}
	// turn off write enable
	AddonFlashResetWriteEnable(); 
	WriteGIO(CSPLNDA_ADDON_FLASH_CMD, CSPLNDA_ADDON_ACCESS_DISABLE);
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Function   : ReadEeprom 
// Description: 
//-----------------------------------------------------------------------------
int32   CsSplendaDev::ReadEeprom(void *Eeprom, uInt32 Addr, uInt32 Size)
{
	uInt8	*f_ptr = (uInt8 *)(Eeprom);
	uInt32  i;

	if ( ! m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( ! m_PlxData->InitStatus.Info.u32AddOnPresent )
		return CS_ADDON_NOT_CONNECTED;

	WriteGIO(CSPLNDA_ADDON_FLASH_CMD, CSPLNDA_ADDON_ACCESS_THRU_CPLD);
	AddonFlashWriteEnable();

	for (i = Addr ; i < Size + Addr  ; i++)
	{
		*(f_ptr) = (uInt8) AddonReadFlashByte(i);
		f_ptr++;
	}
	AddonFlashResetWriteEnable();

	WriteGIO(CSPLNDA_ADDON_FLASH_CMD, CSPLNDA_ADDON_ACCESS_DISABLE);
	return CS_SUCCESS;
}

//---------------
// Function   : AddonFlashWriteEnable 
// Description:  
// Enable write acess to Flash
//---------------
int32 CsSplendaDev::AddonFlashWriteEnable()
{
	int cmd = CSPLNDA_WRITE_SPI_FLASH_CMD | (CSPLNDA_ADDON_WRITE_ENABLE << 8);
	int32 i32Ret = WriteRegisterThruNios(0, cmd);
	return i32Ret;
}

//---------------
// Function   : AddonFlashResetWriteEnable 
// Description:  
// Reset write acess to Flash
//---------------
int32 CsSplendaDev::AddonFlashResetWriteEnable()
{
	uInt32 u32Cmd = CSPLNDA_WRITE_SPI_FLASH_CMD | (CSPLNDA_ADDON_RESET_WRITE_ENABLE << 8);
	int32 i32Ret = WriteRegisterThruNios(0, u32Cmd);
	return i32Ret;
}

//---------------
// Function   : AddonFlashReadStatus
// Description:  
// Reads the status of the Addon flash
//---------------
uInt32 CsSplendaDev::AddonFlashReadStatus()
{
	uInt32 u32Cmd = CSPLNDA_WRITE_SPI_FLASH_CMD | (CSPLNDA_ADDON_READ_STATUS << 8); 
	return (ReadRegisterFromNios(0, u32Cmd));
}

//---------------
// Function   : ReadFlashByte 
// Description:  
// Flash read custom implementation						
// Flash access is 32 bits aligned						
// the LSByte is data, upper bytes don't care			    
//---------------

uInt32 CsSplendaDev::AddonReadFlashByte(uInt32 Addr)
{
	uInt32 u32Cmd = CSPLNDA_WRITE_SPI_FLASH_CMD | (CSPLNDA_ADDON_READ << 8);
	uInt32 u32Data = (Addr & 0xffff) << 8;
	
	return (ReadRegisterFromNios(u32Data, u32Cmd));
}

//---------------
// Function   : WriteFlashByte 
// Description:  
// Flash write custom implementation						
// Flash access is 32 bits aligned						
// the LSByte is data, upper bytes don't care			    
//---------------

void CsSplendaDev::AddonWriteFlashByte(uInt32 Addr, uInt32 Data)
{
	uInt32 u32Cmd = CSPLNDA_WRITE_SPI_FLASH_CMD | (CSPLNDA_ADDON_WRITE << 8);
	uInt32 u32Data = Data & 0xff;
	u32Data |= (Addr & 0xffff) << 8;
	WriteRegisterThruNios(u32Data, u32Cmd);
}

//---------------
// Function   : ConfigReset
// Description:	Reinit the cfg state machine: set the address pointer before
//---------------
int32 CsSplendaDev::AddonConfigReset(uInt32 Addr)
{
	unsigned int status = 0;
	CSTIMER		CsTimer;

	UNREFERENCED_PARAMETER(Addr);

	WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_NCONFIG | CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE);
	CsTimer.Set(CS_WAIT_TIMEOUT);
	do
	{
		status = ReadGageRegister(NIOS_GEN_STATUS);
		if ( CsTimer.State() )
			break;
	} while(!(status & CSPLNDA_CFG_STATUS));

	
	status = ReadGageRegister(NIOS_GEN_STATUS);
	WriteGageRegister(GEN_COMMAND_R_REG, 0);	
	if ( status & CSPLNDA_CFG_STATUS )
		return CS_SUCCESS;
	else
		return CS_ADDONINIT_ERROR; 
}

