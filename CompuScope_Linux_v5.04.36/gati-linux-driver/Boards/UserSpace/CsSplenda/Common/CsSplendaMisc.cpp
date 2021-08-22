

#include "StdAfx.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "CsSplenda.h"
#include "CsSplendaFlash.h"
#include "CsSplendaOptions.h"
#include "CsEventLog.h"
#include "CsPrivatePrototypes.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif
#include "CsNucleonBaseFlash.h"
#ifdef __linux__
#include "CsBaseHwDLL.h"
#include "GageCfgParser.h"
#endif

extern SHAREDDLLMEM Globals;

const uInt8  CsSplendaDev::c_u8DacSettleDelay = 5;
const uInt16 CsSplendaDev::c_u16CalDacCount = 4096;
const uInt32 CsSplendaDev::c_u32CalSrcDelay = 30;
const uInt32 CsSplendaDev::c_u32CalSrcSettleDelay = 40;			 // 4 mS	
const uInt32 CsSplendaDev::c_u32CalSrcSettleDelay_1MOhm = 50; // 5 mS

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define	NO_CONFIG_BOOTCPLD			0x17


int32	CsSplendaDev::DeviceInitialize(void)
{
	m_u16NextSlave	= 0;
	m_bOptionBoard = false;
	m_UseIntrTrigger = m_UseIntrBusy = m_UseIntrTransfer = FALSE;

	m_TrigAddrOffsetAdjust = 0;
	m_u8TrigSensitivity = CSPLNDA_DEFAULT_TRIG_SENSITIVITY;
	m_IntPresent		= FALSE;
	m_pNextInTheChain	= NULL;
	m_pTriggerMaster	= this;
	m_MasterIndependent = this;
	m_bInvalidHwConfiguration = FALSE;
	m_bSkipMasterSlaveDetection = false;

	m_u32IgnoreCalibErrorMask = 0xff;
	m_u32IgnoreCalibErrorMaskLocal = 0;

	// Extras samples to ensure post trigger depth
	m_u32TailExtraData = 0;

#ifdef _WINDOWS_    
	ReadCommonRegistryOnBoardInit();
#else
	ReadCommonCfgOnBoardInit();
#endif   

	int32 i32Ret = CS_SUCCESS;
	i32Ret = CsPlxBase::Initialized();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;


#ifdef USE_SHARED_CARDSTATE

	if ( m_pCardState->bCardStateValid )
	{
		if (  m_pCardState->bAddOnRegistersInitialized )
			return CS_SUCCESS;
	}
	else if ( 0 != m_pCardState->u8ImageId  )
		CsLoadBaseBoardFpgaImage( 0 );

#else
	SPLENDA_CARDSTATE			tempCardState = {0};
	i32Ret = IoctlGetCardState(&tempCardState);
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( tempCardState.bCardStateValid )
	{
		GageCopyMemoryX(m_pCardState, &tempCardState,  sizeof(SPLENDA_CARDSTATE));
		if ( m_pCardState->bAddOnRegistersInitialized )
			return CS_SUCCESS;
	}
	else
	{
		m_pCardState->u16DeviceId = tempCardState.u16DeviceId;
		m_pCardState->bPciExpress = tempCardState.bPciExpress;
		m_pCardState->bAlarm		= tempCardState.bAlarm;

		if ( 0 != (m_pCardState->u8ImageId = tempCardState.u8ImageId) )
			CsLoadBaseBoardFpgaImage( 0 );
	}
#endif

	m_PlxData->CsBoard.u32BoardType		= CS16xyy_BASEBOARD;
	m_pCardState->eclkoClockOut			= CSPLNDA_DEFAULT_CLOCK_OUT;
	m_pCardState->bDisableTrigOut		= TRUE;
	m_pCardState->u32InstalledResolution = 12;
	m_pCardState->ermRoleMode			= ermStandAlone;
	m_pCardState->bAddOnRegistersInitialized = false;

	// Reaset Auto update firmware variables
	m_PlxData->CsBoard.u8BadAddonFirmware = m_PlxData->CsBoard.u8BadBbFirmware = 0;
	PCS_BOARD_NODE  pBdParam = &m_PlxData->CsBoard;
	memset( pBdParam->u32RawFwVersionB, 0, sizeof(pBdParam->u32RawFwVersionB) );
	memset( pBdParam->u32RawFwVersionA, 0, sizeof(pBdParam->u32RawFwVersionA) );

	// Reset the firmware version variables before reading firmware version
	m_pCardState->VerInfo.u32Size = sizeof(CS_FW_VER_INFO);
	RtlZeroMemory( pBdParam->u32RawFwVersionB, sizeof(pBdParam->u32RawFwVersionB) );
	RtlZeroMemory( pBdParam->u32RawFwVersionA, sizeof(pBdParam->u32RawFwVersionA) );


	int32 i32Status = CS_SUCCESS;

	if ( m_bNucleon )
	{
		if ( m_bClearConfigBootLocation )
		{
			if ( (uInt32) -1 != ReadConfigBootLocation() )
				WriteConfigBootLocation( (uInt32)-1 );
		}
		else if ( m_bNoConfigReset )
		{
			char szText[256];

			// Assume that we boot by the safe Boot image
			m_pCardState->bSafeBootImageActive = TRUE;
			sprintf_s( szText, sizeof(szText), TEXT("Current boot location: Safe Boot"));

			if ( NO_CONFIG_BOOTCPLD <= ReadBaseBoardCpldVersion() )
			{
				uInt32 u32Location = ReadConfigBootLocation();
				if ( u32Location != 0x18801880 )
					WriteConfigBootLocation( 0x18801880 );
				else
				{
					// Check the last Firmware page loaded
					uInt32 u32RegVal = ReadGioCpld(6);
					if ( (u32RegVal & 0x40) == 0 )
					{
						m_pCardState->bSafeBootImageActive = FALSE;
						sprintf_s( szText, sizeof(szText), TEXT("Current boot location: 0x%x"),  u32Location );
					}
					else
					{
						// We are not able to boot by the image 1
						// check if the image 1 is valid, if not force update firmware on image 1
						if ( ! IsImageValid(1) )
							m_PlxData->CsBoard.u8BadBbFirmware = BAD_CFG_BOOT_FW;
					}
				}
			}

			CsEventLog EventLog;
			EventLog.Report( CS_INFO_TEXT, szText );
		}

		m_pCardState->bVirginBoard = (false == IsImageValid(0));
		if ( ! m_pCardState->bVirginBoard )
		{
			// ConfigurationReset can generate bogus interrupt. Make sure all interrupts are diabled
			ClearInterrupts();
			if ( 0 == ConfigurationReset(0) )
				m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
		}
		else
		{
			CsEventLog EventLog;
			EventLog.Report( CS_INFO_TEXT, "Nucleon Virgin board detected." );
		}
	}
	else
	{
		PLX_NVRAM_IMAGE	nvRAM_Image;
		i32Status = CsReadPlxNvRAM( &nvRAM_Image );
		if ( CS_FAILED( i32Status ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Error while reading NVRAM." );
			return i32Status;
		}

		m_PlxData->CsBoard.u32BaseBoardVersion = nvRAM_Image.Data.BaseBoardVersion;
		m_PlxData->CsBoard.u32SerNum = nvRAM_Image.Data.BaseBoardSerialNumber;
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions = nvRAM_Image.Data.BaseBoardHardwareOptions;
	}

	BOOL bNiosOK = IsNiosInit();
	if( bNiosOK )
	{
		ClearInterrupts();
		ReadBaseBoardHardwareInfoFromFlash(FALSE);

		IsAddonBoardPresent();
		if ( m_PlxData->InitStatus.Info.u32AddOnPresent )
		{
			i32Status = ReadCsiFileAndProgramFpga();
			if (CS_SUCCEEDED(i32Status))
				InitAddOnRegisters();
		}
		else
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Addon board is not connected." );
		}
	}

	if ( ! m_pCardState->bAddOnRegistersInitialized || ! bNiosOK )
	{
		// The problem may cause by invalid or incompatible firmwares
		m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;

		// Default board property
		AssignBoardType();

		CsEventLog EventLog;
		if ( ! m_pCardState->bAddOnRegistersInitialized )
			EventLog.Report( CS_ERROR_TEXT, "Failed to initialize Addon." );
		else
			EventLog.Report( CS_ERROR_TEXT, "Nios initialization error." );
	}
	
	// Read Firmware versions
	ReadVersionInfo();
	ReadVersionInfoEx();
	m_pCardState->bCardStateValid = true;
	UpdateCardState();

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::WriteFpgaString(unsigned char *buffer, int numbytes)
{
	int			count = 0, bytes = numbytes;
	int			status = 0;
	CSTIMER		CsTimeout;

	while(0 != bytes--)
	{
		WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));
		WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));
		WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20) | CSPLNDA_CFG_WRITE_SELECT);
		WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20) | CSPLNDA_CFG_WRITE_SELECT);
		WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));
		WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));

		CsTimeout.Set(CS_DP_WAIT_TIMEOUT);
		do
		{
			status = ReadGageRegister(NIOS_GEN_STATUS);
			if (CsTimeout.State())
			{
				status = ReadGageRegister(NIOS_GEN_STATUS);
				if( 0 != (status & CSPLNDA_CFG_BUSY) )
					return CS_ADDON_FPGA_LOAD_FAILED;
			}
		}
		while( 0 != (status & CSPLNDA_CFG_BUSY));

		count++;
		buffer++;
	}

	return count - numbytes;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSplendaDev::ProgramAddonFpga(uInt8 *pBuffer, uInt32 u32ImageSize)
{
	CSTIMER	CsTimeout;
	unsigned long count=0;
	unsigned int status = 0;
	int nChunkSize = 0x10000;
	int32 i32Ret = 0;
	uInt8	*fstr = pBuffer;
	int		nRemaining = 0;

	WriteGageRegister( GEN_COMMAND_R_REG, 0 );
	CsTimeout.Set(CS_DP_WAIT_TIMEOUT);
	do
	{
		status = ReadGageRegister(NIOS_GEN_STATUS);
		if (CsTimeout.State())
		{
			status = ReadGageRegister(NIOS_GEN_STATUS);
			if( 0 != ( status & CSPLNDA_CFG_STATUS) )
			{
				return CS_ADDON_FPGA_LOAD_FAILED;
			}
		}
	}
	while( 0 != (status & CSPLNDA_CFG_STATUS) );

	WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_NCONFIG | CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE);

	CsTimeout.Set(CS_DP_WAIT_TIMEOUT);
	do
	{
		status = ReadGageRegister(NIOS_GEN_STATUS);
		if (CsTimeout.State())
		{
			status = ReadGageRegister(NIOS_GEN_STATUS);
			if( 0 == (status & CSPLNDA_CFG_STATUS) )
			{
				return CS_ADDON_FPGA_LOAD_FAILED;
			}
		}
	}
	while( 0 == (status & CSPLNDA_CFG_STATUS));

	WriteGageRegister(GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE);

	CsTimeout.Set(CS_DP_WAIT_TIMEOUT);
	do
	{
		status = ReadGageRegister(NIOS_GEN_STATUS);
		if (CsTimeout.State())
		{
			status = ReadGageRegister(NIOS_GEN_STATUS);
			if( 0 != (status & CSPLNDA_CFG_STATUS) )
			{
				return CS_ADDON_FPGA_LOAD_FAILED;
			}
		}
	}
	while(0 != (status & CSPLNDA_CFG_STATUS));

	fstr = pBuffer;
	nRemaining = (int)u32ImageSize;
	do
	{
		count = (nChunkSize < nRemaining) ? nChunkSize : nRemaining;
		i32Ret = WriteFpgaString( fstr, count);
		status = ReadGageRegister(NIOS_GEN_STATUS);
		if (CS_FAILED(i32Ret) && 0 != (status & CSPLNDA_CFG_STATUS) )
		{
			return CS_ADDON_FPGA_LOAD_FAILED;
		}
		fstr += nChunkSize;
		nRemaining -= nChunkSize;
	}while (nRemaining > 0);

	CsTimeout.Set(CS_DP_WAIT_TIMEOUT);
	do
	{
		status = ReadGageRegister(NIOS_GEN_STATUS);
		if (CsTimeout.State())
		{
			status = ReadGageRegister(NIOS_GEN_STATUS);
			if( 0 == (status & CSPLNDA_CFG_DONE) )
			{
				return CS_ADDON_FPGA_LOAD_FAILED;
			}
		}
	}while( 0 == (status & CSPLNDA_CFG_DONE) );

	WriteGageRegister(GEN_COMMAND_R_REG, 0);

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSplendaDev::ReadCsiFileAndProgramFpga()
{
	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry = {0};
	int32			i32RetCode = CS_SUCCESS;

	if(! m_PlxData->InitStatus.Info.u32AddOnPresent)
		return CS_ADDON_NOT_CONNECTED;

	uInt8 *pBuffer = (uInt8 *) GageAllocateMemoryX( Max(BB_FLASHIMAGE_SIZE, SPIDER_FLASHIMAGE_SIZE) );
	if ( NULL == pBuffer )
		return CS_INSUFFICIENT_RESOURCES;

	FILE_HANDLE hCsiFile;
	uInt32	u32Target = m_bNucleon ? CSI_SPLENDA_PCIEx_TARGET : CSI_CSSPLENDA_TARGET;
	if ( ! m_bNucleon )
	{
		// Attempt to open Csi file
		hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName );
		if ( 0 == hCsiFile )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_CSIFILE_NOTFOUND, m_szCsiFileName );
			return CS_CSI_FILE_ERROR;
		}

		GageReadFile( hCsiFile, &header, sizeof(header) );
		if ( ( header.u32Size == sizeof(CSI_FILE_HEADER) ) &&
			 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) )
		{
			for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
			{
				if ( 0 == GageReadFile( hCsiFile, &CsiEntry, sizeof(CsiEntry) ) )
				{
					i32RetCode = CS_ADDONINIT_ERROR;
					break;
				}

				if( CsiEntry.u32Target == u32Target )
				{
					if ( CsiEntry.u32ImageSize > SPIDER_FLASHIMAGE_SIZE )
					{
						i32RetCode = CS_ADDONINIT_ERROR;
						break;
					}
					else
					{
						RtlCopyMemory( &m_AddonCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
						break;
					}
				}
				else
					continue;
			}
		}
		else
		{
			i32RetCode = CS_CSI_FILE_ERROR;
		}
		if (CS_FAILED(i32RetCode))
		{
			GageFreeMemoryX( pBuffer );
			GageCloseFile( hCsiFile );
			return (i32RetCode);
		}

		RtlZeroMemory( pBuffer, Max(BB_FLASHIMAGE_SIZE, SPIDER_FLASHIMAGE_SIZE) );
		GageReadFile(hCsiFile, pBuffer, m_AddonCsiEntry.u32ImageSize, &m_AddonCsiEntry.u32ImageOffset);

		i32RetCode = IoctlProgramAddonFpga(pBuffer, m_AddonCsiEntry.u32ImageSize);
		if ( CS_FAILED(i32RetCode) )
		{
			GageFreeMemoryX(pBuffer);
			GageCloseFile(hCsiFile);
			return i32RetCode;
		}
		GageCloseFile(hCsiFile);
	}

	if( m_bNucleon || !VerifySwappedFpgaBus() )
		m_PlxData->CsBoard.u32AddonHardwareOptions = m_AddonCsiEntry.u32HwOptions ^ CSPLNDA_ADDON_HW_OPTION_ALT_PINOUT;
	else
	{
		ReadAddOnHardwareInfoFromEeprom();
		m_PlxData->CsBoard.u32AddonHardwareOptions &= ~CSPLNDA_ADDON_HW_OPTION_ALT_PINOUT;
		m_PlxData->CsBoard.u32AddonHardwareOptions |= (m_AddonCsiEntry.u32HwOptions & CSPLNDA_ADDON_HW_OPTION_ALT_PINOUT);
	}

	// Get the hardware options from the flash and compare it to the addon hardware options
	// If they're the same, we're done. If they're not, we have to look for an addon entry in
	// the csi file that does match.
	if ( (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_AOHW_OPT_FWREQUIRED_MASK)  !=
		 (CsiEntry.u32HwOptions & CSPLNDA_AOHW_OPT_FWREQUIRED_MASK) )
	{
		// Reopen the Csi file
		hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName );
		if ( 0 == hCsiFile )
		{
			GageFreeMemoryX(pBuffer);
			CsEventLog EventLog;
			EventLog.Report( CS_CSIFILE_NOTFOUND, m_szCsiFileName );
			return CS_CSI_FILE_ERROR;
		}

		// Keep checking the addon entries in the CSI file until we find one that
		// matches the hardware options. If we don't find one, it's an error
		bool bFound = false;
		// we've read the header before so it should be ok
		GageReadFile( hCsiFile, &header, sizeof(header) );

		for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
		{
			if ( 0 == GageReadFile( hCsiFile, &CsiEntry, sizeof(CsiEntry) ) )
			{
				i32RetCode = CS_ADDONINIT_ERROR;
				break;
			}
			if( CsiEntry.u32Target == u32Target )
			{
				if ( CsiEntry.u32ImageSize > SPIDER_FLASHIMAGE_SIZE )
				{
					i32RetCode = CS_ADDONINIT_ERROR;
					break;
				}
				else
				{
					if ( (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_AOHW_OPT_FWREQUIRED_MASK)  ==
						 (CsiEntry.u32HwOptions & CSPLNDA_AOHW_OPT_FWREQUIRED_MASK) )
					{
						RtlCopyMemory( &m_AddonCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
						RtlZeroMemory( pBuffer, Max(BB_FLASHIMAGE_SIZE, SPIDER_FLASHIMAGE_SIZE));
						GageReadFile(hCsiFile, pBuffer, m_AddonCsiEntry.u32ImageSize, &m_AddonCsiEntry.u32ImageOffset);
						i32RetCode = IoctlProgramAddonFpga(pBuffer, m_AddonCsiEntry.u32ImageSize);
						bFound = true;
						break;
					}
				}
			}
		}
		i32RetCode = bFound ? CS_SUCCESS : CS_ADDONINIT_ERROR;
		GageCloseFile(hCsiFile);
	}
	GageFreeMemoryX(pBuffer);
	if ( CS_SUCCEEDED(i32RetCode) )
	{
		AddonReset();
		SendAddonInit();
		ReadAddOnHardwareInfoFromEeprom();
		AssignBoardType();
		m_PlxData->CsBoard.u32AddonOptions[0] = m_PlxData->CsBoard.u32ActiveAddonOptions = m_AddonCsiEntry.u32FwOptions;

	}
	return i32RetCode;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::MsCsResetDefaultParams(bool bReset)
{
	int32			i32Ret(CS_SUCCESS);
	int32			i32RetCal(CS_SUCCESS);
	CsSplendaDev*	pDevice(NULL);
	// Set Default configuration
	for( pDevice = m_MasterIndependent->m_Next; pDevice != NULL; pDevice = pDevice->m_Next )
	{
		if ( bReset )
		{
			pDevice->ResetCachedState();
			RtlZeroMemory( pDevice->m_CfgTrigParams, sizeof( m_CfgTrigParams ) );
		}
		i32Ret = pDevice->CsSetAcquisitionConfig();
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}


	if ( bReset )
	{
		m_MasterIndependent->ResetCachedState();
		RtlZeroMemory( m_MasterIndependent->m_CfgTrigParams, sizeof( m_CfgTrigParams ) );
	}

	i32Ret = m_MasterIndependent->CsSetAcquisitionConfig();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	for( pDevice = m_MasterIndependent; pDevice != NULL; pDevice = pDevice->m_Next )
	{
		if(!pDevice->m_PlxData->InitStatus.Info.u32Nios )
			return CS_FLASH_NOT_INIT;
		if ( bReset )
		{
			i32Ret = pDevice->InitAddOnRegisters(true);
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}

		i32Ret = pDevice->CsSetChannelConfig();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = pDevice->CsSetTriggerConfig();
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	
	MsClearChannelProtectionStatus(true);
	i32Ret = MsCalibrateAllChannels();
	if( CS_FAILED(i32Ret) )
		i32RetCal = i32Ret;

	m_MasterIndependent->ResyncClock();
	if( CS_FAILED(i32RetCal) )
		i32Ret = i32RetCal;

	i32Ret = WriteRegisterThruNios(m_u16TrigOutWidth-1, CSPLNDA_TRIGOUT_WIDTH_WR_CMD);
	if( CS_FAILED(i32Ret) )
	{
//		t.Trace(TraceInfo, " ! #  Send CSPLNDA_TRIGOUT_WIDTH_WR_CMD returned %d\n", i32Ret);
		return i32Ret;
	}

	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::ResetCachedState(void)
{
	CsSplendaDev* pCard = m_MasterIndependent;
	while (pCard)
	{
		for( uInt16 i = 1; i <= CSPLNDA_CHANNELS; i++ )
		{
			pCard->m_pCardState->bFastCalibSettingsDone[i-1] = FALSE;
		}
		memset( &(pCard->m_pCardState->AddOnReg), 0, sizeof(CSSPLENDA_ADDON_REGS) );
		pCard = pCard->m_Next;
	}
}


#define EEPROM_OPTIONS_POSITION_CONTROL	0x10000000L

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt16	CsSplendaDev::DetectAcquisitionSystems( BOOL bForceInd )
{
	CsSplendaDev	*pDevice = (CsSplendaDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsSplendaDev	*pFirstCardInSystem[20];

	while( pDevice )
	{
		// Break the M/S links in the kernel driver
		pDevice->IoctlSetMasterSlaveLink( NULL, 0 );
		pDevice->m_pCardState->ermRoleMode = ermStandAlone;
		pDevice->m_pNextInTheChain = 0;
		pDevice->m_Next =  pDevice->m_HWONext;
		if ( NULL == pDevice->m_Next )
		{
			m_ProcessGlblVars->pLastUnCfg = pDevice;
		}
		pDevice = pDevice->m_HWONext;
	}

	uInt32			i = 0;
	uInt16			NbCardInSystem;
	CsSplendaDev	*FirstCard = NULL;
	DRVSYSTEM		*pSystem;
	CSSYSTEMINFO	*SysInfo;

	PCS_BOARD_NODE  pBdParam = &m_PlxData->CsBoard;


	if( !bForceInd )
	{
		int16 i16NumMasterSlave = 0;
		int16 i16NumStanAlone = 0;

		pSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

		if ( ! m_bSkipMasterSlaveDetection )
			DetectMasterSlave( &i16NumMasterSlave, &i16NumStanAlone );
		if( i16NumMasterSlave > 0 )
		{
			while( (NbCardInSystem = BuildMasterSlaveSystem( &FirstCard ) ) != 0 )
			{
				pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];
				pFirstCardInSystem[i] = FirstCard;

				pBdParam = &FirstCard->m_PlxData->CsBoard;

				pSystem->DrvHandle	= FirstCard->m_PseudoHandle;
				pSystem->u8FirstCardIndex = FirstCard->m_u8CardIndex;

				SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
				SysInfo->u32BoardCount = NbCardInSystem;
				SysInfo->i64MaxMemory = pBdParam->i64MaxMemory;
				SysInfo->u32AddonOptions = pBdParam->u32ActiveAddonOptions;
				SysInfo->u32BaseBoardOptions = pBdParam->u32BaseBoardOptions[0];

				FirstCard->UpdateSystemInfo( SysInfo );
				i++;
			}
		}
	}


	while( ( NbCardInSystem = BuildIndependantSystem( &FirstCard, (BOOLEAN) bForceInd ) ) != 0 )
	{
		pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];
		pFirstCardInSystem[i] = FirstCard;

		pBdParam = &FirstCard->m_PlxData->CsBoard;

		pSystem->DrvHandle	= FirstCard->m_PseudoHandle;
		pSystem->u8FirstCardIndex = FirstCard->m_u8CardIndex;

		SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		SysInfo->u32BoardCount = NbCardInSystem;
		SysInfo->i64MaxMemory = pBdParam->i64MaxMemory;
		SysInfo->u32AddonOptions = pBdParam->u32ActiveAddonOptions;
		SysInfo->u32BaseBoardOptions = pBdParam->u32BaseBoardOptions[0];

		FirstCard->UpdateSystemInfo( SysInfo );
		i++;
	}

	if ( 0 == i )
	{
		return 0;
	}

	m_ProcessGlblVars->NbSystem = (uInt16) i;

	// Set default Acquisition Systems configuration
	int32	j = 0;


	for (i = 0; i< m_ProcessGlblVars->NbSystem; i ++ )
	{
		pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];

		CsSplendaDev *pDevice = (CsSplendaDev *) pFirstCardInSystem[i];
		j = 0;

		memset(pSystem->u8CardIndexList, 0, sizeof(pSystem->u8CardIndexList));
		while( pDevice )
		{
			// Set M/S card index list
			pSystem->u8CardIndexList[j++] = pDevice->m_u8CardIndex;

			pDevice->m_pSystem = pSystem;
			pDevice = pDevice->m_Next;
		}

		pDevice = (CsSplendaDev *) pFirstCardInSystem[i];
		SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		SysInfo->u32AddonOptions |= EEPROM_OPTIONS_POSITION_CONTROL;
		pSystem->u16AcqStatus = ACQ_STATUS_READY;
	}

	return m_ProcessGlblVars->NbSystem;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsSplendaDev::BuildMasterSlaveSystem( CsSplendaDev** ppFirstCard )
{
	CsSplendaDev*	pCurUnCfg	= (CsSplendaDev*) m_ProcessGlblVars->pFirstUnCfg;
	CsSplendaDev*	pMaster = NULL;

	//Look for the Master
	while( NULL != pCurUnCfg )
	{
		//if ( ermMaster == pCurUnCfg->m_pCardState->ermRoleMode )
		if (  0 == pCurUnCfg->m_pCardState->u8MsCardId )
		{
			pMaster = pCurUnCfg;
			break;
		}

		pCurUnCfg = pCurUnCfg->m_Next;
	}

	if( NULL == pMaster )
	{
		*ppFirstCard  = NULL;
		return 0;
	}

	uInt16 u16CardInSystem = 0;

	*ppFirstCard  = pMaster;
	CsSplendaDev* pCard = pMaster;

	LONGLONG llMaxMem = pMaster->m_PlxData->CsBoard.i64MaxMemory;

	while ( pCard )
	{
		//remove card from unconfigured list
		pCard->RemoveDeviceFromUnCfgList();

		pCard->m_pTriggerMaster = pMaster;
		pCard->m_Next = pCard->m_pNextInTheChain;
		pCard->m_MasterIndependent = pMaster;

		pCard->m_pCardState->u16CardNumber = ++u16CardInSystem;

		llMaxMem = llMaxMem < pCard->m_PlxData->CsBoard.i64MaxMemory ? llMaxMem : pCard->m_PlxData->CsBoard.i64MaxMemory;

		pCard = pCard->m_pNextInTheChain;
	}

	if( 0 == llMaxMem )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "Zero memory" );
		return 0;
	}

	bool bRemoveSystem = false;

	// Check for Master/|Slave discrepancy
	pCard = pMaster;
	while ( pCard )
	{
		if( pMaster->m_PlxData->CsBoard.NbAnalogChannel != pCard->m_PlxData->CsBoard.NbAnalogChannel ||
		    pMaster->m_PlxData->CsBoard.u32BoardType != pCard->m_PlxData->CsBoard.u32BoardType )
		{
			char szErrorStr[128];
			sprintf_s( szErrorStr, sizeof(szErrorStr), "M %d ch. %d MS/s - S%d %d ch. %d MS/s",
					pMaster->m_PlxData->CsBoard.NbAnalogChannel, int(pMaster->m_pCardState->SrInfo[0].llSampleRate/1000000),
					pCard->m_pCardState->u8MsCardId, pCard->m_PlxData->CsBoard.NbAnalogChannel, int(pCard->m_pCardState->SrInfo[0].llSampleRate/1000000));

			::OutputDebugString(szErrorStr);
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Master/Slave descrepancy." );
			bRemoveSystem = true;
			break;
		}

		pCard = pCard->m_pNextInTheChain;
	}

	if( bRemoveSystem )
		return 0;
	else
	{
		MASTER_SLAVE_LINK_INFO	SlaveInfoArray[20] = {0};
		uInt16					u16SlaveCount = 0;

		pCard = pMaster;
		while ( pCard )
		{
			pCard->m_PlxData->CsBoard.i64MaxMemory = llMaxMem;
			pCard->UpdateCardState();

			// Build Master/Slave card info to be sent to kernel driver
			if ( pCard != pCard->m_MasterIndependent )
			{
				SlaveInfoArray[u16SlaveCount].CardHandle	= pCard->m_CardHandle;
				SlaveInfoArray[u16SlaveCount++].u8MsCardId	= pCard->m_pCardState->u8MsCardId;
			}
			pCard = pCard->m_pNextInTheChain;
		}

		// Send Master/Slave link info to kernel driver
		pMaster->IoctlSetMasterSlaveLink( SlaveInfoArray, u16SlaveCount );

		char	szText[128];
		sprintf_s( szText, sizeof(szText), TEXT("Razor Master / Slave system of %d boards initialized"), u16CardInSystem);
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_TEXT, szText );
	}

	return u16CardInSystem;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsSplendaDev::BuildIndependantSystem( CsSplendaDev** FirstCard, BOOLEAN bForceInd )
{
	CsSplendaDev*	pCard	= (CsSplendaDev*) m_ProcessGlblVars->pFirstUnCfg;

	*FirstCard = NULL;
	if( NULL != pCard )
	{
		ASSERT(ermStandAlone == pCard->m_pCardState->ermRoleMode);
		pCard->m_MasterIndependent = pCard;
		pCard->m_pTriggerMaster = pCard;

		//remove card from unconfigured list
		pCard->RemoveDeviceFromUnCfgList();

		if( !bForceInd )
		{
			bool bRemoveSystem = true;
			if( !pCard->m_PlxData->InitStatus.Info.u32BbMemTest )
			{
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Zero memory" );
				return 0;
			}

			if (m_PlxData->InitStatus.Info.u32Nios && m_pCardState->bAddOnRegistersInitialized )
					pCard->AddonReset();
			else
				return 0;

			if ( (m_PlxData->CsBoard.u8BadAddonFirmware != 0)  || (m_PlxData->CsBoard.u8BadBbFirmware != 0) )
			{
				pCard->AddonConfigReset(0);
			}
			else
			{
				bRemoveSystem = false;
			}

			if(!bRemoveSystem)
			{
				char	szText[128];
				sprintf_s( szText, sizeof(szText), TEXT("Razor System %x initialized"), pCard->m_PlxData->CsBoard.u32SerNum);
				CsEventLog EventLog;
				EventLog.Report( CS_INFO_TEXT, szText );

				pCard->m_pCardState->u16CardNumber = 1;
				pCard->UpdateCardState();
				pCard->m_Next = pCard->m_pNextInTheChain = NULL;

				*FirstCard = pCard;
				return 1;
			}
		}
		else
		{
			pCard->m_pCardState->u16CardNumber = 1;
			pCard->UpdateCardState();
			pCard->m_Next = pCard->m_pNextInTheChain = NULL;
			pCard->m_u16NextSlave = 0;

			*FirstCard = pCard;
			return 1;
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
//----- CsSplendaDev::UpdateSystemInfo(PSYSTEM_NODE pSys)
//-----------------------------------------------------------------------------

void	CsSplendaDev::UpdateSystemInfo( CSSYSTEMINFO *SysInfo )
{
	PCS_BOARD_NODE  pBdParam = &m_PlxData->CsBoard;


	SysInfo->u32Size				= sizeof(CSSYSTEMINFO);
	SysInfo->u32SampleBits			= m_pCardState->AcqConfig.u32SampleBits;
	SysInfo->i32SampleResolution	= m_pCardState->AcqConfig.i32SampleRes;
	SysInfo->u32SampleSize			= m_pCardState->AcqConfig.u32SampleSize;
	SysInfo->i32SampleOffset		= m_pCardState->AcqConfig.i32SampleOffset;
	SysInfo->u32BoardType			= pBdParam->u32BoardType;
	// External trigger only avaiable from master card
	SysInfo->u32TriggerMachineCount = 1 + (pBdParam->NbTriggerMachine-1)*SysInfo->u32BoardCount;
	SysInfo->u32ChannelCount		= pBdParam->NbAnalogChannel * SysInfo->u32BoardCount;
	SysInfo->u32BaseBoardOptions	= pBdParam->u32BaseBoardOptions[0];
	SysInfo->u32AddonOptions		= pBdParam->u32ActiveAddonOptions;

	AssignAcqSystemBoardName( SysInfo );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems)
{
	CsSplendaDev* pHead	= (CsSplendaDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsSplendaDev* pCurrent = pHead;
	bool	         bFoundMaster = false;

	uInt32	u32Data;

	*i16NumStandAloneSystems = 0;
	*i16NumMasterSlaveSystems = 0;

	// First pass. Search for master cards
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnFpga == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}

		// Search for Master card
		u32Data = pCurrent->ReadRegisterFromNios( 0, CSPLNDA_MS_REG_RD );
		pCurrent->m_pCardState->u8MsCardId = ((uInt8) (u32Data >>10)) & 0x7;

		if ( 0 == pCurrent->m_pCardState->u8MsCardId )
		{
			bFoundMaster = true;
			pCurrent->m_pCardState->ermRoleMode = ermMaster;
		}
		pCurrent = pCurrent->m_Next;
	}
	// If Master card not found, it is not worth to go further
	// All cards will behave as stand alone cards
	if ( !bFoundMaster )
	{
		pCurrent = pHead;
		while( NULL != pCurrent )
		{
			// Skip all boards that have errors on Nios init or AddOn connection
			if( pCurrent->m_PlxData->InitStatus.Info.u32Nios != 0 &&
				pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent != 0 &&
				pCurrent->m_PlxData->InitStatus.Info.u32AddOnFpga != 0 )
			{
				*i16NumStandAloneSystems++;
			}
			pCurrent = pCurrent->m_Next;
		}
		return;
	}

	// Second pass. Enable MS BIST
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnFpga == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}
		pCurrent->EnableMsBist(true);
		pCurrent = pCurrent->m_Next;
	}

	// Third pass. Find slaves that belong to the master
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnFpga == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}

		if( ermMaster == pCurrent->m_pCardState->ermRoleMode  )
		{
			if( CS_SUCCEEDED(pCurrent->DetectSlaves()) )
				(*i16NumMasterSlaveSystems)++;
			else
				pCurrent->m_pCardState->ermRoleMode = ermStandAlone;
		}
		pCurrent = pCurrent->m_Next;
	}
	// Third pass. Count remaining stand alone cards
	// and disable the bist
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnFpga == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}
		if( pCurrent->m_pCardState->ermRoleMode == ermStandAlone )
			(*i16NumStandAloneSystems)++;
		pCurrent->EnableMsBist(false);
		pCurrent = pCurrent->m_Next;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::DetectSlaves()
{
	CsSplendaDev* pHead	= (CsSplendaDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsSplendaDev* pCurrent = pHead;
	CsSplendaDev* pMS = this;
	CsSplendaDev* pMSTable[8] = {NULL};
	char			 szText[128] = {0};
	int				 char_i = 0;
	uInt32           u32Data;

	if ( 0 != m_pCardState->u8MsCardId )
		return CS_FALSE;

	pMSTable[0] = this;			// Master candiate
	szText[char_i++] = '0';
	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = CSPLNDA_BIST_EN|CSPLNDA_BIST_DIR1;
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_CTL1_WR);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->AddOnReg.u16MsBist2 = 0;
	i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPLNDA_MS_BIST_CTL2_WR);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Detect all slave cards conntected to the master
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		if( ( pCurrent->m_DeviceId == pMS->m_DeviceId ) && pCurrent->m_pCardState->u8MsCardId != 0 )
		{
			u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST1_RD);
			if(  0 == ((u32Data & CSPLNDA_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
			{
				m_pCardState->AddOnReg.u16MsBist2 = 0x7FFF;
				i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPLNDA_MS_BIST_CTL2_WR);
				if( CS_FAILED(i32Ret) )	return i32Ret;

				u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST1_RD);
				if(  0 != ((u32Data & CSPLNDA_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
				{
					m_pCardState->AddOnReg.u16MsBist2 = 0;
					i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPLNDA_MS_BIST_CTL2_WR);
					if( CS_FAILED(i32Ret) )	return i32Ret;

					u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST1_RD);
					if(  0 == ((u32Data & CSPLNDA_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
					{
						pCurrent->m_pCardState->ermRoleMode = ermSlave;
						pMSTable[pCurrent->m_pCardState->u8MsCardId] = pCurrent;
						pCurrent = pCurrent->m_Next;
						continue;
					}
				}
			}
		}
		pCurrent = pCurrent->m_Next;
	}

	// Re-organize slave cards
	pMS =this;
	for (int i = 1; i <= 7; i++)
	{
		if ( NULL != pMSTable[i] )
		{
			pMS->m_pNextInTheChain = pMSTable[i];
			pMS = pMSTable[i];
			m_pCardState->u8SlaveConnected |= (1 << (pMS->m_pCardState->u8MsCardId-1));

			szText[char_i++] = ',';
			szText[char_i++] = (char)('0' + i );
		}
	}

	int32 i32Status = TestMsBridge();
	if (CS_FAILED(i32Status))
	{
		CsSplendaDev* pTemp;
		// Undo master/slave
		m_pCardState->u8SlaveConnected = 0;
		pCurrent = this;
		while( pCurrent )
		{
			pCurrent->m_pCardState->ermRoleMode = ermStandAlone;
			pTemp		= pCurrent;
			pCurrent	= pCurrent->m_pNextInTheChain;
			pTemp->m_pNextInTheChain = pTemp->m_Next;
		}
		return i32Status;
	}

	// A Master slave system should have at least one slave connected
	if ( m_pCardState->u8SlaveConnected  )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_MSDETECT, szText );
		return CS_SUCCESS;
	}
	else
		return CS_MISC_ERROR;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::SendMasterSlaveSetting()
{
	uInt16 u16Data = 0;

	uInt8	u8MsMode = 0;
	switch(m_pCardState->ermRoleMode)
	{
		default: return CS_INVALID_PARAMETER;
		case ermStandAlone:
			break;
		case ermMaster:
		case ermSlave:
		case ermLast:
			u8MsMode = CSPLNDA_DEFAULT_MSTRIGMODE;
			break;
	}
	u16Data = (u8MsMode) << 11 | (m_pCardState->u8SlaveConnected << 2);

	int32 i32Ret = WriteRegisterThruNios(u16Data, CSPLNDA_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}

	m_pCardState->AddOnReg.u16MasterSlave = u16Data;
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSplendaDev::EnableMsBist(bool bEnable)
{
	if(bEnable)
	{
		m_pCardState->AddOnReg.u16MsBist1 |= CSPLNDA_BIST_EN;
		m_pCardState->AddOnReg.u16MasterSlave = (CSPLNDA_DEFAULT_MSTRIGMODE) << 11 ;
	}
	else
	{
		if( ermStandAlone == m_pCardState->ermRoleMode )
			m_pCardState->AddOnReg.u16MasterSlave = 0;

		m_pCardState->AddOnReg.u16MsBist1 &=~CSPLNDA_BIST_EN;
	}
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MasterSlave, CSPLNDA_MS_REG_WR);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	return WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_CTL1_WR);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::ResetMasterSlave()
{
	int32 i32Ret = CS_SUCCESS;

	// This function is only for master card ...
	if ( ermMaster != m_pCardState->ermRoleMode )
		return i32Ret;

	uInt16	u16Data = m_pCardState->AddOnReg.u16MasterSlave;

	u16Data |= CSPLNDA_MS_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, CSPLNDA_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	u16Data &= ~CSPLNDA_MS_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, CSPLNDA_MS_REG_WR);

	BBtiming(1000);

	return i32Ret;
}


