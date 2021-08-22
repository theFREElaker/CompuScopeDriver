
#include "StdAfx.h"

#ifdef __linux__
	#include "DllStruct.h"		// needed for FILED_OFFSET macro
#endif

#include "CsTypes.h"
#include "CsDefines.h"
#include "CsPlxDefines.h"
#include "GageWrapper.h"
#include "CsPlxBase.h"
#include "CsiHeader.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 	CsPlxBase::Initialized()
{
	// Read the PCI configuration Registers
	return ReadPciConfigSpace( &m_PlxData->PCIConfigHeader, sizeof(PCI_CONFIG_HEADER_0),  0 );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt16	CsPlxBase::GetDeviceId()
{
	return m_PlxData->PCIConfigHeader.DeviceID;
}


//-----------------------------------------------------------------------------
//	
// The configuration space of a PCI device is a total of 256 bytes. The PCI specification
// defines the format of the first 64 bytes (cf. Type PCI_CONFIG_HEADER_0).
// The format of the remaining 192 bytes is device specific. This function is provided
// for accessing the device specific area of the configuration space.
//
//-----------------------------------------------------------------------------

void	CsPlxBase::WriteDeviceSpecificConfig( void *pBuffer, uInt32 u32Offset, uInt32 Count )
{
	WritePciConfigSpace( pBuffer, Count, u32Offset + 64 );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsPlxBase::ReadDeviceSpecificConfig( void *pBuffer, uInt32 u32Offset, uInt32 Count )
{
	ReadPciConfigSpace( pBuffer, Count, u32Offset + 64 );
}


//-----------------------------------------------------------------------------
//  Read data using the VPD
//-----------------------------------------------------------------------------

unsigned int CsPlxBase::PlxVpdRead(unsigned short Offset)
{
    unsigned int VpdDelay;
    unsigned int RegisterValue;

	unsigned int Tmp;

	if (Offset >= MAX_PLX_NVRAM_OFFSET)
		return FALSE;

	//----- Enable EEDO
	Tmp = ReadPciRegister(PCI9056_EEPROM_CTRL_STAT);
	Tmp |= 0x80000000;
	WritePciRegister(PCI9056_EEPROM_CTRL_STAT, Tmp);

	Tmp = ReadPciRegister(PCI9056_EEPROM_CTRL_STAT);
	Tmp &= 0x7FFFFFFF;
	WritePciRegister(PCI9056_EEPROM_CTRL_STAT, Tmp);

   //----- Prepare VPD command, address is Dword aligned, 0x7ffC, bit 31=0
    RegisterValue = (( unsigned int)(Offset & 0x7FFC) << 16) | 0x3;

    //----- Send VPD Command (address + Id)
	WriteDeviceSpecificConfig(&RegisterValue, PCI9056_VPD_CAP_ID, ONE_DWORD);


    //----- Delay for a bit for VPD operation
    VpdDelay = VPD_STATUS_POLL_DELAY;
    while (VpdDelay-- != 0);

    //----- Get VPD Status
	ReadDeviceSpecificConfig(&RegisterValue, PCI9056_VPD_CAP_ID, ONE_DWORD);
	while (!(RegisterValue & (1 << 31)))//----- Check for command completion
		ReadDeviceSpecificConfig(&RegisterValue, PCI9056_VPD_CAP_ID, ONE_DWORD);
	//----- The VPD successfully read the EEPROM. Get
    //----- the value & return a status of SUCCESS.
    //----- Get the VPD Data result
	ReadDeviceSpecificConfig(&RegisterValue, PCI9056_VPD_DATA, ONE_DWORD);

    return RegisterValue;
}


//-----------------------------------------------------------------------------
// Write data using the VPD
//-----------------------------------------------------------------------------

unsigned char CsPlxBase::PlxVpdWrite( unsigned short Offset, unsigned int VpdData)
{
	unsigned int VpdDelay;
    unsigned int RegisterValue;
	unsigned int Tmp;

	if (Offset >= MAX_PLX_NVRAM_OFFSET)
		return FALSE;
	
	//----- Enable EEDO
	Tmp = ReadPciRegister(PCI9056_EEPROM_CTRL_STAT);
	Tmp |= 0x80000000;
	WritePciRegister(PCI9056_EEPROM_CTRL_STAT, Tmp);
	//----- Enable EEDO
	Tmp = ReadPciRegister(PCI9056_EEPROM_CTRL_STAT);
	Tmp &= 0x7FFFFFFF;
	WritePciRegister(PCI9056_EEPROM_CTRL_STAT, Tmp);

	//----- if we want to write the write-protected area
	if ( Offset < MAX_PLX_NVRAM_OFFSET/2) 
	{

		Tmp = ReadPciRegister(PCI9056_ENDIAN_DESC);
		Tmp &= 0xFF00FFFF;
		WritePciRegister(PCI9056_ENDIAN_DESC, Tmp);
	}

    //----- Put write value into VPD Data register
	WriteDeviceSpecificConfig(&VpdData, PCI9056_VPD_DATA, ONE_DWORD);

    //----- Prepare VPD command
    RegisterValue = (1 << 31) | ((unsigned int)(Offset & 0xFFFC) << 16) | 0x3;

 
    //----- Send VPD command
	WriteDeviceSpecificConfig(&RegisterValue, PCI9056_VPD_CAP_ID, ONE_DWORD);

    //----- Delay for a bit for VPD operation
    VpdDelay = VPD_STATUS_POLL_DELAY;
    while (VpdDelay-- != 0);

    //----- Get VPD Status
    ReadDeviceSpecificConfig(&RegisterValue, PCI9056_VPD_CAP_ID, ONE_DWORD);
	while (!((RegisterValue & (1 << 31)) == 0))//----- Check for command completion
		ReadDeviceSpecificConfig(&RegisterValue, PCI9056_VPD_CAP_ID, ONE_DWORD);

    //----- The VPD successfully wrote to the EEPROM.
	if ( Offset < MAX_PLX_NVRAM_OFFSET/2) //----- restore the write-protected area
		{
			Tmp = ReadPciRegister(PCI9056_ENDIAN_DESC);
			Tmp |= 0x300000;
			WritePciRegister(PCI9056_ENDIAN_DESC, Tmp);
		}
    return TRUE;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsPlxBase::CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image )
{
	uInt16   *nvi_ptr = (uInt16 *)nvRAM_image;//----word aligned
	uInt32	EepromRead;
	uInt16  i = 0;
	BOOLEAN	bVirgin = TRUE;

	for (i = 0 ; i < sizeof(PLX_NVRAM_IMAGE) ; i = i+4)
	{
		EepromRead = (PlxVpdRead (i));

		if ( EepromRead != (uInt32)(-1) )
			bVirgin = FALSE;

		*nvi_ptr = (uInt16) (EepromRead & 0xFFFF);
		nvi_ptr++;
		*nvi_ptr = (uInt16) ((EepromRead >> 16) & 0xFFFF);
		nvi_ptr++;
	}

	if ( bVirgin )
		return CS_NVRAM_NOT_INIT;
	else
	{
		m_PlxData->u32NvRamSize = 0x2000000;

		if ( ( -1 != nvRAM_image->Data.MemorySize ) && ( 0 != nvRAM_image->Data.MemorySize ) )
			m_PlxData->u32NvRamSize = nvRAM_image->Data.MemorySize; //in kBytes

		return CS_SUCCESS;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsPlxBase::CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image )
{
	uInt32	*nvi_ptr = (uInt32 *)(nvRAM_image);//----- Dword aligned
	uInt16	i, j=0;
	int32	i32Status = CS_SUCCESS;
	
	for (i = 0 ; i < sizeof(PLX_NVRAM_IMAGE) ; i = i+4)//----- Dword aligned
	{
		i32Status = PlxVpdWrite (i, nvi_ptr[j++]);
		if ( i32Status != CS_SUCCESS )
			break;
	}

	return i32Status;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32  CsPlxBase::ReadRawMemorySize_Kb( uInt32	u32BaseBoardHwOptions )
{
	uInt32 u32DimmSize;		// in KBytes unit
	uInt32 u32Ret = ReadRegisterFromNios( 0, CSPLXBASE_READ_MCFG_CMD);

	u32BaseBoardHwOptions;

	// The value return from CSPLXBASE_READ_MCFG_CMD is in MBytes Total 
	u32DimmSize  = ((u32Ret >> 24) & 0xff) * RANK_WIDTH;
	u32DimmSize  = u32DimmSize << ( ((u32Ret >> 24) & 0x3) ? 8 : 0 );

	u32DimmSize *= (u32Ret & PLX_2_SODIMMS) == PLX_2_SODIMMS ? 2:1;
	u32DimmSize *= (u32Ret & PLX_2_RANKS) == PLX_2_RANKS ? 2:1;

	// Convert to KBytes unit
	u32DimmSize *= 1024;

	return u32DimmSize;		// in KBytes unit
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsPlxBase::UpdateMaxMemorySize( uInt32	u32BaseBoardHardwareOptions, uInt32 u32SampleSize )
{
	uInt32	u32DimmSize = ReadRawMemorySize_Kb( u32BaseBoardHardwareOptions ); //in kBytes
	uInt32	u32LimitRamSize = m_PlxData->u32NvRamSize;

	m_PlxData->CsBoard.i64MaxMemory = ( u32LimitRamSize < u32DimmSize ) ? u32LimitRamSize : u32DimmSize;
	m_PlxData->CsBoard.i64MaxMemory *= (1024 / u32SampleSize);				//in Samples
	m_PlxData->InitStatus.Info.u32BbMemTest = m_PlxData->CsBoard.i64MaxMemory != 0 ? 1 : 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsPlxBase::ClearInterrupts(void)
{
	//QQQWDK
//	if( m_PlxData->InitStatus.Info.u32Nios )			
	{
		//----- Clear INT_CFG => disable all the interrupts
		WriteGageRegister(INT_CFG, 0);
		m_PlxData->IntBaseMask =	ReadPciRegister(INT_CFG);
		m_PlxData->IntBaseMask = 0;

		//----- Clear INT_FIFO and INT_COUNT
		WriteGageRegister(MODESEL, INT_CLR);
		WriteGageRegister(MODESEL, 0);
	}
}


//------------------------------------------------------------------------------
//       EnableInterrupts: all the interrupts except DMA int
//------------------------------------------------------------------------------

void CsPlxBase::EnableInterrupts(uInt32 IntType)
{
	m_PlxData->IntBaseMask|= IntType;
	WriteGageRegister(INT_CFG, m_PlxData->IntBaseMask);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsPlxBase::DisableInterrupts(uInt32 IntType)
{
	m_PlxData->IntBaseMask &= ~IntType;
	WriteGageRegister(INT_CFG, m_PlxData->IntBaseMask);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------


void  CsPlxBase::EnableInterruptDMA0( BOOL bDmaDemandMode )
{	
	uInt32		u32Config = 0x00020FC3;

	if ( bDmaDemandMode )
		u32Config |= (1<< 12);

	WritePciRegister(PCI9056_DMA0_MODE, u32Config);
	PlxInterruptEnable(TRUE);
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  CsPlxBase::DisableInterruptDMA0( BOOL bDmaDemandMode )
{
	uInt32		u32Config = 0x00020BC3;

	if ( bDmaDemandMode )
		u32Config |= (1<< 12);

#ifdef USE_DMA_64
	// Enable the descriptor to load the PCI Dual Adsress Cycles value
	u32Config |= (1<< 18);
#endif

	WritePciRegister(PCI9056_DMA0_MODE, u32Config);;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  CsPlxBase::ClearInterruptDMA0()
{
	uInt32 u32Value = ReadPciRegister(PCI9056_DMA_COMMAND_STAT);

	// Set Dma Clear interrupt bit
	u32Value |= 0x8;

	WritePciRegister(PCI9056_DMA_COMMAND_STAT, u32Value);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	CsPlxBase::AbortDmaTransfer()
{
	uInt32 u32Value = ReadPciRegister(PCI9056_DMA_COMMAND_STAT);

	if ( u32Value & 0x10 )
		return;				// Dma done !! no need to abort

	// Clear Dma enable and start bits
	u32Value &= 0xFFFC;
	// set Abort bit
	u32Value |= 1 << 2;

	WritePciRegister(PCI9056_DMA_COMMAND_STAT, u32Value);
}

//---------------
// Function   : BaseBoardConfigReset
// Description:	Reinit the cfg state machine: set the address pointer before
//---------------

int32 CsPlxBase::BaseBoardConfigReset(uInt32 Addr)
{
	//---- write host control bit
	WriteFlashRegister(COMMAND_REG_PLD, SEND_HOST_CMD);

	//---- write flash location address[7..0]
	WriteFlashRegister(FLASH_ADD_REG_1,Addr & 0xFF);
	//---- write flash location address[15..8]
	WriteFlashRegister(FLASH_ADD_REG_2, (Addr >> 8) & 0xFF);
	//---- write flash location address[21..16]
	WriteFlashRegister(FLASH_ADD_REG_3, (Addr >> 16) & 0x3F);
	//---- send the read command to the PLD
	WriteFlashRegister(COMMAND_REG_PLD,SEND_HOST_CMD);
	WriteFlashRegister(COMMAND_REG_PLD,SEND_HOST_CMD);
	WriteFlashRegister(COMMAND_REG_PLD,SEND_HOST_CMD);

	ReadFlashRegister(STATUS_REG_PLX);

	//----- release control
	WriteFlashRegister(COMMAND_REG_PLD,0);
	WriteFlashRegister(COMMAND_REG_PLD,0);

	// Without this delay the whole computer will freeze in IsNiosInit() ??
	Sleep(500);

	// Wait for baseboard ready
	if( !IsNiosInit() )
	{
		return CS_NIOS_FAILED;	// timeout !!!
	}
	
	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL	CsPlxBase::IsNiosInit(void)
{
	uInt32		u32Status = CS_SUCCESS;;
	CSTIMER		CsTimeout;


	Sleep(200);		// 200 ms

	// Wait for initialization of Nios
	for ( int i = 0; i < 8; i ++ )
	{
		u32Status = ReadFlashRegister(STATUS_REG_PLX);
		if(ERROR_FLAG != (u32Status & ERROR_FLAG) )
			break;
	}

	u32Status = ReadFlashRegister(STATUS_REG_PLX);
	if(ERROR_FLAG == (u32Status & ERROR_FLAG) )
	{
		m_PlxData->InitStatus.Info.u32Fpga = 0;
		m_PlxData->InitStatus.Info.u32Nios = 0;

		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "NiosInit error: STATUS_REG_PLX error" );
	}
	else
	{
		m_PlxData->InitStatus.Info.u32Fpga = 1;

		CsTimeout.Set( KTIME_SECOND(4) );	// Max 4 second timeout

		uInt32 u32DimmSize = ReadRegisterFromNios( 0, CSPLXBASE_READ_MCFG_CMD);
		while ( FAILED_NIOS_DATA == u32DimmSize )
		{
			u32DimmSize = ReadRegisterFromNios( 0, CSPLXBASE_READ_MCFG_CMD);
			if ( CsTimeout.State() )
			{
				u32DimmSize = ReadRegisterFromNios( 0, CSPLXBASE_READ_MCFG_CMD);
				if( FAILED_NIOS_DATA == u32DimmSize )
				{
					m_PlxData->InitStatus.Info.u32Nios = 0;
					CsEventLog EventLog;
					EventLog.Report( CS_ERROR_TEXT, "NiosInit error: timeout" );
					return false;	// timeout !!!
				}
			}
		}
		m_PlxData->InitStatus.Info.u32Nios = 1;
	}

	return (bool)(m_PlxData->InitStatus.Info.u32Nios);

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

unsigned char  CsPlxBase:: PlxInterruptEnable( BOOL bDmaIntr )
{
    unsigned long IntCsr;
    unsigned long RegValue;

    //---- Interrupt Control/Status Register 
    IntCsr = ReadPciRegister(PCI9056_INT_CTRL_STAT);

    if (bDmaIntr)
    {
        IntCsr |= (1 << 18);

        //----- Make sure DMA interrupt is routed to PCI 
        RegValue = ReadPciRegister(PCI9056_DMA0_MODE);
        WritePciRegister(PCI9056_DMA0_MODE, RegValue | (1 << 17));
    }

    IntCsr |= (1 << 8);
    IntCsr |= (1 << 11);

	//----- Write Status registers
    WritePciRegister( PCI9056_INT_CTRL_STAT, IntCsr);

	return TRUE;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

unsigned char  CsPlxBase:: PlxInterruptDisable( BOOL bDmaIntr )
{
    unsigned long IntCsr;
    unsigned long RegValue;

	//----- Interrupt Control/Status Register
    IntCsr = ReadPciRegister(PCI9056_INT_CTRL_STAT);

    if ( bDmaIntr )
    {
        //----- Check if DMA interrupt is routed to PCI
        RegValue = ReadPciRegister(PCI9056_DMA0_MODE);

        if (RegValue & (1 << 17))
        {
            IntCsr &= ~(1 << 18);
            //----- Make sure DMA interrupt is routed to Local
            WritePciRegister( PCI9056_DMA0_MODE, RegValue & ~(1 << 17));
        } //----- if (RegValue & (1 << 17)){

		//------ clear the interrupt
		RegValue = ReadPciRegister(PCI9056_DMA_COMMAND_STAT);
        WritePciRegister( PCI9056_DMA_COMMAND_STAT, RegValue & ~(1 << 3));
    }

   IntCsr &= ~(1 << 11);
   IntCsr &= ~(1 << 8);

    //----- Write Status registers
    WritePciRegister( PCI9056_INT_CTRL_STAT, IntCsr);

	return TRUE;
}

//-----------------------------------------------------------------------------
// Function   :  PlxPciBoardReset
// Description:  Resets a device using software reset feature of PLX chip
//-----------------------------------------------------------------------------

void  CsPlxBase:: PlxPciBoardReset()
{
#if 0
    unsigned long RegValue = 0;
    unsigned long RegInterrupt = 0;
    unsigned long RegMailbox0 = 0;
    unsigned long RegMailbox1 = 0;
    unsigned long RegHotSwap = 0;
    unsigned long RegPowerMgmnt = 0;


    //----- Added to avoid compiler warnings
    RegMailbox0 = 0;
    RegMailbox1 = 0;

    //----- Clear any PCI errors
	RegValue = m_PlxData->PCIConfigHeader.Command;

    if (RegValue & (0xf8 << 24))
    {
        //----- Write value back to clear aborts, writing back 1 will clr abort and error bit
//		m_PCIConfig.WriteHeader(&PCIConfigHeader, 0);//ICI masquer pour ne changer que celui qu'on veut
		WritePciConfigSpace( &m_PlxData->PCIConfigHeader, sizeof(PCI_CONFIG_HEADER_0),  0 );	//---- Read the PCI configuration Registers
    }

    //----- Determine if an EEPROM is present
    RegValue = ReadPciRegister(PCI9056_EEPROM_CTRL_STAT);

    //----- Make sure S/W Reset & EEPROM reload bits are clear
    RegValue &= ~((1 << 30) | (1 << 29));

    //----- Save some registers if EEPROM present
    if (RegValue & (1 << 28))
    {
        RegMailbox0 = ReadPciRegister(PCI9056_MAILBOX0);
        RegMailbox1 = ReadPciRegister(PCI9056_MAILBOX1);
		RegInterrupt = PCIConfigHeader.InterruptLine;
		m_PCIConfig.ReadDeviceSpecificConfig(&RegHotSwap, PCI9056_HS_CAP_ID, ONE_DWORD);
		m_PCIConfig.ReadDeviceSpecificConfig(&RegPowerMgmnt, PCI9056_PM_CSR, ONE_DWORD);
	}

    //----- Issue Software Reset to hold PLX chip in reset
    PLX_REG_WRITE(PCI9056_EEPROM_CTRL_STAT, RegValue | (1 << 30)); //----- Bit 30 is the PLX SW Reset

    //----- Delay for a bit
    PlxSleep(100);

    //----- Bring chip out of reset
    PLX_REG_WRITE(PCI9056_EEPROM_CTRL_STAT, RegValue);

    //----- If EEPROM present, issue reload & restore registers
    if (RegValue & (1 << 28))
    {
        //----- Issue EEPROM reload the local configuration registers from the serial EEPROM, 0-1 Transition
        PLX_REG_WRITE( PCI9056_EEPROM_CTRL_STAT, RegValue | (1 << 29) );

        //----- Delay for a bit
        PlxSleep(10);

        //----- Clear EEPROM reload
        PLX_REG_WRITE( PCI9056_EEPROM_CTRL_STAT, RegValue);

        //----- Restore saved registers
        PLX_REG_WRITE(PCI9056_MAILBOX0, RegMailbox0);
        PLX_REG_WRITE(PCI9056_MAILBOX1, RegMailbox1);
		m_PCIConfig.WriteHeader(&PCIConfigHeader, 0);
        //----- Mask out HS bits that can be cleared
        RegHotSwap &= ~((1 << 23) | (1 << 22) | (1 << 17));
        RegHotSwap |= 0x3;

		m_PCIConfig.WriteDeviceSpecificConfig(&RegHotSwap, PCI9056_HS_CAP_ID, ONE_DWORD);
        //----- Mask out PM bits that can be cleared
        RegPowerMgmnt &= ~(1 << 15);
		m_PCIConfig.WriteDeviceSpecificConfig(&RegPowerMgmnt, PCI9056_PM_CSR, ONE_DWORD);
	}

	PlxSleep(3000);
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsPlxBase::ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum )
{
	// Read BaseBoard board Hardware info
	uInt32		u32Offset;
	int32		i32Status = CS_FLASH_NOT_INIT;

	//
	// Old Combine Baseboard
	//

	// Verify the default image checksum
	uInt32	u32ChecksumSignature[3];
	u32Offset = FIELD_OFFSET(BaseBoardFlash32Mbit, Data) + FIELD_OFFSET(BaseBoardFlashData, u32ChecksumSignature);
	i32Status = ReadFlash( u32ChecksumSignature, u32Offset, sizeof(u32ChecksumSignature) );

	if (  CS_FAILED(i32Status) )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "Cannot read Fw Checkum (ReadFlash Error)." );
		m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
		return CS_INVALID_FW_VERSION;
	}

	if ( bChecksum  &&  (u32ChecksumSignature[0] != CSI_FWCHECKSUM_VALID ) )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_FWCHECKSUM_ERROR, "Baseboard" );
		m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
		return CS_INVALID_FW_VERSION;
	}

	// Old Expert cards have invalid permission (permission field not set)
	BOOLEAN	bInvaldPermission = FALSE;

	int64	i64PermissionsForExpert = {0};
	u32Offset = FIELD_OFFSET(BaseBoardFlash32Mbit, Data);
	u32Offset += FIELD_OFFSET(BaseBoardFlashData, i64ExpertPermissions);
	i32Status = ReadFlash( &i64PermissionsForExpert, u32Offset, sizeof(i64PermissionsForExpert) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		if ( -1 == i64PermissionsForExpert )
		{
			bInvaldPermission = TRUE;
			i64PermissionsForExpert = 0;
		}
	}
	else
		i64PermissionsForExpert = 0;

	u32Offset = FIELD_OFFSET(BaseBoardFlash32Mbit, Data);
	u32Offset += FIELD_OFFSET(BaseBoardFlashData, ImagesHeader);
	u32Offset += FIELD_OFFSET(BaseBoardFlashHeader, HdrElement);

	HeaderElement		hHeader[3];
	memset( m_PlxData->CsBoard.u32BaseBoardOptions, 0, sizeof(m_PlxData->CsBoard.u32BaseBoardOptions) );

	i32Status = ReadFlash( hHeader, u32Offset, sizeof(hHeader) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		// Read FPGA images options
		for ( uInt32 i = 0; i < 3; i ++ )
		{
			if (  u32ChecksumSignature[i] == CSI_FWCHECKSUM_VALID  )
			{
				// Make sure that the default image is valid
				if (i == 0)
				{
					m_PlxData->InitStatus.Info.u32Flash = 1;
					i32Status = CS_SUCCESS;
				}

				//Raw flash is considered empty
				if( 0xFFFFFFFF == hHeader[i].u32FpgaOptions )
					hHeader[i].u32FpgaOptions = 0;

				// Permission has priority over the FPGA header 
				if ( bInvaldPermission )
				{
					m_PlxData->CsBoard.u32BaseBoardOptions[i] = hHeader[i].u32FpgaOptions;
					i64PermissionsForExpert					 |= hHeader[i].u32FpgaOptions;
				}
				else
				{
					// Preserve the bit CSRB_BBOPTIONS_BIG_TRIGGER because is not belong to Permission bits
					m_PlxData->CsBoard.u32BaseBoardOptions[i] = hHeader[i].u32FpgaOptions & (i64PermissionsForExpert | CSMV_BBOPTIONS_BIG_TRIGGER );
				}
			}
		}
	}

	m_PlxData->CsBoard.i64ExpertPermissions = i64PermissionsForExpert;


	return i32Status;
}
