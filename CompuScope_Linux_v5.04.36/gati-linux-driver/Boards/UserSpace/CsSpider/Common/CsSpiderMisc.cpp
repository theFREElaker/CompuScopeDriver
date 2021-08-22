

#include "StdAfx.h"
#include "CsSpider.h"
#include "CsEventLog.h"
#include "CsPrivatePrototypes.h"
#include "CsIoctl.h"
#include "CsNucleonBaseFlash.h"

#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif

extern SHAREDDLLMEM Globals;

#define	NO_CONFIG_BOOTCPLD			0x17
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::DeviceInitialize(void)
{
	int32 i32Ret = CS_SUCCESS;
	i32Ret = CsPlxBase::Initialized();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	m_u16NextSlave	= 0;
	m_ermRoleMode = ermStandAlone;
	m_UseIntrTrigger = m_UseIntrBusy = m_UseIntrTransfer = FALSE;
	m_TrigAddrOffsetAdjust = 0;
	m_IntPresent		= FALSE;
	m_pNextInTheChain	= NULL;
	m_pTriggerMaster	= this;
	m_MasterIndependent = this;
	m_bInvalidHwConfiguration = FALSE;
	m_bSkipMasterSlaveDetection = false;
	m_u32IgnoreCalibErrorMask = 0xff;
	m_u32IgnoreCalibErrorMaskLocal = 0;
	m_i16LpXferBuffer	= NULL;
	// Extras samples to ensure post trigger depth
	m_u32TailExtraData = 0;

#ifdef _WINDOWS_   
	ReadCommonRegistryOnBoardInit();
#else
	ReadCommonCfgOnBoardInit();
#endif   

#ifdef USE_SHARED_CARDSTATE
	if ( m_pCardState->bCardStateValid )
	{
		if (  m_pCardState->bAddOnRegistersInitialized )
			return CS_SUCCESS;
	}
	else if ( 0 != m_pCardState->u8ImageId  )
		CsLoadBaseBoardFpgaImage( 0 );
#else
		SPIDER_CARDSTATE			tempCardState = {0};
		i32Ret = IoctlGetCardState(&tempCardState);
		if ( CS_FAILED(i32Ret) )
			return i32Ret;

		if ( tempCardState.bCardStateValid )
		{
			GageCopyMemoryX(m_pCardState, &tempCardState,  sizeof(SPIDER_CARDSTATE));
			if ( m_pCardState->bAddOnRegistersInitialized )
				return CS_SUCCESS;
		}
		else
		{
			m_pCardState->u16DeviceId = tempCardState.u16DeviceId;
			m_pCardState->bPciExpress = tempCardState.bPciExpress;

			if ( 0 != (m_pCardState->u8ImageId = tempCardState.u8ImageId) )
				CsLoadBaseBoardFpgaImage( 0 );
		}
#endif

	AbortAcquisition ();
	// Make sure that the SPI is ON because it may have been turned OFF during the previous session
	ControlSpiClock(true);

	ResetCachedState();
	m_pCardState->bAddOnRegistersInitialized = false;
	m_pCardState->bAuxConnector = false;

	m_pCardState->u16CardNumber			= 1;
	m_pCardState->eclkoClockOut			= CSPDR_DEFAULT_CLOCK_OUT;
	m_pCardState->u16AdcOffsetAdjust	= CSPDR_DEFAULT_ADCOFFSET_ADJ;
	m_pCardState->epmPulseOut			= epmNone;
	m_pCardState->u16ChanProtectStatus	= 0;
	m_pCardState->bAddonInit			= FALSE;

	// Reaset Auto update firmware variables
	m_PlxData->CsBoard.u8BadAddonFirmware = m_PlxData->CsBoard.u8BadBbFirmware = 0;
	memset( &m_BaseBoardCsiEntry, 0, sizeof(m_BaseBoardCsiEntry) );
	memset( &m_AddonCsiEntry, 0, sizeof(m_AddonCsiEntry) );
	memset( m_BaseBoardExpertEntry, 0, sizeof(m_BaseBoardExpertEntry) );
	memset( m_AddonBoardExpertEntry, 0, sizeof(m_AddonBoardExpertEntry) );

	PCS_BOARD_NODE  pBdParam = &m_PlxData->CsBoard;

	memset( pBdParam->u32RawFwVersionB, 0, sizeof(pBdParam->u32RawFwVersionB) );
	memset( pBdParam->u32RawFwVersionA, 0, sizeof(pBdParam->u32RawFwVersionA) );

	// Reset the firmware version variables before reading firmware version
	RtlZeroMemory( &m_pCardState->VerInfo, sizeof(m_pCardState->VerInfo) );
	m_pCardState->VerInfo.u32Size = sizeof(CS_FW_VER_INFO);

	memset(&m_AddonCsiEntry, 0, sizeof( m_AddonCsiEntry ) );
	memset(&m_BaseBoardCsiEntry, 0, sizeof( m_AddonCsiEntry ) );

	RtlZeroMemory( GetBoardNode()->u32RawFwVersionB, sizeof(GetBoardNode()->u32RawFwVersionB) );
	RtlZeroMemory( GetBoardNode()->u32RawFwVersionA, sizeof(GetBoardNode()->u32RawFwVersionA) );

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

		m_pCardState->bVirginBoard = (false ==IsImageValid(0));
		if ( ! m_pCardState->bVirginBoard )
		{
			// ConfigurationReset can generate bogus interrupt. Make sure all interrupts are diabled
			ClearInterrupts();
			if ( ! ConfigurationReset(0) )
				m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
		}
		else
		{
			CsEventLog EventLog;
			EventLog.Report( CS_WARNING_TEXT, "Nucleon Virgin board detected. Update firmware is disabled." );
		}
	}
	else
	{
		PLX_NVRAM_IMAGE	nvRAM_Image;

		int32 i32Status = CsReadPlxNvRAM( &nvRAM_Image );
		if ( CS_FAILED( i32Status ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Error while reading NVRAM." );
			return i32Status;
		}

		m_PlxData->CsBoard.u32BoardType = CS8xxx_BASEBOARD;
		m_PlxData->CsBoard.u32BaseBoardVersion = nvRAM_Image.Data.BaseBoardVersion;
		m_PlxData->CsBoard.u32SerNum = nvRAM_Image.Data.BaseBoardSerialNumber;
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions = nvRAM_Image.Data.BaseBoardHardwareOptions;
	}

	BOOL	bNiosOK = IsNiosInit();
	if ( bNiosOK )
	{
		ClearInterrupts();
		ReadBaseBoardHardwareInfoFromFlash(FALSE);

		m_PlxData->InitStatus.Info.u32AddOnPresent = IsAddonBoardPresent();
		if ( m_PlxData->InitStatus.Info.u32AddOnPresent )
		{
			if ( AssignBoardProperty() )
			{
				InitAddOnRegisters();
				if ( !m_pCardState->bAddOnRegistersInitialized )
				{
					// Fail to initialize the addon. The problem may cause by invalid or incompatible firmwares
					m_PlxData->CsBoard.u8BadAddonFirmware = BAD_DEFAULT_FW;
					m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
					CsEventLog EventLog;
					EventLog.Report( CS_ERROR_TEXT, "Failed to initialize Addon." );
				}
			}
			else if ( m_bInvalidHwConfiguration )
			{
				// This may caused by bad base board firmware
				// Set Flag to force update firmware later
				m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;

				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Fatal Bad Hardware Configuration" );
			}
		}
	}

	if ( ! bNiosOK || !m_PlxData->InitStatus.Info.u32AddOnPresent )
	{
		// Set the board type so that the utility ImageZero could detect the board ten run setup later
		// Default board property
		AssignBoardProperty();

		// This may caused by bad base board firmware
		// Set Flag to force update firmware later
		m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
		
		CsEventLog EventLog;
		if ( ! m_PlxData->InitStatus.Info.u32AddOnPresent )
			EventLog.Report( CS_ERROR_TEXT, "Addon board is not connected." );
		else
			EventLog.Report( CS_ERROR_TEXT, "Nios Initialization error." );
	}
	else
		ReadAddOnHardwareInfoFromEeprom(FALSE);

	// Read Firmware versions
	ReadVersionInfo();
	ReadVersionInfoEx();
	UpdateCardState();

	// Intentional return success to allows some utilitiy programs to update firmware in case of errors
	return CS_SUCCESS;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsSpiderDev::Abort()
{
	CsSpiderDev* pCard = m_MasterIndependent;

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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::MsResetDefaultParams( BOOLEAN bReset )
{
	int32		i32Ret = CS_SUCCESS;
	CsSpiderDev *pDevice = m_MasterIndependent;

	// Set Default configuration
	while( pDevice )
	{
		if ( m_pCardState->bSpiderLP )
		{
			::GageFreeMemoryX( pDevice->m_i16LpXferBuffer );
			// ST81LP has default firmware as Hw AVG simulated as regular firmware. By consquence
			// Data is not save on memory and will be destroy on read
			// this buffer is used to keep a beckup of capture data in case if application perform multiple
			// data transfer
			// Allocation should be done in the first place inorder to make sure the buffer is allocated even if
			// the cards has some problem with calibration ...
			pDevice->m_i16LpXferBuffer = (int16 *)::GageAllocateMemoryX( (m_u32MaxHwAvgDepth + AV_ENSURE_POSTTRIG)*sizeof(int16) );
		}

		if ( bReset )
			pDevice->ResetCachedState();

		if( pDevice != m_pTriggerMaster )
		{
			i32Ret = pDevice->CsSetAcquisitionConfig();
			if( CS_FAILED(i32Ret) )
				return i32Ret;
		}

		pDevice->SetDefaultTrigAddrOffset();
		pDevice = pDevice->m_Next;
	}

	if ( bReset )
		m_pTriggerMaster->ResetCachedState();

	i32Ret = m_pTriggerMaster->CsSetAcquisitionConfig();
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		if(!pDevice->m_PlxData->InitStatus.Info.u32Nios )
			return CS_FLASH_NOT_INIT;

		i32Ret = pDevice->InitAddOnRegisters(true);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		pDevice->AddonReset();

		i32Ret = pDevice->CsSetChannelConfig();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = pDevice->CsSetTriggerConfig();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		pDevice->SetMsTriggerEnable( ermStandAlone != m_pCardState->ermRoleMode );

		pDevice->UpdateCardState();
		pDevice = pDevice->m_Next;
	}

	m_MasterIndependent->MsCalibrateAllChannels();
	m_MasterIndependent->ResetMasterSlave(true);
	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//----- CsSpiderDev::UpdateSystemInfo(PSYSTEM_NODE pSys)
//-----------------------------------------------------------------------------

void	CsSpiderDev::UpdateSystemInfo( CSSYSTEMINFO *SysInfo )
{
	PCS_BOARD_NODE  pBdParam = &m_PlxData->CsBoard;


	SysInfo->u32Size				= sizeof(CSSYSTEMINFO);
	SysInfo->u32SampleBits			= m_pCardState->AcqConfig.u32SampleBits;
	SysInfo->i32SampleResolution	= m_pCardState->AcqConfig.i32SampleRes;
	SysInfo->u32SampleSize			= m_pCardState->AcqConfig.u32SampleSize;
	SysInfo->i32SampleOffset		= m_pCardState->AcqConfig.i32SampleOffset;
	SysInfo->u32BoardType			= pBdParam->u32BoardType;
	SysInfo->u32TriggerMachineCount = 2 * m_PlxData->CsBoard.NbAnalogChannel * SysInfo->u32BoardCount + 1;
	SysInfo->u32ChannelCount		= pBdParam->NbAnalogChannel * SysInfo->u32BoardCount;
	SysInfo->u32BaseBoardOptions	= pBdParam->u32BaseBoardOptions[0];
	SysInfo->u32AddonOptions		= pBdParam->u32ActiveAddonOptions;

	AssignAcqSystemBoardName( SysInfo );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsSpiderDev::AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo )
{
	char strBoarNameSuffix[10] = {0};
	uInt16	u16DeviceId = GetDeviceId();

	if ( ( u16DeviceId & CS_DEVID_STAVG_SPIDER ) == CS_DEVID_STAVG_SPIDER )
	{
		if ( ( u16DeviceId & CS_DEVID_SPIDER_LP ) == CS_DEVID_SPIDER_LP )
			strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"STLP81");
		else
			strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"ST81AVG");
	}
	else
	{
		if ( m_bNucleon )
			strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"CSE" );
		else
			strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"CS" );

		if ( m_pCardState->u32DataMask == CSPDR_12BIT_DATA_MASK )
		{
			strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"82" );
		}
		else if ( m_pCardState->u32DataMask == CSPDR_14BIT_DATA_MASK )
		{
			if( m_pCardState->bZap )
				strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"84" );
			else
				strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"83" );
		}
		sprintf_s( strBoarNameSuffix, sizeof(strBoarNameSuffix), "%x%x", m_PlxData->CsBoard.NbAnalogChannel, m_PlxData->CsBoard.SampleRateLimit );
		strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), strBoarNameSuffix );
	}

	if( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions&CSPDR_AOHW_OPT_EMONA_FF) )
		strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"FF" );

	if ( pSysInfo->u32BoardCount > 1 )
	{
		char tmp[32];
		sprintf_s( tmp, sizeof(tmp), " M/S - %d", pSysInfo->u32BoardCount );
		strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), tmp );
	}
}