//-----------------------------------------------------------------------------
// Step 1. All 0 one bit 1 CSPLNDA_BIST_MS_MULTIDROP_MASK
// Step 2. All 1 one bit 0
// Step 3. Clock counters
//-----------------------------------------------------------------------------


int32 CsSplendaDev::TestMsBridge()
{
	int32 i32Ret = CS_SUCCESS;
	if ( ermMaster != m_pCardState->ermRoleMode )
		return CS_FALSE;

	uInt16 u16(0);
	uInt32 u32Data(0);
	CsSplendaDev* pSlave(NULL);
	char szText[128] = {0};

	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = CSPLNDA_BIST_EN|CSPLNDA_BIST_DIR2;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_CTL1_WR);

	//Step 1.
	m_pCardState->AddOnReg.u16MsBist2 = 0;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPLNDA_MS_BIST_CTL2_WR);
	for( u16 = CSPLNDA_BIST_MS_MULTIDROP_FIRST; u16 <= CSPLNDA_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = u16;
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPLNDA_MS_BIST_CTL2_WR);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST1_RD);
			if( (u32Data & CSPLNDA_BIST_MS_MULTIDROP_MASK) != u16 )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Bit 0x%x card %d low"), u16, pSlave->m_pCardState->u8MsCardId );
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}
	}
	//Step 2.
	m_pCardState->AddOnReg.u16MsBist2 = CSPLNDA_BIST_MS_MULTIDROP_MASK;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPLNDA_MS_BIST_CTL2_WR);
	for( u16 = CSPLNDA_BIST_MS_MULTIDROP_FIRST; u16 <= CSPLNDA_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = (CSPLNDA_BIST_MS_MULTIDROP_MASK & ~u16);
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPLNDA_MS_BIST_CTL2_WR);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST1_RD);
			if( (u32Data & CSPLNDA_BIST_MS_MULTIDROP_MASK) != (uInt32)(CSPLNDA_BIST_MS_MULTIDROP_MASK & ~u16) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Bit 0x%x card %d high"), u16, pSlave->m_pCardState->u8MsCardId );
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}
	}
	//Step 3
	uInt32 u32FpgaClkCnt(0), u32AdcClkCnt(0);
	u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST2_RD);
	u32FpgaClkCnt = u32Data & CSPLNDA_BIST_MS_FPGA_COUNT_MASK;
	u32AdcClkCnt  = u32Data & CSPLNDA_BIST_MS_ADC_COUNT_MASK;

	u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST2_RD);
	if( u32FpgaClkCnt == (u32Data & CSPLNDA_BIST_MS_FPGA_COUNT_MASK) )
	{
		sprintf_s( szText, sizeof(szText), TEXT("Fpga count failed. Card %d"), m_pCardState->u8MsCardId );
		CsEventLog EventLog;
		EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		i32Ret = CS_MS_BRIDGE_FAILED;
	}
	if( u32AdcClkCnt == (u32Data & CSPLNDA_BIST_MS_ADC_COUNT_MASK) )
	{
		sprintf_s( szText, sizeof(szText), TEXT("ADC count failed. Card %d"), m_pCardState->u8MsCardId );
		CsEventLog EventLog;
		EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		i32Ret = CS_MS_BRIDGE_FAILED;
	}
	// Not available on Splenda
	//if( 0 == (CSPLNDA_BIST_MS_SYNC&u32Data) )
	//{
	//	sprintf_s( szText, sizeof(szText), TEXT("PLL sync failed. Card %d"), m_pCardState->u8MsCardId );
	//	CsEventLog EventLog;
	//	EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
	//	i32Ret = CS_MS_BRIDGE_FAILED;
	//}
	pSlave = m_pNextInTheChain;
	while( NULL != pSlave )
	{
		u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST2_RD);
		u32FpgaClkCnt = u32Data & CSPLNDA_BIST_MS_FPGA_COUNT_MASK;
		u32AdcClkCnt  = u32Data & CSPLNDA_BIST_MS_ADC_COUNT_MASK;

		u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSPLNDA_MS_BIST_ST2_RD);
		if( u32FpgaClkCnt == (u32Data & CSPLNDA_BIST_MS_FPGA_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("Fpga count failed. Card %d"), pSlave->m_pCardState->u8MsCardId );
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
			i32Ret =CS_MS_BRIDGE_FAILED;
		}
		if( u32AdcClkCnt == (u32Data & CSPLNDA_BIST_MS_ADC_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("ADC count failed. Card %d"), pSlave->m_pCardState->u8MsCardId );
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
			i32Ret = CS_MS_BRIDGE_FAILED;
		}
		// Not available on Splenda
		//if( 0 == (CSPLNDA_BIST_MS_SYNC&u32Data) )
		//{
		//	sprintf_s( szText, sizeof(szText), TEXT("PLL sync failed. Card %d"), pSlave->m_pCardState->u8MsCardId );
		//	CsEventLog EventLog;
		//	EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		//	i32Ret =CS_MS_BRIDGE_FAILED;
		//}
		pSlave = pSlave->m_pNextInTheChain;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//	User channel skip depends on mode amd number of channels per card
//-----------------------------------------------------------------------------
uInt16	CsSplendaDev::GetUserChannelSkip()
{
	uInt16	u16ChannelSkip = 0;

	switch (m_PlxData->CsBoard.NbAnalogChannel)
	{
		case 8:
			{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_OCT )
				u16ChannelSkip = 1;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD )
				u16ChannelSkip = 2;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 4;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 8;
			}
			break;
		case 4:
			{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD )
				u16ChannelSkip = 1;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 2;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 4;
			}
			break;
		case 2:
			{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 1;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 2;
			}
			break;
		case 1:
			{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 1;
			}
			break;
	}

	ASSERT( u16ChannelSkip != 0 );
	return u16ChannelSkip;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOLEAN	CsSplendaDev::IsChannelIndexValid( uInt16 u16ChannelIndex )
{
	uInt16	u16ChannelSkip = GetUserChannelSkip();

	if ( 0 == u16ChannelIndex || u16ChannelIndex > m_pSystem->SysInfo.u32ChannelCount )
		return FALSE;

	for( uInt32 i = 0; i < m_pSystem->SysInfo.u32ChannelCount / u16ChannelSkip ; i ++ )
	{
		if ( u16ChannelIndex == 1 + i*u16ChannelSkip )
			return TRUE;
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsSplendaDev *CsSplendaDev::GetCardPointerFromBoardIndex( uInt16 BoardIndex )
{
	CsSplendaDev *pDevice = m_MasterIndependent;

	while ( pDevice )
	{
		if (pDevice->m_pCardState->u16CardNumber == BoardIndex)
			break;
		pDevice = pDevice->m_Next;
	}

	return pDevice;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsSplendaDev::NormalizedChannelIndex( uInt16 u16ChannelIndex )
{
	ASSERT( u16ChannelIndex != 0 );

	uInt16 Temp = (u16ChannelIndex - 1)  / m_PlxData->CsBoard.NbAnalogChannel;
	return (u16ChannelIndex - Temp * m_PlxData->CsBoard.NbAnalogChannel);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::NormalizedTriggerSource( int32 i32TriggerSource )
{
	if ( i32TriggerSource ==  CS_TRIG_SOURCE_DISABLE )
		return CS_TRIG_SOURCE_DISABLE;
	else if ( i32TriggerSource < 0 )
		return CS_TRIG_SOURCE_EXT;
	else
	{
		return NormalizedChannelIndex( (uInt16)  i32TriggerSource );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::InitAddOnRegisters(bool bForce)
{
	int32	i32Retry = 500;
	int32 i32Ret = CS_SUCCESS;

	if( m_pCardState->bAddOnRegistersInitialized && !bForce )
		return i32Ret;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	//Check Addon status
	if ( m_PlxData->InitStatus.Info.u32AddOnPresent )
	{
		if ( ! IsAddonInit() )
		{
			// Addon board is present but it is not initialzied
			// This may caused by bad Add-on firmware. Attempt to update add-on firmware later
			m_u8BadAddonFirmware = BAD_DEFAULT_FW;
			return CS_ADDONINIT_ERROR;
		}
	}
	else
		return CS_ADDON_NOT_CONNECTED;

	// Wait for Addon board ready
	for(;;)
	{
		if ( FAILED_NIOS_DATA != ReadRegisterFromNios( 0, CSPLNDA_FWV_RD_CMD )  )
			break;

		if ( i32Retry-- <= 0 )
		{
			return CS_ADDONINIT_ERROR;
		}
	}

//	i32Ret = CheckCalibrationReferences();
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	i32Ret = ReadCalibTableFromEeprom();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	//Make led display  PLL_lock, MS_lock, clk_filter, GD_Valid
	WriteGIO(CSPLNDA_ADDON_LED_CFG_REG, CSPLNDA_ADDON_LED_DEFAULT_CFG);

	i32Ret = SendClockSetting();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCaptureModeSetting();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendTimeStampMode();
	if( CS_FAILED(i32Ret) )
		return i32Ret;

//	i32Ret = SendTimeStampMode();
//	if( CS_FAILED(i32Ret) )	return i32Ret;

	for ( uInt16 i = 1; i <=  m_PlxData->CsBoard.NbAnalogChannel; i ++ )
	{
		uInt16	u16HardwareChannel = ConvertToHwChannel(i);
	
		// Firmware Bug
		// Patch for the firmware newer than 8.2.3.173 (Razor)
		// The problem of calibration: when first power up, the Gain calibration fails. The calibration succeed only when
		// we switch to AC coupling.
		// Workaround w/o fixing the FW  --> Force toggle the coupling from AC-DC 
		//i32Ret = SendChannelSetting( ConvertToHwChannel(i), GetDefaultRange(), CS_COUPLING_AC );

		i32Ret = SendChannelSetting( u16HardwareChannel, GetDefaultRange() );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		SendTriggerEngineSetting( u16HardwareChannel, CS_TRIG_SOURCE_DISABLE, CSPLNDA_DEFAULT_CONDITION, 0,
													  CS_TRIG_SOURCE_DISABLE, CSPLNDA_DEFAULT_CONDITION, 0 );
		if( CS_FAILED(i32Ret) ) return i32Ret;
	}

	i32Ret = SendExtTriggerSetting( FALSE, eteAlways, 0, CSPLNDA_DEFAULT_CONDITION, CSPLNDA_DEFAULT_EXT_GAIN, CSPLNDA_DEFAULT_EXT_COUPLING );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToSelfTestRegister();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->bAddOnRegistersInitialized = true;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CsSplendaDev::IsAddonInit()
{
	int32	i32Retry = 500;

	if( m_pCardState->bAddonInit )
		return true;

	if ( !m_PlxData->InitStatus.Info.u32AddOnPresent )
		return false;

	uInt32	u32Status = ReadGIO(CSPLNDA_ADDON_STATUS);
	while( (u32Status & CSPLNDA_ADDON_STAT_CONFIG_DONE) == 0 )
	{
		u32Status = ReadGIO(CSPLNDA_ADDON_STATUS);
		if ( i32Retry-- <= 0 )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: CSPLNDA_ADDON_STAT_CONFIG_DONE Timeout" );
			return false;	// timeout !!!
		}
	}

	m_PlxData->InitStatus.Info.u32AddOnFpga = 1;

	if( !m_bSkipSpiTest)
	{
		// test the SPI communication
		uInt32 u32TestData = 0x2eb3;
		int32 i32Ret = WriteRegisterThruNios( u32TestData, CSPLNDA_TEST_REG_WR_CMD );
		if ( CS_FAILED( i32Ret) )
		{
			::OutputDebugString(" ! #  AddonInit: Spi chip test error 1.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip test error." );
			return false;
		}
		// changed command from write to read
		i32Ret = ReadRegisterFromNios( 0, CSPLNDA_TEST_REG_WR_CMD ^ 0x10000 );
		// mask of the high 2 bits of the returned data because we can only write 14 bits
		if ( u32TestData != (uInt32)(i32Ret & 0x3FFF) )
		{
			::OutputDebugString(" ! #  AddonInit: Spi chip test error 2.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip test error." );
			return false;
		}

		u32TestData = 0x12345678;

		i32Ret = CsDevIoCtl::WriteGIO(CSPLNDA_ADDON_CPLD_TEST_REG, u32TestData);
		if ( 0 == i32Ret )
		{
			::OutputDebugString(" ! #  AddonInit: CPLD chip test error 1.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: CPLD chip test error." );
			return false;
		}
		uInt32 u32Data = ReadGIO(CSPLNDA_ADDON_CPLD_TEST_REG);
		if (u32Data != u32TestData)
		{
			::OutputDebugString(" ! #  AddonInit: CPLD chip test error 2.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: CPLD chip test error." );
			return false;
		}

		i32Ret = CsDevIoCtl::WriteGIO(CSPLNDA_ADDON_FPGA_TEST_REG, u32TestData);
		if ( 0 == i32Ret )
		{
			::OutputDebugString(" ! #  AddonInit: FPGA chip test error 1.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: FPGA chip test error." );
			return false;
		}
		u32Data = ReadGIO(CSPLNDA_ADDON_FPGA_TEST_REG);
		if (u32Data != u32TestData)
		{
			::OutputDebugString(" ! #  AddonInit: FPGA chip test error 2.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: FPGA chip test error." );
			return false;
		}

/*		WILL PROBABLY NEED TO ADD THIS AFTER VLAD IMPLEMENTS IT
		// Spi chip select test
		WriteGIO(CSPLNDA_ADDON_SPI_TEST_WR ,0x01);

		uInt32 u32CmdMask = 0x00810000; //spi write

		int32 i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x01);
		if( CS_FAILED ( i32Ret ) )
		{
			t.Trace(TraceInfo, " ! #  AddonInit: Spi chip select #1 test error.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #1 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x02);
		if( CS_FAILED ( i32Ret ) )
		{
			t.Trace(TraceInfo, " ! #  AddonInit: Spi chip select #2 test error.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #2 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x04);
		if( CS_FAILED ( i32Ret ) )
		{
			t.Trace(TraceInfo, " ! #  AddonInit: Spi chip select #3 test error.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #3 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x08);
		if( CS_FAILED ( i32Ret ) )
		{
			t.Trace(TraceInfo, " ! #  AddonInit: Spi chip select #4 test error.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #4 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x10);
		if( CS_FAILED ( i32Ret ) )
		{
			t.Trace(TraceInfo, " ! #  AddonInit: Spi chip select #5 test error.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #5 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x20);
		if( CS_FAILED ( i32Ret ) )
		{
			t.Trace(TraceInfo, " ! #  AddonInit: Spi chip select #6 test error.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #6 test error." );
			return false;
		}

		uInt32 u32Data = ReadGIO(CSPLNDA_ADDON_SPI_TEST_RD);

		WriteGIO(CSPLNDA_ADDON_SPI_TEST_WR ,0x02);		//close the test module
		WriteGIO(CSPLNDA_ADDON_SPI_TEST_WR ,0);

		if( 0x3f != u32Data )
		{
			t.Trace(TraceInfo, " ! #  AddonInit: CSPLNDA_ADDON_SPI_TEST_RD error.\n");
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: CSPLNDA_ADDON_SPI_TEST_RD error." );
			return false;
		}
*/
	}//SkipSpitest

	m_pCardState->bAddonInit = TRUE;

	return true;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsSplendaDev::ReadAddOnHardwareInfoFromEeprom()
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32Offset;
	SPLNDA_FLASH_FIRMWARE_INFOS	FwInfo = {0};

	u32Offset = FIELD_OFFSET(SPLNDA_FLASH_IMAGE_STRUCT, FwInfo);
	i32Status = ReadEeprom( &FwInfo, u32Offset, sizeof(FwInfo) );
	if ( CS_FAILED(i32Status) )	return i32Status;

	SPLENDA_FLASHFOOTER BoardHwInfo;
	u32Offset = FIELD_OFFSET(SPLNDA_FLASH_LAYOUT, Footer);
	i32Status = ReadEeprom( &BoardHwInfo, u32Offset, sizeof(BoardHwInfo) );
	if ( CS_SUCCESS == i32Status )
	{
		m_PlxData->CsBoard.u32SerNumEx			= BoardHwInfo.Ft.u32SerialNumber;
		m_PlxData->CsBoard.u32AddonBoardVersion	= BoardHwInfo.Ft.u32HwVersion;

		if ( BoardHwInfo.Ft.u32HwOptions == (uInt32)(-1) )
			BoardHwInfo.Ft.u32HwOptions = 0;

		if ( 0 != ( m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_ALT_PINOUT ))
			m_PlxData->CsBoard.u32AddonHardwareOptions = BoardHwInfo.Ft.u32HwOptions | CSPLNDA_ADDON_HW_OPTION_ALT_PINOUT;
		else
		{
			// The option alt Pinout will be set later when testing capability of HW
			m_PlxData->CsBoard.u32AddonHardwareOptions = BoardHwInfo.Ft.u32HwOptions & ~CSPLNDA_ADDON_HW_OPTION_ALT_PINOUT;
		}

		// Force option 100 Mhz sample rate to be set for Oscar cards
		if ( m_bOscar )
			m_PlxData->CsBoard.u32AddonHardwareOptions |= CSPLNDA_ADDON_HW_OPTION_100MS;

		if ( (uInt16) -1 != BoardHwInfo.u16ClkOut )
			m_pCardState->eclkoClockOut		= ( eClkOutMode ) BoardHwInfo.u16ClkOut;
		if ( (uInt16) -1 != BoardHwInfo.u16TrigOutEnabled )
			m_pCardState->bDisableTrigOut		= (0 == BoardHwInfo.u16TrigOutEnabled);
	}
	else
	{
		m_PlxData->CsBoard.u32AddonHardwareOptions = 0;
		m_PlxData->CsBoard.u32AddonBoardVersion = 0;
		m_PlxData->CsBoard.u32SerNum = 0;
		m_PlxData->CsBoard.u32AddonOptions[0] = 0;
	}

	return i32Status;
}

uInt8	CsSplendaDev::ReadBaseBoardCpldVersion()
{
	if ( 0 == m_pCardState->VerInfo.BbCpldInfo.u8Reg )
	{
		if (m_bNucleon)
		{
			m_pCardState->VerInfo.BbCpldInfo.u8Reg = (uInt8)ReadGioCpld(8);
		}
		else
		{
		    uInt8	u8Status = (uInt8)ReadFlashRegister( STATUS_REG_PLX );

			if( NO_VER != (u8Status & NO_VER) )
			{
				m_pCardState->VerInfo.BbCpldInfo.Info.uMinor = 0;
				m_pCardState->VerInfo.BbCpldInfo.Info.uMajor = 1;
			}
			else
			{
				m_pCardState->VerInfo.BbCpldInfo.u8Reg = (uInt8)ReadFlashRegister( VERSION_REG_PLX );
			}
		}
	}

	return m_pCardState->VerInfo.BbCpldInfo.u8Reg;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::ReadVersionInfo()
{
	if ( 0 == m_pCardState->VerInfo.BbCpldInfo.u8Reg )
	{
		if (m_bNucleon)
		{
			m_pCardState->VerInfo.BbCpldInfo.u8Reg = (uInt8)ReadGioCpld(8);
		}
		else
		{
		    uInt8	u8Status = (uInt8)ReadFlashRegister( STATUS_REG_PLX );

			if( NO_VER != (u8Status & NO_VER) )
			{
				m_pCardState->VerInfo.BbCpldInfo.Info.uMinor = 0;
				m_pCardState->VerInfo.BbCpldInfo.Info.uMajor = 1;
			}
			else
			{
				m_pCardState->VerInfo.BbCpldInfo.u8Reg = (uInt8)ReadFlashRegister( VERSION_REG_PLX );
			}
		}
	}

	if ( GetInitStatus()->Info.u32Nios && 0 == GetBoardNode()->u32RawFwVersionB[0] )
	{
		PCS_FW_VER_INFO pFwVerInfo = &m_pCardState->VerInfo;
		if ( m_bNucleon )
		{
			uInt32 u32Data = 0x7FFF & ReadGageRegister( CSNUCLEON_HWR_RD_CMD );	
			u32Data <<= 16;
			u32Data |= (0x7FFF & ReadGageRegister( CSNUCLEON_SWR_RD_CMD ));
			pFwVerInfo->BbFpgaInfo.u32Reg = m_PlxData->CsBoard.u32RawFwVersionB[m_pCardState->u8ImageId] = u32Data; 
			pFwVerInfo->u32BbFpgaOptions = ReadGageRegister( CSNUCLEON_OPT_RD_CMD );

			// Build Csi and user version info
			pFwVerInfo->BaseBoardFwVersion.Version.uRev		= pFwVerInfo->u32BbFpgaOptions;
			pFwVerInfo->BaseBoardFwVersion.Version.uIncRev	= (64*pFwVerInfo->BbFpgaInfo.Version.uRev + pFwVerInfo->BbFpgaInfo.Version.uIncRev);
		}
		else
		{
			pFwVerInfo->BbFpgaInfo.u32Reg = (0x7FFF7FFF & ReadGageRegister( FIRMWARE_VERSION ));
			GetBoardNode()->u32RawFwVersionB[0] = pFwVerInfo->BbFpgaInfo.u32Reg;
			pFwVerInfo->u32BbFpgaOptions = 0x7FFF & ReadGageRegister( FIRMWARE_OPTIONS );

			// Build Csi and user version info
			pFwVerInfo->BaseBoardFwVersion.Version.uRev		= pFwVerInfo->BbFpgaInfo.Version.uRev;
			pFwVerInfo->BaseBoardFwVersion.Version.uIncRev	= pFwVerInfo->BbFpgaInfo.Version.uIncRev;
		}

		// Build Csi and user version info
		pFwVerInfo->BaseBoardFwVersion.Version.uMajor	= pFwVerInfo->BbFpgaInfo.Version.uMajor;
		pFwVerInfo->BaseBoardFwVersion.Version.uMinor	= pFwVerInfo->BbFpgaInfo.Version.uMinor;
		GetBoardNode()->u32UserFwVersionB[0].u32Reg		= m_pCardState->VerInfo.BaseBoardFwVersion.u32Reg;
	}

	if ( m_PlxData->InitStatus.Info.u32Nios && m_pCardState->bAddOnRegistersInitialized )
	{
		PCS_FW_VER_INFO		pFwVerInfo = &m_pCardState->VerInfo;

		// Cpld Version
		// Add-on trigger CPLD is not present
		m_pCardState->VerInfo.TrigCpldFwInfo.u32Reg = 0;

		// Add-on FPGA firmware version and option
		pFwVerInfo->AddonFpgaFwInfo.u32Reg		= (0x7FFF7FFF & ReadGIO(CSPLNDA_ADDON_FPGA_FMW_REV));
		m_PlxData->CsBoard.u32RawFwVersionA[0]	= pFwVerInfo->AddonFpgaFwInfo.u32Reg;
		pFwVerInfo->u32AddonFpgaOptions			= 0x7FFF & ReadGIO(CSPLNDA_ADDON_FPGA_OPT);

		pFwVerInfo->AddonFwVersion.Version.uMajor	= pFwVerInfo->AddonFpgaFwInfo.Version.uMajor;
		pFwVerInfo->AddonFwVersion.Version.uMinor	= pFwVerInfo->AddonFpgaFwInfo.Version.uMinor;
		pFwVerInfo->AddonFwVersion.Version.uRev		= pFwVerInfo->AddonFpgaFwInfo.Version.uRev;
		pFwVerInfo->AddonFwVersion.Version.uIncRev	= pFwVerInfo->AddonFpgaFwInfo.Version.uIncRev;

		FPGABASEBOARD_OLDFWVER	BbFpgaInfo;
		BbFpgaInfo.u32Reg = ReadGIO(CSPLNDA_ADDON_CPLD_FMW_REV);

		m_pCardState->VerInfo.ConfCpldInfo.u32Reg = 0;
		m_pCardState->VerInfo.ConfCpldInfo.Info.uYear	= BbFpgaInfo.Info.uYear;
		m_pCardState->VerInfo.ConfCpldInfo.Info.uMonth	= BbFpgaInfo.Info.uMonth;
		m_pCardState->VerInfo.ConfCpldInfo.Info.uDay	= BbFpgaInfo.Info.uDay;
		m_pCardState->VerInfo.ConfCpldInfo.Info.uMajor	= BbFpgaInfo.Info.uMajor;
		m_pCardState->VerInfo.ConfCpldInfo.Info.uMinor	= BbFpgaInfo.Info.uMinor;
		m_pCardState->VerInfo.ConfCpldInfo.Info.uRev	= BbFpgaInfo.Info.uRev;

		pFwVerInfo->AddonFwVersion.Version.uMajor	= pFwVerInfo->AddonFpgaFwInfo.Version.uMajor;
		pFwVerInfo->AddonFwVersion.Version.uMinor	= pFwVerInfo->AddonFpgaFwInfo.Version.uMinor;

		if ( m_bNucleon )
		{
			pFwVerInfo->AddonFwVersion.Version.uRev		= pFwVerInfo->u32AddonFpgaOptions;
			pFwVerInfo->AddonFwVersion.Version.uIncRev	= (64*pFwVerInfo->AddonFpgaFwInfo.Version.uRev + pFwVerInfo->AddonFpgaFwInfo.Version.uIncRev);
		}
		else
		{
			pFwVerInfo->AddonFwVersion.Version.uRev		= pFwVerInfo->AddonFpgaFwInfo.Version.uRev;
			pFwVerInfo->AddonFwVersion.Version.uIncRev	= pFwVerInfo->AddonFpgaFwInfo.Version.uIncRev;
		}

		GetBoardNode()->u32UserFwVersionA[0].u32Reg		= pFwVerInfo->AddonFwVersion.u32Reg;	
	}
}


//-----------------------------------------------------------------------------
// 
//
//-----------------------------------------------------------------------------

void CsSplendaDev::ReadVersionInfoEx()
{
	bool			bConfigReset = false;
	FPGA_FWVERSION	RawVerInfo;

	if ( m_PlxData->InitStatus.Info.u32Nios )
	{
		for (uInt8 i = 1; i <= 2; i++ )
		{
			// Read version of Expert 1
			if ( 0 != m_PlxData->CsBoard.u32BaseBoardOptions[i] )
			{
				if ( 0 == m_PlxData->CsBoard.u32RawFwVersionB[i] )
				{
					if ( m_bNucleon )
					{
						if ( m_bNoConfigReset )
						{
							if ( CS_BBOPTIONS_STREAM == m_PlxData->CsBoard.u32BaseBoardOptions[i] )
							{
								GetBoardNode()->u32UserFwVersionB[i].u32Reg		= GetBoardNode()->u32UserFwVersionB[0].u32Reg;
								RawVerInfo.u32Reg = m_PlxData->CsBoard.u32RawFwVersionB[i] = m_PlxData->CsBoard.u32RawFwVersionB[0];
							}
							else
								RawVerInfo.u32Reg = 0;
						}
						else
						{
							ConfigurationReset(i);
							uInt32 u32Data = 0x7FFF & ReadGageRegister(CSPLNDA_PCIE_HWR_RD_CMD);	
							u32Data <<= 16;
							u32Data |= (0x7FFF & ReadGageRegister(CSPLNDA_PCIE_SWR_RD_CMD));
							RawVerInfo.u32Reg = m_PlxData->CsBoard.u32RawFwVersionB[i] = u32Data; 

							GetBoardNode()->u32UserFwVersionB[i].Version.uRev		= ReadGageRegister( CSNUCLEON_OPT_RD_CMD );
							GetBoardNode()->u32UserFwVersionB[i].Version.uIncRev	= (64* RawVerInfo.Version.uRev + RawVerInfo.Version.uIncRev);
						}
					}
					else
					{
						if ( 1 == i )
							BaseBoardConfigReset( FIELD_OFFSET( BaseBoardFlashData, OptionnalImage1 ) );
						else	
							BaseBoardConfigReset( FIELD_OFFSET( BaseBoardFlashData, OptionnalImage2 ) );

						RawVerInfo.u32Reg = m_PlxData->CsBoard.u32RawFwVersionB[i] = (0x7FFF7FFF & ReadGageRegister( FIRMWARE_VERSION ));

						GetBoardNode()->u32UserFwVersionB[i].Version.uRev		= 0x7FFF & ReadGageRegister( FIRMWARE_OPTIONS );
						GetBoardNode()->u32UserFwVersionB[i].Version.uIncRev	= RawVerInfo.Version.uIncRev;
					}

					GetBoardNode()->u32UserFwVersionB[i].Version.uMajor	= RawVerInfo.Version.uMajor;
					GetBoardNode()->u32UserFwVersionB[i].Version.uMinor	= RawVerInfo.Version.uMinor;

					bConfigReset = true;
				}
			}
			else
				GetBoardNode()->u32UserFwVersionB[i].u32Reg = GetBoardNode()->u32RawFwVersionB[i] = 0;
		}
	}

	if ( m_PlxData->InitStatus.Info.u32Nios && m_pCardState->bAddOnRegistersInitialized )
	{
		m_PlxData->CsBoard.u32RawFwVersionA[1] = 0; 
		m_PlxData->CsBoard.u32RawFwVersionA[2] = 0;
	}

	// If ConfigReset has been called, clear all cache inorder to force re-configuration and re-calibrate
	if ( bConfigReset )
	{
		if ( m_bNucleon )
			ConfigurationReset(0);
		else
			BaseBoardConfigReset( FIELD_OFFSET( BaseBoardFlashData, DefaultImage ) );

		SendAddonInit();
		ResetCachedState();
	}
}

//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::CsLoadBaseBoardFpgaImage( uInt8 u8ImageId )
{
	BOOLEAN		bImageChanged =		FALSE;

	// Release the current internal Average Buffer
	if ( m_pAverageBuffer && m_pCardState->u8ImageId != u8ImageId )
	{
		::GageFreeMemoryX( m_pAverageBuffer );
		m_pAverageBuffer = NULL;
	}

	switch( u8ImageId )
	{
		case 1:
		case 2:
		{
			if ( m_pCardState->u8ImageId != u8ImageId )
			{
				if ( m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] != 0 )
				{
					bImageChanged = IsConfigResetRequired(u8ImageId);
					if ( bImageChanged )
					{
						BaseBoardConfigurationReset(u8ImageId);
						m_pCardState->u8ImageId = u8ImageId;
					}

					m_pCardState->AcqConfig.u32Mode &= ~(CS_MODE_USER1 | CS_MODE_USER2);
					m_pCardState->AcqConfig.u32Mode |= ( 1 == u8ImageId ) ? CS_MODE_USER1 : CS_MODE_USER2;
				}
				else
					return CS_INVALID_ACQ_MODE;
			}
		}
		break;

		default:
		{
			if ( m_pCardState->u8ImageId != 0 )
			{
				BaseBoardConfigurationReset(0);
				m_pCardState->u8ImageId = 0;
				m_pCardState->AcqConfig.u32Mode &= ~(CS_MODE_USER1 | CS_MODE_USER2);
			}
		}
		break;
	}

	if ( 0 != u8ImageId )
	{
		m_bMulrecAvgTD = (( m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId ] & CS_BBOPTIONS_MULREC_AVG_TD ) != 0 );
		m_Stream.bEnabled = (( m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId ] & CS_BBOPTIONS_STREAM ) != 0 );
	}
	else
	{
		m_bMulrecAvgTD		= FALSE;
		m_Stream.bEnabled	= FALSE;
	}
	IoctlStmSetStreamMode( m_Stream.bEnabled ? 1: 0 );


	// Set the flag ForceReconfig, this will allow the new settings go down
	// to the HW even if the settings have not been changed
	if ( bImageChanged )
	{
		ResetCachedState();

		// These calls are requireed after BaseBoardConfigReset()
		AddonReset();
		SendAddonInit();

		ReadVersionInfo();
		UpdateCardState();
	}
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8Index = 0;
	int64	i64MaxMemPerChan;
	int64	i64MaxSegmentSize;
	int64	i64MaxDepth;
	int64	i64MaxPreTrigDepth = 0;
	int64	i64MaxTrigDelay;
	uInt32	u32MaxSegmentCount = 1;
	uInt32	u32LimitSegmentCount = 0xFFFFFFFF;
	int32	i32MinSegmentSize = 32;
	uInt32	u32DepthRes = CSPLNDA_TRIGGER_RES;
	uInt32	u32RealSegmentCount = pAcqCfg->u32SegmentCount;
	int64	i64RealSegmentSize	= pAcqCfg->i64SegmentSize;
	int64	i64RealPostDepth	= pAcqCfg->i64Depth;
	bool	bXpertBackup[10];

	// This function is called before ACTION_COMMIT
	// CsValidateAcquisitionConfig may change all Xpert flags
	// so that all algorithm for validation work.
	// At the end we will restore actual value of Xpert Flags 			
	bXpertBackup[0]	= m_bMulrecAvgTD;
	bXpertBackup[1]	= m_Stream.bEnabled;

	m_bMulrecAvgTD = m_Stream.bEnabled = m_Stream.bInfiniteDepth = false;

	// Samples reserved by HW at the end of each segment
	uInt32	u32TailHwReserved = m_32SegmentTailBytes/sizeof(uInt16);
	
	// By FW design, the last segment consumes ~512 samples more than the other segments.
	// We have to substact this amount of data in the calculation of max available memory.
	i64MaxMemPerChan = m_pSystem->SysInfo.i64MaxMemory - 512;

	if ( m_bNucleon )
	{
		i64MaxPreTrigDepth = 4080;

		// Firmware bug related to memory controler !!!!!!
		// The segment size and depth should be multiple of 4 memory location.
		// If this requirement is not met then we will have randomly error on timestamp 
		// That is why we have to make trigger resolution 4 time bigger.
		u32DepthRes = 4*CSPLNDA_TRIGGER_RES;
	}

	if ( pAcqCfg->u32Mode & CS_MODE_USER1 )
		u8Index = 1;
	else if ( pAcqCfg->u32Mode & CS_MODE_USER2 )
		u8Index = 2;

	i64MaxTrigDelay = CSMV_LIMIT_HWDEPTH_COUNTER;
	i64MaxTrigDelay *= 128;
	i64MaxTrigDelay /= (32*(pAcqCfg->u32Mode&CS_MASKED_MODE));

	if ( pAcqCfg->u32Mode & CS_MODE_QUAD )
	{
		i64MaxPreTrigDepth <<= 1;
		u32DepthRes >>= 2;
		u32TailHwReserved >>= 2;
		i64MaxMemPerChan >>= 2;
	}
	else if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
	{
		i64MaxPreTrigDepth <<= 2;
		u32DepthRes >>= 1;
		u32TailHwReserved >>= 1;
		i64MaxMemPerChan >>= 1;
	}
	else	// if ( pAcqCfg->u32Mode & CS_MODE_SINGLE )
	{
		i64MaxPreTrigDepth <<= 3;
	}


	i64MaxMemPerChan -= CsGetPostTriggerExtraSamples(pAcqCfg->u32Mode);
	i64MaxSegmentSize = i64MaxMemPerChan - u32TailHwReserved;
	i64MaxDepth = i64MaxSegmentSize;
	if ( ! m_bNucleon )
		i64MaxPreTrigDepth = i64MaxDepth;

	if ( 0 != u8Index )
	{
		// Make sure that Master/Slave system has all the same optionss
		CsSplendaDev* pDevice = m_MasterIndependent->m_Next;
		while( pDevice )
		{
			if (  m_PlxData->CsBoard.u32BaseBoardOptions[u8Index] != pDevice-> m_PlxData->CsBoard.u32BaseBoardOptions[u8Index] )
			{
				i32Status = CS_MASTERSLAVE_DISCREPANCY;
				goto ExitValidate;
			}

			pDevice = pDevice->m_Next;
		}

		if (  m_PlxData->CsBoard.u32BaseBoardOptions[u8Index] == 0 )
		{
			if ( u32Coerce )
			{
				pAcqCfg->u32Mode &= ~(CS_MODE_USER1|CS_MODE_USER2);
				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				i32Status = CS_INVALID_ACQ_MODE;
				goto ExitValidate;
			}
		}

		if ( IsExpertAVG(u8Index) )
		{	
			m_bMulrecAvgTD		= TRUE;
			u32TailHwReserved	= 0;		// No tail

			// Each sample will be 32 bits sample
			i64MaxMemPerChan	 /= 2;
			i64MaxPreTrigDepth	= 0;
			i32MinSegmentSize	= 32;

			i64MaxSegmentSize	= m_u32MaxHwAvgDepth/(pAcqCfg->u32Mode&CS_MASKED_MODE);
			i64MaxDepth			= i64MaxSegmentSize;
		}

		m_Stream.bEnabled = IsExpertSTREAM(u8Index);
	}
	else
	{
		if ( pAcqCfg->u32Mode & CS_MODE_SW_AVERAGING )
		{
			i64MaxDepth		= MAX_SW_AVERAGING_SEMGENTSIZE/(pAcqCfg->u32Mode&CS_MASKED_MODE);
			i64MaxSegmentSize  = i64MaxDepth;
			// No pretrigger is allowed in SW averaging
			i64MaxPreTrigDepth = 0;
			// Trigger Delay is not supported in SW averaging
			i64MaxTrigDelay = 0;
			u32LimitSegmentCount = MAX_SW_AVERAGE_SEGMENTCOUNT;
		}
		else if ( pAcqCfg->i64TriggerDelay != 0 )
			i64MaxPreTrigDepth = 0;		// No pretrigger if TriggerDelay is enable
	}

	// Validation of Segment Size
	if ( m_Stream.bEnabled && 0 > pAcqCfg->i64SegmentSize && 0 > pAcqCfg->i64Depth )
		m_Stream.bInfiniteDepth = true;
	else if ( 0 > pAcqCfg->i64SegmentSize )
		return CS_INVALID_SEGMENT_SIZE;
	else if ( 0 > pAcqCfg->i64Depth )
		return CS_INVALID_TRIG_DEPTH;

	// Validation of Segment Size
	if ( ! m_Stream.bInfiniteDepth )
	{
		// Valdate the min segment size 
		if ( pAcqCfg->i64SegmentSize < i32MinSegmentSize  )
		{
			if ( u32Coerce )
			{
				pAcqCfg->i64Depth		+= i32MinSegmentSize - pAcqCfg->i64SegmentSize;
				pAcqCfg->i64SegmentSize = i32MinSegmentSize;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				i32Status = CS_INVALID_SEGMENT_SIZE;
				goto ExitValidate;
			}
		}

		if ( 0 >= pAcqCfg->i64SegmentSize  )
		{
			if (u32Coerce)
			{
				pAcqCfg->i64SegmentSize = CSPLNDA_DEFAULT_SEGMENT_SIZE;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_SEGMENT_SIZE;
		}

		if ( pAcqCfg->i64SegmentSize % u32DepthRes )
		{
			if (u32Coerce)
			{
				pAcqCfg->i64SegmentSize -= (pAcqCfg->i64SegmentSize % u32DepthRes);
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_SEGMENT_SIZE;
		}

		// Validate triggger depth
		if ( 0 >= pAcqCfg->i64Depth  )
		{
			if (u32Coerce)
			{
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_DEPTH;
		}

		if ( pAcqCfg->i64Depth % u32DepthRes )
		{
			if (u32Coerce)
			{
				pAcqCfg->i64Depth -= (pAcqCfg->i64Depth % u32DepthRes);
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_DEPTH;
		}

		if ( pAcqCfg->i64SegmentSize < pAcqCfg->i64Depth )
		{
			if (u32Coerce)
			{
				pAcqCfg->i64SegmentSize = pAcqCfg->i64Depth;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_SEGMENT_SIZE;
		}

		// Nulceon is "square" and requires pAcqCfg->i64TriggerHoldoff >= pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth;
		if ( m_bNucleon && (pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth > pAcqCfg->i64TriggerHoldoff) )
		{
			if (u32Coerce)
			{
				pAcqCfg->i64TriggerHoldoff = pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIGHOLDOFF;
		}

	}

	// Adjust Segemnt Parameters for Hardware
	AdjustedHwSegmentParameters( pAcqCfg->u32Mode, &u32RealSegmentCount, &i64RealSegmentSize, &i64RealPostDepth );

	// Valdate the max post trigger depth if it is not in stream mode
	if ( !m_Stream.bEnabled && i64RealPostDepth > i64MaxDepth )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64Depth -= (i64RealPostDepth - i64MaxDepth);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
		{
			i32Status = CS_INVALID_SEGMENT_SIZE;
			goto ExitValidate;
		}
	}

	// Valdate the max segment size if it is not in stream mode
	if ( !m_Stream.bEnabled && i64RealSegmentSize > i64MaxSegmentSize  )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize -= (i64RealSegmentSize - i64MaxSegmentSize);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
		{
			i32Status = CS_INVALID_PRETRIGGER_DEPTH;
			goto ExitValidate;
		}
	}

	if ( pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth > i64MaxPreTrigDepth )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize = pAcqCfg->i64Depth + i64MaxPreTrigDepth;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
		{
			i32Status = CS_INVALID_PRETRIGGER_DEPTH;
			goto ExitValidate;
		}
	}

	if ( pAcqCfg->i64TriggerDelay > i64MaxTrigDelay || 
		0 != (pAcqCfg->i64TriggerDelay % u32DepthRes) )
	{
		// Trigger delay should be multiple of 8 in Single, 4 in Dual and 2 in Quad
		if ( u32Coerce )
		{
			if ( pAcqCfg->i64TriggerDelay > i64MaxTrigDelay )
				pAcqCfg->i64TriggerDelay = i64MaxTrigDelay;
			else
				pAcqCfg->i64TriggerDelay -= (pAcqCfg->i64TriggerDelay % u32DepthRes);

			i32Status = CS_CONFIG_CHANGED;
		}
		else
		{
			i32Status = CS_INVALID_TRIGDELAY;
			goto ExitValidate;
		}
	}

	// Valdate the max segment size if it is not in stream mode
	if ( !m_Stream.bEnabled && pAcqCfg->i64SegmentSize > i64MaxSegmentSize )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize = i64MaxSegmentSize;
			if ( pAcqCfg->i64Depth > pAcqCfg->i64SegmentSize )
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;

			i32Status = CS_CONFIG_CHANGED;
		}
		else
		{
			i32Status = CS_INVALID_SEGMENT_SIZE;
			goto ExitValidate;
		}
	}

	if ( CS_CONFIG_CHANGED == i32Status )
	{
		u32RealSegmentCount = pAcqCfg->u32SegmentCount;
		i64RealSegmentSize	= pAcqCfg->i64SegmentSize;
		i64RealPostDepth	= pAcqCfg->i64Depth;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( pAcqCfg->u32Mode, &u32RealSegmentCount, &i64RealSegmentSize, &i64RealPostDepth );
	}

	// Validation of segment count
	// In Stream mode, the segment count is ignored. Otherwise, the segment count should be calculated base on available memory
	if ( ! m_Stream.bEnabled )
	{
		u32MaxSegmentCount = (uInt32)( i64MaxMemPerChan / (i64RealSegmentSize + u32TailHwReserved ));
		u32MaxSegmentCount = Min( u32MaxSegmentCount, u32LimitSegmentCount );
		if ( pAcqCfg->u32SegmentCount > u32MaxSegmentCount )
		{
			if ( u32Coerce )
			{
				pAcqCfg->u32SegmentCount = u32MaxSegmentCount;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				i32Status = CS_INVALID_SEGMENT_COUNT;
		}	
	}

ExitValidate:

	// Restore the current expert flags
	m_bMulrecAvgTD		= bXpertBackup[0];
	m_Stream.bEnabled	= bXpertBackup[1];

	return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsSplendaDev::Abort()
{
	CsSplendaDev* pCard = m_MasterIndependent;

	while (pCard)
	{
		if( pCard != m_pTriggerMaster )
		{
			pCard->AbortAcquisition();
		}

		pCard = pCard->m_Next;
	}

	m_pTriggerMaster->AbortAcquisition();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	CsSplendaDev::AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32 *u32SegmentCount, int64 *i64SegmentSize, int64 *i64Depth )
{
	if ( m_bNucleon )	// Nothing to adjust for Nucleon
		return;

	UNREFERENCED_PARAMETER(u32SegmentCount);

	// Number of extra sample to be added in segment size to compensate negative trigger address offset
	uInt32	u32HeadExtraSamples = CSPLNDA_PRE_TRIG_PIPELINE;

	// Number of extra samples to be added into segment and depth setting to ensurw
	// post trigger dapth( cover the lost due invalid samples and trigger address offset )
	m_u32TailExtraData = CsGetPostTriggerExtraSamples( u32Mode );

	if ( m_bMulrecAvgTD )
	{ 
		// Ususally the effect of trigger delay is being set by PostTrig depth > Segment size but
		// in Expert Mulrec AVG TD, the trigger delay is being set by a specific register.
		// Make sure PostTrig depth = Segment size
		*i64Depth			+= m_u32TailExtraData;
		*i64SegmentSize	+= m_u32TailExtraData;
	}
	else
	{
		*i64SegmentSize = *i64SegmentSize + m_u32TailExtraData;
		*i64SegmentSize += u32HeadExtraSamples;

		*i64Depth = *i64Depth + m_pCardState->AcqConfig.i64TriggerDelay + m_u32TailExtraData;
	}
}



//--------------------------------------------------------------------------------------------------
//	Internal extra samples to be added in the post trigger depth and segment size in order to
// 	ensure post trigger depth.
//  Withou ensure post trigger depth, the lost is because of trigger address offset and invalid
//  samples at the end of each record.
//---------------------------------------------------------------------------------------------------

uInt32	CsSplendaDev::CsGetPostTriggerExtraSamples( uInt32 u32Mode )
{
	if ( m_bZeroDepthAdjust || m_bNucleon )
		return 0;

	uInt32	u32TailExtraData = 0;

	if ( m_bMulrecAvgTD )
	{
		// The amount of invalid sample vary depending on post trigger depth
		// So we have to adjust in worse case
		if ( u32Mode & CS_MODE_QUAD )
		{
			// Usually invalid samples < 16 but at 200M/s invalid samples > 16 samples
			u32TailExtraData = 24;
		}
		if ( u32Mode & CS_MODE_DUAL )
		{
			// Usually invalid samples < 32
			u32TailExtraData = 32;
		}
		if ( u32Mode & CS_MODE_SINGLE )
		{
			// Usually invalid samples < 64 but at Depth = 48992 invalid samples > 64 samples
			u32TailExtraData =  96;
		}
	}
	else
	{
		if ( m_bNucleon )
			u32TailExtraData = 4*16;//MAX_SPDR_TAIL_INVALID_SAMPLES;
		else
			u32TailExtraData = MAX_SPDR_TAIL_INVALID_SAMPLES;

		if ( u32Mode & CS_MODE_QUAD )
			u32TailExtraData >>= 2;
		if ( u32Mode & CS_MODE_DUAL )
			u32TailExtraData >>= 1;
		if ( u32Mode & CS_MODE_SINGLE )
			u32TailExtraData >>= 0;

		if ( ! m_bNucleon )
			u32TailExtraData += MAX_SPDR_TRIGGER_ADDRESS_OFFSET;
	}

	return u32TailExtraData;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt16 CsSplendaDev::GetControlDacId( eCalDacId ecdId, uInt16 u16Channel )
{
	switch ( ecdId )
	{
		case ecdPosition: return CSPLNDA_POSITION_1 + u16Channel - 1;
		case ecdOffset: return CSPLNDA_VCALFINE_1 + u16Channel - 1;
		case ecdGain: return CSPLNDA_CAL_GAIN_1 + u16Channel - 1;
	}

	return 0xffff;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::CsSetDataFormat( _CSDATAFORMAT csFormat )
{
	switch(m_pCardState->u32InstalledResolution)
	{
		default:
		case 12: m_pCardState->AcqConfig.u32SampleBits   = CS_SAMPLE_BITS_12;
				 m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_12_LJ;
				 break;
		case 14: m_pCardState->AcqConfig.u32SampleBits   = CS_SAMPLE_BITS_14;
				 m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_14_LJ;
				 break;
		case 16: m_pCardState->AcqConfig.u32SampleBits   = CS_SAMPLE_BITS_16;
				 m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_16_LJ;
				 break;
	}
	m_pCardState->AcqConfig.i32SampleRes = CSPLNDA_DEFAULT_RES;

	switch ( csFormat )
	{
		case formatSoftwareAverage:
			m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
			m_pCardState->AcqConfig.i32SampleRes = CSPLNDA_DEFAULT_RES;
			m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_14_LJ;
			break;
		case formatHwAverage:
			// Full resolution, 26 bit data left aligned inside 32 bit
			m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
			m_pCardState->AcqConfig.i32SampleOffset <<= 6;
			m_pCardState->AcqConfig.i32SampleRes	<<= 6;
			break;
		default:
			m_pCardState->AcqConfig.u32SampleSize = CSPLNDA_DEFAULT_SAMPLE_SIZE;
			break;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsSplendaDev::GetFreeTriggerEngine( int32 i32TrigSource, uInt16 *u16TriggerEngine )
{
	if ( i32TrigSource < 0 )
	{
		if ( 0 == m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].u32UserTrigIndex )
		{
			*u16TriggerEngine = CSPLNDA_TRIGENGINE_EXT;
			return CS_SUCCESS;
		}
	}
	else
	{
		uInt16	u16Start = 0;
		uInt16	u16End = CSPLNDA_TOTAL_TRIGENGINES;

		if ( i32TrigSource > 0 ) // disable may be in any position for channel trigger need to define appropriate engine
		{
			uInt16 u16Hw = ConvertToHwChannel(uInt16(i32TrigSource));
		
			u16End = u16Hw * 2; 
			u16Start = u16End-1;

			for ( uInt16 i = u16Start; i <= u16End; i ++ )
			{
				if ( 0 == m_CfgTrigParams[i].u32UserTrigIndex )
				{
					*u16TriggerEngine = i;
					return CS_SUCCESS;
				}
			}
		}
		else
		{
			if ( 0 == m_CfgTrigParams[0].u32UserTrigIndex )
			{
				*u16TriggerEngine = 0;
				return CS_SUCCESS;
			}

			for ( uInt16 i = 0; i < m_PlxData->CsBoard.NbAnalogChannel; i ++ )
			{
				uInt16 u16Hw = ConvertToHwChannel(i+1);
				uInt16 u16Index = 2*u16Hw-1;
				if ( 0 == m_CfgTrigParams[u16Index].u32UserTrigIndex )
				{
					*u16TriggerEngine = u16Index;
					return CS_SUCCESS;
				}
				u16Index ++;
				if ( 0 == m_CfgTrigParams[u16Index].u32UserTrigIndex )
				{
					*u16TriggerEngine = u16Index;
					return CS_SUCCESS;
				}
			}
		}
	}
	return CS_ALLTRIGGERENGINES_USED;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------	------
int32	CsSplendaDev::CsAutoUpdateFirmware()
{
	int32	i32RetStatus = CS_SUCCESS;

	if ( m_bSkipFirmwareCheck )
		return CS_FAIL;

	CsSplendaDev *pCard = (CsSplendaDev*) m_ProcessGlblVars->pFirstUnCfg;

	while( pCard )
	{
		int32	i32Status = CS_SUCCESS;

		// Open handle to kernel driver
		pCard->InitializeDevIoCtl();

		if ( pCard->m_bNucleon )
		{
			i32Status = pCard->CsNucleonCardUpdateFirmware();
			if ( CS_FWUPDATED_SHUTDOWN_REQUIRED == i32Status )
				i32RetStatus = i32Status;
		}	
		else
			pCard->CombineCardUpdateFirmware();

		pCard->ReadVersionInfo();
		pCard->ReadVersionInfoEx();

		pCard->UnInitializeDevIoCtl();
		pCard = pCard->m_HWONext;
	}

	return i32RetStatus; 
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSplendaDev::CombineCardUpdateFirmware()
{
	uInt32					u32HeaderOffset = 0;
	uInt32					u32ChecksumOffset = 0;
	uInt32					u32Checksum = 0;
	int32					i32Status = CS_SUCCESS;
	uInt8					*pBuffer;
	int32					i32FwUpdateStatus[3] = { 0 };
	char					szText[128];
	char					OldFwFlVer[40];
	char					NewFwFlVer[40];

	if ( m_bSkipFirmwareCheck )
		return CS_FAIL;

	FILE_HANDLE hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName );
	if ( 0 == hCsiFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_OPEN_FILE, m_szCsiFileName );
		return CS_FAIL;
	}
	
	// check if any expert firmware needs updating. If not don't bother reading the fwl file.
	// if they do need updating and we can't open the file it's an error
	BOOL bUpdateFwl = m_PlxData->CsBoard.u8BadBbFirmware & (BAD_EXPERT1_FW | BAD_EXPERT2_FW);

	FILE_HANDLE hFwlFile = (FILE_HANDLE)0;
	if ( bUpdateFwl )
	{
		hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName );
		if ( (FILE_HANDLE)0 == hFwlFile )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_OPEN_FILE, m_szFwlFileName );
			GageCloseFile( hCsiFile );
			return CS_FAIL;
		}
	}

	pBuffer = (uInt8 *) GageAllocateMemoryX( Max(BB_FLASHIMAGE_SIZE, SPIDER_FLASHIMAGE_SIZE) );
	if ( NULL == pBuffer )
	{
		GageCloseFile( hCsiFile );
		if ( (FILE_HANDLE)0 != hFwlFile )
			GageCloseFile( hFwlFile );

		return CS_INSUFFICIENT_RESOURCES;
	}

	HeaderElement	HdrElement[3];
	u32HeaderOffset = FIELD_OFFSET(BaseBoardFlashData, ImagesHeader) +
						FIELD_OFFSET(BaseBoardFlashHeader, HdrElement);
	u32ChecksumOffset = FIELD_OFFSET(BaseBoardFlashData, u32ChecksumSignature);

	if ( m_PlxData->CsBoard.u8BadBbFirmware )
	{
		i32Status = ReadFlash( HdrElement, u32HeaderOffset, sizeof(HdrElement) );

		// Clear valid Checksum
		u32Checksum = 0;
		i32Status = WriteFlashEx( &u32Checksum, u32ChecksumOffset, sizeof(u32Checksum), AccessFirst );

		// Update the primary image
		if ( m_PlxData->CsBoard.u8BadBbFirmware & BAD_DEFAULT_FW )
		{
			// Load new image from file if it is not already in buffer
			RtlZeroMemory( pBuffer, BB_FLASHIMAGE_SIZE);
			uInt32 u32ImageOffset = m_BaseBoardCsiEntry.u32ImageOffset;
			GageReadFile(hCsiFile, pBuffer, m_BaseBoardCsiEntry.u32ImageSize, &u32ImageOffset);

			// Write Base board FPGA image
			i32Status = WriteFlashEx( pBuffer, 0, m_BaseBoardCsiEntry.u32ImageSize, AccessMiddle );
			if ( CS_SUCCEEDED(i32Status) )
			{
				HdrElement[0].u32Version = m_BaseBoardCsiEntry.u32FwVersion;
				HdrElement[0].u32FpgaOptions = m_BaseBoardCsiEntry.u32FwOptions;
				i32FwUpdateStatus[0] = CS_SUCCESS;
			}

			// The firmware has been changed.
			// Invalidate this firmware version in order to force reload the firmware version later
			GetBoardNode()->u32RawFwVersionB[0] = 0;
		}

		// Update the Expert images
		if ( hFwlFile )
		{
			for (int i = 0; i < 2; i++)
			{
				if ( m_PlxData->CsBoard.u8BadBbFirmware & (BAD_EXPERT1_FW << i)  )
				{
					// Load new image from file if it is not already in buffer
					RtlZeroMemory( pBuffer, BB_FLASHIMAGE_SIZE);
					GageReadFile(hFwlFile, pBuffer, m_BaseBoardExpertEntry[i].u32ImageSize, &m_BaseBoardExpertEntry[i].u32ImageOffset);

					// Write Base board FPGA image
					i32Status = WriteFlashEx( pBuffer, BB_FLASHIMAGE_SIZE*(i+1), m_BaseBoardExpertEntry[i].u32ImageSize, AccessMiddle );
					if ( CS_SUCCEEDED(i32Status) )
					{
						HdrElement[i+1].u32Version		= m_BaseBoardExpertEntry[i].u32Version;
						HdrElement[i+1].u32FpgaOptions	= m_BaseBoardExpertEntry[i].u32Option;
						i32FwUpdateStatus[i+1] = CS_SUCCESS;
					}

					// The firmware has been changed.
					// Invalidate this firmware version in order to force reload the firmware version later
					GetBoardNode()->u32RawFwVersionB[i+1] = 0;
				}
			}
			GageCloseFile( hFwlFile );
		}

		// Update Header with the new versions
		i32Status = WriteFlashEx( HdrElement, u32HeaderOffset, sizeof(HdrElement), AccessMiddle );

		// Set valid Checksum
		if ( CS_SUCCEEDED(i32Status) )
		{
			u32Checksum	= CSI_FWCHECKSUM_VALID;
			i32Status = WriteFlashEx( &u32Checksum, u32ChecksumOffset, sizeof(u32Checksum), AccessLast );
		}

		if ( CS_SUCCEEDED(i32Status) )
		{
			m_PlxData->CsBoard.u8BadBbFirmware = 0;

			for (int i = 0; i < 3; i++)
			{
				if ( CS_SUCCESS ==  i32FwUpdateStatus[i] )
				{
					if ( 0 == i )
						FriendlyFwVersion(m_BaseBoardCsiEntry.u32FwVersion, NewFwFlVer, sizeof(NewFwFlVer), m_bCsiOld);
					else
						FriendlyFwVersion(m_BaseBoardExpertEntry[i-1].u32Version, NewFwFlVer, sizeof(NewFwFlVer), m_bFwlOld);

					FriendlyFwVersion( m_PlxData->CsBoard.u32UserFwVersionB[i].u32Reg, OldFwFlVer, sizeof(OldFwFlVer), false);
					sprintf_s( szText, sizeof(szText), TEXT("Image%d from v%s to v%s,"), i, OldFwFlVer, NewFwFlVer );
					
					CsEventLog EventLog;
					EventLog.Report( CS_BBFIRMWARE_UPDATED, szText );
				}
			}

			ClearInterrupts();
			if ( !m_pCardState->bAddOnRegistersInitialized )
			{
				IsAddonBoardPresent();
				if ( m_PlxData->InitStatus.Info.u32AddOnPresent )
				{
					InitAddOnRegisters();
				}
			}
		}
		else
			BaseBoardConfigReset(0);

		AddonReset();
		SendAddonInit();

		// Board type depends on Hw Config table. And this information is read through a register.
		// Some of old Fw version does not support this register. In this case, the boardtype is reset to the deault one.
		// That why when we update baseboard firmware we have to update also the boardtype.
		AssignBoardType();

		// Memory size may have changed (Memory less option)
		UpdateMaxMemorySize( m_PlxData->CsBoard.u32BaseBoardHardwareOptions, CSPLNDA_DEFAULT_SAMPLE_SIZE );

		// Close the handle to kernel driver
		UpdateCardState();
	}

	GageCloseFile( hCsiFile );
	if ( (FILE_HANDLE)0 != hFwlFile )
		GageCloseFile( hFwlFile );
	GageFreeMemoryX( pBuffer );

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::AcquireData(uInt32 u32IntrEnabled)
{
	// Reset notification flag before acquisition
	m_pTriggerMaster->m_PlxData->CsBoard.bBusy = true;
	m_pTriggerMaster->m_PlxData->CsBoard.bTriggered = false;
	m_PlxData->CsBoard.DpcIntrStatus = 0;

	m_pTriggerMaster->RelockReference();


	// Send startacquisition command
	in_STARTACQUISITION_PARAMS AcqParams = {0};

	AcqParams.u32IntrMask = u32IntrEnabled;
	IoctlDrvStartAcquisition( &AcqParams );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev:: ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg )
{
	if ( m_bNucleon )
	{
		// Set Mode select 0
		WriteGageRegister(MODESEL, MSEL_CLR | 0);
		WriteGageRegister(MODESEL, (~MSEL_CLR) & 0);

		for (uInt32 i = 0; i < u32NumOfSeg; i ++)
		{
			pBuffer[i] = ReadRegisterFromNios( StartSegment, CSPLXBASE_READ_TIMESTAMP0_CMD );
			pBuffer[i] &= 0xFFFF;
			pBuffer[i] <<= 32;
			pBuffer[i] |= ReadRegisterFromNios( StartSegment++, CSPLXBASE_READ_TIMESTAMP1_CMD );
		}
	}
	else
	{
		ReadTriggerTimeStamp( StartSegment, pBuffer, u32NumOfSeg );

		if ( 0 == (m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK) )
		{
			uInt8	u8Shift = 0;

			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD )
				u8Shift = 1;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
				u8Shift = 2;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u8Shift = 3;

			uInt32	u32TriggerAddress;
			uInt8	u8Etb;

			for (uInt32 i = 0; i < u32NumOfSeg; i ++ )
			{
				u32TriggerAddress	= ReadRegisterFromNios(i, CSPLXBASE_READ_TRIG_ADD_CMD);
				u8Etb = (uInt8) ((u32TriggerAddress >> 4) & 0xf) >> 1;

				// Convert to full timestamp resolution
				pBuffer[i] = pBuffer[i]  << u8Shift;
				pBuffer[i] += u8Etb;
			}
		}
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsSplendaDev::IsAddonBoardPresent()
{
	uInt32 u32Reg = ReadGIO(CSPLNDA_ADDON_CPLD_FMW_REV);
	const uInt32 cu32AddEcho = CSPLNDA_ADDON_CPLD_FMW_REV | (CSPLNDA_ADDON_CPLD_FMW_REV << 8) | (CSPLNDA_ADDON_CPLD_FMW_REV << 16) | (CSPLNDA_ADDON_CPLD_FMW_REV<< 24);

	if( 0 == u32Reg || 0xffffffff == u32Reg || cu32AddEcho == u32Reg )
		m_PlxData->InitStatus.Info.u32AddOnPresent = 0;
	else
		m_PlxData->InitStatus.Info.u32AddOnPresent = 1;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsSplendaDev::AssignBoardType()
{
	bool	bErrorCapChannel	= false;
	bool	bErrorCapResolution	= false;

	//Read BoardInfoVia NIOS
	m_pCardState->u32InstalledResolution = ReadRegisterFromNios( SPLENDACAPS_RESOLUTION, CSPLXBASE_READ_HW_ALLOWS_CAPS );
	m_PlxData->CsBoard.NbAnalogChannel = (uInt8)ReadRegisterFromNios( SPLENDACAPS_CHANNEL, CSPLXBASE_READ_HW_ALLOWS_CAPS );

	if ( m_PlxData->CsBoard.NbAnalogChannel == 2 )
		m_pCardState->AcqConfig.u32Mode = CS_MODE_DUAL;
	else if ( m_PlxData->CsBoard.NbAnalogChannel == 1 )
		m_pCardState->AcqConfig.u32Mode = CS_MODE_SINGLE;
	else if ( m_PlxData->CsBoard.NbAnalogChannel == 4 )
		m_pCardState->AcqConfig.u32Mode = CS_MODE_QUAD;
	else
	{
		// Set Default channel count in case of errors
		bErrorCapChannel = true;
		m_PlxData->CsBoard.NbAnalogChannel = 4;
		m_pCardState->AcqConfig.u32Mode = CS_MODE_QUAD;
	}

	m_pCardState->u32LowCutoffFreq[0] = m_pCardState->u32LowCutoffFreq[1] = 0; // All options are low pass
	m_pCardState->u32HighCutoffFreq[1] = 26000000; // Filter is  26 MHz
	m_pCardState->u32HighCutoffFreq[0] = 125000000; //default bandwidth is 125 MHz
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_100MS) )
		m_pCardState->u32HighCutoffFreq[0] = 65000000; // 100 MS board has a 65 MHz bandwidth

	if ( m_bOscar )
	{
		switch(m_pCardState->u32InstalledResolution)
		{
			case 12: m_PlxData->CsBoard.u32BoardType	 = CSE42x0_BOARDTYPE;
					 break;
			case 14: m_PlxData->CsBoard.u32BoardType	 = CSE43x0_BOARDTYPE;
					 break;
			case 16: m_PlxData->CsBoard.u32BoardType	 = CSE44x0_BOARDTYPE;
					 break;
			default:
					bErrorCapResolution = true;
					m_PlxData->CsBoard.u32BoardType	 = CSE42x0_BOARDTYPE;
					break;
		}

		// Set the maximum sampling rate
		switch ( m_PlxData->CsBoard.u32AddonHardwareOptions & (CSPLNDA_ADDON_HW_OSCAR_SR0 | CSPLNDA_ADDON_HW_OSCAR_SR1) )
		{
		case 0x30:
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_10MHZ;
			break;
		case 0x20:
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_25MHZ;
			m_PlxData->CsBoard.u32BoardType += 1;
			break;
		case 0x10:
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_50MHZ;
			m_PlxData->CsBoard.u32BoardType += 2;
			break;
		default:
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_100MHZ;
			m_PlxData->CsBoard.u32BoardType += 3;
			break;
		}
	}
	else
	{
		switch(m_pCardState->u32InstalledResolution)
		{
			case 12: m_PlxData->CsBoard.u32BoardType	 = CS12x1_BOARDTYPE;
					 break;
			case 14: m_PlxData->CsBoard.u32BoardType	 = CS14x1_BOARDTYPE;
					 break;
			case 16: m_PlxData->CsBoard.u32BoardType	 = CS16x1_BOARDTYPE;
					 break;
			default:
					bErrorCapResolution = true;
					m_PlxData->CsBoard.u32BoardType	 = CS12x1_BOARDTYPE;
					break;
		}

		// Set the maximum sampling rate
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_100MS) )
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_100MHZ;
		else
		{
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_200MHZ;
			m_PlxData->CsBoard.u32BoardType++;
		}
	}

	if ( bErrorCapResolution || bErrorCapChannel )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "Error while reading the Channel or Resolution Caps\n");
	}

	if ( m_bNucleon )
		m_PlxData->CsBoard.u32BoardType |= CSPCIEx_BOARDTYPE;

	m_pCardState->AcqConfig.u32ExtClk = 0;
	CsSetDataFormat( formatDefault );

	m_PlxData->CsBoard.NbDigitalChannel	= 0;
	m_PlxData->CsBoard.NbTriggerMachine	= 2*m_PlxData->CsBoard.NbAnalogChannel + 1;

	UpdateMaxMemorySize( m_PlxData->CsBoard.u32BaseBoardHardwareOptions, CSPLNDA_DEFAULT_SAMPLE_SIZE );
	InitBoardCaps();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsSplendaDev::AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo )
{
	char	szModel[128] = {0};

	if ( m_bNucleon )
		strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CSE");
	else
		strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CS");

	if ( m_bOscar )
	{
		switch( m_pCardState->AcqConfig.i64SampleRate )
		{
		case CS_SR_50MHZ:
			strcpy_s(szModel, sizeof(szModel), "4xy4");
			break;
		case  CS_SR_25MHZ:
			strcpy_s(szModel, sizeof(szModel), "4xy2");
			break;
		case CS_SR_10MHZ:
			strcpy_s(szModel, sizeof(szModel), "4xy0");
			break;
		default:
			strcpy_s(szModel, sizeof(szModel), "4xy7");
		}

		// Resolution 'x'
		switch(m_pCardState->u32InstalledResolution)
		{
			default:
			case 12: szModel[1] = '2'; break;
			case 14: szModel[1] = '3'; break;
			case 16: szModel[1] = '4'; break;
		}
	}
	else
	{
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_100MS) )
			strcpy_s(szModel, sizeof(szModel), "1xy1");
		else
			strcpy_s(szModel, sizeof(szModel), "1xy2");

		// Resolution 'x'
		switch(m_pCardState->u32InstalledResolution)
		{
			default:
			case 12: szModel[1] = '2'; break;
			case 14: szModel[1] = '4'; break;
			case 16: szModel[1] = '6'; break;
		}
	}

	// Number of channesl 'y'
	switch(m_PlxData->CsBoard.NbAnalogChannel)
	{
		default:
		case 2: szModel[2] = '2'; break;
		case 4: szModel[2] = '4'; break;
		case 1: szModel[2] = '1'; break;
	}

	strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), szModel );

	if ( pSysInfo->u32BoardCount > 1 )
	{
		char tmp[32];
		sprintf_s( tmp, sizeof(tmp), " M/S - %d", pSysInfo->u32BoardCount );
		strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), tmp );
	}
}