#if 0
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::ReadCalibTableFromEeprom(uInt8	u8CalibTableId)
{
	uInt32	u32Addr;
	uInt32	u32EepromSize;


	switch ( u8CalibTableId )
	{
		case 2:
			u32Addr = FIELD_OFFSET( FASTBL_FLASH_LAYOUT, FlashImage2 );
			break;
		default:
			u32Addr = FIELD_OFFSET( FASTBL_FLASH_LAYOUT, FlashImage1 );
			break;
	}

	u32Addr += FIELD_OFFSET(FASTBL_FLASH_IMAGE_STRUCT, Calib);
	u32EepromSize = sizeof((((FASTBL_FLASH_IMAGE_STRUCT *)0)->Calib));

	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  CSRB_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....

	uInt32 u32CalibTableSize = sizeof(m_CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	CsSpiderFlash		FlashObj( this );


	int32 i32Status = FlashObj.FlashRead( u32Addr, &m_CalibInfoTable, u32CalibTableSize );

	if ( CS_FAILED( i32Status ) || (-1 == (int32) m_CalibInfoTable.u32Valid ) ||
		  CSRBCALIBINFO_CHECKSUM_VALID != m_CalibInfoTable.u32Checksum )
	{
//		if( m_pCardState->bHighBandwidth )
//		{
//				aultCalibTableValues();
//			RtlCopyMemory( &m_DacInfo, &m_CalibInfoTable.ChanCalibInfo, sizeof(m_DacInfo) );
//		}
	}
	else if ( m_bUseDacCalibFromEEprom )
		RtlCopyMemory( &m_DacInfo, &m_CalibInfoTable.ChanCalibInfo, sizeof(m_DacInfo) );

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid )
{
	uInt32	u32Addr;
	uInt32	u32EepromSize;

	switch ( u8CalibTableId )
	{
		case 2:
			u32Addr = FIELD_OFFSET( FASTBL_FLASH_LAYOUT, FlashImage2 );
			break;
		default:
			u32Addr = FIELD_OFFSET( FASTBL_FLASH_LAYOUT, FlashImage1 );
			break;
	}

	u32Addr += FIELD_OFFSET(FASTBL_FLASH_IMAGE_STRUCT, Calib);
	u32EepromSize = sizeof((((FASTBL_FLASH_IMAGE_STRUCT *)0)->Calib));

	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  CSRB_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....

	uInt32 u32CalibTableSize = sizeof(m_CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	// Update the Calib Table with all latest Dac values
	RtlCopyMemory( &m_CalibInfoTable.ChanCalibInfo, &m_DacInfo, sizeof(m_DacInfo) );

	m_CalibInfoTable.u32Checksum = CSRBCALIBINFO_CHECKSUM_VALID;
	uInt32 u32InvalidMask = m_CalibInfoTable.u32Valid & (CSRB_VALID_MASK >> 1 );
	m_CalibInfoTable.u32Valid &= ~(u32InvalidMask << 1);

	//clear valid bits from input
	m_CalibInfoTable.u32Valid &= (u32Valid << 1) & CSRB_VALID_MASK;

	//set valid bits from input
	m_CalibInfoTable.u32Valid |= u32Valid & CSRB_VALID_MASK;

	CsSpiderFlash		FlashObj( this );

	return FlashObj.FlashWriteEx(u32Addr, &m_CalibInfoTable, sizeof(m_CalibInfoTable) );
}

#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	CsSpiderDev::BbFwVersionConvert( uInt32 u32OldVersion )
{
	FPGA_FWVERSION			NewFwVersion;
	FPGABASEBOARD_OLDFWVER	OldFwVer;

	OldFwVer.u32Reg = u32OldVersion;
	NewFwVersion.u32Reg = 0;
	NewFwVersion.Version.uMajor = OldFwVer.Info.uRev & 0xF;
	NewFwVersion.Version.uMinor = OldFwVer.Info.uMajor;
	NewFwVersion.Version.uRev	= OldFwVer.Info.uMinor;

	return NewFwVersion.u32Reg;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsSpiderDev::AddonFwVersionConvert( uInt32 u32OldVersion )
{
	// Convert a old version format to a new one
	FPGA_FWVERSION			NewFwVersion;
	FPGAADDON_OLDFWVER		OldFwVer;

	OldFwVer.u32Reg = u32OldVersion;
	NewFwVersion.u32Reg = 0;
	NewFwVersion.Version.uMajor = OldFwVer.Info.uRev & 0xF;
	NewFwVersion.Version.uMinor = OldFwVer.Info.uMajor;
	NewFwVersion.Version.uRev	= OldFwVer.Info.uMinor;

	return NewFwVersion.u32Reg;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt8	CsSpiderDev::ReadBaseBoardCpldVersion()
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

void CsSpiderDev::ReadVersionInfo()
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
			FPGABASEBOARD_OLDFWVER		OldFwVer;
			OldFwVer.u32Reg = ReadGageRegister( FIRMWARE_VERSION );

			pFwVerInfo->BbFpgaInfo.u32Reg		= BbFwVersionConvert( OldFwVer.u32Reg );
			GetBoardNode()->u32RawFwVersionB[0] = pFwVerInfo->BbFpgaInfo.u32Reg;
			pFwVerInfo->u32BbFpgaOptions		= OldFwVer.Info.uRev;

			// Build Csi and user version info
			pFwVerInfo->BaseBoardFwVersion.Version.uRev		= pFwVerInfo->BbFpgaInfo.Version.uRev;
			pFwVerInfo->BaseBoardFwVersion.Version.uIncRev	= pFwVerInfo->BbFpgaInfo.Version.uIncRev;
		}

		// Build Csi and user version info
		pFwVerInfo->BaseBoardFwVersion.Version.uMajor	= pFwVerInfo->BbFpgaInfo.Version.uMajor;
		pFwVerInfo->BaseBoardFwVersion.Version.uMinor	= pFwVerInfo->BbFpgaInfo.Version.uMinor;
		GetBoardNode()->u32UserFwVersionB[0].u32Reg		= m_pCardState->VerInfo.BaseBoardFwVersion.u32Reg;
	}

	if ( m_pCardState->bAddOnRegistersInitialized && 0 == GetBoardNode()->u32RawFwVersionA[0]  )
	{
		// Cpld Version
		FPGABASEBOARD_OLDFWVER BbFpgaInfo;
		PCS_FW_VER_INFO				pFwVerInfo = &m_pCardState->VerInfo;

		BbFpgaInfo.u32Reg = ReadGIO(CSPDR_ADDON_FDV);
		m_pCardState->VerInfo.ConfCpldInfo.u32Reg = 0;

		pFwVerInfo->ConfCpldInfo.Info.uYear		= BbFpgaInfo.Info.uYear;
		pFwVerInfo->ConfCpldInfo.Info.uMonth	= BbFpgaInfo.Info.uMonth;
		pFwVerInfo->ConfCpldInfo.Info.uDay		= BbFpgaInfo.Info.uDay;
		pFwVerInfo->ConfCpldInfo.Info.uMajor	= BbFpgaInfo.Info.uMajor;
		pFwVerInfo->ConfCpldInfo.Info.uMinor	= BbFpgaInfo.Info.uMinor;
		pFwVerInfo->ConfCpldInfo.Info.uRev		= BbFpgaInfo.Info.uRev;

		// Add-on firmware date and version
		FPGAADDON_OLDFWVER	AoFpgaInfo;
		uInt32 u32Data = ReadRegisterFromNios(0,CSPDR_FWV_RD_CMD);
		u32Data <<= 16;
		u32Data |= 0xffff & ReadRegisterFromNios(0,CSPDR_FWD_RD_CMD);
		AoFpgaInfo.u32Reg = u32Data;

		pFwVerInfo->AddonFpgaFwInfo.u32Reg		= AddonFwVersionConvert( AoFpgaInfo.u32Reg );
		m_PlxData->CsBoard.u32RawFwVersionA[0]	= pFwVerInfo->AddonFpgaFwInfo.u32Reg;
		pFwVerInfo->u32AddonFpgaOptions			= AoFpgaInfo.Info.uRev;

		pFwVerInfo->AddonFwVersion.Version.uMajor	= pFwVerInfo->AddonFpgaFwInfo.Version.uMajor;
		pFwVerInfo->AddonFwVersion.Version.uMinor	= pFwVerInfo->AddonFpgaFwInfo.Version.uMinor;
		pFwVerInfo->AddonFwVersion.Version.uRev		= pFwVerInfo->AddonFpgaFwInfo.Version.uRev;
		pFwVerInfo->AddonFwVersion.Version.uIncRev	= pFwVerInfo->AddonFpgaFwInfo.Version.uIncRev;
		GetBoardNode()->u32UserFwVersionA[0].u32Reg	= pFwVerInfo->AddonFwVersion.u32Reg;

		// Add-on Analog CPLD1 date and version
		u32Data = ReadRegisterFromNios(0,CSPDR_ACV1_RD_CMD);
		u32Data <<= 16;
		u32Data |= 0xffff & ReadRegisterFromNios(0,CSPDR_ACD1_RD_CMD);
		m_pCardState->VerInfo.AnCpldFwInfo.u32Reg = u32Data;

		WriteGIO(CSPDR_ADDON_SPI_TEST_WR, CSPDR_TCPLD_INFO);
		WriteGIO(CSPDR_ADDON_SPI_TEST_WR, 0);
		Sleep(1);

		// Add-on Trigger CPLD date and version
		u32Data = ReadRegisterFromNios(0, CSPDR_CPLD_FV_RD_CMD);
		u32Data <<= 16;
		u32Data |= 0xffff & ReadRegisterFromNios(0,CSPDR_CPLD_FD_RD_CMD);
		m_pCardState->VerInfo.TrigCpldFwInfo.u32Reg = u32Data;
	}
}


//-----------------------------------------------------------------------------
// 
//
//-----------------------------------------------------------------------------

void CsSpiderDev::ReadVersionInfoEx()
{
	FPGA_FWVERSION				RawVerInfo;
	BOOLEAN						bConfigReset = FALSE;

	uInt32 u32ValidExpertFw = CS_BBOPTIONS_MULREC_AVG_TD | CS_BBOPTIONS_STREAM;

	// Reading Expert firmware versions
	if ( 0 != m_PlxData->InitStatus.Info.u32Nios )
	{
		for (uInt8 i = 1; i <= 2; i++ )
		{
			if ( 0 != (m_PlxData->CsBoard.u32BaseBoardOptions[i] & u32ValidExpertFw)  )
			{
				if ( 0 == GetBoardNode()->u32RawFwVersionB[i] )
				{
					if ( m_bNucleon )
					{
						if ( m_bNoConfigReset )
						{
							// eXpert Data Streaming does not need ConfigReset
							// Take the current version as eXpert Streaming version
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
							uInt32 u32Data = 0x7FFF & ReadGageRegister(CSNUCLEON_HWR_RD_CMD);	
							u32Data <<= 16;
							u32Data |= (0x7FFF & ReadGageRegister(CSNUCLEON_SWR_RD_CMD));
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

						FPGABASEBOARD_OLDFWVER		OldFwVer;
						OldFwVer.u32Reg = ReadGageRegister( FIRMWARE_VERSION );
						RawVerInfo.u32Reg = GetBoardNode()->u32RawFwVersionB[i] = BbFwVersionConvert( OldFwVer.u32Reg );

						GetBoardNode()->u32UserFwVersionB[i].Version.uRev		= RawVerInfo.Version.uRev;
						GetBoardNode()->u32UserFwVersionB[i].Version.uIncRev	= RawVerInfo.Version.uIncRev;
					}

					GetBoardNode()->u32UserFwVersionB[i].Version.uMajor	= RawVerInfo.Version.uMajor;
					GetBoardNode()->u32UserFwVersionB[i].Version.uMinor	= RawVerInfo.Version.uMinor;

					bConfigReset = TRUE;
				}
			}
			else
				GetBoardNode()->u32UserFwVersionB[i].u32Reg = m_PlxData->CsBoard.u32RawFwVersionB[i] = 0;
		}

		if ( bConfigReset )
		{
			// If ConfigReset has been called, clear all cache in order to force re-configuration and re-calibrate
			if ( m_bNucleon )
				ConfigurationReset(0);
			else
				BaseBoardConfigReset( FIELD_OFFSET( BaseBoardFlashData, DefaultImage ) );
			ResetCachedState();
			InitAddOnRegisters(true);
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8Index = 0;
	int64	i64MaxMemPerChan = 0;
	int64	i64MaxSegmentSize = 0;
	int64	i64MaxDepth = 0 ;
	int64	i64MaxPreTrigDepth = 0;
	int64	i64MaxTrigDelay = 0;
	uInt32	u32LimitSegmentCount = 0xFFFFFFFF;
	uInt32	u32MaxSegmentCount = 1;
	int32	i32MinSegmentSize = 32;
	uInt32	u32DepthRes = CSPDR_TRIGGER_RES;
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
	uInt32	u32TailHwReserved = BYTESRESERVED_HWMULREC_TAIL/sizeof(uInt16);

	if ( m_bNucleon )
	{
		i64MaxPreTrigDepth = 4088;
		u32TailHwReserved *= 2;
	}
	
	// By FW design, the last segment consumes ~512 samples more than the other segments.
	// We have to substact this amount of data in the calculation of max available memory.
	i64MaxMemPerChan = m_pSystem->SysInfo.i64MaxMemory - 512;
	if ( pAcqCfg->u32Mode & CS_MODE_OCT )
	{
		u32DepthRes >>= 3;
		u32TailHwReserved >>= 3;
		i64MaxMemPerChan >>= 3;
	}
	else if ( pAcqCfg->u32Mode & CS_MODE_QUAD )
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
		i64MaxPreTrigDepth <<= 3;

	// Calculate the Max allowed Segment Size
	i64MaxTrigDelay = CSMV_LIMIT_HWDEPTH_COUNTER;
	i64MaxTrigDelay *= 128;
	i64MaxTrigDelay /= (32*(pAcqCfg->u32Mode&CS_MASKED_MODE));

	i64MaxMemPerChan -= CsGetPostTriggerExtraSamples(pAcqCfg->u32Mode);
	i64MaxSegmentSize = i64MaxMemPerChan - u32TailHwReserved;
	i64MaxDepth = i64MaxSegmentSize;
	if ( ! m_bNucleon )
		i64MaxPreTrigDepth = i64MaxDepth;


	if ( pAcqCfg->u32Mode & CS_MODE_USER1 )
		u8Index = 1;
	else if ( pAcqCfg->u32Mode & CS_MODE_USER2 )
		u8Index = 2;

	if ( 0 != u8Index )
	{
		// Make sure that Master/Slave system has all the same optionss
		CsSpiderDev* pDevice = m_MasterIndependent->m_Next;
		while( pDevice )
		{
			if ( m_PlxData->CsBoard.u32BaseBoardOptions[u8Index] != pDevice-> m_PlxData->CsBoard.u32BaseBoardOptions[u8Index] )
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
			i64MaxMemPerChan	/= 2;
			i64MaxPreTrigDepth	= 0;
			i32MinSegmentSize	= 64;

			i64MaxSegmentSize	= m_u32MaxHwAvgDepth/(pAcqCfg->u32Mode&CS_MASKED_MODE);;
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
			// No pretrigger is allowed in HW averaging
			i64MaxPreTrigDepth = 0;
			// Trigger Delay is not supported in HW averaging
			i64MaxTrigDelay = 0;
			u32LimitSegmentCount = MAX_SW_AVERAGE_SEGMENTCOUNT;
		}
		else if ( pAcqCfg->i64TriggerDelay != 0 )
			i64MaxPreTrigDepth = 0;		// No pretrigger if TriggerDelay is enable
	}


	// Validation of Segment Size and trigger depth
	if ( m_Stream.bEnabled && 0 > pAcqCfg->i64SegmentSize && 0 > pAcqCfg->i64Depth )
		m_Stream.bInfiniteDepth = true;
	else if ( 0 > pAcqCfg->i64SegmentSize )
		return CS_INVALID_SEGMENT_SIZE;
	else if ( 0 > pAcqCfg->i64Depth )
		return CS_INVALID_TRIG_DEPTH;

	if ( ! m_Stream.bInfiniteDepth )
	{
		if ( pAcqCfg->i64Depth > pAcqCfg->i64SegmentSize )
		{
			if (u32Coerce)
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;
			else
				return CS_DEPTH_SIZE_TOO_BIG;
		}

		// Validate the min segment size
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
				pAcqCfg->i64SegmentSize = CSPDR_DEFAULT_SEGMENT_SIZE;
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

		// Nulceon is "square" and requires pAcqCfg->i64TriggerHoldoff >= pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth;
		if ( m_bNucleon )
		{
			uInt32 u32MinTrgHoldOff = 	(uInt32) (pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth);
			if ( pAcqCfg->i64TriggerHoldoff < u32MinTrgHoldOff )
			{
				if ( u32Coerce )
				{
					pAcqCfg->i64TriggerHoldoff = u32MinTrgHoldOff;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
				{
					i32Status = CS_INVALID_TRIGHOLDOFF;
					goto ExitValidate;
				}
			}
		}
	}

	// Adjust Segment Parameters for Hardware
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

	if ( pAcqCfg->i64TriggerDelay > i64MaxTrigDelay || 0 != (pAcqCfg->i64TriggerDelay % u32DepthRes) )
	{
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

		// Adjust Segment Parameters for Hardware
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

	m_bMulrecAvgTD = bXpertBackup[0];
	m_Stream.bEnabled = bXpertBackup[1];

	return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	CsSpiderDev::AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32 *u32SegmentCount, int64 *i64SegmentSize, int64 *i64Depth )
{
	if ( m_bNucleon )
		return;			// No adjustment for Nucleon;


	// Number of extra samples to be added into segment and depth setting to ensurw
	// post trigger dapth( cover the lost due invalid samples and trigger address offset )
	uInt32	u32TailExtraSamples = 0;

	// Number of extra sample to be added in segment size to compensant negayive trigger address offset
	uInt32	u32HeadExtraSamples = 0;

	if ( 0 != ( u32Mode & CS_MODE_OCT ) )
	{
		int8 i8NegTrigAddr =  GetTriggerAddressOffsetAdjust( u32Mode, (uInt32) m_pCardState->AcqConfig.i64SampleRate );
		if ( i8NegTrigAddr < 0 )
			u32HeadExtraSamples = (uInt32) (-1*i8NegTrigAddr);
	}

	m_u32TailExtraData = u32TailExtraSamples = CsGetPostTriggerExtraSamples( u32Mode );

	*i64SegmentSize = *i64SegmentSize + u32TailExtraSamples;
	*i64SegmentSize += u32HeadExtraSamples;
	*i64Depth = *i64Depth + m_pCardState->AcqConfig.i64TriggerDelay + u32TailExtraSamples;

	m_u32HwSegmentCount = *u32SegmentCount;
	m_i64HwSegmentSize = *i64SegmentSize;
	m_i64HwPostDepth = *i64Depth;
	m_i64HwTriggerHoldoff = m_pCardState->AcqConfig.i64TriggerHoldoff;
}

//--------------------------------------------------------------------------------------------------
//	Internal extra samples to be added in the post trigger depth and segment size in order to
// 	ensure post trigger depth.
//  Withou ensure post trigger depth, the lost is because of trigger address offset and invalid
//  samples at the end of each record.
//---------------------------------------------------------------------------------------------------

uInt32	CsSpiderDev::CsGetPostTriggerExtraSamples( uInt32 u32Mode )
{
	if ( m_bZeroDepthAdjust || m_bNucleon )
		return 0;

	uInt32	u32TailExtraData = 0;

	if ( m_bHardwareAveraging )
	{
		u32TailExtraData = AV_ENSURE_POSTTRIG;
	}
	else if ( m_bMulrecAvgTD )
	{
		// The amount of invalid sample vary depending on post trigger depth
		// So we have to adjust in worse case
		if ( u32Mode & CS_MODE_OCT )
			u32TailExtraData = 32;
		if ( u32Mode & CS_MODE_QUAD )
			u32TailExtraData = 64;
		if ( u32Mode & CS_MODE_DUAL )
			u32TailExtraData = 96;
		if ( u32Mode & CS_MODE_SINGLE )
			u32TailExtraData =  192;
	}
	else
	{
		u32TailExtraData = MAX_SPDR_TAIL_INVALID_SAMPLES;

		if ( u32Mode & CS_MODE_OCT )
			u32TailExtraData >>= 3;
		if ( u32Mode & CS_MODE_QUAD )
			u32TailExtraData >>= 2;
		if ( u32Mode & CS_MODE_DUAL )
			u32TailExtraData >>= 1;
		if ( u32Mode & CS_MODE_SINGLE )
			u32TailExtraData >>= 0;

		u32TailExtraData += MAX_SPDR_TRIGGER_ADDRESS_OFFSET;
	}

	return u32TailExtraData;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSpiderDev::FindFeIndex( const uInt32 u32Swing, const uInt32 u32Imed, uInt32& u32Index )
{
	uInt32* pSwingTable;
	uInt32 u32MaxIndex = 0;

	if(CS_REAL_IMP_1M_OHM == u32Imed)
	{
		pSwingTable = m_pCardState->u32SwingTable[1];
		u32MaxIndex = m_pCardState->u32SwingTableSize[1];
	}
	else if (CS_REAL_IMP_50_OHM == u32Imed)
	{
		pSwingTable = m_pCardState->u32SwingTable[0];
		u32MaxIndex = m_pCardState->u32SwingTableSize[0];
	}
	else
	{
		return CS_INVALID_IMPEDANCE;
	}

	for( uInt32 i = 0; i < u32MaxIndex; i++ )
	{
		if ( u32Swing == pSwingTable[i] )
		{
			u32Index = i;
			return CS_SUCCESS;
		}
	}

	return CS_INVALID_GAIN;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int8	CsSpiderDev::GetTriggerAddressOffsetAdjust( uInt32 u32Mode, uInt32 u32SampleRate )
{
	int8	i8Offset = 0;
	uInt8	u8IndexSr = 0;

	if ( m_bZeroTrigAddrOffset || m_bNucleon )
		return 0;

	u32Mode &= CS_MASKED_MODE;

	switch( u32SampleRate )
	{
		default:
		case 125000000:	u8IndexSr = 0;  break;
		case 100000000:	u8IndexSr = 1;  break;
		case 50000000:	u8IndexSr = 2;  break;
		case 25000000:	u8IndexSr = 3;  break;
		case 12500000:	u8IndexSr = 4;  break;
		case 10000000:	u8IndexSr = 5;  break;
		case 5000000:	u8IndexSr = 6;  break;
		case 2000000:	u8IndexSr = 7;  break;
		case 1000000:	u8IndexSr = 8;  break;
		case 500000:	u8IndexSr = 9;  break;
		case 200000:	u8IndexSr = 10; break;
		case 100000:	u8IndexSr = 11; break;
		case 50000:		u8IndexSr = 12; break;
		case 20000:		u8IndexSr = 13; break;
		case 10000:		u8IndexSr = 14; break;
		case 5000:		u8IndexSr = 15; break;
		case 2000:		u8IndexSr = 16; break;
		case 1000:		u8IndexSr = 17; break;
	}

	if ( m_bHardwareAveraging )
	{
		switch ( u32Mode  )
		{
			case CS_MODE_SINGLE:
				if ( m_pCardState->bSpiderLP )
				{
					if ( u32SampleRate >= CS_SR_25MHZ )
						i8Offset = 19;
					else
						i8Offset = 20;
				}
				else
					i8Offset = 18;
				break;
			case CS_MODE_DUAL:	i8Offset = 10;	break;
			case CS_MODE_QUAD:	i8Offset = 6;	break;
			case CS_MODE_OCT:   i8Offset = 2;	break;
		}
	}
	else if ( m_bMulrecAvgTD )
	{
		if ( m_bNucleon )
		{
			if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_OCT) != 0)
				i8Offset  = 2;
			else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD) != 0)
				i8Offset  = 6;
			else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0)
				i8Offset  = 10;
			else
				i8Offset  = 18;
		}
		else if( m_pCardState->bZap )
		{
			switch ( u32Mode )
			{
				case CS_MODE_SINGLE: i8Offset = 26; break;
				case CS_MODE_DUAL:	 i8Offset = 14; break;
				case CS_MODE_QUAD:   i8Offset =  8; break;
				case CS_MODE_OCT:    i8Offset =  4; break;
			}
		}
		else
		{
			int8 i8TaOff1[] = { 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
			int8 i8TaOff2[] = { 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
			int8 i8TaOff4[] = { 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
			int8 i8TaOff8[] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
			switch ( u32Mode )
			{
				case CS_MODE_SINGLE: i8Offset = i8TaOff1[u8IndexSr]; break;
				case CS_MODE_DUAL:	 i8Offset = i8TaOff2[u8IndexSr]; break;
				case CS_MODE_QUAD:   i8Offset = i8TaOff4[u8IndexSr]; break;
				case CS_MODE_OCT:    i8Offset = i8TaOff8[u8IndexSr]; break;
			}
		}
	}
	else if ( m_bNucleon )
	{
		if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_OCT) != 0)
			i8Offset  = 0;
		else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD) != 0)
			i8Offset  = 2;
		else if ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0)
			i8Offset  = 2;
		else
			i8Offset  = 2;

		if ( m_pCardState->SrInfo[0].llSampleRate < 5000000 )
			i8Offset += 1;
	}
	else
	{
		if ( m_pCardState->bSpiderLP )
		{
			switch ( u32Mode )
			{
				case CS_MODE_OCT:
					{
						if ( u32SampleRate >= CS_SR_25MHZ )
							i8Offset = -2;
						else
							i8Offset = -1;
					}
					break;
				case CS_MODE_SINGLE:
					{
						if ( u32SampleRate < CS_SR_25MHZ )
							i8Offset = 29;
						else
							i8Offset = 28;
					}
					break;
				case CS_MODE_DUAL:
				case CS_MODE_QUAD:	break;
			}
		}
		else
		{
			int8 i8TaOff1[] = { -1, -1, -1, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3 };
			int8 i8TaOff2[] = { -1, -1, -1, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3 };
			int8 i8TaOff4[] = { -1, -1, -1, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3 };
			int8 i8TaOff8[] = { -1, -1, -1, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3 };

			switch ( u32Mode )
			{
				case CS_MODE_SINGLE: i8Offset = i8TaOff1[u8IndexSr]; break;
				case CS_MODE_DUAL:	 i8Offset = i8TaOff2[u8IndexSr]; break;
				case CS_MODE_QUAD:   i8Offset = i8TaOff4[u8IndexSr]; break;
				case CS_MODE_OCT:    i8Offset = i8TaOff8[u8IndexSr]; break;
			}
		}
	}
	return i8Offset;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsSpiderDev::BuildIndependantSystem( CsSpiderDev** FirstCard, BOOLEAN bForceInd )
{
	CsSpiderDev*	pCard	= (CsSpiderDev*) m_ProcessGlblVars->pFirstUnCfg;

	*FirstCard = NULL;
	if( NULL != pCard )
	{
		ASSERT(ermStandAlone == pCard->m_ermRoleMode);

		pCard->m_MasterIndependent = pCard;
		pCard->m_pTriggerMaster = pCard;

		//remove card from unconfigured list
		pCard->RemoveDeviceFromUnCfgList();

		if( !bForceInd )
		{
			if( !pCard->m_PlxData->InitStatus.Info.u32BbMemTest )
			{
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Zero memory" );
				return 0;
			}
			else if (!pCard->m_bInvalidHwConfiguration && m_PlxData->InitStatus.Info.u32Nios
				&& m_pCardState->bAddOnRegistersInitialized )
			{
				pCard->AddonReset();
				if ( (m_PlxData->CsBoard.u8BadAddonFirmware != 0)  || (m_PlxData->CsBoard.u8BadBbFirmware != 0) )
				{
					CsEventLog EventLog;
					EventLog.Report( CS_ERROR_TEXT, "Baseboard or Addon firmware mismatch" );
					pCard->AddonConfigReset();
				}
				else
				{
					char	szText[128];
					sprintf_s( szText, sizeof(szText), TEXT("Octopus System %x initialized"), pCard->m_PlxData->CsBoard.u32SerNum);
					CsEventLog EventLog;
					EventLog.Report( CS_INFO_TEXT, szText );

					pCard->m_pCardState->u16CardNumber = 1;
					pCard->UpdateCardState();
					pCard->m_Next = pCard->m_pNextInTheChain = NULL;
					pCard->m_u16NextSlave = 0;

					*FirstCard = pCard;
					return 1;
				}
			}
			else
			{
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Error in BuildIndependantSystem" );
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


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSpiderDev::RemoveDeviceFromUnCfgList()
{
	CsSpiderDev	*Current	= (CsSpiderDev	*) m_ProcessGlblVars->pFirstUnCfg;
	CsSpiderDev	*Prev = NULL;

	if ( this  == (CsSpiderDev	*) m_ProcessGlblVars->pFirstUnCfg )
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

	if ( this  == (CsSpiderDev	*) m_ProcessGlblVars->pLastUnCfg )
	{
		if (Prev)
			Prev->m_Next = NULL;
		m_ProcessGlblVars->pLastUnCfg = (PVOID) Prev;
	}

	return CS_SUCCESS;

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt16	CsSpiderDev::DetectAcquisitionSystems( BOOL bForceInd )
{
	CsSpiderDev	*pDevice = (CsSpiderDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsSpiderDev	*pFirstCardInSystem[20];

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
	CsSpiderDev		*FirstCard = NULL;
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

				pSystem->DrvHandle			= FirstCard->m_PseudoHandle;
				pSystem->u8FirstCardIndex	= FirstCard->m_u8CardIndex;

				SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
				SysInfo->u32BoardCount		= NbCardInSystem;
				SysInfo->i64MaxMemory		= pBdParam->i64MaxMemory;
				SysInfo->u32AddonOptions	= pBdParam->u32ActiveAddonOptions;
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

		pSystem->DrvHandle			= FirstCard->m_PseudoHandle;
		pSystem->u8FirstCardIndex	= FirstCard->m_u8CardIndex;

		SysInfo			= &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		SysInfo->u32BoardCount		= NbCardInSystem;
		SysInfo->i64MaxMemory		= pBdParam->i64MaxMemory;
		SysInfo->u32AddonOptions	= pBdParam->u32ActiveAddonOptions;
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

		CsSpiderDev *pDevice = (CsSpiderDev *) pFirstCardInSystem[i];
		j = 0;

		memset(pSystem->u8CardIndexList, 0, sizeof(pSystem->u8CardIndexList));
		while( pDevice )
		{
			// Set M/S card index list
			pSystem->u8CardIndexList[j++] = pDevice->m_u8CardIndex;

			pDevice->m_pSystem = pSystem;
			pDevice = pDevice->m_Next;
		}

		pDevice = (CsSpiderDev *) pFirstCardInSystem[i];
		pSystem->u16AcqStatus = ACQ_STATUS_READY;
	}

	return m_ProcessGlblVars->NbSystem;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsSpiderDev::SetDefaultTrigAddrOffset(void)
{
		//QQWDK

}




//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSpiderDev::SetupHwAverageMode()
{
	uInt32		DevideSelector = 1;
	BOOLEAN		b32BitAveraging = TRUE;		// force using full resolution

	if ( ! m_bHardwareAveraging && m_pCardState->bSpiderLP )
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
		case 256:
			{
			DevideSelector = 0;
			}
			break;
		case 128:
			{
			DevideSelector = 1;
			}
			break;
		case 64:
			{
			DevideSelector = 2;
			}
			break;
		case 32:
			{
			DevideSelector = 3;
			}
			break;
		case 16:
			{
			DevideSelector = 4;
			}
			break;
		case 8:
			{
			DevideSelector = 5;
			}
			break;
		case 4:
			{
			DevideSelector = 6;
			}
			break;
		case 2:
			{
			DevideSelector = 7;
			}
			break;
		case 1:
		default:
			{
			DevideSelector = 8;
			}
			break;
		}
	}

	#define	SPDR_12BITS_CARD	0x8000

	uInt32 u32Data = ReadGageRegister( GEN_COMMAND_R_REG );
	uInt32 u32ChannelMode = 0;

	u32Data &= ~0x3FF00;

	if ( CSPDR_12BIT_DATA_MASK == m_pCardState->u32DataMask )
		u32Data |= SPDR_12BITS_CARD;

	if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE ) != 0 )
		u32ChannelMode = 0;
	else if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ) != 0 )
		u32ChannelMode = 1;
	else if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD ) != 0 )
		u32ChannelMode = 2;
	else
		u32ChannelMode = 3;

	u32Data = (u32ChannelMode <<16) | (DevideSelector << 8) | u32Data;

	WriteGageRegister( GEN_COMMAND_R_REG, u32Data);

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSpiderDev:: ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg )
{
	uInt8	u8Shift = 0;

	if ( m_bNucleon )
	{
		// Set Mode select 0
		WriteGageRegister(MODESEL, MSEL_CLR | 0);
		WriteGageRegister(MODESEL, (~MSEL_CLR) & 0);

		for (uInt32 i = 0; i < u32NumOfSeg; i ++)
		{
			// Mask the channel triggered info bits
			pBuffer[i] = (0xFFFF & ReadRegisterFromNios( StartSegment, CSPLXBASE_READ_TIMESTAMP0_CMD ));
			pBuffer[i] &= 0xFFFF;
			pBuffer[i] <<= 32;
			pBuffer[i] |= ReadRegisterFromNios( StartSegment++, CSPLXBASE_READ_TIMESTAMP1_CMD );
		}
	}
	else
	{
		if ( m_pCardState->bSpiderLP )
		{
			// return all 0
			::GageZeroMemoryX( pBuffer, u32NumOfSeg * sizeof(int64) );
			return CS_SUCCESS;
		}

		ReadTriggerTimeStamp( StartSegment, pBuffer, u32NumOfSeg );

		if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_OCT )
			return CS_SUCCESS;							// there is no etb in octal mode
		else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD )
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

			// Timestamp is the count of memory locations
			// Convert to number of samples
			pBuffer[i] = pBuffer[i]  << u8Shift;
			pBuffer[i] += u8Etb;
		}
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void	CsSpiderDev:: SetDataMaskModeTransfer ( uInt32 u32TxMode )
{
	uInt32	u32Data = 0;


	// Clear Slave mode bit
	u32TxMode &=  ~TxMODE_SLAVE;

	switch( u32TxMode )
	{
		case TxMODE_TIMESTAMP:
		{
			// Seting for Time stamp transfer
			u32Data = 0x00000;
		}
		break;

		case TxMODE_DATA_FLOAT:
		{
			// Seting for Data float transfer
			u32Data = 0x00000;
		}
		break;

		default:		// default mode binary
		case TxMODE_DATA_ANALOGONLY:
		{
			u32Data = 0x0000;
		}

		break;
	}

	WriteGageRegister( MASK_REG, u32Data );

}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
uInt32	CsSpiderDev:: GetDataMaskModeTransfer ( uInt32 u32TxMode )
{
	uInt32	u32DataMask = 0;


	// Clear Slave mode bit
	u32TxMode &=  ~TxMODE_SLAVE;

	switch( u32TxMode )
	{
		case TxMODE_TIMESTAMP:
		{
			// Seting for Time stamp transfer
			u32DataMask = 0x00000;
		}
		break;

		case TxMODE_DATA_FLOAT:
		{
			// Seting for Data float transfer
			u32DataMask = 0x00000;
		}
		break;

		case TxMODE_DATA_ONLYDIGITAL:
		{
			if( m_pCardState->bZap )
				u32DataMask = (uInt32) ~0x00000; // No digital bits
			else
				u32DataMask = (uInt32) ~0x30003; // Masking everything but 2 bits for digital channel
		}
		break;

		case TxMODE_DATA_32:
		case TxMODE_DATA_16:
		{
			// Seting for full 16 bit data transfer
			u32DataMask = 0x00000;
		}
		break;

		default:		// default mode binary
		case TxMODE_DATA_ANALOGONLY:
		{
			u32DataMask = 0x0000;
		}

		break;
	}

	return u32DataMask;
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

BOOLEAN 	CsSpiderDev::CheckRequiredFirmwareVersion()
{

#ifndef _WINDOWS_
	return CS_SUCCESS;
#else
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

			CsiVer.u32Reg = m_AddonCsiEntry.u32FwVersion;

			if ( m_pCardState->VerInfo.AddonFpgaFwInfo.Version.uMajor != CsiVer.Version.uMajor ||
				 m_pCardState->VerInfo.AddonFpgaFwInfo.Version.uMinor != CsiVer.Version.uMinor ||
				 m_pCardState->VerInfo.AddonFpgaFwInfo.Version.uRev != CsiVer.Version.uRev ||
				 m_pCardState->VerInfo.AddonFpgaFwInfo.Version.uIncRev != CsiVer.Version.uIncRev )
				m_PlxData->CsBoard.u8BadAddonFirmware = BAD_DEFAULT_FW;
		}
		else
		{
			if ( GetBoardNode()->u32UserFwVersionB[0].u32Reg != m_BaseBoardCsiEntry.u32FwVersion )
				m_PlxData->CsBoard.u8BadBbFirmware |= BAD_DEFAULT_FW;

			if ( GetBoardNode()->u32UserFwVersionA[0].u32Reg != m_AddonCsiEntry.u32FwVersion )
				m_PlxData->CsBoard.u8BadAddonFirmware = BAD_DEFAULT_FW;
		}
	}

	// added March 22, 2011 because of problem when reinitialing from CsExpertUpdate
	// we would failinitialization below with BAD_EXPERT1_FW because the version of 
	// CsiVer.u32Reg and ExpertVer.u32Reg.  Now, if we don't detect expert firmware
	// we don't even check the version of the expert firmware

	uInt32 u32ValidExpertFw = CS_BBOPTIONS_MULREC_AVG_TD | CS_BBOPTIONS_STREAM;

	BOOLEAN bExpertFwDetected = FALSE;
	for ( int i = 1; i <= 2; i ++ )
	{
		if ( (GetBoardNode()->u32BaseBoardOptions[i] & u32ValidExpertFw) != 0 )
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
					m_PlxData->CsBoard.u8BadBbFirmware |= BAD_EXPERT1_FW << i;	
			}
		}
	}

	return (0 == (m_PlxData->CsBoard.u8BadAddonFirmware | m_PlxData->CsBoard.u8BadBbFirmware));
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSpiderDev::ValidateCsiTarget(uInt32 u32Target)
{
	if ( m_pCardState->bSpiderLP && CSI_CS8xxxLP_TARGET == u32Target )
		return CS_SUCCESS;
	else if ( m_pCardState->bZap && CSI_CSZAP_TARGET == u32Target )
		return CS_SUCCESS;
	else if ( m_pCardState->bSpiderV12 && CSI_CS8xxxV12_TARGET == u32Target )
		return CS_SUCCESS;
	else if ( CSI_CS8xxx_TARGET == u32Target )
		return CS_SUCCESS;
	else if ( CSI_CS8xxxPciEx_TARGET == u32Target )
		return CS_SUCCESS;
	else if ( CSI_ZAP_PCIEx_TARGET == u32Target )
		return CS_SUCCESS;

	return CS_FAIL;
}