//-----------------------------------------------------------------------------
//	Initialization upon received IOCTL_ACQUISITION_SYSTEM_INIT
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::AcqSystemInitialize()
{
	int32	i32Ret = CS_SUCCESS;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return i32Ret;

	i32Ret = InitAddOnRegisters();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	if( NULL == m_pi32CalibA2DBuffer)
	{
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( CSPLNDA_CALIB_BLOCK_SIZE*sizeof(int32) );
		if( NULL == m_pi32CalibA2DBuffer) 
			return CS_INSUFFICIENT_RESOURCES;
	}
	UpdateCalibVariables();
#ifdef _WINDOWS_    
	ReadCommonRegistryOnAcqSystemInit();
#else
	ReadCommonCfgOnAcqSystemInit();
#endif   

	if ( m_bInvalidateCalib )
		MsInvalidateCalib();

	return i32Ret;
}

#define CSISEPARATOR_LENGTH			16
const char CsiSeparator[] = "-- SEPARATOR --|";		// 16 character long + NULL character

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsSplendaDev::ReadAndValidateCsiFile( uInt32 u32BbHwOptions )
{
	int32			i32RetCode = CS_SUCCESS;
	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry;
	FILE_HANDLE		hCsiFile;

	// Attempt to open Csi file
	hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName );

	if ( ! hCsiFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_NOTFOUND, m_szCsiFileName );
		return CS_MISC_ERROR;
	}

	uInt32	u32BBTarget = CSI_BBSPLENDA_TARGET;
	uInt32	u32ImageSize = BB_FLASHIMAGE_SIZE;

	if ( m_bNucleon )
	{
		u32BBTarget = m_bOscar ? CSI_BBOSCAR_PCIEx_TARGET : CSI_BBSPLENDA_PCIEx_TARGET;
		u32ImageSize = NUCLEON_FLASH_FWIMAGESIZE;
	}

	GageReadFile( hCsiFile, &header, sizeof(header) );
	if ( header.u32Version <= CSIFILE_VERSION )
		m_bCsiOld = ( header.u32Version == PREVIOUS_CSIFILE_VERSION );		// Old Csi 2.0.1 Csi file

	if ( ( header.u32Size == sizeof(CSI_FILE_HEADER) ) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) &&
		 (( header.u32Version == 0 ) || ( header.u32Version == CSIFILE_VERSION ) || ( header.u32Version == PREVIOUS_CSIFILE_VERSION )) )
	{
		for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
		{
			if ( 0 == GageReadFile( hCsiFile, &CsiEntry, sizeof(CsiEntry) ) )
			{
				i32RetCode = CS_FAIL;
				break;
			}

			if ( u32BBTarget == CsiEntry.u32Target )
			{
				// Found Add-on board firmware info
				if ( (CsiEntry.u32HwOptions & CSPLNDA_BBHW_OPT_FWREQUIRED_MASK) ==
					 (u32BbHwOptions & CSPLNDA_BBHW_OPT_FWREQUIRED_MASK ) )
				{
					if ( CsiEntry.u32ImageSize > u32ImageSize )
					{
						i32RetCode = CS_FAIL;
						break;
					}
					else
						GageCopyMemoryX( &m_BaseBoardCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
				}
				else
					continue;
			}
		}
	}
	else
	{
		i32RetCode = CS_FAIL;
	}

	// Additional validation...
	if ( CS_FAIL != i32RetCode )
	{
		if (m_BaseBoardCsiEntry.u32ImageOffset == 0  )
		{
			i32RetCode = CS_FAIL;
		}
		else
		{
			uInt8	u8Buffer[CSISEPARATOR_LENGTH];
			uInt32	u32ReadPosition = m_BaseBoardCsiEntry.u32ImageOffset - CSISEPARATOR_LENGTH;

			if ( 0 == GageReadFile( hCsiFile, u8Buffer, sizeof(u8Buffer), &u32ReadPosition ) )
				i32RetCode = CS_FAIL;
			else
			{
				if ( 0 != GageCompareMemoryX( (void *)CsiSeparator, u8Buffer, sizeof(u8Buffer) ) )
					i32RetCode = CS_FAIL;
			}
		}
	}

	GageCloseFile ( hCsiFile );

	if ( CS_FAIL == i32RetCode )
	{
		m_bCsiFileValid = FALSE;
		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_INVALID, m_szCsiFileName );
	}
	else
		m_bCsiFileValid = TRUE;

	return i32RetCode;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

#define BASEBOARD_FIRMWARE 2

int32	CsSplendaDev::ReadAndValidateFwlFile()
{
	int32				i32RetCode = CS_SUCCESS;
	FWL_FILE_HEADER		header = {0};
	FPGA_HEADER_INFO	FwlEntry = {0};


	// Attempt to open Fwl file
	FILE_HANDLE hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName );

	// If we can't open the file, report it to the event log whether or not we
	// actually will be using it.
	if ( (FILE_HANDLE)0 == hFwlFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_NOTFOUND, m_szFwlFileName );
		return CS_MISC_ERROR;
	}

	// If expert firmware is not present or not allowed on the current card
	// then do not bother reading Fwl file, just close it and return
	if ( 0 == m_PlxData->CsBoard.i64ExpertPermissions )
	{
		GageCloseFile ( hFwlFile );
		return CS_SUCCESS;
	}

	uInt32	u32ImageSize = BB_FLASHIMAGE_SIZE;

	if ( m_bNucleon )
		u32ImageSize = NUCLEON_FLASH_FWIMAGESIZE;

	GageReadFile( hFwlFile, &header, sizeof(header) );
	if ( header.u32Version <= CSIFILE_VERSION )
		m_bFwlOld = ( header.u32Version == PREVIOUS_FWLFILE_VERSION );		// Old Csi 1.02.01 Fwl file

	if ( ( header.u32Size == sizeof(FWL_FILE_HEADER) ) &&
		( ( header.u32BoardType >= CS16xyy_BT_FIRST_BOARD ) && ( CsIsSplendaBoard(header.u32BoardType)) ) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) &&
		 ( (header.u32Version == FWLFILE_VERSION ) || ( header.u32Version == PREVIOUS_FWLFILE_VERSION )) )
	{
		for( uInt32 i = 0; i < header.u32Count; i++ )
		{
			if ( 0 == GageReadFile( hFwlFile, &FwlEntry, sizeof(FwlEntry) ) )
			{
				i32RetCode = CS_FAIL;
				break;
			}
			if( FwlEntry.u32Destination == BASEBOARD_FIRMWARE )
			{
				if ( FwlEntry.u32ImageSize <= u32ImageSize )
				{
					for ( int j = 0; j < sizeof(m_BaseBoardExpertEntry) / sizeof(m_BaseBoardExpertEntry[0]); j ++ )
					{
						if ( m_PlxData->CsBoard.u32BaseBoardOptions[j+1] == FwlEntry.u32Option )
						{
							FPGA_FWVERSION	   FileFwVer;
							RtlCopyMemory( &m_BaseBoardExpertEntry[j], &FwlEntry, sizeof(FwlEntry) );
							// Mask the options bits
							FileFwVer.u32Reg = m_BaseBoardExpertEntry[j].u32Version;
							FileFwVer.Version.uMajor &= 0xF;
							m_BaseBoardExpertEntry[j].u32Version = FileFwVer.u32Reg;
						}
					}
				}
				else
					i32RetCode = CS_FAIL;
			}

			if ( CS_FAIL == i32RetCode )	break;
		}
	}
	else
		i32RetCode = CS_FAIL;

	GageCloseFile ( hFwlFile );
	if ( CS_FAIL == i32RetCode )
	{
		m_bFwlFileValid = FALSE;
		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_INVALID, m_szFwlFileName );
	}
	else
		m_bFwlFileValid = TRUE;

	return i32RetCode;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::SetupHwAverageMode()
{
	uInt32		DevideSelector = 1;
	BOOLEAN		b32BitAveraging = TRUE;		// force using full resolution

	if ( !m_bMulrecAvgTD )
		b32BitAveraging = FALSE;

	// HW Averaging
	// Using full resolution, all devide factor will be ignored
	if ( b32BitAveraging )
	{
		// Set full resolution data format depending on boardtype
		DevideSelector = 0x40;
	}
	else
	{
		//
		//  This mode is not used for now
		//  (Division is made by HW, so we are losing precision)
		//
		switch ( DevideSelector )
		{
			case 256: DevideSelector = 0;	break;
			case 128: DevideSelector = 1;	break;
			case 64:  DevideSelector = 2;	break;
			case 32:  DevideSelector = 3;	break;
			case 16:  DevideSelector = 4;	break;
			case 8:	  DevideSelector = 5;	break;
			case 4:	  DevideSelector = 6;	break;
			case 2:	  DevideSelector = 7;	break;
			case 1:
			default: DevideSelector = 8;	break;
		}
	}

	uInt32 u32Data = ReadGageRegister( GEN_COMMAND_R_REG );
	uInt32 u32ChannelMode = 0;
	u32Data &= ~0x3FF00;

	// Nucleon AVG firmware later than v8.2.3.158 no longer use bits for Channel mode
	if ( ! m_bNucleon )
	{	
		if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE ) != 0 )
			u32ChannelMode = 0;
		else if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ) != 0 )
			u32ChannelMode = 1;
		else if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD ) != 0 )
			u32ChannelMode = 2;
		else
			u32ChannelMode = 3;
	}

	u32Data = (u32ChannelMode <<16) | (DevideSelector << 8) | u32Data;

	WriteGageRegister( GEN_COMMAND_R_REG, u32Data);

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int16	CsSplendaDev::GetTriggerAddressOffsetAdjust( uInt32 u32Mode, uInt32 u32SampleRate )
{
	if ( m_bZeroTrigAddrOffset || m_bNucleon )
		return 0;

	int16 i16Off = 0;

	if ( m_bMulrecAvgTD )
	{
		if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD) != 0)
			i16Off  = 3;
		else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0)
			i16Off  = 6;
		else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE) != 0)
			i16Off  = 12;
	}
	else 
	{
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_ALT_PINOUT)  )
		{
			// Based on Add-on firmware 1.4.0.31
			if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD) != 0)
				i16Off  = -10;
			else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0)
				i16Off  = -12;
			else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE) != 0)
				i16Off  = -16;
		}
		else
			i16Off = CSPLNDA_DEFAULT_TRIG_ADDR_OFF_SINGLE / int16(u32Mode & CS_MASKED_MODE);

		if( m_pCardState->SrInfo[0].llSampleRate / u32SampleRate > 4 )
			i16Off++;
	}

	return i16Off;

}