#define CSISEPARATOR_LENGTH			16
const char CsiSeparator[] = "-- SEPARATOR --|";		// 16 character long + NULL character

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsSpiderDev::ReadAndValidateCsiFile( uInt32 u32BbHwOptions, uInt32 u32AoHwOptions )
{
	int32			i32RetCode = CS_SUCCESS;
	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry;
	char			szCsiFileName[MAX_FILENAME_LENGTH];


	UNREFERENCED_PARAMETER(u32AoHwOptions);

	// Attempt to open Csi file
	sprintf_s(szCsiFileName, sizeof(szCsiFileName), "%s.csi", m_pCardState->szFpgaFileName );
	FILE_HANDLE	hCsiFile = GageOpenFileSystem32Drivers( szCsiFileName );

	if ( ! hCsiFile )
	{
		m_bCsiFileValid =  false;

		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_NOTFOUND, szCsiFileName );
		return CS_MISC_ERROR;
	}

	uInt32	u32BBTarget = CSI_BBSPIDER_TARGET;	
	uInt32	u32ImageSize = BB_FLASHIMAGE_SIZE;

	if ( m_bNucleon )
	{
		if ( m_pCardState->bZap )
			u32BBTarget = CSI_BBZAP_PCIEx_TARGET;
		else
			u32BBTarget = CSI_BBSPIDER_PCIEx_TARGET;

		u32ImageSize = NUCLEON_FLASH_FWIMAGESIZE;
	}
	else
	{
		if ( m_pCardState->bSpiderLP )
			u32BBTarget = CSI_BBSPIDER_LP_TARGET;
		else if ( m_pCardState->bSpiderV12 )
			u32BBTarget = CSI_BBSPIDERV12_TARGET;
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
				// Found Base board firmware info
				if ( (CsiEntry.u32HwOptions & CSPDR_BBHW_OPT_FWREQUIRED_MASK) ==
					 (u32BbHwOptions & CSPDR_BBHW_OPT_FWREQUIRED_MASK ) )
				{
					if ( CsiEntry.u32ImageSize > u32ImageSize )
					{
						i32RetCode = CS_FAIL;
						break;
					}
					else
						::GageCopyMemoryX( &m_BaseBoardCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
				}
				else
					continue;
			}
			else
			{
				if ( CS_SUCCESS != ValidateCsiTarget(CsiEntry.u32Target) )
				{
					i32RetCode = CS_FAIL;
					break;
				}
				else
				{
					// Found Add-on board firmware info
					if ( (CsiEntry.u32HwOptions & CSPDR_BBHW_OPT_FWREQUIRED_MASK) ==
						 (u32AoHwOptions & CSPDR_BBHW_OPT_FWREQUIRED_MASK ) )
					{
						if ( CsiEntry.u32ImageSize > SPIDER_FLASHIMAGE_SIZE )
						{
							i32RetCode = CS_FAIL;
							break;
						}
						else
							::GageCopyMemoryX( &m_AddonCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
					}
					else
						continue;
				}
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
		if (m_AddonCsiEntry.u32ImageOffset == 0  )
		{
			i32RetCode = CS_FAIL;
		}
		else
		{
			uInt8	u8Buffer[CSISEPARATOR_LENGTH];
			uInt32	u32ReadPosition = m_AddonCsiEntry.u32ImageOffset - CSISEPARATOR_LENGTH;

			if ( 0 == GageReadFile( hCsiFile, u8Buffer, sizeof(u8Buffer), &u32ReadPosition ) )
				i32RetCode = CS_FAIL;
			else
			{
				if ( 0 != ::GageCompareMemoryX( (void *)CsiSeparator, u8Buffer, sizeof(u8Buffer) ) )
					i32RetCode = CS_FAIL;
			}
		}
	}

	GageCloseFile ( hCsiFile );

	if ( CS_FAIL == i32RetCode )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_INVALID, szCsiFileName );
	}
	m_bCsiFileValid = ( CS_SUCCESS == i32RetCode );

	return i32RetCode;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSpiderDev::ReadAndValidateFwlFile()
{
	int32				i32RetCode = CS_SUCCESS;
	FWL_FILE_HEADER		header = {0};
	FPGA_HEADER_INFO	FwlEntry = {0};
	char				szFwlFileName[MAX_FILENAME_LENGTH];
	PCS_BOARD_NODE		pBdParam = &m_PlxData->CsBoard;


	// Attempt to open Fwl file
	sprintf_s(szFwlFileName, sizeof(szFwlFileName), "%s.fwl", m_pCardState->szFpgaFileName );
	FILE_HANDLE hFwlFile = GageOpenFileSystem32Drivers( szFwlFileName );

	// If we can't open the file, report it to the event log whether or not we
	// actually will be using it.
	if ( ! hFwlFile )
	{
		m_bFwlFileValid =  false;

		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_NOTFOUND, szFwlFileName );
		return CS_MISC_ERROR;
	}

	// If expert firmware is not present or not allowed on the current card
	// then do not bother reading Fwl file, just close it and return
	if ( 0 == pBdParam->i64ExpertPermissions )
	{
		GageCloseFile ( hFwlFile );
		return CS_SUCCESS;
	}

	uInt32	u32ImageSize = m_bNucleon ? NUCLEON_FLASH_FWIMAGESIZE : BB_FLASHIMAGE_SIZE;

	GageReadFile( hFwlFile, &header, sizeof(header) );

	if ( header.u32Version <= CSIFILE_VERSION )
		m_bFwlOld = ( header.u32Version == PREVIOUS_FWLFILE_VERSION );		// Old Csi 1.02.01 Fwl file

	if ( ( header.u32Size == sizeof(FWL_FILE_HEADER) ) && CsIsSpiderBoard(header.u32BoardType) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID )  &&
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
						if ( pBdParam->u32BaseBoardOptions[j+1] == FwlEntry.u32Option )
						{
							FPGA_FWVERSION	   FileFwVer;

							::GageCopyMemoryX( &m_BaseBoardExpertEntry[j], &FwlEntry, sizeof(FwlEntry) );
							// Mask the options bits
							FileFwVer.u32Reg = m_BaseBoardExpertEntry[j].u32Version;
							FileFwVer.Version.uMajor &= 0xF;
							m_BaseBoardExpertEntry[j].u32Version = FileFwVer.u32Reg;
						}
					}
				}
				else
				{
					i32RetCode = CS_FAIL;
				}
			}

			if ( CS_FAIL == i32RetCode )
				break;
		}
	}
	else
	{
		i32RetCode = CS_FAIL;
	}

	GageCloseFile ( hFwlFile );

	if ( CS_FAIL == i32RetCode )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_INVALID, szFwlFileName );
	}
	m_bFwlFileValid = (CS_SUCCESS == i32RetCode);

	return i32RetCode;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSpiderDev::CsAutoUpdateFirmware()
{
	int32	i32RetStatus = CS_SUCCESS;
	int32	i32Status = CS_SUCCESS;

	if ( m_bSkipFirmwareCheck )
		return CS_FAIL;

	CsSpiderDev *pCard = (CsSpiderDev*) m_ProcessGlblVars->pFirstUnCfg;

	while( pCard )
	{
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

		pCard->UpdateCardState();

		pCard->UnInitializeDevIoCtl();
		pCard = pCard->m_HWONext;
	}

	return i32RetStatus; 
}