//-----------------------------------------------------------------------------
// Convert all firmware information to the rrrrrrrrMMMMmmmm format where
// r is the revision
// M is major version
// m is minor version
//
// Ignore date
// Base board Cpld has format MMMMmmmm
//
//-----------------------------------------------------------------------------

BOOL 	CsSplendaDev::CheckRequiredFirmwareVersion()
{
	if ( m_pCardState->bVirginBoard || m_bSkipFirmwareCheck )
		return CS_SUCCESS;

	if ( m_bCsiFileValid )
	{
		// Csi file present, auto update firmware will start later.
		if ( m_bCsiOld )
		{
			// The Csi file is the old version with old firmware version format
			// The version info has been saved using FPGA_FWVERSION structure.
			// The new Csi file use CSFWVERSION instead.

			FPGA_FWVERSION		CsiVer;		// Fw version from csi for Fwl file

			CsiVer.u32Reg = m_BaseBoardCsiEntry.u32FwVersion;
		
			if ( m_pCardState->VerInfo.BbFpgaInfo.Version.uMajor != CsiVer.Version.uMajor ||
				 m_pCardState->VerInfo.BbFpgaInfo.Version.uMinor != CsiVer.Version.uMinor ||
				 m_pCardState->VerInfo.BbFpgaInfo.Version.uRev != CsiVer.Version.uRev ||
				 m_pCardState->VerInfo.BbFpgaInfo.Version.uIncRev != CsiVer.Version.uIncRev )
				m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
		}
		else if ( GetBoardNode()->u32UserFwVersionB[0].u32Reg != m_BaseBoardCsiEntry.u32FwVersion )
		{
			m_PlxData->CsBoard.u8BadBbFirmware |= BAD_DEFAULT_FW;
		}
	}

	// added March 22, 2011 because of problem when reinitialing from CsExpertUpdate
	// we would failinitialization below with BAD_EXPERT1_FW because the version of 
	// CsiVer.u32Reg and ExpertVer.u32Reg.  Now, if we don't detect expert firmware
	// we don't even check the version of the expert firmware

	BOOLEAN bExpertFwDetected = FALSE;
	for ( int i = 1; i <= 2; i ++ )
	{
		if ( GetBoardNode()->u32BaseBoardOptions[i] != 0 )
		{
			bExpertFwDetected = TRUE;
			break;
		}
	}

	// Check the Expert image firmwares
	if ( m_bFwlFileValid && bExpertFwDetected )
	{
		if ( m_bFwlOld )
		{
			// The Fwl file is the old version with old firmware version format
			// The version info has been saved using FPGA_FWVERSION structure.
			// The new Fwl file use CSFWVERSION instead.

			FPGA_FWVERSION		CsiVer;
			FPGA_FWVERSION		ExpertVer;

			for ( int i = 0; i < sizeof(m_BaseBoardExpertEntry) / sizeof(m_BaseBoardExpertEntry[0]); i ++ )
			{
				CsiVer.u32Reg = m_BaseBoardExpertEntry[i].u32Version;
				ExpertVer.u32Reg = GetBoardNode()->u32RawFwVersionB[i+1];

				if ( ExpertVer.Version.uMajor != CsiVer.Version.uMajor ||
					 ExpertVer.Version.uMinor != CsiVer.Version.uMinor ||
					 ExpertVer.Version.uRev != CsiVer.Version.uRev ||
					 ExpertVer.Version.uIncRev != CsiVer.Version.uIncRev )
					m_PlxData->CsBoard.u8BadBbFirmware |= BAD_EXPERT1_FW << i;		
			}
		}
		else
		{
			for ( int i = 0; i < sizeof(m_BaseBoardExpertEntry) / sizeof(m_BaseBoardExpertEntry[0]); i ++ )
			{
				if ( ( GetBoardNode()->u32UserFwVersionB[i+1].u32Reg != 0 ) && 
					( GetBoardNode()->u32UserFwVersionB[i+1].u32Reg != m_BaseBoardExpertEntry[i].u32Version ) )
				{
					m_PlxData->CsBoard.u8BadBbFirmware |= BAD_EXPERT1_FW << i;	
				}
			}
		}
	}

	return (0 == m_PlxData->CsBoard.u8BadBbFirmware);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
uInt32	CsSplendaDev:: GetFifoModeTransfer( uInt16 u16UserChannel )
{
	uInt32	u32ModeSel = 0;

	// Convert user channel number (ex :1,3, 5 ..)
	// to Nios channel number (1, 2, 3)
	u16UserChannel = (u16UserChannel - 1) / GetUserChannelSkip() + 1;
	u32ModeSel = (u16UserChannel - 1) << 4;

	if ( (m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD) != 0  )
		u32ModeSel |= MODESEL2;
	else if ( (m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0  )
		u32ModeSel |= MODESEL1;

	return u32ModeSel;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::UpdateMaxMemorySize( uInt32	u32BaseBoardHwOptions, uInt32 u32SampleSize )
{
	UNREFERENCED_PARAMETER(u32BaseBoardHwOptions);

	uInt32 u32MemSizeKb = ReadRegisterFromNios(0, CSPLXBASE_GET_MEMORYSIZE_CMD);

	// Convert to number of samples
	m_PlxData->CsBoard.i64MaxMemory = (LONGLONG(u32MemSizeKb) << 10)/u32SampleSize;
	m_PlxData->InitStatus.Info.u32BbMemTest = m_PlxData->CsBoard.i64MaxMemory != 0 ? 1 : 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsSplendaDev::VerifySwappedFpgaBus()
{
	SPLENDA_FLASHFOOTER BoardHwInfo;
	uInt32 u32Offset = FIELD_OFFSET(SPLNDA_FLASH_LAYOUT, Footer);
	int32 i32Ret = ReadEeprom( &BoardHwInfo, u32Offset, sizeof(BoardHwInfo) );
	if( CS_SUCCEEDED(i32Ret) )
	{
		if ( 0 != BoardHwInfo.Ft.i64ExpertPermissions ) //There should not be any experts permission but wrong layout would produce all 1's
		{
			BoardHwInfo.Ft.i64ExpertPermissions = 0;
			i32Ret = WriteEepromEx( &BoardHwInfo, u32Offset, sizeof(BoardHwInfo), false );
			if( CS_FAILED(i32Ret) )	return false;

			i32Ret = ReadEeprom( &BoardHwInfo, u32Offset, sizeof(BoardHwInfo) );
			return ( 0 == BoardHwInfo.Ft.i64ExpertPermissions );
		}
		else
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsSplendaDev::ClearFlag_bCalNeeded()
{
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		m_pCardState->bCalNeeded[ConvertToHwChannel(i)-1] = false;
	}
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSplendaDev::RemoveDeviceFromUnCfgList()
{
	CsSplendaDev	*Current	= (CsSplendaDev	*) m_ProcessGlblVars->pFirstUnCfg;
	CsSplendaDev	*Prev = NULL;

	if ( this  == (CsSplendaDev	*) m_ProcessGlblVars->pFirstUnCfg )
	{
		m_ProcessGlblVars->pFirstUnCfg = (PVOID) this->m_Next;
	}


	while( Current->m_Next )
	{
		if ( this == Current )
		{
			if ( Prev )
				Prev->m_Next = Current->m_Next;
		}

		Prev = Current;
		Current = Current->m_Next;
	}

	if ( this  == (CsSplendaDev	*) m_ProcessGlblVars->pLastUnCfg )
	{
		if (Prev)
			Prev->m_Next = NULL;
		m_ProcessGlblVars->pLastUnCfg = (PVOID) Prev;
	}

	return CS_SUCCESS;

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::IoctlProgramAddonFpga(uInt8 *pBuffer, uInt32 u32ImageSize)
{
	uInt32			u32RetCode;
	uInt32			u32BytesReturned;
	CS_IOCTL_STATUS	IoCtlStatus;

#ifdef _WINDOWS_
	u32RetCode = DeviceIoControl( GetIoctlHandle(),
					 CS_IOCTL_PROGRAM_ADDON_FPGA,
					 pBuffer,
					 u32ImageSize,
					 &IoCtlStatus,
					 sizeof(IoCtlStatus),
					 &u32BytesReturned,
					 NULL );
#else
	ADDON_FPGA_DESC		AddonFpgaDesc;

	AddonFpgaDesc.pFpgaBuffer 	= pBuffer;
	AddonFpgaDesc.u32BufferSize = u32ImageSize;
	u32RetCode = GAGE_DEVICE_IOCTL( GetIoctlHandle(),
						 CS_IOCTL_PROGRAM_ADDON_FPGA,
						 &AddonFpgaDesc);

	// IoCtlStatus not used for Linux.
	// Check for u32RetCode in next section...
	IoCtlStatus.i32Status = CS_SUCCESS;

#endif

	_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		u32RetCode = GetLastError();
		return CS_DEVIOCTL_ERROR;
	}

	return IoCtlStatus.i32Status;
}


//-----------------------------------------------------------------------------
//	Convert the User channel to Hw channel
//	Ex: User Channel 2 from 2 channels card is actually channel 3 from HW point of view
//		because only channels 1 and 3 (of the 4 channels) are being used.
//-----------------------------------------------------------------------------
uInt16	CsSplendaDev::ConvertToHwChannel( uInt16 u16UserChannel )
{
	if ( 0 == u16UserChannel ) return 0;
	uInt8	u8Skip(1);
	switch( m_PlxData->CsBoard.NbAnalogChannel )
	{
		case 4:	u8Skip = 1; break;
		case 2:	u8Skip = 2;	break;
		case 1: u8Skip = 4;	break;
	}
	return (u16UserChannel - 1 ) * u8Skip + 1;
}

//-----------------------------------------------------------------------------
//	Convert the Hw channel to User channel
//	Ex: Hardware channel 3 from 4 channels card is actually channel 2 from user point of view
//-----------------------------------------------------------------------------
uInt16	CsSplendaDev::ConvertToUserChannel( uInt16 u16HwChannel )
{
	if ( 0 == u16HwChannel ) return 0;
	uInt8	u8Skip(1);
	switch( m_PlxData->CsBoard.NbAnalogChannel )
	{
		case 4:	u8Skip = 1;	break;
		case 2:	u8Skip = 2; break;
		case 1:	u8Skip = 4;	break;
	}
	return (u16HwChannel - 1 ) / u8Skip + 1;
}

#ifdef _WINDOWS_

//------------------------------------------------------------------------------
//
//	Parameters on Board initialization.
//  Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

void	CsSplendaDev::ReadCommonRegistryOnBoardInit()
{
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Cs16xyyWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_bSkipFirmwareCheck ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipCheckFirmware"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipFirmwareCheck = (ul != 0);

		ul = m_bSkipSpiTest ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipSpiTest"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipSpiTest = (ul != 0);

		ul = m_bForce100MSps ? 1: 0;
		::RegQueryValueEx( hKey, _T("Force100"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bForce100MSps = (0 != ul);

		ul = m_b5V1MDiv50 ? 1 : 0;
		::RegQueryValueEx( hKey, _T("5V1MDiv50"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_b5V1MDiv50 = (ul != 0);

		ul = m_bSkipMasterSlaveDetection ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipMasterSlaveDetection"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipMasterSlaveDetection = (ul != 0);

		ul = m_bNoConfigReset ? 1 : 0;
		::RegQueryValueEx( hKey, _T("NoConfigReset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bNoConfigReset = (ul != 0);

		ul = m_bClearConfigBootLocation ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ClearCfgBootLocation"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bClearConfigBootLocation = (ul != 0);


		::RegCloseKey( hKey );
	}	

}



//------------------------------------------------------------------------------
//
//	Parameters on Acquisition system initialization.
//  Restart application is nessesary 
//
//------------------------------------------------------------------------------

void	CsSplendaDev::ReadCommonRegistryOnAcqSystemInit()
{
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Cs16xyyWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = 0;
		::RegQueryValueEx( hKey, _T("ZeroDepthAdjust"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroDepthAdjust = (ul != 0);

		ul = 0;
		::RegQueryValueEx( hKey, _T("ZeroTrigAddrOffset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroTrigAddrOffset = (ul != 0);

		ul = 0;
		::RegQueryValueEx( hKey, _T("ZeroTrigAddress"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroTriggerAddress = (ul != 0);

		ul = m_u8TrigSensitivity;
		::RegQueryValueEx( hKey, _T("DTriggerSensitivity"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul <= 0xFF )
			m_u8TrigSensitivity = (uInt8) ul;
		else
			m_u8TrigSensitivity = 0xFF;

		ul = m_u16AdcOffsetAdjust;
		::RegQueryValueEx( hKey, _T("AdcOffsetAdjust"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u16AdcOffsetAdjust = (uInt16) ul;

		ul = m_bClockFilter ? 1:0;
		::RegQueryValueEx( hKey, _T("ClockFilter"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bClockFilter = (ul != 0);

		ul = m_bNoFilter ? 1 : 0;
		::RegQueryValueEx( hKey, _T("NoFilter"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bNoFilter = 0 != ul;

		ul = m_u16TrigOutWidth;
		::RegQueryValueEx( hKey, _T("TrigOutWidth"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u16TrigOutWidth = (uInt16) ul;

		ul = m_bGainTargetAdjust ? 1 : 0;
		::RegQueryValueEx( hKey, _T("GainTargetAdjust"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bGainTargetAdjust = (ul != 0);

		::RegCloseKey( hKey );
	}	

	ReadCalibrationSettings(NULL);

}

#else
#define DRV_SECTION_NAME      "Razor"

//------------------------------------------------------------------------------
// Linux only
//	Parameters on Board initialization.
// Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::ReadCommonCfgOnBoardInit()
{
   char szIniFile[256]={0};
   char szSectionName[16]={0};
   char szDummy[128]={0};
   ULONG	ul(0);
	DWORD	DataSize = sizeof(ul);
   
   if (!GetCfgFile(szIniFile,sizeof(szIniFile)))
      return -1;
   
   strcpy(szSectionName, DRV_SECTION_NAME);
   
   if ( !GetCfgSection(szSectionName,szDummy, 100, szIniFile) )
   {
      /* nothing set for the boad, use the default value */
      strcpy(szSectionName, DRV_CFG_DEFAULT);
      if ( !GetCfgSection(szSectionName,szDummy, 100, szIniFile) )
         return -1;
   }
   
   ul = 0;
   ul = GetCfgKeyInt(szSectionName, DISPLAY_CFG_KEYINFO, ul, szIniFile);	
   if (ul)
      ShowCfgKeyInfo();
   
#if 0 // Key not support Linux
	ul = m_bSkipFirmwareCheck ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("SkipCheckFirmware"),  ul,  szIniFile);
	m_bSkipFirmwareCheck = (ul != 0);

	ul = m_bSkipSpiTest ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("SkipSpiTest"),  ul,  szIniFile);
	m_bSkipSpiTest = (ul != 0);

	ul = m_bClearConfigBootLocation ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ClearCfgBootLocation"),  ul,  szIniFile);
	m_bClearConfigBootLocation = (ul != 0);
      
	ul = m_bSkipMasterSlaveDetection ? 1 : 0;
	::RegQueryValueEx( hKey, _T("SkipMasterSlaveDetection"), NULL, NULL, (LPBYTE)&ul, &DataSize );
   ul = GetCfgKeyInt(szSectionName, _T("SkipMasterSlaveDetection"),  ul,  szIniFile);
	m_bSkipMasterSlaveDetection = (ul != 0);
#endif      

	ul = m_bForce100MSps ? 1: 0;
   ul = GetCfgKeyInt(szSectionName, _T("Force100"),  ul,  szIniFile);
	m_bForce100MSps = (0 != ul);

	ul = m_b5V1MDiv50 ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("5V1MDiv50"),  ul,  szIniFile);
	m_b5V1MDiv50 = (ul != 0);

	ul = m_bNoConfigReset ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NoConfigReset"),  ul,  szIniFile);
	m_bNoConfigReset = (ul != 0);
   
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
// Linux only
//------------------------------------------------------------------------------
int32 CsSplendaDev::ReadCommonCfgOnAcqSystemInit()
{
	ULONG	ul(0);
	DWORD	DataSize = sizeof(ul);
   char szSectionName[16] = {0};
   char szDummy[128]={0};
   char szIniFile[256] = {0};

   if (!GetCfgFile(szIniFile,sizeof(szIniFile)))
      return -1;

   strcpy(szSectionName, DRV_SECTION_NAME);
      
   if ( !GetCfgSection(szSectionName,szDummy, 100, szIniFile) )
   {
      /* Nothing set for the boad, search the default value */
      strcpy(szSectionName, DRV_CFG_DEFAULT);
      if ( !GetCfgSection(szSectionName,szDummy, 100, szIniFile) )
         return -1;
   }
   
   //fprintf(stdout, "Loading [%s] parameters from file : %s \n",szSectionName, szIniFile);

	ul = 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroDepthAdjust"),  ul,  szIniFile);	
	m_bZeroDepthAdjust = (ul != 0);

	ul = 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroTrigAddrOffset"),  ul,  szIniFile);	
	m_bZeroTrigAddrOffset = (ul != 0);

	ul = 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroTrigAddress"),  ul,  szIniFile);	
	m_bZeroTriggerAddress = (ul != 0);

	ul = m_u8TrigSensitivity;
   ul = GetCfgKeyInt(szSectionName, _T("DTriggerSensitivity"),  ul,  szIniFile);	
	if ( ul <= 0xFF )
		m_u8TrigSensitivity = (uInt8) ul;
	else
		m_u8TrigSensitivity = 0xFF;

	ul = m_u16AdcOffsetAdjust;
   ul = GetCfgKeyInt(szSectionName, _T("AdcOffsetAdjust"),  ul,  szIniFile);	
	m_u16AdcOffsetAdjust = (uInt16) ul;

	ul = m_bClockFilter ? 1:0;
   ul = GetCfgKeyInt(szSectionName, _T("ClockFilter"),  ul,  szIniFile);	
	m_bClockFilter = (ul != 0);

	ul = m_bNoFilter ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NoFilter"),  ul,  szIniFile);	
	m_bNoFilter = 0 != ul;

	ul = m_u16TrigOutWidth;
   ul = GetCfgKeyInt(szSectionName, _T("TrigOutWidth"),  ul,  szIniFile);	
	m_u16TrigOutWidth = (uInt16) ul;

	ul = m_bGainTargetAdjust ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("GainTargetAdjust"),  ul,  szIniFile);	
	m_bGainTargetAdjust = (ul != 0);

	ReadCalibrationSettings(szIniFile, szSectionName);

	return CS_SUCCESS;
}

int32	CsSplendaDev::ShowCfgKeyInfo()   
{

   printf("---------------------------------\n");
   printf("    Driver Config Setting Infos \n");
   printf("---------------------------------\n");
   printf("    SectionName: [%s] \n", DRV_SECTION_NAME);
   printf("    List of Cfg keys supported: \n");
   
   
   printf("\t%s\n\n","Force100");
   printf("\t%s\n\n","5V1MDiv50");
   printf("\t%s\n\n","NoConfigReset");
   
   printf("\t%s\n\n","ZeroDepthAdjust");
   printf("\t%s\n\n","ZeroTrigAddrOffset");
   printf("\t%s\n\n","ZeroTrigAddress");
   printf("\t%s\n\n","DTriggerSensitivity");
   printf("\t%s\n","AdcOffsetAdjust");
   printf("\t%s\n","ClockFilter");
   printf("\t%s\n","NoFilter");
   printf("\t%s\n","TrigOutWidth");
   printf("\t%s\n","GainTargetAdjust");
   
   
   printf("\t%s\n","FastDcCal");
   printf("\t%s\n","UseEepromCal");
   printf("\t%s\n","UseEepromCalBackup");
   printf("\t%s\n","NoCalibrationDelay");
   printf("\t%s\n","NeverCalibrateDc");
   printf("\t%s\n","DebugCalibSource");
   printf("\t%s\n","TraceCalib");
   printf("\t%s\n","LogFaluireOnly");
   printf("\t%s\n","CalDacSettleDelay");
   printf("\t%s\n","CalSrcDelay");
   printf("\t%s\n","CalSrcSettleDelay");
   printf("\t%s\n","CalSrcSettleDelay1MOhm");
   printf("\t%s\n","IgnoreCalibError");
   printf("\t%s\n","InvalidateCalib");

   printf("---------------------------------\n");
   
}      
#endif

void CsSplendaDev::UpdateCardState(BOOLEAN bReset)
{
//	m_pCardState->bCardStateValid = !bReset;
#if !defined(USE_SHARED_CARDSTATE)
	IoctlSetCardState(m_pCardState);
#else
	UNREFERENCED_PARAMETER(bReset);
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool	CsSplendaDev::IsImageValid( uInt8 u8Index )
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32Offset;
	CS_FIRMWARE_INFOS	FwInfo = {0};

	u32Offset = FIELD_OFFSET(NUCLEON_FLASH_IMAGE_STRUCT, FwInfo) +
				FIELD_OFFSET(NUCLEON_FLASH_FWINFO_SECTOR, FwInfoStruct);

	switch ( u8Index )
	{
		case 1:	u32Offset += FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage1); break;
		case 2:	u32Offset += FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage2); break;
		case 3:	u32Offset += FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage3); break;
		default:
			u32Offset += FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, BootImage); break;
	}

	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_FAILED(i32Status) )
		return false;

	if ( CSI_FWCHECKSUM_VALID != FwInfo.u32ChecksumSignature )
		return false;
	else
		return true;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int64	CsSplendaDev::GetTimeStampFrequency()
{
	int64	i64TimeStampFreq = 0;

	if ( m_bNucleon )
	{
		uInt8 u8Shift = 0;
		switch(m_pCardState->AcqConfig.u32Mode & (CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE))
		{
			default:
			case CS_MODE_QUAD:	u8Shift = 3; break;
			case CS_MODE_DUAL:	u8Shift = 2; break;
			case CS_MODE_SINGLE:u8Shift = 1; break;
		}
		if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
		{
			i64TimeStampFreq = CSPLNDA_PCIEX_MEMORY_CLOCK;
			i64TimeStampFreq <<= 4;
		}
		else
		{
			i64TimeStampFreq = m_pCardState->AcqConfig.i64SampleRate;
			i64TimeStampFreq <<= u8Shift;
		}
	}
	else if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
	{
		uInt8 u8Shift = 0;
		switch(m_pCardState->AcqConfig.u32Mode & (CS_MODE_DUAL | CS_MODE_SINGLE))
		{
			default:
			case CS_MODE_OCT:	u8Shift = 0; break;
			case CS_MODE_QUAD:	u8Shift = 1; break;
			case CS_MODE_DUAL:	u8Shift = 2; break;
			case CS_MODE_SINGLE:u8Shift = 3; break;
		}
		i64TimeStampFreq = CSPLNDA_MED_MEMORY_CLOCK;
		i64TimeStampFreq <<= u8Shift;
	}
	else
	{
		i64TimeStampFreq = m_pCardState->AcqConfig.i64SampleRate;
	}

	return i64TimeStampFreq;
}



//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
uInt32	CsSplendaDev:: GetDataMaskModeTransfer ( uInt32 u32TxMode )
{
	uInt32	u32DataMask = 0;

	// Clear Slave mode bit
	u32TxMode &=  ~TxMODE_SLAVE;

	switch(m_pCardState->u32InstalledResolution)
	{
	case 12:
		u32DataMask = 0xF000F;
		break;
	case 14:
		u32DataMask = 0x30003;
		break;
	}

	switch( u32TxMode )
	{
		case TxMODE_DATA_FLOAT:
		case TxMODE_TIMESTAMP:
		case TxMODE_DATA_32:
		case TxMODE_DATA_16:
			// Seting for full 16 bit data transfer
			u32DataMask = 0x00000;
		break;

		case TxMODE_DATA_ONLYDIGITAL:
			u32DataMask = ~(u32DataMask); // Masking everything but bits for digital channel
		break;
	}

	return u32DataMask;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsSplendaDev::IsConfigResetRequired( uInt8 u8ExpertImageId )
{
	// For Nucleon PCIe, Streaming is supported by the default firmware. We do not have to change to the expert firmware for streaming 
	// unless the expert firmware version is different to the default one.
	// For other expert, switching to expert firmware is required.
	if ( ( (0 != u8ExpertImageId) && (m_PlxData->CsBoard.u32BaseBoardOptions[u8ExpertImageId] & CS_BBOPTIONS_STREAM ) == 0) )
		return true;
	else 
		return false; // ( m_PlxData->CsBoard.u32UserFwVersionB[m_pCardState->u8ImageId].u32Reg != m_PlxData->CsBoard.u32UserFwVersionB[u8ExpertImageId].u32Reg );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSplendaDev::MsInitializeDevIoCtl()
{
	int32		 i32Status = CS_SUCCESS;
	CsSplendaDev* pDevice = m_MasterIndependent;

	while ( pDevice )
	{
		// Open handle to kernel driver
		i32Status = pDevice->InitializeDevIoCtl();

#if !defined( USE_SHARED_CARDSTATE )
		i32Status = pDevice->IoctlGetCardState(pDevice->m_pCardState);
#endif
		if ( CS_FAILED(i32Status) )
			break;

		pDevice = pDevice->m_Next;
	}

	if ( CS_FAILED(i32Status) )
	{
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			// Close handle to kernel driver
			pDevice->UnInitializeDevIoCtl();
			pDevice = pDevice->m_Next;
		}
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsSplendaDev::IsExpertAVG(uInt8 u8ImageId)
{
	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & CS_BBOPTIONS_MULREC_AVG_TD) != 0);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsSplendaDev::IsExpertSTREAM(uInt8 u8ImageId)
{
	if ( 0 == u8ImageId )
		u8ImageId = m_pCardState->u8ImageId;

	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & CS_BBOPTIONS_STREAM) != 0);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsSplendaDev::PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg )
{
	int64	i64SegmentCount = pCsAcqCfg->i32SegmentCountHigh;
	i64SegmentCount = i64SegmentCount << 32 | pCsAcqCfg->u32SegmentCount;

	m_Stream.bInfiniteSegmentCount	= ( i64SegmentCount < 0 );
	m_Stream.bRunForever			= m_Stream.bInfiniteDepth || m_Stream.bInfiniteSegmentCount;

	if ( m_Stream.bRunForever )
		m_Stream.u64TotalDataSize	= (ULONGLONG)-1;
	else
	{
		// Calulcate the total data size for stream
		m_Stream.u64TotalDataSize = pCsAcqCfg->i64SegmentSize*(pCsAcqCfg->u32Mode & CS_MASKED_MODE) * pCsAcqCfg->u32SampleSize * i64SegmentCount;
		if ( ! m_bMulrecAvgTD )
			m_Stream.u64TotalDataSize += m_32SegmentTailBytes * i64SegmentCount;
	}
}