//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSpiderDev::CombineCardUpdateFirmware()
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
	FILE_HANDLE				hCsiFile = (FILE_HANDLE)0;
	FILE_HANDLE				hFwlFile = (FILE_HANDLE)0;
	char					szCsiFileName[MAX_FILENAME_LENGTH];
	char					szFwlFileName[MAX_FILENAME_LENGTH];


	sprintf_s(szCsiFileName, sizeof(szCsiFileName), "%s.csi", m_pCardState->szFpgaFileName );
	sprintf_s(szFwlFileName, sizeof(szCsiFileName), "%s.fwl", m_pCardState->szFpgaFileName );

	if ( (hCsiFile = GageOpenFileSystem32Drivers( szCsiFileName )) == 0 )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_OPEN_FILE, szCsiFileName );
		return CS_FAIL;
	}

	// check if any expert firmware needs updating. If not don't bother reading the fwl file.
	// if they do need updating and we can't open the file it's an error
	BOOL bUpdateFwl = ( m_PlxData->CsBoard.u8BadBbFirmware & (BAD_EXPERT1_FW | BAD_EXPERT2_FW) );

	if ( bUpdateFwl )
	{
		if ( (hFwlFile = GageOpenFileSystem32Drivers( szFwlFileName )) == 0 )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_OPEN_FILE, szFwlFileName );
			GageCloseFile( hCsiFile );
			return CS_FAIL;
		}
	}

	pBuffer = (uInt8 *) GageAllocateMemoryX( Max(BB_FLASHIMAGE_SIZE, SPIDER_FLASHIMAGE_SIZE) );
	if ( NULL == pBuffer )
	{
		GageCloseFile( hCsiFile );
		if ( 0 != hFwlFile )
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
			::GageZeroMemoryX( pBuffer, BB_FLASHIMAGE_SIZE);
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

			// The frimware has been changed.
			// Invalidate this firmware version inorder to force reload the firmware version later
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
					::GageZeroMemoryX( pBuffer, BB_FLASHIMAGE_SIZE);
					GageReadFile(hFwlFile, pBuffer, m_BaseBoardExpertEntry[i].u32ImageSize, &m_BaseBoardExpertEntry[i].u32ImageOffset);

					// Write Base board FPGA image
					i32Status = WriteFlashEx( pBuffer, BB_FLASHIMAGE_SIZE*(i+1), m_BaseBoardExpertEntry[i].u32ImageSize, AccessMiddle );
					if ( CS_SUCCEEDED(i32Status) )
					{
						HdrElement[i+1].u32Version = m_BaseBoardExpertEntry[i].u32Version;
						HdrElement[i+1].u32FpgaOptions = m_BaseBoardExpertEntry[i].u32Option;
						i32FwUpdateStatus[i+1] = CS_SUCCESS;
					}

					// The frimware has been changed.
					// Invalidate this firmware version inorder to force reload the firmware version later
					GetBoardNode()->u32RawFwVersionB[i+1] = 0;
				}
			}
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

			// Memory size may have changed (Memory less option)
			UpdateMaxMemorySize( m_PlxData->CsBoard.u32BaseBoardHardwareOptions, CSPDR_DEFAULT_SAMPLE_SIZE );
			ClearInterrupts();
			if ( !m_pCardState->bAddOnRegistersInitialized )
			{
				m_PlxData->InitStatus.Info.u32AddOnPresent = IsAddonBoardPresent();
				if ( m_PlxData->InitStatus.Info.u32AddOnPresent )
				{
					InitAddOnRegisters();
				}
			}
		}
		else
		{
			SpiderFlashReset();
			//Reset the pointer to the default image
			BaseBoardConfigReset(0);
		}
	}

	// Update Addon firmware
	i32Status = UpdateAddonFirmware( hCsiFile, pBuffer );

	GageCloseFile( hCsiFile );

	if ( hFwlFile )
		GageCloseFile( hFwlFile );

	GageFreeMemoryX( pBuffer );

	return i32Status;
}


//-----------------------------------------------------------------------------
//	User channel skip depends on mode amd number of channels per card
//-----------------------------------------------------------------------------
uInt16	CsSpiderDev::GetUserChannelSkip()
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
BOOLEAN	CsSpiderDev::IsChannelIndexValid( uInt16 u16ChannelIndex )
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

CsSpiderDev *CsSpiderDev::GetCardPointerFromBoardIndex( uInt16 BoardIndex )
{
	CsSpiderDev *pDevice = m_MasterIndependent;

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

CsSpiderDev *CsSpiderDev::GetCardPointerFromMsCardId( uInt8 MsCardId )
{
	CsSpiderDev *pDevice = m_MasterIndependent;

	while ( pDevice )
	{
		if ( pDevice->m_pCardState->u8MsCardId == MsCardId )
			break;
		pDevice = pDevice->m_Next;
	}

	return pDevice;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsSpiderDev::NormalizedChannelIndex( uInt16 u16ChannelIndex )
{
	ASSERT( u16ChannelIndex != 0 );

	uInt16 Temp = (u16ChannelIndex - 1)  / m_PlxData->CsBoard.NbAnalogChannel;
	return (u16ChannelIndex - Temp * m_PlxData->CsBoard.NbAnalogChannel);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::NormalizedTriggerSource( int32 i32TriggerSource )
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

BOOLEAN	CsSpiderDev::IsAddonBoardPresent()
{
	uInt32 u32Reg = ReadGIO(CSPDR_ADDON_FDV);
	const uInt32 cu32AddEcho = CSPDR_ADDON_FDV | (CSPDR_ADDON_FDV << 8) | (CSPDR_ADDON_FDV << 16) | (CSPDR_ADDON_FDV << 24);

	if( 0 == u32Reg || 0xffffffff == u32Reg || cu32AddEcho == u32Reg )
		return FALSE;

	return TRUE;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN	CsSpiderDev::AssignBoardProperty()
{
	int32	i32RetCode;
	uInt16	u16StartBoardType = CS8280_BOARDTYPE;
	uInt16	u16DeviceId = GetDeviceId();

	if ( m_bNucleon )
	{
		m_pCardState->bSpiderV12 = true;

		if ( ( u16DeviceId == CS_DEVID_ZAP_PCIE ) )
		{
			m_pCardState->bZap = true;
			// Set the correct fpga image file name and cal file
			strcpy_s( m_pCardState->szFpgaFileName, sizeof(m_pCardState->szFpgaFileName), CSZAPPCIEX_FPGAIMAGE_FILENAME );
		}
		else
		{
			// Set the correct fpga image file name and cal file
			strcpy_s( m_pCardState->szFpgaFileName, sizeof(m_pCardState->szFpgaFileName), CS8YYYPCIEX_FPGAIMAGE_FILENAME );
		}
	}
	else if ( ( u16DeviceId == CS_DEVID_SPIDER_v12 ) || ( u16DeviceId == CS_DEVID_ST81AVG ) )
	{
		m_pCardState->bSpiderV12 = true;

		// Set the correct fpga image file name and cal file
		strcpy_s( m_pCardState->szFpgaFileName, sizeof(m_pCardState->szFpgaFileName), CS8YYY_FPGA_IMAGE_FILE_NAME );
	}
	else if ( ( u16DeviceId == CS_DEVID_SPIDER_LP ) || ( u16DeviceId == CS_DEVID_STLP81 ) )
	{
		m_pCardState->bSpiderV12 = true;
		m_pCardState->bSpiderLP = true;

		// Set the correct fpga image file name and cal file
		strcpy_s( m_pCardState->szFpgaFileName, sizeof(m_pCardState->szFpgaFileName), CS8ZZZ_FPGA_IMAGE_FILE_NAME );
	}
	else if ( ( u16DeviceId == CS_DEVID_ZAP ) )
	{
		m_pCardState->bZap = true;
		m_pCardState->bSpiderV12 = true;
		// Set the correct fpga image file name and cal file
		strcpy_s( m_pCardState->szFpgaFileName, sizeof(m_pCardState->szFpgaFileName), CSZAP_FPGA_IMAGE_FILE_NAME );
	}
	else
	{
		// Set the correct fpga image file name and cal file
		strcpy_s( m_pCardState->szFpgaFileName, sizeof(m_pCardState->szFpgaFileName), CS8XXX_FPGA_IMAGE_FILE_NAME );
	}

	i32RetCode = InitHwTables();

	if ( CS_SUCCEEDED(i32RetCode) )
	{
		if ( CSPDR_12BIT_DATA_MASK == m_pCardState->u32DataMask )
		{
			switch( m_PlxData->CsBoard.NbAnalogChannel )
			{
				case 8: u16StartBoardType = CS8280_BOARDTYPE; break;
				case 4:	u16StartBoardType = CS8240_BOARDTYPE; break;
				case 2:	u16StartBoardType = CS8220_BOARDTYPE; break;
				case 1:	u16StartBoardType = CS8210_BOARDTYPE; break;
				default: m_bInvalidHwConfiguration = TRUE;    break;
			}
		}
		else if ( CSPDR_14BIT_DATA_MASK == m_pCardState->u32DataMask )
		{
			if( m_pCardState->bZap )
			{
				switch( m_PlxData->CsBoard.NbAnalogChannel )
				{
					case 8:	u16StartBoardType = CS8480_BOARDTYPE; break;
					case 4:	u16StartBoardType = CS8440_BOARDTYPE; break;
					case 2:	u16StartBoardType = CS8420_BOARDTYPE; break;
					case 1:	u16StartBoardType = CS8410_BOARDTYPE; break;
					default: m_bInvalidHwConfiguration = TRUE;    break;
				}
			}
			else
			{
				switch( m_PlxData->CsBoard.NbAnalogChannel )
				{
					case 8:	u16StartBoardType = CS8380_BOARDTYPE; break;
					case 4:	u16StartBoardType = CS8340_BOARDTYPE; break;
					case 2:	u16StartBoardType = CS8320_BOARDTYPE; break;
					case 1:	u16StartBoardType = CS8310_BOARDTYPE; break;
					default: m_bInvalidHwConfiguration = TRUE;    break;
				}
			}
		}
		else
		{
			m_bInvalidHwConfiguration = TRUE;
		}
	}

	if ( m_bInvalidHwConfiguration || CS_FAILED(i32RetCode) )
	{
		// Continue initializtion with some default value
		// This allows CsHwConfig program access to the card later
		m_PlxData->CsBoard.NbAnalogChannel =  8;
		m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_14;
		m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_14_LJ;
		m_PlxData->CsBoard.SampleRateLimit = 0;
		u16StartBoardType = CS8380_BOARDTYPE;
		m_pCardState->AcqConfig.u32SampleSize = CSPDR_DEFAULT_SAMPLE_SIZE;
	}

	m_PlxData->CsBoard.NbTriggerMachine = 2*m_PlxData->CsBoard.NbAnalogChannel + 1;
	m_PlxData->CsBoard.NbDigitalChannel = 0;

	m_PlxData->CsBoard.u32BoardType = u16StartBoardType + m_PlxData->CsBoard.SampleRateLimit;
	if ( m_bNucleon )
		m_PlxData->CsBoard.u32BoardType |= CSNUCLEONBASE_BOARDTYPE;

	switch ( m_PlxData->CsBoard.NbAnalogChannel )
	{
		case 8:	m_pCardState->u32DefaultMode = CS_MODE_OCT;	break;
		case 4:	m_pCardState->u32DefaultMode = CS_MODE_QUAD; break;
		case 2:	m_pCardState->u32DefaultMode = CS_MODE_DUAL; break;
		case 1:	m_pCardState->u32DefaultMode = CS_MODE_SINGLE; break;
		default:
			m_pCardState->u32DefaultMode = CS_MODE_SINGLE; break;
	}

	m_pCardState->AcqConfig.u32Mode = m_pCardState->u32DefaultMode;

	return	(! m_bInvalidHwConfiguration);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsSpiderDev::UpdateMaxMemorySize( uInt32	u32BaseBoardHwOptions, uInt32 u32SampleSize )
{
	u32BaseBoardHwOptions;

	if ( m_pCardState->bSpiderLP )
	{
		m_PlxData->CsBoard.i64MaxMemory = m_u32MaxHwAvgDepth;
	}
	else
	{
		uInt32 u32MemSize = ReadRegisterFromNios(0, CSPLXBASE_GET_MEMORYSIZE_CMD);
		m_PlxData->CsBoard.i64MaxMemory = (LONGLONG(u32MemSize) << 10)/u32SampleSize;
	}

	m_PlxData->InitStatus.Info.u32BbMemTest = m_PlxData->CsBoard.i64MaxMemory != 0 ? 1 : 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSpiderDev::CsSetDataFormat( _CSDATAFORMAT csFormat )
{
	if ( CSPDR_14BIT_DATA_MASK == m_pCardState->u32DataMask )
	{
		if( m_pCardState->bZap ) // 16 Bits cards
		{
			switch ( csFormat )
			{
				case formatHwAverage:
					// Full resolution, 24 bit data left aligned inside 32 bit
					m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_16;
					m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
					m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_16_LJ << 6;
					m_pCardState->AcqConfig.i32SampleRes	= CSPDR_DEFAULT_RES << 6;
					break;

				case formatSoftwareAverage:
					m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_16;
					m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
					m_pCardState->AcqConfig.i32SampleRes = CSPDR_DEFAULT_RES;
					m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_16_LJ;
					break;

				default:
					m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_16;
					m_pCardState->AcqConfig.i32SampleRes = CSPDR_DEFAULT_RES;
					m_pCardState->AcqConfig.u32SampleSize = CSPDR_DEFAULT_SAMPLE_SIZE;
					m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_16_LJ;
					break;
			}
		}
		else  // 14 Bits cards
		{
			switch ( csFormat )
			{
				case formatHwAverage:
					// Full resolution, 24 bit data left aligned inside 32 bit
					m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_14;
					m_pCardState->AcqConfig.u32SampleSize		= CS_SAMPLE_SIZE_4;
					m_pCardState->AcqConfig.i32SampleOffset	= CSPDR_DEFAULT_SAMPLE_OFFSET << 6;
					m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES << 6;
					break;

				case formatSoftwareAverage:
					m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_14;
					m_pCardState->AcqConfig.u32SampleSize		= CS_SAMPLE_SIZE_4;
					m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES;
					m_pCardState->AcqConfig.i32SampleOffset	= CSPDR_DEFAULT_SAMPLE_OFFSET;
					break;

				case formatFIR:
					m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_14;
					m_pCardState->AcqConfig.u32SampleSize		= CS_SAMPLE_SIZE_2;
					m_pCardState->AcqConfig.i32SampleOffset	= CS_SAMPLE_OFFSET_16;
					m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES >> 2;
					break;

				default:
					m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_14;
					m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES;
					m_pCardState->AcqConfig.u32SampleSize		= CSPDR_DEFAULT_SAMPLE_SIZE;
					m_pCardState->AcqConfig.i32SampleOffset	= CSPDR_DEFAULT_SAMPLE_OFFSET;
					break;
			}
		}
	}
	else if ( CSPDR_12BIT_DATA_MASK == m_pCardState->u32DataMask )
	{
		// 12 Bits cards
		switch ( csFormat )
		{
			case formatHwAverage:
				// Full resolution, 24 bit data left aligned inside 32 bit
				m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_12;
				m_pCardState->AcqConfig.u32SampleSize		= CS_SAMPLE_SIZE_4;
				m_pCardState->AcqConfig.i32SampleOffset	= CS_SAMPLE_OFFSET_12_LJ << 4;
				m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES << 4;
				if ( m_bNucleon )
				{
					m_pCardState->AcqConfig.i32SampleOffset	<<= 2;
					m_pCardState->AcqConfig.i32SampleRes		<<= 2;
				}
				break;

			case formatSoftwareAverage:
				m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_12;
				m_pCardState->AcqConfig.u32SampleSize		= CS_SAMPLE_SIZE_4;
				m_pCardState->AcqConfig.i32SampleOffset	= CS_SAMPLE_OFFSET_12_LJ;
				m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES;
				break;

			case formatFIR:
				m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_12;
				m_pCardState->AcqConfig.u32SampleSize		= CS_SAMPLE_SIZE_2;
				m_pCardState->AcqConfig.i32SampleOffset	= -4;
				m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES >> 2;	// Without 2 Dgital bits
				break;

			default:
				if ( m_pCardState->bSpiderLP )
				{
					m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_12;
					m_pCardState->AcqConfig.u32SampleSize		= CSPDR_DEFAULT_SAMPLE_SIZE;
					m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES;
					m_pCardState->AcqConfig.i32SampleOffset	= CS_SAMPLE_OFFSET_12;
				}
				else
				{
					m_pCardState->AcqConfig.u32SampleBits		= CS_SAMPLE_BITS_12;
					m_pCardState->AcqConfig.u32SampleSize		= CSPDR_DEFAULT_SAMPLE_SIZE;
					m_pCardState->AcqConfig.i32SampleRes		= CSPDR_DEFAULT_RES;
					m_pCardState->AcqConfig.i32SampleOffset	= CS_SAMPLE_OFFSET_12_LJ;
				}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CsSpiderDev::IsAddonInit()
{
	int32	i32Retry = 500;

	if( m_pCardState->bAddonInit )
		return true;

	if ( !m_PlxData->InitStatus.Info.u32AddOnPresent )
		return false;

	uInt32	u32Status = ReadGIO(CSPDR_ADDON_STATUS);
	while( (u32Status & CSPDR_ADDON_STAT_CONFIG_DONE) != 0 )
	{
		u32Status = ReadGIO(CSPDR_ADDON_STATUS);
		if ( i32Retry-- <= 0 )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: CSPDR_ADDON_STAT_CONFIG_DONE Timeout" );
			return false;	// timeout !!!
		}
	}

	m_PlxData->InitStatus.Info.u32AddOnFpga = 1;

	if( !m_bSkipSpiTest)
	{
		// Spi chip select test
		WriteGIO(CSPDR_ADDON_SPI_TEST_WR ,0x01);

		uInt32 u32CmdMask = 0x00810000; //spi write

		int32 i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x01);
		if( CS_FAILED ( i32Ret ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #1 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x02);
		if( CS_FAILED ( i32Ret ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #2 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x04);
		if( CS_FAILED ( i32Ret ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #3 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x08);
		if( CS_FAILED ( i32Ret ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #4 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x10);
		if( CS_FAILED ( i32Ret ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #5 test error." );
			return false;
		}

		i32Ret = WriteRegisterThruNios(0x1f,u32CmdMask|0x20);
		if( CS_FAILED ( i32Ret ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: Spi chip select #6 test error." );
			return false;
		}

		uInt32 u32Data = ReadGIO(CSPDR_ADDON_SPI_TEST_RD);

		WriteGIO(CSPDR_ADDON_SPI_TEST_WR ,0x02);		//close the test module
		WriteGIO(CSPDR_ADDON_SPI_TEST_WR ,0);

		if( 0x3f != u32Data )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "AddonInit: CSPDR_ADDON_SPI_TEST_RD error." );
			return false;
		}
	}//SkipSpitest

	m_pCardState->bAddonInit = TRUE;

	return true;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::InitAddOnRegisters(bool bForce)
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
			m_PlxData->CsBoard.u8BadAddonFirmware = BAD_DEFAULT_FW;
			return CS_ADDONINIT_ERROR;
		}
	}
	else
		return CS_ADDON_NOT_CONNECTED;

	// Wait for Addon board ready
	for(;;)
	{
		if ( FAILED_NIOS_DATA != ReadRegisterFromNios( 0, CSPDR_FWV_RD_CMD )  )
			break;

		if ( i32Retry-- <= 0 )
		{
			return CS_ADDONINIT_ERROR;
		}
	}

	i32Ret = SetPulseOut();
	if( CS_FAILED ( i32Ret ) )	return i32Ret;

	i32Ret = SetClockOut(m_pCardState->eclkoClockOut);
	if( CS_FAILED ( i32Ret ) )	return i32Ret;

	i32Ret = ReadCalibTableFromEeprom();
	if ( CS_FAILED(i32Ret) )return i32Ret;

//	i32Ret = SendTimeStampMode();
//	if( CS_FAILED(i32Ret) )	return i32Ret;

	for ( uInt16 i = 1; i <=  m_PlxData->CsBoard.NbAnalogChannel; i ++ )
	{
		uInt16	u16HardwareChannel = ConvertToHwChannel(i);

		i32Ret = SendChannelSetting( u16HardwareChannel, GetDefaultRange() );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		SendTriggerEngineSetting( u16HardwareChannel, CS_TRIG_SOURCE_DISABLE, CSPDR_DEFAULT_CONDITION, 0,
													  CS_TRIG_SOURCE_DISABLE, CSPDR_DEFAULT_CONDITION, 0 );
		if( CS_FAILED(i32Ret) ) return i32Ret;
	}

	i32Ret = SendExtTriggerSetting( FALSE, eteAlways, 0, CSPDR_DEFAULT_CONDITION, CSPDR_DEFAULT_EXT_GAIN, CSPDR_DEFAULT_EXT_COUPLING );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToSelfTestRegister();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendClockSetting();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCaptureModeSetting();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->bAddOnRegistersInitialized = true;

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsSpiderDev::ReadAddOnHardwareInfoFromEeprom( BOOLEAN bCheckSum )
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32Offset;

	// Read Checksum
	uInt32		u32Checksum[CSPDR_MAX_IMAGE_NUM];

	u32Offset = SPDR_FLASH_INFO_ADDR  + FIELD_OFFSET(SPDR_GENERAL_INFO_STRUCTURE, dwSignature);
	i32Status = ReadEeprom( u32Checksum, u32Offset, sizeof(u32Checksum) );
	if ( CS_FAILED(i32Status) )
		return i32Status;

	if ( ! m_bSkipFirmwareCheck  )
	{
		// Validate Checksum of the default image
		if ( bCheckSum &&  (u32Checksum[0] != CSI_FWCHECKSUM_VALID ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_FWCHECKSUM_ERROR, "Addon board" );
			m_PlxData->CsBoard.u8BadAddonFirmware = BAD_DEFAULT_FW;
			return CS_INVALID_FW_VERSION;
		}
	}

	SPDR_GENERAL_INFO_STRUCTURE	GeneralInfoStr = {0};
	u32Offset = SPDR_FLASH_INFO_ADDR  + FIELD_OFFSET(SPDR_FLASH_STRUCT, Info);

	i32Status = ReadEeprom( &GeneralInfoStr, u32Offset, sizeof(GeneralInfoStr) );
	if ( CS_SUCCESS == i32Status )
	{
		m_PlxData->CsBoard.u32AddonBoardVersion = GeneralInfoStr.dwHWVersion;
		m_PlxData->CsBoard.u32SerNumEx = GeneralInfoStr.dwSerialNumber;

		for ( uInt32 i = 0; i < CSPDR_MAX_IMAGE_NUM; i ++ )
		{
			if ( u32Checksum[i] == CSI_FWCHECKSUM_VALID && GeneralInfoStr.dwImageOptions[i] != -1)
				m_PlxData->CsBoard.u32AddonOptions[i] = GeneralInfoStr.dwImageOptions[i];
			else
				m_PlxData->CsBoard.u32AddonOptions[i] = 0;
		}

		m_PlxData->CsBoard.u32ActiveAddonOptions = m_PlxData->CsBoard.u32AddonOptions[0];
		m_PlxData->CsBoard.u32AddonHardwareOptions = GeneralInfoStr.dwHWOptions;

		if ( -1 != GeneralInfoStr.u32ClkOut )
			m_pCardState->eclkoClockOut		= ( eClkOutMode ) GeneralInfoStr.u32ClkOut;
		if ( -1 != GeneralInfoStr.u32TrigOutEnabled )
			m_pCardState->bDisableTrigOut		= (0 == GeneralInfoStr.u32TrigOutEnabled);
	}
	else
	{
		::GageZeroMemoryX( m_PlxData->CsBoard.u32AddonOptions, sizeof(m_PlxData->CsBoard.u32AddonOptions) );
		m_PlxData->CsBoard.u32AddonBoardVersion = 0;
		m_PlxData->CsBoard.u32SerNumEx = 0;
		m_PlxData->CsBoard.u32ActiveAddonOptions = 0;
		m_PlxData->CsBoard.u32AddonHardwareOptions = 0;
	}

	DetectAuxConnector();
	DetectExtTrigConnector();

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSpiderDev::SetTrigOut( uInt16 u16Value )
{
	uInt32 u32Data = m_pCardState->AddOnReg.u16TrigEnable & ~CSPDR_SET_DIG_TRIG_TRIGOUTENABLED;
	if ( m_pCardState->bSpiderV12 && ( CS_TRIGOUT == u16Value  ) )
	{
		u32Data |= CSPDR_SET_DIG_TRIG_TRIGOUTENABLED;
		m_pCardState->bDisableTrigOut = FALSE;
	}
	else
		m_pCardState->bDisableTrigOut = TRUE;
	
	m_pCardState->AddOnReg.u16TrigEnable = (uInt16) u32Data;

	int32 i32Ret = WriteRegisterThruNios(u32Data, CSPDR_TRIG_SET_WR_CMD);
	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----	SetPulseOut
//----- ADD-ON REGISTER
//----- Address	CSPDR_OPTION_SET_WR_CMD
//-----------------------------------------------------------------------------

int32 CsSpiderDev::SetPulseOut(ePulseMode epmSet)
{
	if( !m_pCardState->bAuxConnector )
	{
		return CS_FALSE;
	}

	uInt16 u16Data = CSPDR_SPI;

	switch (epmSet)
	{
		default:				epmSet = epmDataValid;
		case epmDataValid:      u16Data |= CSPDR_PULSE_GD_VAL; break;
		case epmNotDataValid:   u16Data |= CSPDR_PULSE_GD_INV; break;
		case epmStartDataValid: u16Data |= CSPDR_PULSE_GD_RISE;break;
		case epmEndDataValid:   u16Data |= CSPDR_PULSE_GD_FALL;break;
		case epmSync:           u16Data |= CSPDR_PULSE_SYNC;   break;
	}

	int32 i32Ret = WriteRegisterThruNios(u16Data, CSPDR_OPTION_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}
	m_pCardState->AddOnReg.u16Option= u16Data;
	m_pCardState->epmPulseOut = epmSet;

	//Allow relay to settle
	timing(CSPDR_RELAY_SETTLING_TIME);

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::SetClockOut( eClkOutMode ecomSet )
{
	uInt16	u16Data(0);
	int32	i32Ret;

	if( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions&CSPDR_AOHW_OPT_EMONA_FF) )
	{
		m_pCardState->eclkoClockOut = eclkoNone;
		return CS_FALSE;
	}

	switch (ecomSet)
	{
		case eclkoRefClk:
			u16Data |= CSPDR_SET_REFCLK_OUT|CSPRD_CLK_OUT_ENABLE;
			break;

		case eclkoNone:
			break;

		case eclkoSampleClk:
			ecomSet = eclkoSampleClk;
			u16Data |= CSPRD_CLK_OUT_ENABLE;
		    break;
		default:
			return CS_INVALID_PARAMETER;
	}

	uInt16		u16CurrentSetting = m_pCardState->AddOnReg.u16ClkSelect;

	m_pCardState->eclkoClockOut = ecomSet;

	u16CurrentSetting &= ~CSPDR_CLK_OUT_MUX_MASK;
	u16CurrentSetting |= (u16Data & ~CSPRD_CLK_OUT_ENABLE);

	i32Ret = WriteRegisterThruNios(u16CurrentSetting, CSPDR_CLK_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	timing(100);

	i32Ret = WriteRegisterThruNios(u16CurrentSetting | u16Data, CSPDR_CLK_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->AddOnReg.u16ClkSelect = u16CurrentSetting | u16Data;
	m_pCardState->AddOnReg.u16ClkOut = u16Data;

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::AddonConfigReset()
{
	CSTIMER		CsTimeout;


	WriteGIO(CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_CONFIG);
	WriteGIO(CSPDR_ADDON_FLASH_CMD, 0);

	// Wait for Config done ...
	CsTimeout.Set( KTIME_SECOND(4) );		// Wait for max 8 x 1sec

	uInt32	u32Status = ReadGIO(CSPDR_ADDON_STATUS);
	while( (u32Status & CSPDR_ADDON_STAT_CONFIG_DONE) == 0 )
	{
		u32Status = ReadGIO(CSPDR_ADDON_STATUS);
		if ( CsTimeout.State() )
		{
			u32Status = ReadGIO(CSPDR_ADDON_STATUS);
			if( (u32Status & CSPDR_ADDON_STAT_CONFIG_DONE) == 0 )
			{
				return CS_FALSE;
			}
		}
	}

	// Patch for AddonConfigReset()
	// Addon is still not ready, we have to wait a little bit
	// Otherwise, all subsequence the addon setting will not work ....
	Sleep(300);

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//	Initialization upon received IOCTL_ACQUISITION_SYSTEM_INIT
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::AcqSystemInitialize()
{
	int32	i32Ret = CS_SUCCESS;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return i32Ret;

	if ( m_pCardState->bZap )
		m_u16TrigSensitivity = CSPDR_DEFAULT_ZAP_TRIG_SENSE;
	else
		m_u16TrigSensitivity = CSPDR_DEFAULT_TRIG_SENSITIVITY;
#ifdef _WINDOWS_
	ReadCommonRegistryOnAcqSystemInit();
#else
	ReadCommonCfgOnAcqSystemInit();
#endif   

	i32Ret = InitAddOnRegisters();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	//SetClockStopageParmeters
	if( 0 != m_u32ExtClkStop )
	{
		//write to the board
		uInt32 u32Data = CSPDR_CLK_STOP_CFG | ((m_u32ExtClkStop & 0xFFF) << 2);
		i32Ret = WriteRegisterThruNios(u32Data, CSPDR_CLK_STOP_CFG_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSpiderDev::ResetCachedState(void)
{
	for( uInt16 i = 1; i <= CSPDR_CHANNELS; i++ )
	{
		m_pCardState->bFastCalibSettingsDone[i-1] = FALSE;
	}
	memset( &(m_pCardState->AddOnReg), 0, sizeof(CSSPIDER_ADDON_REGS) );

	m_u32CalibRange_uV = 0;		
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::CsLoadBaseBoardFpgaImage( uInt8 u8ImageId )
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
			bImageChanged = (0 != m_pCardState->u8ImageId);
			if ( bImageChanged )
			{
				bImageChanged = TRUE;
				BaseBoardConfigurationReset(0);
				m_pCardState->u8ImageId = 0;

				m_bHardwareAveraging = FALSE;
				m_pCardState->AcqConfig.u32Mode &= ~(CS_MODE_USER1 | CS_MODE_USER2);
			}
		}
		break;

	}

	m_bMulrecAvgTD = FALSE;

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
		if ( m_bNucleon )
		{
			UpdateBaseBoardMsStatus();
			SetMsTriggerEnable( ermStandAlone != m_pCardState->ermRoleMode );
		}

		ResetCachedState();
		ReadVersionInfo();
		UpdateCardState();
	}

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
uInt32 CsSpiderDev::GetDefaultRange()
{
	uInt32*	pSwingTable;
	uInt32	u32MaxIndex = 0;
	uInt32  u32DefaultImped = CSPDR_DEFAULT_IMPEDANCE;


	if(CS_REAL_IMP_1M_OHM == u32DefaultImped)
	{
		pSwingTable = m_pCardState->u32SwingTable[1];
		u32MaxIndex = m_pCardState->u32SwingTableSize[1];
	}
	else
	{
		pSwingTable = m_pCardState->u32SwingTable[0];
		u32MaxIndex = m_pCardState->u32SwingTableSize[0];
	}

	for( uInt32 i = 0; i < u32MaxIndex; i++ )
	{
		if ( CSPDR_DEFAULT_GAIN == pSwingTable[i] )
			return CSPDR_DEFAULT_GAIN;
	}

	return pSwingTable[0];
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
uInt32	CsSpiderDev:: GetFifoModeTransfer( uInt16 u16UserChannel, bool bInterleaved )
{
	uInt32	u32ModeSel = 0;

	if ( ! bInterleaved )
	{
		// Convert user channel number (ex :1,3, 5 ..)
		// to Nios channel number (1, 2, 3)
		u16UserChannel = (u16UserChannel - 1) / GetUserChannelSkip() + 1;

		u32ModeSel = (u16UserChannel - 1) << 4;

		if ( (m_pCardState->AcqConfig.u32Mode & CS_MODE_OCT) != 0  )
			u32ModeSel |= MODESEL3;
		else if ( (m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD) != 0  )
			u32ModeSel |= MODESEL2;
		else if ( (m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0  )
			u32ModeSel |= MODESEL1;
	}

	return u32ModeSel;
}


void CsSpiderDev::UpdateCardState(BOOLEAN bReset)
{
	m_pCardState->bCardStateValid = !bReset;
	m_pCardState->bCardStateValid &= m_PlxData->InitStatus.Info.u32Nios;

#if !defined(USE_SHARED_CARDSTATE)
	IoctlSetCardState(m_pCardState);
#endif
}

#ifdef _WINDOWS_

//------------------------------------------------------------------------------
//
//	Parameters on Board initialization.
//  Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

int32	CsSpiderDev::ReadCommonRegistryOnBoardInit()
{
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Cs8xxxWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_bSkipFirmwareCheck ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipCheckFirmware"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipFirmwareCheck = (ul != 0);

		ul = m_bSkipSpiTest ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipSpiTest"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipSpiTest = (ul != 0);

		ul = m_bNoConfigReset ? 1 : 0;
		::RegQueryValueEx( hKey, _T("NoConfigReset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bNoConfigReset = (ul != 0);
		
		ul = m_bClearConfigBootLocation ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ClearCfgBootLocation"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bClearConfigBootLocation = (ul != 0);

		::RegCloseKey( hKey );
	}	

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32 CsSpiderDev::ReadCommonRegistryOnAcqSystemInit()
{
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Cs8xxxWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_bZeroTriggerAddress ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ZeroTrigAddress"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroTriggerAddress = (ul != 0);


		ul = m_bZeroTrigAddrOffset ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ZeroTrigAddrOffset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroTrigAddrOffset = (ul != 0);

		ul = m_bZeroDepthAdjust ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ZeroDepthAdjust"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroDepthAdjust = (ul != 0);


		ul = m_u16TrigSensitivity;
		::RegQueryValueEx( hKey, _T("DTriggerSensitivity"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul <= 255 )
			m_u16TrigSensitivity = (uInt8) ul;
		else
			m_u16TrigSensitivity = 255;

		ul = m_pCardState->u16AdcOffsetAdjust;
		::RegQueryValueEx( hKey, _T("ExtClkStop"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_pCardState->u16AdcOffsetAdjust = (uInt16) ul;
		
		ul = m_u32ExtClkStop;
		::RegQueryValueEx( hKey, _T("ExtClkStop"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32ExtClkStop = (uInt16) ul;


		// Calibration settings
		ul = m_bSkipCalib ? 1 : 0;
		::RegQueryValueEx( hKey, _T("FastDcCal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipCalib = 0 != ul;

		ul = m_bSkipTrim ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipTrim"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipTrim = 0 != ul;

		ul = m_u16SkipCalibDelay;
		::RegQueryValueEx( hKey, _T("NoCalibrationDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );

		if(ul > 24 * 60 ) //limitted to 1 day
			ul = 24*60;
		m_u16SkipCalibDelay = (uInt16)ul;

		ul = m_bNeverCalibrateDc ? 1 : 0;
		::RegQueryValueEx( hKey, _T("NeverCalibrateDc"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bNeverCalibrateDc = 0 != ul;

		ul = m_u32DebugCalibSrc;
		::RegQueryValueEx( hKey, _T("DebugCalibSource"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32DebugCalibSrc = ul;

		ul = m_u32DebugCalibTrace;
		::RegQueryValueEx( hKey, _T("TraceCalib"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32DebugCalibTrace = ul;

		ul = m_u8CalDacSettleDelay;
		::RegQueryValueEx( hKey, _T("CalDacSettleDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u8CalDacSettleDelay = (uInt8)ul;

		ul = m_u32IgnoreCalibErrorMask;
		::RegQueryValueEx( hKey, _T("IgnoreCalibError"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32IgnoreCalibErrorMask = ul;

		ul = m_bDisableUserOffet ? 1 : 0;
		::RegQueryValueEx( hKey, _T("DisableUserOffet"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bDisableUserOffet = (ul != 0);

		::RegCloseKey( hKey );
	}	

	return CS_SUCCESS;
}

#else
   
#define DRV_SECTION_NAME      "Octopus"

//------------------------------------------------------------------------------
// Linux only
//	Parameters on Board initialization.
// Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::ReadCommonCfgOnBoardInit()
{
   char szSectionName[16]={0};
   char szDummy[128]={0};
   ULONG	ul(0);
	DWORD	DataSize = sizeof(ul);
   char szIniFile[256]={0};
     
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
   
#if 0
   // keys not supported for Linux
	ul = m_bSkipFirmwareCheck ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("SkipCheckFirmware"),  ul,  szIniFile);
	m_bSkipFirmwareCheck = (ul != 0);    

	ul = m_bSkipSpiTest ? 1 : 0;
	ul = GetCfgKeyInt(szSectionName, _T("SkipSpiTest"),  ul,  szIniFile);
   m_bSkipSpiTest = (ul != 0);

	ul = m_bClearConfigBootLocation ? 1 : 0;
	ul = GetCfgKeyInt(szSectionName, _T("ClearCfgBootLocation"),  ul,  szIniFile);	
   m_bClearConfigBootLocation = (ul != 0);
#endif
   
	ul = m_bNoConfigReset ? 1 : 0;
	ul = GetCfgKeyInt(szSectionName, _T("NoConfigReset"),  ul,  szIniFile);
   m_bNoConfigReset = (ul != 0);
 
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
// Linux only
//------------------------------------------------------------------------------

int32 CsSpiderDev::ReadCommonCfgOnAcqSystemInit()
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
   
	ul = m_bZeroTriggerAddress ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroTrigAddress"),  ul,  szIniFile);	
	m_bZeroTriggerAddress = (ul != 0);

	ul = m_bZeroTrigAddrOffset ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroTrigAddrOffset"),  ul,  szIniFile);	
	m_bZeroTrigAddrOffset = (ul != 0);

	ul = m_bZeroDepthAdjust ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroDepthAdjust"),  ul,  szIniFile);	
	m_bZeroDepthAdjust = (ul != 0);

	ul = m_u16TrigSensitivity;
   ul = GetCfgKeyInt(szSectionName, _T("DTriggerSensitivity"),  ul,  szIniFile);	
	if ( ul <= 255 )
		m_u16TrigSensitivity = (uInt8) ul;
	else
		m_u16TrigSensitivity = 255;

	ul = m_pCardState->u16AdcOffsetAdjust;
   ul = GetCfgKeyInt(szSectionName, _T("AdcOffsetAdjust"),  ul,  szIniFile);	
	m_pCardState->u16AdcOffsetAdjust = (uInt16) ul;
		
	ul = m_u32ExtClkStop;
   ul = GetCfgKeyInt(szSectionName, _T("ExtClkStop"),  ul,  szIniFile);	
	m_u32ExtClkStop = (uInt16) ul;

	// Calibration settings
	ul = m_bSkipCalib ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("FastDcCal"),  ul,  szIniFile);	
	m_bSkipCalib = 0 != ul;

	ul = m_bSkipTrim ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("SkipTrim"),  ul,  szIniFile);	
	m_bSkipTrim = 0 != ul;

	ul = m_u16SkipCalibDelay;
   ul = GetCfgKeyInt(szSectionName, _T("NoCalibrationDelay"),  ul,  szIniFile);	

	if(ul > 24 * 60 ) //limitted to 1 day
		ul = 24*60;
	m_u16SkipCalibDelay = (uInt16)ul;

	ul = m_bNeverCalibrateDc ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverCalibrateDc"),  ul,  szIniFile);	
	m_bNeverCalibrateDc = 0 != ul;

	ul = m_u32DebugCalibSrc;
   ul = GetCfgKeyInt(szSectionName, _T("DebugCalibSource"),  ul,  szIniFile);	
	m_u32DebugCalibSrc = ul;

	ul = m_u32DebugCalibTrace;
   ul = GetCfgKeyInt(szSectionName, _T("TraceCalib"),  ul,  szIniFile);	
	m_u32DebugCalibTrace = ul;
   
	ul = m_u8CalDacSettleDelay;
   ul = GetCfgKeyInt(szSectionName, _T("CalDacSettleDelay"),  ul,  szIniFile);	
	m_u8CalDacSettleDelay = (uInt8)ul;

	ul = m_u32IgnoreCalibErrorMask;
   ul = GetCfgKeyInt(szSectionName, _T("IgnoreCalibError"),  ul,  szIniFile);	
	m_u32IgnoreCalibErrorMask = ul;

	ul = m_bDisableUserOffet ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("DisableUserOffet"),  ul,  szIniFile);	
	m_bDisableUserOffet = (ul != 0);

	return CS_SUCCESS;
}

int32	CsSpiderDev::ShowCfgKeyInfo()   
{

   printf("---------------------------------\n");
   printf("    Driver Config Setting Infos \n");
   printf("---------------------------------\n");
   printf("    SectionName: [%s] \n", DRV_SECTION_NAME);
   printf("    List of Cfg keys supported: \n");
   printf("\t%s\n\n","NoConfigReset");
   printf("\t%s\n","ZeroTrigAddress");
   printf("\t%s\n","ZeroTrigAddrOffset");
   printf("\t%s\n","ZeroDepthAdjust");
   printf("\t%s\n","DTriggerSensitivity");
   printf("\t%s\n","ExtClkStop");
   printf("\t%s\n","FastDcCal");
   printf("\t%s\n","SkipTrim");
   printf("\t%s\n","NoCalibrationDelay");
   printf("\t%s\n","NeverCalibrateDc");
   printf("\t%s\n","DebugCalibSource");
   printf("\t%s\n","TraceCalib");
   printf("\t%s\n","CalDacSettleDelay");
   printf("\t%s\n","DisableUserOffet");

   printf("---------------------------------\n");
   
}   
#endif   
//-----------------------------------------------------------------------------
//	Convert the User channel to Hw channel
//	Ex: User Channel 2 from 4 channels card is actually channel 3 from HW point of view
//      User Channel 2 from 2 channels card is actually channel 5 from HW point of view
//-----------------------------------------------------------------------------
uInt16	CsSpiderDev::ConvertToHwChannel( uInt16 u16UserChannel )
{
	uInt16	u16HwChannel;
	uInt8	u8Skip = 1;

	switch( m_PlxData->CsBoard.NbAnalogChannel )
	{
		case 8:
			u8Skip = 1;
			break;
		case 4:
			u8Skip = 2;
			break;
		case 2:
			u8Skip = 4;
			break;
		case 1:
			u8Skip = 8;
			break;
	}

	u16HwChannel = (u16UserChannel - 1 ) * u8Skip + 1; 

	return u16HwChannel;
}

//-----------------------------------------------------------------------------
//	Convert the Hw channel to User channel
//	Ex: Hardware channel 3 from 4 channels card is actually channel 2 from user point of view
//-----------------------------------------------------------------------------
uInt16	CsSpiderDev::ConvertToUserChannel( uInt16 u16HwChannel )
{
	if ( 0 == u16HwChannel )
		return 0;

	uInt8	u8Skip(1);

	switch( m_PlxData->CsBoard.NbAnalogChannel )
	{
		case 8:
			u8Skip = 1;
			break;
		case 4:
			u8Skip = 2;
			break;
		case 2:
			u8Skip = 4;
			break;
		case 1:
			u8Skip = 8;
			break;
	}

	uInt16	u16UserChannel = (u16HwChannel - 1 ) / u8Skip + 1; 

	return u16UserChannel;
}

//-----------------------------------------------------------------------------
// HW channel skip depends only on mode
//-----------------------------------------------------------------------------
uInt16	CsSpiderDev::GetHardwareChannelSkip()
{
	uInt16	u16ChannelSkip = 0;

	if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_OCT )
		u16ChannelSkip = 1;
	else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD )
		u16ChannelSkip = 2;
	else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
		u16ChannelSkip = 4;
	else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
		u16ChannelSkip = 8;

	ASSERT( u16ChannelSkip != 0 );
	return u16ChannelSkip;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool	CsSpiderDev::IsImageValid( uInt8 u8Index )
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

int64	CsSpiderDev::GetTimeStampFrequency()
{
	int64	i64TimeStampFreq = 0;
	if ( m_bNucleon )
	{
		if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
		{
			i64TimeStampFreq = CSPDR_PCIEX_MEMORY_CLOCK;
			i64TimeStampFreq <<= 4;
		}
		else
		{
			uInt8 u8Shift = 0;
			switch(m_pCardState->AcqConfig.u32Mode & (CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE))
			{
				default:
				case CS_MODE_OCT:	u8Shift = 4; break;
				case CS_MODE_QUAD:	u8Shift = 3; break;
				case CS_MODE_DUAL:	u8Shift = 2; break;
				case CS_MODE_SINGLE:u8Shift = 1; break;
			}
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
		i64TimeStampFreq = CSPDR_MED_MEMORY_CLOCK;
		i64TimeStampFreq <<= u8Shift;
	}
	else
	{
		i64TimeStampFreq = m_pCardState->AcqConfig.i64SampleRate;
	}

	return i64TimeStampFreq;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 	CsSpiderDev::CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSizeSamples )
{
	int32 i32Status	= CS_SUCCESS;

	if ( ! m_Stream.bEnabled )
		return CS_INVALID_REQUEST;

	// Check if there is data remain
	if ( ! m_Stream.bRunForever && m_Stream.u64DataRemain == 0  )
	{
		i32Status = CS_STM_COMPLETED;
		return i32Status;
	}

	// Validation of the transfer size
	if ( m_Stream.bFifoFull )
	{
		// Fifo overflow has occured. Allow only to transfer valid data
		if ( m_Stream.i64DataInFifo > 0 )
		{
			if ( m_Stream.i64DataInFifo < u32TransferSizeSamples )
				u32TransferSizeSamples = (uInt32) m_Stream.i64DataInFifo;
		}
		else
			i32Status = CS_STM_FIFO_OVERFLOW;
	}
	else if ( ! m_Stream.bRunForever )
	{	
		if ( m_Stream.u64DataRemain < u32TransferSizeSamples )
			u32TransferSizeSamples = (uInt32) m_Stream.u64DataRemain;
	}

	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Only one DMA can be active 
	if ( ::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 1, 0 ) ) 
		return CS_INVALID_STATE;

	m_Stream.u32TransferSize = u32TransferSizeSamples;
	i32Status = IoctlStmTransferDmaBuffer( m_Stream.u32TransferSize*m_pCardState->AcqConfig.u32SampleSize, pVa );

	if ( CS_FAILED(i32Status) )
		::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 0, 1 );

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSpiderDev::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData )
{
	if ( ! m_Stream.bEnabled )
		return CS_INVALID_REQUEST;

	int32 i32Status = CsStmGetTransferStatus( u32TimeoutMs, pu32ErrorFlag );
	if (CS_SUCCESS == i32Status)
	{
		if ( ! m_Stream.bRunForever )
		{
			m_Stream.u64DataRemain -= m_Stream.u32TransferSize;
			if ( pu8EndOfData )
				*pu8EndOfData	= ( m_Stream.u64DataRemain == 0 );
		}
		if ( pu32ActualSize )
			*pu32ActualSize		= m_Stream.u32TransferSize;		// Return actual DMA data size
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSpiderDev::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag )
{
	// Check if there is any DMA active
	if ( 0 == m_Stream.u32BusyTransferCount )
		return CS_INVALID_STATE;

	// Only one Wait per DMA transfer is allowed
	if ( ::InterlockedCompareExchange( &m_Stream.u32WaitTransferCount, 1, 0 ) ) 
		return CS_INVALID_STATE;

	bool	bDmaCompleted = false;
	uInt32	u32LocalErrorFlag = 0;
	int32 i32Status =  IoctlStmWaitDmaComplete( u32TimeoutMs, &u32LocalErrorFlag, &bDmaCompleted );

	if ( CS_SUCCESS == i32Status )
	{
		if ( pu32ErrorFlag )
			*pu32ErrorFlag = u32LocalErrorFlag;

		if ( ! m_Stream.bFifoFull )
		{	
			if ( u32LocalErrorFlag  & STM_TRANSFER_ERROR_FIFOFULL )
			{
				// Fifo full has occured. From this point all data in memory is still valid
				// We notify the application about the state of FIFO full only when all valid data are upload
				// to application
				m_Stream.bFifoFull = true;
				if ( m_Stream.bRunForever || m_Stream.u64DataRemain > (ULONGLONG) m_PlxData->CsBoard.i64MaxMemory )
					m_Stream.i64DataInFifo = m_PlxData->CsBoard.i64MaxMemory;
				else
					m_Stream.i64DataInFifo = m_Stream.u64DataRemain;

				m_Stream.i64DataInFifo -= m_Stream.u32TransferSize;
			}
		}
		else
		{
			// Fifo full has occured. Return error if there is no more valid data to transfer.
			if ( m_Stream.i64DataInFifo > 0 )
				m_Stream.i64DataInFifo -= m_Stream.u32TransferSize;
		}
	}
	
	// The DMA request may completed with success or with error
	if (  bDmaCompleted )
		::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 0, 1 );

	::InterlockedCompareExchange( &m_Stream.u32WaitTransferCount, 0, 1 );

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void 	CsSpiderDev::MsStmResetParams()
{
	CsSpiderDev *pDev = m_MasterIndependent;
	
	while( pDev )
	{
		PSTREAMSTRUCT pStreamStr = &pDev->m_Stream;

		pStreamStr->bFifoFull			 = false;
		pStreamStr->u32BusyTransferCount = 0;
		pStreamStr->u32WaitTransferCount = 0;
		pStreamStr->u64DataRemain		 = pStreamStr->u64TotalDataSize/m_pCardState->AcqConfig.u32SampleSize;

		pDev = pDev->m_Next;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsSpiderDev::IsConfigResetRequired( uInt8 u8ExpertImageId )
{
	if ( m_pCardState->u8ImageId  == u8ExpertImageId )
		return false;
	// For Nucleon PCIe, Streaming is supported by the default firmware. We do not have to change to the expert firmware for streaming 
	// unless the expert firmware version is different to the default one.
	// For other expert, switching to expert firmware is required.
	if ( ( m_PlxData->CsBoard.u32BaseBoardOptions[u8ExpertImageId] & CS_BBOPTIONS_STREAM ) == 0 )
		return true;
	else 
		return false; //( m_PlxData->CsBoard.u32UserFwVersionB[0].u32Reg != m_PlxData->CsBoard.u32UserFwVersionB[u8ExpertImageId].u32Reg );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSpiderDev::MsInitializeDevIoCtl()
{
	int32		 i32Status = CS_SUCCESS;
	CsSpiderDev* pDevice = m_MasterIndependent;

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

bool CsSpiderDev::IsExpertAVG(uInt8 u8ImageId)
{
	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & CS_BBOPTIONS_MULREC_AVG_TD) != 0);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsSpiderDev::IsExpertSTREAM(uInt8 u8ImageId)
{
	if ( 0 == u8ImageId )
		u8ImageId = m_pCardState->u8ImageId;

	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & CS_BBOPTIONS_STREAM) != 0);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsSpiderDev::PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg )
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


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsSpiderDev::ReadConfigBootLocation()
{
	uInt32	u32Val = 0;

	//int32	i32Status = IoctlReadFlash( (0x1FFFFFE<<1), &u32Val, sizeof(u32Val) );
	int32	i32Status = IoctlReadFlash( (0x1FFFFFE<<1), &u32Val, sizeof(u32Val) );
	if ( CS_FAILED(i32Status) )
		return 0;
	
	return u32Val;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSpiderDev::WriteConfigBootLocation( uInt32	u32Location )
{
	char	szText[256];

	uInt32 Test = (0x1FFFFFE<<1);
	Test = 0x2000000-0x4;
  
	int32 i32Status = NucleonWriteFlash( Test, &u32Location, sizeof(u32Location) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		uInt32	u32Val = 0xFFFF;
		i32Status = IoctlReadFlash( Test, &u32Val, sizeof(u32Val) );
		if ( CS_SUCCEEDED(i32Status) )
		{
			if ( u32Location == u32Val )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Boot location updated successfully 0x%x."), u32Location );
			}
			else
				sprintf_s( szText, sizeof(szText), TEXT("Boot location Read/Write Discrepancy."));
		}
	}
	else
		sprintf_s( szText, sizeof(szText), TEXT("Boot location updated failed (WriteFlash error)."));


	CsEventLog EventLog;
	EventLog.Report( CS_INFO_TEXT, szText );

	return i32Status;
}