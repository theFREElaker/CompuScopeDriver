

#include "StdAfx.h"
#include "CsTypes.h"
#include "CsIoctl.h"
#include "CsDrvDefines.h"
#include "CsRabbit.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif
#include "xml.h"


extern SHAREDDLLMEM Globals;

#define CSPRD_DAC_SETTLE_DELAY  5
#define	NO_CONFIG_BOOTCPLD			0x17


int32	CsRabbitDev::DeviceInitialize(void)
{
	int32 i32Ret = CS_SUCCESS;
#if !defined(USE_SHARED_CARDSTATE)
      IoctlGetCardState(m_pCardState );
      m_pCardState->u16CardNumber = 1;
#endif 
   
	i32Ret = CsPlxBase::Initialized();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	// Assume the pulse for calibration is present
	m_bPulseCalib = TRUE;
#ifdef _WINDOWS_
	ReadCommonRegistryOnBoardInit();
#else
   ReadCommonCfgOnBoardInit();               // Use .ini config file
#endif
   
#ifdef USE_SHARED_CARDSTATE
	if ( m_pCardState->bCardStateValid )
	{
		if (  m_pCardState->bAddOnRegistersInitialized )
		{
			if( m_pCardState->bHighBandwidth )
			{
				// Disable all calibrations and use Calib Table from Eeprom
				m_bNeverCalibrateDc			= true;
				m_bNeverCalibrateMs			=
				m_bNeverCalibExtTrig		= TRUE;
			}
			return CS_SUCCESS;
		}
	}
	else if ( 0 != m_pCardState->u8ImageId  )
		CsLoadBaseBoardFpgaImage( 0 );
#else
		CSRABBIT_CARDSTATE			tempCardState = {0};
		i32Ret = IoctlGetCardState(&tempCardState);
		if ( CS_FAILED(i32Ret) )
			return i32Ret;

//      fprintf(stdout," CsRabbitDev::DeviceInitialize bCardStateValid %d \n", tempCardState.bCardStateValid);
      
		if ( tempCardState.bCardStateValid )
		{
			GageCopyMemoryX(m_pCardState, &tempCardState,  sizeof(CSRABBIT_CARDSTATE));
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

	m_pCardState->bAddOnRegistersInitialized = false;
	m_pCardState->u8MsTriggMode		= CSRB_DEFAULT_MSTRIGMODE;
	m_pCardState->u8VerifyTrigOffsetModeIx = (uInt8)-1;
	m_pCardState->u32VerifyTrigOffsetRateIx = (uInt32)-1;
	SetDefaultAuxIoCfg();

	ResetCachedState();
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
			if ( 0 == ConfigurationReset(0) )
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
		// Read Baseboard HW info
		PLX_NVRAM_IMAGE	nvRAM_Image;
		int32 i32Status = CsReadPlxNvRAM( &nvRAM_Image );
		if ( CS_FAILED( i32Status ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Error while reading NVRAM." );
			return i32Status;
		}

		GetBoardNode()->u32BaseBoardVersion = nvRAM_Image.Data.BaseBoardVersion;
		GetBoardNode()->u32SerNum = nvRAM_Image.Data.BaseBoardSerialNumber;
		GetBoardNode()->u32BaseBoardHardwareOptions = nvRAM_Image.Data.BaseBoardHardwareOptions;
	}

	//Assumed. May otherwise be unitialized when read
	GetBoardNode()->u32BoardType = 0;

	//Assumed. May otherwise be unitialized when read and cause crashes.
	GetBoardNode()->u32BaseBoardOptions[0] = 0;

	if( IsNiosInit() )
	{
		ReadBaseBoardHardwareInfoFromFlash(FALSE);
		if ( m_bNucleon )
			NucleonGetMemorySize();
		else
			UpdateMaxMemorySize( GetBoardNode()->u32BaseBoardHardwareOptions, CSRB_DEFAULT_SAMPLE_SIZE );

		ClearInterrupts();
		GetInitStatus()->Info.u32AddOnPresent = IsAddonBoardPresent();
		if ( GetInitStatus()->Info.u32AddOnPresent )
		{
			ReadAddOnHardwareInfoFromEeprom(FALSE);

			AssignBoardType();
			InitBoardCaps();

			int32 i32Status = InitAddOnRegisters();
			if ( CS_FAILED (i32Status) )
			{
#ifdef _WINDOWS_            
				// Fail to init the addon board.
				// Possible because of invalid firmwares. Attempt to update firmware later
				CheckRequiredFirmwareVersion();
				if( 0 == m_PlxData->CsBoard.u8BadBbFirmware && 0 == m_PlxData->CsBoard.u8BadAddonFirmware )
				{
					CsEventLog EventLog;
					EventLog.Report( CS_ERROR_TEXT, "Fail to initialize Addon board." );
				}
#else
            fprintf(stdout, "InitAddOnRegisters() failed!  Status = %d \n", i32Status);
#endif   
			}
		}
		else
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Addon board is not connected." );
		}
	}
	else
	{
		// Nios error. This may caused by bad base board firmware
		// Set Flag to force update firmware later
		m_PlxData->CsBoard.u8BadBbFirmware = BAD_DEFAULT_FW;
	}

	// Reset the firmware version variables before reading firmware version
	RtlZeroMemory( &m_pCardState->VerInfo, sizeof(m_pCardState->VerInfo) );
	m_pCardState->VerInfo.u32Size = sizeof(CS_FW_VER_INFO);
	RtlZeroMemory( GetBoardNode()->u32RawFwVersionB, sizeof(GetBoardNode()->u32RawFwVersionB) );
	RtlZeroMemory( GetBoardNode()->u32RawFwVersionA, sizeof(GetBoardNode()->u32RawFwVersionA) );
	
	// Read Firmware versions
	ReadVersionInfo();
	ReadVersionInfoEx();
	
#ifdef _WINDOWS_
	ReadAndValidateCsiFile( GetBoardNode()->u32BaseBoardHardwareOptions, GetBoardNode()->u32AddonHardwareOptions );
#endif
	if ( m_bCobraMaxPCIe && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
		CobraMaxSonoscanCalibOnStartup();

	if( m_pCardState->bHighBandwidth )
	{
		// Disable all calibrations and use Calib Table from Eeprom
		m_bNeverCalibrateDc			= true;
		m_bNeverCalibrateMs			=
		m_bNeverCalibExtTrig		= TRUE;
	}

    UpdateCardState();

	// Intentional return success to allows some utilitiy programs to update firmware in case of errors
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::MsCsResetDefaultParams( BOOLEAN bReset )
{
	int32		i32Ret = CS_SUCCESS;
	CsRabbitDev *pDevice = m_MasterIndependent;

	MsHistogramActivate( false );
	if ( bReset )
	{
		while( pDevice )
		{
			// Clear all cache state and force reconfig the hardware
			pDevice->ResetCachedState();
			pDevice->SendMasterSlaveSetting();
			pDevice = pDevice->m_Next;
		}
	}

	// Set Default configuration
	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		pDevice->m_u16DebugMsPulse = 0;
		pDevice->m_bMsOffsetAdjust = TRUE;
		pDevice->m_u16DebugExtTrigCalib = 0;

		if( pDevice != m_pTriggerMaster )
		{
			i32Ret = pDevice->CsSetAcquisitionConfig();
			if( CS_FAILED(i32Ret) )
				return i32Ret;
		}

		pDevice->SetDefaultTrigAddrOffset();
		pDevice = pDevice->m_Next;
	}

	i32Ret = m_pTriggerMaster->CsSetAcquisitionConfig();
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( m_bCobraMaxPCIe )
	{
		i32Ret = SendInternalHwExtTrigAlign();
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		RtlZeroMemory( pDevice->m_CfgTrigParams, sizeof( m_CfgTrigParams ) );

		// Restore Calibration DacInfo in case they have been altered by calibration program
		pDevice->RestoreCalibDacInfo();

		if(!pDevice->GetInitStatus()->Info.u32Nios )
			return CS_FLASH_NOT_INIT;

		i32Ret = pDevice->InitAddOnRegisters(bReset==TRUE);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = pDevice->CsSetChannelConfig();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = pDevice->CsSetTriggerConfig();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		pDevice = pDevice->m_Next;
	}

	if ( m_pCardState->bLAB11G )
	{
		// After initialization as DUAL channel mode we havr to set back as SINGLE channel mode
		CSACQUISITIONCONFIG CalibAcqConfig;

		CalibAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
		CsGetAcquisitionConfig( &CalibAcqConfig );

		CalibAcqConfig.u32Mode = CS_MODE_SINGLE;

		pDevice = m_MasterIndependent;
		// Set Default configuration
		while( pDevice )
		{
			if( pDevice != m_pTriggerMaster )
			{
				i32Ret = pDevice->CsSetAcquisitionConfig( &CalibAcqConfig );
				if( CS_FAILED(i32Ret) )
					return i32Ret;
			}
			pDevice->SetDefaultTrigAddrOffset();
			pDevice = pDevice->m_Next;
		}

		i32Ret = m_pTriggerMaster->CsSetAcquisitionConfig( &CalibAcqConfig );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	if ( NULL != m_MasterIndependent->m_Next )
	{
		int32	i32MsCalResult = CS_SUCCESS;
		
		// Calibration for M/S skew
		if ( m_bNucleon )
		{
			i32MsCalResult = MsCalibrateSkew(false);
			if ( CS_SUCCEEDED(i32MsCalResult) )
				MsAdjustDigitalDelay();
		}
		else
			i32MsCalResult = CalibrateMasterSlave();
		if ( CS_FAILED (i32MsCalResult)  )
		{
			m_MasterIndependent->m_pCardState->bMasterSlaveCalib = true;
			CsEventLog EventLog;
			EventLog.Report( CS_SYSTEMINIT_MSCALIB_ERROR, "" );
		}
	}

	i32Ret = VerifyTriggerOffset();

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		pDevice->UpdateCardState();
		pDevice = pDevice->m_Next;
	}

	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN	CsRabbitDev::IsCalibrationRequired()
{
	if ( GetBoardNode()->NbAnalogChannel > 1 )
		return ( m_pCardState->bCalNeeded[0] || m_pCardState->bCalNeeded[1] || m_bDebugCalibSrc );
	else
		return ( m_pCardState->bCalNeeded[0] || m_bDebugCalibSrc );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::AcquireData(uInt32 u32IntrEnabled)
{
	// Send startacquisition command
	in_STARTACQUISITION_PARAMS AcqParams = {0};

	// If Stream mode is enable, we can send start acquisition in either STREAM mode or REGULAR mode .
	// If we are in calibration mode then we have to capture data in REGULAR mode.
	// Use the u32Params1 to let the kernel driver know what mode we want to capture.
	if ( m_Stream.bEnabled && ! m_bCalibActive )
 		AcqParams.u32Param1 = 1;					

	AcqParams.u32IntrMask = u32IntrEnabled;
	int32 i32Status = IoctlDrvStartAcquisition( &AcqParams );

	if ( CS_SUCCEEDED(i32Status) )
	{
		if ( ecmCalModeDc != m_CalibMode && ecmCalModeNormal != m_CalibMode )
			GeneratePulseCalib();
	}

	return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsRabbitDev::MsAbort()
{
	CsRabbitDev* pCard = m_MasterIndependent;

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
//----- CsRabbitDev::UpdateSystemInfo(PSYSTEM_NODE pSys)
//-----------------------------------------------------------------------------

void	CsRabbitDev::UpdateSystemInfo( CSSYSTEMINFO *SysInfo )
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
	SysInfo->u32BaseBoardOptions	= pBdParam->u32BaseBoardOptions[0];
	SysInfo->u32AddonOptions		= pBdParam->u32ActiveAddonOptions;

	if ( m_pCardState->bLAB11G ) 
		SysInfo->u32ChannelCount = SysInfo->u32BoardCount;	// 1 channel per cards
	else
		SysInfo->u32ChannelCount = pBdParam->NbAnalogChannel * SysInfo->u32BoardCount;

	AssignAcqSystemBoardName( SysInfo );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsRabbitDev::AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo )
{
	if ( m_bNucleon )
		strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"CSE" );
	else
		strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"CS" );

	switch ( pSysInfo->u32BoardType & ~CSNUCLEONBASE_BOARDTYPE )
	{
	case CSX24G8_BOARDTYPE:
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"24G8" );
		break;

	case CSX14G8_BOARDTYPE:
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"14G8" );
		break;

	case CS21G8_BOARDTYPE:
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"21G8" );
		break;

	case CS11G8_BOARDTYPE:
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"11G8" );
		break;

	case LAB11G_BOARDTYPE:
		if ( m_bNucleon )
			strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName),(char*)"LAB11eX" );
		else
			strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"LAB11G" );
		break;

	default:
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"22G8" );
		break;
	}

	if (m_bNucleon)
	{
		if( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_UTEXPE) )	
			strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"-STP" );
		else if( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) )	
			strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"-RFS" );
	}
	else if ( m_pCardState->bHighBandwidth )
		strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char *)"-1GHz" );

	if ( pSysInfo->u32BoardCount > 1 )
	{
		char tmp[32];
		sprintf_s( tmp, sizeof(tmp), " M/S - %d", pSysInfo->u32BoardCount );
		strcat_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), tmp );
	}
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsRabbitDev::ReadCalibTableFromEeprom(uInt8	u8CalibTableId)
{
	uInt32	u32Addr;
	uInt32	u32EepromSize;


	switch ( u8CalibTableId )
	{
		case 2:
			u32Addr = FIELD_OFFSET( RBT_ADDONFLASH_LAYOUT, FlashImage2 );
			break;
		default:
			u32Addr = FIELD_OFFSET( RBT_ADDONFLASH_LAYOUT, FlashImage1 );
			break;
	}

	u32Addr += FIELD_OFFSET( RBT_ADDONFLASH_IMAGE_STRUCT, Calib );
	u32EepromSize = sizeof((((RBT_ADDONFLASH_IMAGE_STRUCT *)0)->Calib));

	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  CSRB_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....

	uInt32 u32CalibTableSize = sizeof(m_pCardState->CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	int32 i32Status = ReadEeprom( &m_pCardState->CalibInfoTable, u32Addr, u32CalibTableSize );

	if ( CS_FAILED( i32Status ) || (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
		  BUNNYCALIBINFO_CHECKSUM_VALID != m_pCardState->CalibInfoTable.u32Checksum )
	{
		m_pCardState->CalibInfoTable.u32Active = 0;
		if( m_pCardState->bHighBandwidth )
		{
//			SetDefaultCalibTableValues();
			GageCopyMemoryX( &m_pCardState->DacInfo, &m_pCardState->CalibInfoTable.ChanCalibInfo, sizeof(m_pCardState->DacInfo) );
		}
	}
	else
	{
		if ( -1 == m_pCardState->CalibInfoTable.u32Active )
			m_pCardState->CalibInfoTable.u32Active = 0;

		m_pCardState->CalibInfoTable.u32Active &= m_pCardState->CalibInfoTable.u32Valid;
		m_pCardState->bUseDacCalibFromEEprom = (m_pCardState->CalibInfoTable.u32Active & BUNNY_CAL_DAC_VALID) ? true : false;

		if ( m_pCardState->bUseDacCalibFromEEprom )
		{
			char		szBoardId[] = "0";

			szBoardId[0] += m_u8CardIndex;
			CsEventLog	EventLog;
			EventLog.Report( CS_CALIBTABLE_ENABLED, szBoardId );

			GageCopyMemoryX( &m_pCardState->DacInfo, &m_pCardState->CalibInfoTable.ChanCalibInfo, sizeof(m_pCardState->DacInfo) );
		}
	}

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsRabbitDev::WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid )
{
	uInt32	u32Addr;
	uInt32	u32EepromSize;

	switch ( u8CalibTableId )
	{
		case 2:
			u32Addr = FIELD_OFFSET( RBT_ADDONFLASH_LAYOUT, FlashImage2 );
			break;
		default:
			u32Addr = FIELD_OFFSET( RBT_ADDONFLASH_LAYOUT, FlashImage1 );
			break;
	}

	u32Addr += FIELD_OFFSET( RBT_ADDONFLASH_IMAGE_STRUCT, Calib );
	u32EepromSize = sizeof((((RBT_ADDONFLASH_IMAGE_STRUCT *)0)->Calib));

	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  CSRB_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....

	uInt32 u32CalibTableSize = sizeof(m_pCardState->CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	// Update the Calib Table with all latest Dac values
	//GageCopyMemoryX( &m_pCardState->CalibInfoTable.ChanCalibInfo, &m_pCardState->DacInfo, sizeof(m_pCardState->DacInfo) );

	m_pCardState->CalibInfoTable.u32Checksum = BUNNYCALIBINFO_CHECKSUM_VALID;
	uInt32 u32InvalidMask = m_pCardState->CalibInfoTable.u32Valid & (BUNNY_VALID_MASK >> 1 );
	m_pCardState->CalibInfoTable.u32Valid &= ~(u32InvalidMask << 1);

	//set valid bits from input
	m_pCardState->CalibInfoTable.u32Valid |= u32Valid & BUNNY_VALID_MASK;

	//clear valid bits from input
	m_pCardState->CalibInfoTable.u32Valid &= ~((u32Valid << 1) & BUNNY_VALID_MASK);


	return WriteEepromEx( &m_pCardState->CalibInfoTable, u32Addr, sizeof(m_pCardState->CalibInfoTable) );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt8	CsRabbitDev::ReadBaseBoardCpldVersion()
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

void CsRabbitDev::ReadVersionInfo()
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

	if ( m_pCardState->bAddOnRegistersInitialized && 0 == m_pCardState->VerInfo.AddonFpgaFwInfo.u32Reg )
	{
		// Cpld Version
		FPGABASEBOARD_OLDFWVER		BbFpgaInfo;
		PCS_FW_VER_INFO				pFwVerInfo = &m_pCardState->VerInfo;


		BbFpgaInfo.u32Reg = ReadGIO(CSRB_ADDON_FDV);
		m_pCardState->VerInfo.ConfCpldInfo.u32Reg = 0;

		pFwVerInfo->ConfCpldInfo.Info.uYear		= BbFpgaInfo.Info.uYear;
		pFwVerInfo->ConfCpldInfo.Info.uMonth	= BbFpgaInfo.Info.uMonth;
		pFwVerInfo->ConfCpldInfo.Info.uDay		= BbFpgaInfo.Info.uDay;
		pFwVerInfo->ConfCpldInfo.Info.uMajor	= BbFpgaInfo.Info.uMajor;
		pFwVerInfo->ConfCpldInfo.Info.uMinor	= BbFpgaInfo.Info.uMinor;
		pFwVerInfo->ConfCpldInfo.Info.uRev		= BbFpgaInfo.Info.uRev;

		// Add-on firmware date and version
		uInt32 u32Data = 0x7FFF & ReadRegisterFromNios(0,CSRB_HWR_RD_CMD);
		// firmware date = 0 because Rabit has no longer firmware date
		u32Data <<= 16;
		u32Data |= (0x7FFF & ReadRegisterFromNios(0,CSRB_SWR_RD_CMD));

		pFwVerInfo->AddonFpgaFwInfo.u32Reg = GetBoardNode()->u32RawFwVersionA[0] = u32Data;
		if ( m_bNucleon )
			pFwVerInfo->u32AddonFpgaOptions = 0xFF & ReadRegisterFromNios(0, CSRB_OPT_RD_CMD);
		else
			pFwVerInfo->u32AddonFpgaOptions = 0;

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

void CsRabbitDev::ReadVersionInfoEx()
{
	BOOLEAN			bConfigReset = FALSE;
	FPGA_FWVERSION	RawVerInfo;

	// Reading Expert firmware versions
	if ( GetInitStatus()->Info.u32Nios )
	{
		for (uInt8 i = 1; i <= 2; i++ )
		{
			if ( 0 != GetBoardNode()->u32BaseBoardOptions[i] )
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
							{
								GetBoardNode()->u32UserFwVersionB[i].u32Reg = 0;
								RawVerInfo.u32Reg = 0;
							}
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

						RawVerInfo.u32Reg = GetBoardNode()->u32RawFwVersionB[i] = (0x7FFF7FFF & ReadGageRegister( FIRMWARE_VERSION ));

						GetBoardNode()->u32UserFwVersionB[i].Version.uRev		= 0x7FFF & ReadGageRegister( FIRMWARE_OPTIONS );
						GetBoardNode()->u32UserFwVersionB[i].Version.uIncRev	= RawVerInfo.Version.uIncRev;
					}

					GetBoardNode()->u32UserFwVersionB[i].Version.uMajor	= RawVerInfo.Version.uMajor;
					GetBoardNode()->u32UserFwVersionB[i].Version.uMinor	= RawVerInfo.Version.uMinor;

					bConfigReset = TRUE;
				}
			}
			else
				GetBoardNode()->u32UserFwVersionB[i].u32Reg = GetBoardNode()->u32RawFwVersionB[i] = 0;
		}

		if ( bConfigReset )
		{
			// If ConfigReset has been called, clear all cache in order to force re-configuration and re-calibrate
			if ( m_bNucleon )
				ConfigurationReset(0);
			else
			{
				BaseBoardConfigReset( FIELD_OFFSET( BaseBoardFlashData, DefaultImage ) );
		
				// Patch for problem with M/S startup
				Sleep(200);
			}

			// ConfigReset will reset the clock & adc setting.
			// Force to reconfig the clock & adc 
			ResetCachedState();
			InitAddOnRegisters(true);
		}
	}
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	CsRabbitDev::AdjustForOneSampleResolution( uInt32 u32Mode, int64 *i64SegmentSize, int64 *i64Depth, int64 *i64TrigDelay )
{
	uInt32	u32ChannelMode = u32Mode & CS_MASKED_MODE;
	uInt8	u8Shift = 4;	

	switch(u32ChannelMode)
	{
	case CS_MODE_DUAL:
		u8Shift = 3;
		break;
	case CS_MODE_SINGLE:
		u8Shift = 4;
		break;
	}

	m_u32SegmentResolution = (1 << u8Shift);
	if ( m_bNucleon && m_bOneSampleResolution )
	{
		int64	i64StartPosition = *i64SegmentSize - *i64Depth - *i64TrigDelay;

		if ( 0 != (*i64Depth % m_u32SegmentResolution) || 0 != (*i64TrigDelay % m_u32SegmentResolution)  )
		{
			m_i32OneSampleResAdjust    = (int32) ( (i64StartPosition % ( (uInt32)(1 <<  u8Shift) )));

			// Round up the post triggerr depth
			if ( 0 != (*i64Depth % m_u32SegmentResolution) )
			{
				*i64Depth	+= m_u32SegmentResolution;
				*i64Depth	/= m_u32SegmentResolution;
				*i64Depth	*= m_u32SegmentResolution;
			}

			// Round down the trigger delay
			if ( 0 != (*i64TrigDelay % m_u32SegmentResolution) )
				*i64TrigDelay	-= (*i64TrigDelay % m_u32SegmentResolution);
		}
		else
			m_i32OneSampleResAdjust = 0;		
	}

	m_i64HwPostDepth	= *i64Depth;
	m_i64HwTriggerDelay	= *i64TrigDelay;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	CsRabbitDev::AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32 *u32SegmentCount, int64 *i64SegmentSize, int64 *i64Depth )
{
	// Extras pre trigger required be caud of negative trigger address offset
	uInt32 u32ExtraPreTrig = (m_bZeroDepthAdjust || m_bNucleon ) ? 0 : 16;

	if ( !m_bHardwareAveraging )
		u32ExtraPreTrig /= (u32Mode & CS_MODE_DUAL) ? 2 : 1;

	m_u32TailExtraData = CsGetPostTriggerExtraSamples( u32Mode );

	*i64SegmentSize = *i64SegmentSize + m_u32TailExtraData + m_u32TailExtraData;

	if ( m_bNucleon )
		*i64Depth = *i64Depth + m_u32TailExtraData;
	else
		*i64Depth = *i64Depth + m_pCardState->AcqConfig.i64TriggerDelay + m_u32TailExtraData;

	m_u32HwSegmentCount = *u32SegmentCount;
	m_i64HwSegmentSize = *i64SegmentSize;
	m_i64HwPostDepth = *i64Depth;
	m_i64HwTriggerHoldoff = m_pCardState->AcqConfig.i64TriggerHoldoff + u32ExtraPreTrig;
}

//--------------------------------------------------------------------------------------------------
//	Internal extra samples to be added in the post trigger depth and segment size in order to
// 	ensure post trigger depth.
//  Withou ensure post trigger depth, the lost is because of trigger address offset and invalid
//  samples at the end of each record.
//---------------------------------------------------------------------------------------------------

uInt32	CsRabbitDev::CsGetPostTriggerExtraSamples( uInt32 u32Mode )
{
	if ( m_bZeroDepthAdjust || m_bNucleon )
		return 0;

	uInt32	u32TailExtraData = 0;
	if ( m_bHardwareAveraging )
	{
		u32TailExtraData = AV_ENSURE_POSTTRIG;
		if ( u32Mode & CS_MODE_SINGLE )
			u32TailExtraData <<= 1;
	}
	else
	{

		if ( u32Mode & CS_MODE_SINGLE )
			u32TailExtraData = CSRB_MAX_TRIGGER_ADDRESS_OFFSET + CSRB_MAX_TAIL_INVALID_SAMPLES_SINGLE;
		else
			u32TailExtraData = CSRB_MAX_TRIGGER_ADDRESS_OFFSET + CSRB_MAX_TAIL_INVALID_SAMPLES_DUAL;
	}

	return u32TailExtraData;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	CsRabbitDev::GetSegmentPadding(uInt32 u32Mode)
{
	uInt32	u32RealSegmentCount = 1;
	int64	i64RealSegmentSize;
	int64	i64SegmentSize;
	int64	i64RealPostDepth;

	// Calculate the segment padding (in samples)
	i64SegmentSize		=
	i64RealSegmentSize	=
	i64RealPostDepth	= CSRB_DEFAULT_DEPTH;
	AdjustedHwSegmentParameters( u32Mode, &u32RealSegmentCount, &i64RealSegmentSize, &i64RealPostDepth );

	return (uInt32) (i64RealSegmentSize - i64SegmentSize );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsRabbitDev::FindFeIndex( const uInt32 u32Swing, uInt32& u32Index )
{
	for( uInt32 i = 0; i < m_pCardState->u32FeInfoSize; i++ )
	{
		if ( u32Swing == m_pCardState->FeInfoTable[i].u32Swing_mV )
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

int8	CsRabbitDev::GetTriggerAddressOffsetAdjust( uInt32 u32SampleRate )
{
	int8	i8TrigAddr = 0;

	UNREFERENCED_PARAMETER(u32SampleRate);

	if ( m_bZeroTrigAddrOffset )
		return 0;

	if ( m_bNucleon )
		return 0;
	else
	{
		if ( 0 != m_pCardState->AcqConfig.u32ExtClk )
			return GetExtClkTriggerAddressOffsetAdjust();

		uInt8	u8IndexMode = ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0) ? 0 : 1;
		uInt32	u32IndexSR = 0;
		FindSRIndex( m_pCardState->AcqConfig.i64SampleRate, &u32IndexSR );

		if ( m_bHardwareAveraging && (m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE) != 0 )
			i8TrigAddr = m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[u32IndexSR][u8IndexMode] - 4;
		else
			i8TrigAddr = m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[u32IndexSR][u8IndexMode];

		return i8TrigAddr;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsRabbitDev::BuildMasterSlaveSystem( CsRabbitDev** ppFirstCard )
{
	CsRabbitDev*	pCurUnCfg	= (CsRabbitDev*) m_ProcessGlblVars->pFirstUnCfg;
	CsRabbitDev*	pMaster = NULL;

	//Look for the Master
	while( NULL != pCurUnCfg )
	{
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
	CsRabbitDev* pCard = pMaster;

	LONGLONG llMaxMem = pMaster->GetBoardNode()->i64MaxMemory;

	while ( pCard )
	{
		//remove card from unconfigured list
		pCard->RemoveDeviceFromUnCfgList();

		pCard->m_pTriggerMaster = pMaster;
		pCard->m_Next = pCard->m_pNextInTheChain;
		pCard->m_MasterIndependent = pMaster;

		pCard->m_pCardState->u16CardNumber = ++u16CardInSystem;

		// The smallest memory between card will be the memory available for CompuScope system
		llMaxMem = llMaxMem < pCard->GetBoardNode()->i64MaxMemory ? llMaxMem : pCard->GetBoardNode()->i64MaxMemory;

		pCard = pCard->m_pNextInTheChain;
	}

	if( 0 == llMaxMem )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "Zero memory" );
		return 0;
	}

	bool bRemoveSystem = false;

	// Check for Master/Slave discrepancy
	pCard = pMaster;
	while ( pCard )
	{
		if ( pMaster->GetBoardNode()->u32BoardType != pCard->GetBoardNode()->u32BoardType )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Master/Slave descrepancy." );
			bRemoveSystem = true;
			break;
		}

		if ( CS_FAILED( pCard->CheckMasterSlaveFirmwareVersion() ) )
		{
			pCard->AddonConfigReset(0);
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
			pCard->GetBoardNode()->i64MaxMemory = llMaxMem;
			pCard->UpdateCardState();
			pCard->SendMasterSlaveSetting();

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
#ifdef _RABBIT_DRV_
		sprintf_s( szText, sizeof(szText), TEXT("Cobra Master/Slave system of %d boards initialized"), u16CardInSystem);
#else
		sprintf_s( szText, sizeof(szText), TEXT("CobraMax Master/Slave system of %d boards initialized"), u16CardInSystem);
#endif
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_TEXT, szText );
	}

	return u16CardInSystem;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsRabbitDev::BuildIndependantSystem( CsRabbitDev** FirstCard, BOOLEAN bForceInd )
{
	CsRabbitDev*	pCard	= (CsRabbitDev*) m_ProcessGlblVars->pFirstUnCfg;
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
			if( !pCard->GetInitStatus()->Info.u32BbMemTest )
			{
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Zero memory" );
				return 0;
			}

			if (m_PlxData->InitStatus.Info.u32Nios && m_pCardState->bAddOnRegistersInitialized )
				pCard->AddonReset();
			else
			{
				return 0;
			}

			if ( (m_PlxData->CsBoard.u8BadAddonFirmware != 0)  || (m_PlxData->CsBoard.u8BadBbFirmware != 0) )
			{
				pCard->AddonConfigReset();
			}
			else
			{
				bRemoveSystem = false;
			}

			if(!bRemoveSystem)
			{
				char	szText[128];
#ifdef _RABBIT_DRV_
				sprintf_s( szText, sizeof(szText), TEXT("Cobra System %x initialized"), pCard->GetBoardNode()->u32SerNum);
#else
				sprintf_s( szText, sizeof(szText), TEXT("CobraMax System %x initialized"), pCard->GetBoardNode()->u32SerNum);
#endif
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
			pCard->SendMasterSlaveSetting();
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

int32	CsRabbitDev::RemoveDeviceFromUnCfgList()
{
	CsRabbitDev	*Current	= (CsRabbitDev	*) m_ProcessGlblVars->pFirstUnCfg;
	CsRabbitDev	*Prev = NULL;

	if ( this  == (CsRabbitDev	*) m_ProcessGlblVars->pFirstUnCfg )
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

	if ( this  == (CsRabbitDev	*) m_ProcessGlblVars->pLastUnCfg )
	{
		if (Prev)
			Prev->m_Next = NULL;
		m_ProcessGlblVars->pLastUnCfg = (PVOID) Prev;
	}

	return CS_SUCCESS;

}

#define EEPROM_OPTIONS_POSITION_CONTROL	0x10000000L
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt16	CsRabbitDev::DetectAcquisitionSystems( BOOL bForceInd )
{
	CsRabbitDev	*pDevice = (CsRabbitDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsRabbitDev	*pFirstCardInSystem[20];

	while( pDevice )
	{
		// Undo M/S link
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
	CsRabbitDev		*FirstCard = NULL;
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
		// There is no system detected.
		return 0;
	}

	m_ProcessGlblVars->NbSystem = (uInt16) i;

	// Set default Acquisition Systems configuration
	int32	j = 0;


	for (i = 0; i< m_ProcessGlblVars->NbSystem; i ++ )
	{
		pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];

		CsRabbitDev *pDevice = (CsRabbitDev *) pFirstCardInSystem[i];
		j = 0;

		memset(pSystem->u8CardIndexList, 0, sizeof(pSystem->u8CardIndexList));
		while( pDevice )
		{
			// Set M/S card index list
			pSystem->u8CardIndexList[j++] = pDevice->m_u8CardIndex;

			pDevice->m_pSystem = pSystem;
			pDevice = pDevice->m_Next;
		}

		pDevice = (CsRabbitDev *) pFirstCardInSystem[i];
		SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		SysInfo->u32AddonOptions |= EEPROM_OPTIONS_POSITION_CONTROL;
		pSystem->u16AcqStatus = ACQ_STATUS_READY;
	}

	return m_ProcessGlblVars->NbSystem;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsRabbitDev::SetDefaultTrigAddrOffset(void)
{
	if ( m_CalFileTable.u32Valid & BUNNY_CAL_TRIG_ADDR_VALID )
	{
		// Found some valid values in Cal file, use these value as default
		GageCopyMemoryX( m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust,
						  m_CalFileTable.i8TrigAddrOffsetAdjust,
						  sizeof(m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust));
		return;
	}

	if( ( 0 != (m_pCardState->CalibInfoTable.u32Valid & BUNNY_CAL_TRIG_ADDR_VALID) ) &&
		( 0 == (m_pCardState->CalibInfoTable.u32Valid & BUNNY_CAL_TRIG_ADDR_INVALID) ) )
		return;

	// In worst case, if Calibration file is not found, we use some hard coded default values
	for( uInt32 i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		// Avoid to erase the calibrated trig address offset
		if ( i == m_pCardState->u32VerifyTrigOffsetRateIx )
			continue;

		if ( 0 != m_pCardState->SrInfoTable[i].u16PingPong )
		{
			m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][0] = m_NewTrigAddrOffsetTable[0].i8OffsetDual;
			m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][1] = m_NewTrigAddrOffsetTable[0].i8OffsetSingle;

			// Master/Slave system
			if ( m_MasterIndependent->m_Next )
			{
				m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][0] += m_MsTrigAddrOffsetAdjust[0].i8OffsetDual;
				m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][1] += m_MsTrigAddrOffsetAdjust[0].i8OffsetSingle;
			}
		}
		else
		{
			// Find the match decimation
			for ( uInt32 j = 1; j < m_pCardState->u32SrInfoSize; j++ )
			{
				if ( m_pCardState->SrInfoTable[i].u32Decimation == m_NewTrigAddrOffsetTable[j].u32Decimation )
				{
					m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][0] = m_NewTrigAddrOffsetTable[j].i8OffsetDual;
					m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][1] = m_NewTrigAddrOffsetTable[j].i8OffsetSingle;

					// Master/Slave system
					if ( m_MasterIndependent->m_Next )
					{
						m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][0] += m_MsTrigAddrOffsetAdjust[j].i8OffsetDual;
						m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[i][1] += m_MsTrigAddrOffsetAdjust[j].i8OffsetSingle;
					}
					break;
				}
			}
		}
	}

	GageCopyMemoryX( m_pCardState->CalibInfoTable.i8ExtClkTrigAddrOffAdj, m_ExtClkTrigAddrOffsetTable, sizeof( m_ExtClkTrigAddrOffsetTable ));

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsRabbitDev:: ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg )
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
		uInt8	u8Shift = 0;

		ReadTriggerTimeStamp( StartSegment, pBuffer, u32NumOfSeg );

		if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
			u8Shift = 3;		// DUAL: One memory location has 8 samples each of channels
		else
			u8Shift = 4;		// SINGLE: One memory location has 16 samples

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
void	CsRabbitDev:: SetDataMaskModeTransfer ( uInt32 u32TxMode )
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
uInt32	CsRabbitDev:: GetFifoModeTransfer( uInt16 u16UserChannel )
{
	uInt32	u32ModeSel = 0;

	uInt16 u16Channel = ConvertToHwChannel( u16UserChannel );

	u32ModeSel = (u16Channel - 1) << 4;

	if ( (m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0  )
		u32ModeSel |= MODESEL1;

	return u32ModeSel;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
uInt32	CsRabbitDev:: GetDataMaskModeTransfer ( uInt32 u32TxMode )
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

bool 	CsRabbitDev::CheckRequiredFirmwareVersion()
{
	if ( m_bSkipFirmwareCheck )
		return CS_SUCCESS;

	if ( m_hCsiFile )
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
	if ( m_hFwlFile && bExpertFwDetected )
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
				if (( m_PlxData->CsBoard.u32UserFwVersionB[i+1].u32Reg != 0 ) && 
					( m_PlxData->CsBoard.u32UserFwVersionB[i+1].u32Reg != m_BaseBoardExpertEntry[i].u32Version ) )
					m_PlxData->CsBoard.u8BadBbFirmware |= BAD_EXPERT1_FW << i;	
			}
		}
	}

	return (0 == (m_PlxData->CsBoard.u8BadAddonFirmware | m_PlxData->CsBoard.u8BadBbFirmware));
}

#define CSISEPARATOR_LENGTH			16
const char CsiSeparator[] = "-- SEPARATOR --|";		// 16 character long + NULL character

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsRabbitDev::ReadAndValidateCsiFile( uInt32 u32BbHwOptions, uInt32 u32AoHwOptions )
{

	TRACE(DISP_TRACE_LEVEL, "CsRabbitDev::ReadAndValidateCsiFile\n");

	int32			i32RetCode = CS_SUCCESS;
	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry;


	UNREFERENCED_PARAMETER(u32AoHwOptions);

	// Attempt to open Csi file
	m_hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName );

	if ( ! m_hCsiFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_NOTFOUND, m_szCsiFileName );
		TRACE(DISP_TRACE_LEVEL, "CsRabbitDev::Csi file not found\n");
		return CS_MISC_ERROR;
	}

	uInt32	u32BBTarget		= CSI_BBRABBIT_TARGET;
	uInt32	u32AoTarget		= CSI_CSXYG8_TARGET;
	uInt32	u32ImageSize	= BB_FLASHIMAGE_SIZE;
	uInt32	u32AoFwDepend	= CSRB_AOHW_OPT_FWREQUIRED_MASK;
	uInt32	u32BaseBoardFwDepend	= CSRB_BBHW_OPT_FWREQUIRED_MASK;
	if ( m_bNucleon )
	{
		if ( m_bCobraMaxPCIe )
		{
			u32BBTarget = CSI_BBCOBRAMAX_PCIEx_TARGET;
			u32AoTarget = CSI_CSCDG8_PCIEx_TARGET;
			u32AoFwDepend = CSCOBRAMAX_AOHW_OPT_FWREQUIRED_MASK;
			u32BaseBoardFwDepend	= CSCOBRAMAX_BBHW_OPT_FWREQUIRED_MASK;
		}
		else
		{
			u32BBTarget = CSI_BBRABBIT_PCIEx_TARGET;
			u32AoTarget = CSI_CSXYG8_PCIEx_TARGET;
		}
		u32ImageSize = NUCLEON_FLASH_FWIMAGESIZE;
	}

	GageReadFile( m_hCsiFile, &header, sizeof(header) );

	if ( header.u32Version <= CSIFILE_VERSION )
		m_bCsiOld = ( header.u32Version == PREVIOUS_CSIFILE_VERSION );		// Old Csi 2.0.1 Csi file

	if ( ( header.u32Size == sizeof(CSI_FILE_HEADER) ) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) &&
		 (( header.u32Version == 0 ) || ( header.u32Version == CSIFILE_VERSION ) || ( header.u32Version == PREVIOUS_CSIFILE_VERSION )) )
	{
		for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
		{
			if ( 0 == GageReadFile( m_hCsiFile, &CsiEntry, sizeof(CsiEntry) ) )
			{
				TRACE(DISP_TRACE_LEVEL, "1\n");
				i32RetCode = CS_FAIL;
				break;
			}

			if ( u32BBTarget == CsiEntry.u32Target )
			{
				// Found Base board firmware info
				if ( ( ( (CsiEntry.u32HwOptions & u32BaseBoardFwDepend) == (u32BbHwOptions & u32BaseBoardFwDepend ) ) && u32BbHwOptions ) || 
					( !u32BbHwOptions && !CsiEntry.u32HwOptions) )
				{
					if ( CsiEntry.u32ImageSize > u32ImageSize )
					{
						TRACE(DISP_TRACE_LEVEL, "2\n");
						i32RetCode = CS_FAIL;
						break;
					}
					else
						GageCopyMemoryX( &m_BaseBoardCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
				}
				else
					continue;
			}
			else if ( u32AoTarget == CsiEntry.u32Target )
			{
				// Found Add-on board firmware info
				if ( ( ( (CsiEntry.u32HwOptions & u32AoFwDepend) == (u32AoHwOptions & u32AoFwDepend ) ) && u32AoHwOptions ) || 
					( !u32AoHwOptions && !CsiEntry.u32HwOptions) )
				{
					if ( CsiEntry.u32ImageSize > RBT_ADDONFLASH_FWIMAGESIZE )
					{
						TRACE(DISP_TRACE_LEVEL, "3\n");
						i32RetCode = CS_FAIL;
						break;
					}
					else
						GageCopyMemoryX( &m_AddonCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
				}
				else
					continue;
			}
		}
	}
	else
	{
		TRACE(DISP_TRACE_LEVEL, "4\n");
		i32RetCode = CS_FAIL;
	}

	// Additional validation...
	if ( CS_FAIL != i32RetCode )
	{
		if (m_AddonCsiEntry.u32ImageOffset == 0  )
		{
			TRACE(DISP_TRACE_LEVEL, "5\n");
			i32RetCode = CS_FAIL;
		}
		else
		{
			uInt8	u8Buffer[CSISEPARATOR_LENGTH];
			uInt32	u32ReadPosition = m_AddonCsiEntry.u32ImageOffset - CSISEPARATOR_LENGTH;

			if ( 0 == GageReadFile( m_hCsiFile, u8Buffer, sizeof(u8Buffer), &u32ReadPosition ) )
			{
				TRACE(DISP_TRACE_LEVEL, "6\n");
				i32RetCode = CS_FAIL;
			}
			else
			{
				if ( 0 != GageCompareMemoryX( (void *)CsiSeparator, u8Buffer, sizeof(u8Buffer) ) )
				{
					TRACE(DISP_TRACE_LEVEL, "7\n");
					i32RetCode = CS_FAIL;
				}
			}

			u32ReadPosition = m_BaseBoardCsiEntry.u32ImageOffset - CSISEPARATOR_LENGTH;

			if ( 0 == GageReadFile( m_hCsiFile, u8Buffer, sizeof(u8Buffer), &u32ReadPosition ) )
			{
				TRACE(DISP_TRACE_LEVEL, "8\n");
				i32RetCode = CS_FAIL;
			}
			else
			{

				if ( 0/*CSISEPARATOR_LENGTH*/ != GageCompareMemoryX( (void *)CsiSeparator, u8Buffer, sizeof(u8Buffer) ) )
				{
					TRACE(DISP_TRACE_LEVEL, "Mem Compare: %s, %s\n",CsiSeparator, u8Buffer);
					TRACE(DISP_TRACE_LEVEL, "9\n");
					i32RetCode = CS_FAIL;
				}
			}
		}
	}

	GageCloseFile ( m_hCsiFile );

	if ( CS_FAIL == i32RetCode )
	{
		m_hCsiFile = 0;
		TRACE(DISP_TRACE_LEVEL, "Invalid CSI file: %s\n", m_szCsiFileName);
		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_INVALID, m_szCsiFileName );
	}

	return i32RetCode;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsRabbitDev::ReadAndValidateFwlFile()
{
	int32				i32RetCode = CS_SUCCESS;
	FWL_FILE_HEADER		header = {0};
	FPGA_HEADER_INFO	FwlEntry = {0};
	BOOLEAN				bExpertFwDetected = FALSE;
	PCS_BOARD_NODE		pBdParam = &m_PlxData->CsBoard;


	for ( int i = 1; i <= 2; i ++ )
	{
		if ( pBdParam->u32BaseBoardOptions[i] != 0 || pBdParam->u32AddonOptions[i] != 0 )
		{
			bExpertFwDetected = TRUE;
			break;
		}
	}

	// Attempt to open Fwl file
	m_hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName );

	// If we can't open the file, report it to the event log whether or not we
	// actually will be using it.
	if ( ! m_hFwlFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_NOTFOUND, m_szFwlFileName );
		return CS_MISC_ERROR;
	}

	// If expert firmware doe not present on the current card then do not bother reading the Fwl file,
	// just close it and return
	if ( ! bExpertFwDetected )
	{
		GageCloseFile ( m_hFwlFile );
		RtlZeroMemory( m_BaseBoardExpertEntry, sizeof(m_BaseBoardExpertEntry) );
		return CS_SUCCESS;
	}


	uInt32	u32ImageSize = m_bNucleon ? NUCLEON_FLASH_FWIMAGESIZE : BB_FLASHIMAGE_SIZE;

	GageReadFile( m_hFwlFile, &header, sizeof(header) );

	if ( header.u32Version <= CSIFILE_VERSION )
		m_bFwlOld = ( header.u32Version == PREVIOUS_FWLFILE_VERSION );		// Old Csi 1.02.01 Fwl file

	if ( ( header.u32Size == sizeof(FWL_FILE_HEADER) ) && 
		 ( ((uInt32)CSxyG8_BOARDTYPE_MASK & header.u32BoardType) == ((uInt32)CSxyG8_BOARDTYPE_MASK & pBdParam->u32BoardType) ) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) &&
		 ( (header.u32Version == FWLFILE_VERSION ) || ( header.u32Version == PREVIOUS_FWLFILE_VERSION )) )
	{
		for( uInt32 i = 0; i < header.u32Count; i++ )
		{
			if ( 0 == GageReadFile( m_hFwlFile, &FwlEntry, sizeof(FwlEntry) ) )
			{
				i32RetCode = CS_FAIL;
				break;
			}
			// Bypass checking for Addon Histogram frimware
			if ( 0 != (CS_AOPTIONS_HISTOGRAM & FwlEntry.u32Option) )
				continue;

			if( FwlEntry.u32Destination == BASEBOARD_FIRMWARE )
			{
				if ( FwlEntry.u32ImageSize <= u32ImageSize )
				{
					for ( int j = 0; j < sizeof(m_BaseBoardExpertEntry) / sizeof(m_BaseBoardExpertEntry[0]); j ++ )
					{
						if ( pBdParam->u32BaseBoardOptions[j+1] == FwlEntry.u32Option )
						{
							GageCopyMemoryX( &m_BaseBoardExpertEntry[j], &FwlEntry, sizeof(FwlEntry) );
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

	GageCloseFile ( m_hFwlFile );

	if ( CS_FAIL == i32RetCode )
	{
		m_hFwlFile = (FILE_HANDLE)0;
		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_INVALID, m_szFwlFileName );
	}
	
	return i32RetCode;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsRabbitDev *CsRabbitDev::GetCardPointerFromBoardIndex( uInt16 BoardIndex )
{
	CsRabbitDev *pDevice = m_MasterIndependent;

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

uInt16 CsRabbitDev::NormalizedChannelIndex( uInt16 u16ChannelIndex )
{
	if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) == CS_MODE_DUAL )
		return	( u16ChannelIndex % 2) ? CS_CHAN_1: CS_CHAN_2;
	else
		return CS_CHAN_1;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::NormalizedTriggerSource( int32 i32TriggerSource )
{
	if ( i32TriggerSource ==  CS_TRIG_SOURCE_DISABLE )
		return CS_TRIG_SOURCE_DISABLE;
	else if ( i32TriggerSource < 0 )
		return CS_TRIG_SOURCE_EXT;
	else
	{
		if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) == CS_MODE_DUAL )
			return	( i32TriggerSource % 2) ? CS_TRIG_SOURCE_CHAN_1: CS_TRIG_SOURCE_CHAN_2;
		else if( 0 == (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL))
		{
			if  ( ! m_bSingleChannel2 )
			{
				if ( i32TriggerSource % 2 )
					return CS_TRIG_SOURCE_CHAN_1;
				else
					return CS_TRIG_SOURCE_DISABLE;
			}
			else
			{
				if ( 0 != (i32TriggerSource % 2) )
					return CS_TRIG_SOURCE_CHAN_1;
				else
					return CS_TRIG_SOURCE_CHAN_2;
			}
		}
		else
			return CS_TRIG_SOURCE_CHAN_1;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN	CsRabbitDev::IsAddonBoardPresent()
{
	// Check Addon board is present by comparing the addon flash Id
	// If the Addon Flash Id is not the one we expect to see then either Addon Board is not present
	// or Addon Flash is broken.
	// In the case that addon flash is broken, addon board will not functional and it is safe
	// to consider as if addon board is not present.

	return ( CSRB_ADDONFLASH_MFG_ID == GetFlashManufactoryCode() );

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsRabbitDev::CsSetDataFormat( _CSDATAFORMAT csFormat )
{
	switch ( csFormat )
	{
		case formatHwAverage:
			if ( m_bExpertAvgFullRes )
			{
				m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_8;
				m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
				m_pCardState->AcqConfig.i32SampleRes = CSRB_DEFAULT_RES << 14;
				m_pCardState->AcqConfig.i32SampleOffset = 0xFFFFC000;
			}
			else
			{
				// Sign 8-bit data format 
				m_pCardState->AcqConfig.u32SampleBits = CSRB_DEFAULT_SAMPLE_BITS;
				m_pCardState->AcqConfig.u32SampleSize = CSRB_DEFAULT_SAMPLE_SIZE;
				m_pCardState->AcqConfig.i32SampleRes = CSRB_DEFAULT_RES;
				m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_8_LJ;
			}
			break;

		case formatSoftwareAverage:
			m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_8;
			m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
			m_pCardState->AcqConfig.i32SampleRes = CSRB_DEFAULT_RES;
			m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_8_LJ;
			break;

		default:
			m_pCardState->AcqConfig.u32SampleBits = CSRB_DEFAULT_SAMPLE_BITS;
			m_pCardState->AcqConfig.i32SampleRes = CSRB_DEFAULT_RES;
			m_pCardState->AcqConfig.u32SampleSize = CSRB_DEFAULT_SAMPLE_SIZE;
			m_pCardState->AcqConfig.i32SampleOffset = CSRB_DEFAULT_SAMPLE_OFFSET;
			break;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CsRabbitDev::IsAddonInit()
{
	bool bRet = true;


	return bRet;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::InitAddOnRegisters(bool bForce)
{
	int32	i32Ret = CS_SUCCESS;

	if( m_pCardState->bAddOnRegistersInitialized && !bForce )
		return i32Ret;

	if( !GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	//Check Addon status
	if ( ! IsAddonInit() )
	{
		if ( IsAddonBoardPresent() )
		{
			// Addon board is present but it is not initialzied
			// This may caused by bad Add-on firmware. Attempt to update add-on firmware later
			m_PlxData->CsBoard.u8BadAddonFirmware = BAD_DEFAULT_FW;
			return CS_ADDONINIT_ERROR;
		}
		else
			return CS_ADDON_NOT_CONNECTED;
	}

	// Wait for Addon board ready
	CSTIMER		CsTimeout;
	CsTimeout.Set( KTIME_SECOND(4) );		// Wait for max 8 x 1sec

	for(;;)
	{
		GetBoardNode()->u32RawFwVersionA[0] = ReadRegisterFromNios( 0, CSRB_FWV_RD_CMD );
		if ( GetBoardNode()->u32RawFwVersionA[0] != FAILED_NIOS_DATA )
			break;

		if ( CsTimeout.State() )
		{
			GetBoardNode()->u32RawFwVersionA[0] = ReadRegisterFromNios( 0, CSRB_FWV_RD_CMD );
			if ( GetBoardNode()->u32RawFwVersionA[0] != FAILED_NIOS_DATA )
				break;
			return CS_ADDONINIT_ERROR;	// timeout !!!
		}
	}
	// Read but skip validating Addon firmware
	// The firmware will be re-validate later when CsRm calls GetAcquisitionSystemCount()
	i32Ret = ReadAddOnHardwareInfoFromEeprom(FALSE);
	if( CS_FAILED ( i32Ret ) )
		return i32Ret;

	i32Ret = WriteToAdcReg( CSRB_ADC_REG_CFG, m_bCobraMaxPCIe ? COBRAMAX_DEFAULT_ADC_CFG : RB_DEFAULT_ADC_CFG);
	if( CS_FAILED ( i32Ret ) )
		return i32Ret;

	if ( m_bCobraMaxPCIe )
		i32Ret = SetAdcMode(true, 1 == m_PlxData->CsBoard.NbAnalogChannel );
	else
		i32Ret = SetAdcMode( CS_CHAN_1 == ConvertToHwChannel( CS_CHAN_1 ) );
	if( CS_FAILED ( i32Ret ) )
		return i32Ret;

	i32Ret = SetAdcDelay();
	if( CS_FAILED ( i32Ret ) )
		return i32Ret;

	i32Ret = SetAdcOffset( CS_CHAN_1);
	if( CS_FAILED ( i32Ret ) )
		return i32Ret;

	i32Ret = SetAdcGain( CS_CHAN_1 );
	if( CS_FAILED ( i32Ret ) )
		return i32Ret;

	if ( 2 == m_PlxData->CsBoard.NbAnalogChannel )
	{
		i32Ret = SetAdcOffset( CS_CHAN_2);
		if( CS_FAILED ( i32Ret ) )
			return i32Ret;

		i32Ret = SetAdcGain( CS_CHAN_2 );
		if( CS_FAILED ( i32Ret ) )
			return i32Ret;
	}
	i32Ret = ReadCalibTableFromEeprom();
	if ( CS_FAILED(i32Ret) )return i32Ret;

	i32Ret = SendTimeStampMode();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendChannelSetting(CS_CHAN_1);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendChannelSetting(CS_CHAN_2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//initialize unsed dacs

	if ( m_bCobraMaxPCIe )
	{
		i32Ret = WriteRegisterThruNios(0xFF, CSRB_SET_CHAN_OFFSET);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteToDac(BUNNY_FINE_OFF_1, 2048, 0);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = WriteToDac(BUNNY_FINE_OFF_2, 2048, 0);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = WriteToDac(BUNNY_COARSE_OFF_1, 2048, 0);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = WriteToDac(BUNNY_COARSE_OFF_2, 2048, 0);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	else
	{
		i32Ret = WriteToDac(BUNNY_FINE_OFF_1, 0, 0);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = WriteToDac(BUNNY_FINE_OFF_2, 0, 0);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	i32Ret = WriteToCalibCtrl(eccAdcOffset, CS_CHAN_1, 0, 0);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendTriggerEngineSetting( CS_TRIG_SOURCE_DISABLE );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendExtTriggerSetting( CS_TRIG_SOURCE_DISABLE );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SetDataPath();
	if( CS_FAILED(i32Ret) )	return i32Ret;

//	i32Ret = SendOptionSetting();
//	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToSelfTestRegister();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt32 u32DefaultMode = CSRB_DEFAULT_MODE;
	if ( 1 == GetBoardNode()->NbAnalogChannel )
		u32DefaultMode = CS_MODE_SINGLE;

	i32Ret = SendClockSetting( u32DefaultMode );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCaptureModeSetting(u32DefaultMode);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCalibModeSetting(CS_CHAN_1, ecmCalModeNormal, 0);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	if ( 2 == m_PlxData->CsBoard.NbAnalogChannel )
	{
		i32Ret = WriteToCalibCtrl(eccAdcOffset, CS_CHAN_2, 0, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendCalibModeSetting(CS_CHAN_2);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	if ( m_bCobraMaxPCIe )
		AddonReset();


	m_pCardState->bAddOnRegistersInitialized = true;

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsRabbitDev::ReadAddOnHardwareInfoFromEeprom( BOOLEAN bCheckSum )
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32Offset;

	RBT_ADDONFLASH_FIRMWARE_INFOS	FwInfo[RBT_NUMBER_OF_ADDDONIMAGES];

	u32Offset = FIELD_OFFSET(RBT_ADDONFLASH_LAYOUT, FlashImage1) + 	FIELD_OFFSET(RBT_ADDONFLASH_IMAGE_STRUCT, FwInfo) +
				FIELD_OFFSET(RBT_ADDONFLASH_FWINFO_SECTOR, FwInfoStruct);
	i32Status = ReadEeprom( &FwInfo[0], u32Offset, sizeof(RBT_ADDONFLASH_FIRMWARE_INFOS) );
	if ( CS_FAILED(i32Status) )
		return i32Status;

	if ( ! m_bSkipFirmwareCheck ) ///&& ! m_bNeverValidateChksum
	{
		// Validate Checksum of the default image
		if ( bCheckSum &&  (FwInfo[0].u32ChecksumSignature != CSI_FWCHECKSUM_VALID ) )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_FWCHECKSUM_ERROR, "Addon board" );
			m_PlxData->CsBoard.u8BadAddonFirmware = BAD_DEFAULT_FW;
			return CS_INVALID_FW_VERSION;
		}
	}

	// Assume the addon firmware is valid
	m_PlxData->CsBoard.u8BadAddonFirmware = 0;

	u32Offset = FIELD_OFFSET(RBT_ADDONFLASH_LAYOUT, FlashImage2) + 	FIELD_OFFSET(RBT_ADDONFLASH_IMAGE_STRUCT, FwInfo) +
				FIELD_OFFSET(RBT_ADDONFLASH_FWINFO_SECTOR, FwInfoStruct);
	i32Status = ReadEeprom( &FwInfo[1], u32Offset, sizeof(RBT_ADDONFLASH_FIRMWARE_INFOS) );
	if ( CS_FAILED(i32Status) )
		return i32Status;

	RABBIT_FLASH_FOOTER		RabbitFshFooter;

	u32Offset = FIELD_OFFSET(RBT_ADDONFLASH_LAYOUT, Footer);
	i32Status = ReadEeprom( &RabbitFshFooter, u32Offset, sizeof(RabbitFshFooter) );
	if ( CS_SUCCESS == i32Status )
	{
		 GetBoardNode()->u32SerNumEx				= RabbitFshFooter.AddonFooter.u32SerialNumber;
		 GetBoardNode()->u32AddonBoardVersion		= RabbitFshFooter.AddonFooter.u32HwVersion;
		 GetBoardNode()->u32AddonHardwareOptions	= RabbitFshFooter.AddonFooter.u32HwOptions;

		for ( uInt32 i = 0; i < RBT_NUMBER_OF_ADDDONIMAGES; i ++ )
		{
			if ( (FwInfo[i].u32ChecksumSignature == CSI_FWCHECKSUM_VALID) && (FwInfo[i].u32AddOnFwOptions != -1) )
				 GetBoardNode()->u32AddonOptions[i] = FwInfo[i].u32AddOnFwOptions;
			else
				 GetBoardNode()->u32AddonOptions[i] = 0;
		}

		GetBoardNode()->u32ActiveAddonOptions =  GetBoardNode()->u32AddonOptions[m_AddonImageId];
		
		// Reset options only in case of initialized eeprom
		if ( -1 != RabbitFshFooter.ClockOutCfg )
			m_pCardState->eclkoClockOut	= RabbitFshFooter.ClockOutCfg;
		if ( -1 != RabbitFshFooter.TrigEnableCfg )
			m_pCardState->eteTrigEnable	= RabbitFshFooter.TrigEnableCfg;
		if ( (uInt16) -1 != RabbitFshFooter.u16TrigOutCfg )
			m_pCardState->u16TrigOutMode	= RabbitFshFooter.u16TrigOutCfg;
		if ( (uInt16)-1 != RabbitFshFooter.u16TrigOutDisabled)
			m_pCardState->bDisableTrigOut	= (0 != RabbitFshFooter.u16TrigOutDisabled);

		m_pCardState->bEco10	= (0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_ECO10) );

		// Unitiialized Eeprom has all bits set
		// Reset options in case of uninitilzed eeprom
		if ( 0xFFFFFFFF ==  GetBoardNode()->u32ActiveAddonOptions )
			 GetBoardNode()->u32ActiveAddonOptions = 0;
		if ( 0xFFFFFFFF ==  GetBoardNode()->u32AddonHardwareOptions )
			 GetBoardNode()->u32AddonHardwareOptions = 0;

		if ( m_bCobraMaxPCIe )
			m_pCardState->b14G8 = (0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL) );
	}
	else
	{
		RtlZeroMemory(  GetBoardNode()->u32AddonOptions, sizeof( GetBoardNode()->u32AddonOptions) );
		GetBoardNode()->u32AddonBoardVersion = 0;
		GetBoardNode()->u32SerNumEx = 0;
		GetBoardNode()->u32ActiveAddonOptions = 0;
		GetBoardNode()->u32AddonHardwareOptions = 0;
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::AddonConfigReset(uInt32 Addr)
{
	CSTIMER		CsTimeout;

	WriteGIO(CSRB_ADDON_FLASH_CMD, CSRB_ADDON_CONFIG);
	WriteGIO(CSRB_ADDON_FLASH_CMD, Addr);

	// Wait for Config done ...
	CsTimeout.Set( KTIME_SECOND(4) );		// Wait for max 8 x 1sec

	uInt32	u32Status = ReadGIO(CSRB_ADDON_STATUS);
	while( (u32Status & CSRB_ADDON_STAT_CONFIG_DONE) == 0 )
	{
		u32Status = ReadGIO(CSRB_ADDON_STATUS);
		if ( CsTimeout.State() )
		{
			u32Status = ReadGIO(CSRB_ADDON_STAT_CONFIG_DONE);
			if( (u32Status & CSRB_ADDON_STAT_FLASH_READY) == 0 )
			{
				return CS_FALSE;
			}
		}
	}

	// Added for WDF driver
	// 0x35 is the status when addon is really ready
	// What is the meaning of this status ? Ask Nestor  ??????????
	// Without this wait, reading flash will cause addon board not responding (RED LED)
	u32Status = ReadGIO(CSRB_ADDON_STATUS);
	while( (u32Status & 0x35) != 0x35 )
	{
		u32Status = ReadGIO(CSRB_ADDON_STATUS);
		if ( CsTimeout.State() )
		{
			u32Status = ReadGIO(CSRB_ADDON_STATUS);
			if( (u32Status & 0x35) != 0x35 )
			{
				return CS_FALSE;
			}
		}
	}

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//	Initialization upon received IOCTL_ACQUISITION_SYSTEM_INIT
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::AcqSystemInitialize()
{
	int32	i32Ret = CS_SUCCESS;

	m_pCardState->bUseDacCalibFromEEprom = (m_pCardState->CalibInfoTable.u32Active & BUNNY_CAL_DAC_VALID) ? true : false;
#ifdef _WINDOWS_   
	ReadCommonRegistryOnAcqSystemInit();
#else
	ReadCommonCfgOnAcqSystemInit();              // Use .ini Config file
#endif
   
	m_bOneSampleResolution = false;

	if( !GetInitStatus()->Info.u32Nios )
		return i32Ret;

	// Update pointers for SR tables in shared mem
	InitBoardCaps();

	i32Ret = InitAddOnRegisters();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsRabbitDev::ResetCachedState(void)
{
	// After ConfigReset all settings on the card is lost. By reseting these parameters, we make sure
	// that the next call of CssetAcquisitionCfg, CsSetChannelCfg ... will reconfigure HW even if the
	// configuration has not been changed.
	RtlZeroMemory( &m_pCardState->AddOnReg, sizeof(m_pCardState->AddOnReg) );
	m_u32CalibRange_mV = 0;

	m_pCardState->AddOnReg.u16ExtTrigEnable = (uInt16)-1;

	m_pCardState->AddOnReg.u16ClkOut = m_bCobraMaxPCIe ? CSFB_CLK_OUT_NONE : CSRB_CLK_OUT_NONE;
	m_pCardState->bMasterSlaveCalib = m_pCardState->bExtTrigCalib = true;
	m_MasterIndependent->m_pCardState->bFwExtTrigCalibDone	= false;
	m_pCardState->AddOnReg.u16MasterSlave = 0;

	// Mode acquisition normal
	SetCalibBit( false, &m_pCardState->AddOnReg.u32FeSetup[0] );
	SetCalibBit( false, &m_pCardState->AddOnReg.u32FeSetup[1] );
//	m_pCardState->AddOnReg.u32FeSetup[0] = 
//	m_pCardState->AddOnReg.u32FeSetup[1] = CSRB_SET_CHAN_CAL;
}


//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsLoadBaseBoardFpgaImage( uInt8 u8ImageId )
{
	BOOLEAN		bImageChanged = FALSE;

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
				BaseBoardConfigurationReset(0);
				m_pCardState->u8ImageId = 0;

				m_bHardwareAveraging = FALSE;
				m_pCardState->AcqConfig.u32Mode &= ~(CS_MODE_USER1 | CS_MODE_USER2);
			}
		}
		break;

	}

	m_bMulrecAvgTD		= IsExpertAVG( u8ImageId );
	m_Stream.bEnabled	= IsExpertSTREAM( u8ImageId );

	IoctlStmSetStreamMode( m_Stream.bEnabled ? 1: 0 );

	// After ConfigReset all settings on the card is lost. By reseting these parameters, we make sure
	// that the next call of CssetAcquisitionCfg, CsSetChannelCfg ... will reconfigure HW even if the
	// configuration has not been changed.
	if ( bImageChanged )
	{
		ResetCachedState();

		// Invalidate then re-read the firmware version
		GetBoardNode()->u32RawFwVersionB[0] = 0;
		ReadVersionInfo();
		SendMasterSlaveSetting();
		UpdateCardState();
	}

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	CsRabbitDev::FillOutBoardCaps(PCSXYG8BOARDCAPS pCapsInfo)
{
	uInt32		u32Temp0;
	uInt32		u32Temp1;
	uInt32		i;


	RtlZeroMemory( pCapsInfo, sizeof(CSXYG8BOARDCAPS) );

	//Fill SampleRate
	if ( m_pCardState->bLAB11G )
		i = 1;			// removed the highest sampling rate (pingpong)
	else
		i = 0;

	//Fill SampleRate
	for(  ; i < m_pCardState->u32SrInfoSize; i++ )
	{
		pCapsInfo->SrTable[i].PublicPart.i64SampleRate = m_pCardState->SrInfoTable[i].llSampleRate;

		if( m_pCardState->SrInfoTable[i].llSampleRate >= 1000000000 )
		{
			u32Temp0 = (uInt32) m_pCardState->SrInfoTable[i].llSampleRate/1000000000;
			u32Temp1 = (uInt32) (m_pCardState->SrInfoTable[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000000000) /100000000;
			if( u32Temp1 )
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), "%d.%d GS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), "%d GS/s", u32Temp0 );
		}
		else if( m_pCardState->SrInfoTable[i].llSampleRate >= 1000000 )
		{
			u32Temp0 = (uInt32) m_pCardState->SrInfoTable[i].llSampleRate/1000000;
			u32Temp1 = (uInt32) (m_pCardState->SrInfoTable[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000000) /100000;
			if( u32Temp1 )
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), "%d.%d MS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), "%d MS/s", u32Temp0 );
		}
		else if( m_pCardState->SrInfoTable[i].llSampleRate >= 1000 )
		{
			u32Temp0 = (uInt32) m_pCardState->SrInfoTable[i].llSampleRate/1000;
			u32Temp1 = (uInt32) (m_pCardState->SrInfoTable[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000) /100;
			if( u32Temp1 )
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), "%d.%d kS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), "%d kS/s", u32Temp0 );
		}
		else
		{
			sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), "%lld S/s", m_pCardState->SrInfoTable[i].llSampleRate );
		}

		if ( 0 != m_pCardState->SrInfoTable[i].u16PingPong )
			pCapsInfo->SrTable[i].u32Mode = CS_MODE_SINGLE;
		else
			pCapsInfo->SrTable[i].u32Mode = CS_MODE_DUAL | CS_MODE_SINGLE;

	}

	// Fill External Clock info
	pCapsInfo->ExtClkTable[0].i64Max = GetMaxExtClockRate();
	pCapsInfo->ExtClkTable[0].i64Min = CSRB_MIN_EXT_CLK;
	pCapsInfo->ExtClkTable[0].u32Mode = CS_MODE_SINGLE;
	pCapsInfo->ExtClkTable[0].u32SkipCount = m_u32DefaultExtClkSkip	;
	if( GetBoardNode()->NbAnalogChannel > 1 )
		pCapsInfo->ExtClkTable[0].u32Mode |= CS_MODE_DUAL;

	uInt32	u32Size = 0;
	// Fill Range table
	// sorting is done on user level
	for( i = 0; i < m_pCardState->u32FeInfoSize; i++ )
	{
		pCapsInfo->RangeTable[i].u32InputRange = m_pCardState->FeInfoTable[i].u32Swing_mV;
		u32Size =  sizeof(pCapsInfo->RangeTable[i].strText);
		if( pCapsInfo->RangeTable[i].u32InputRange >= 2000 &&  0 == (pCapsInfo->RangeTable[i].u32InputRange % 2000))
			sprintf_s( pCapsInfo->RangeTable[i].strText, u32Size, "±%d V", pCapsInfo->RangeTable[i].u32InputRange / 2000 );
		else
			sprintf_s( pCapsInfo->RangeTable[i].strText, u32Size, "±%d mV", pCapsInfo->RangeTable[i].u32InputRange / 2 );

		pCapsInfo->RangeTable[i].u32Reserved =	CS_IMP_50_OHM;
	}

	//Fill coupling table
	i = 0;
	u32Size =  sizeof(pCapsInfo->CouplingTable[0].strText);
	pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_DC;
	strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "DC" );
	pCapsInfo->CouplingTable[i].u32Reserved = 0;

	if ( ( ! m_bNucleon || ( 0 == (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) )  )
		&&  !m_pCardState->bHighBandwidth ) 
	{
		i++;
		pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_AC;
		strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "AC" );
		pCapsInfo->CouplingTable[i].u32Reserved = 0;
	}

	//Fill impedance table
	u32Size =  sizeof(pCapsInfo->ImpedanceTable[0].strText);
	pCapsInfo->ImpedanceTable[0].u32Imdepdance = CS_REAL_IMP_50_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[0].strText, u32Size, "50 Ohm" );
	pCapsInfo->ImpedanceTable[0].u32Reserved = 0;

	//Fill filter table
	i = 0;
	pCapsInfo->FilterTable[i].u32LowCutoff  = 0;
	pCapsInfo->FilterTable[i].u32HighCutoff = CSRB_FILTER_FULL;
	if( !m_pCardState->bHighBandwidth )
	{
		i++;
		pCapsInfo->FilterTable[i].u32LowCutoff  = 0;
		pCapsInfo->FilterTable[i].u32HighCutoff = CSRB_FILTER_LIMIT;
	}


	//Fill ext trig tables
	// Ranges
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigRangeTable[0].strText);
	pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_2_V;
	sprintf_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "%s", "Â±1 V" );
	pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM|CS_IMP_50_OHM|CS_IMP_EXT_TRIG;
	i++;
	pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_10_V;
	strcpy_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "Â±5 V" );
	pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_50_OHM|CS_IMP_EXT_TRIG;

	//Coupling
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigCouplingTable[0].strText);
	pCapsInfo->ExtTrigCouplingTable[i].u32Coupling = CS_COUPLING_DC;
	strcpy_s( pCapsInfo->ExtTrigCouplingTable[i].strText,u32Size, "DC" );
	pCapsInfo->ExtTrigCouplingTable[i].u32Reserved = 0;
	i++;
	pCapsInfo->ExtTrigCouplingTable[i].u32Coupling = CS_COUPLING_AC;
	strcpy_s( pCapsInfo->ExtTrigCouplingTable[i].strText, u32Size, "AC" );
	pCapsInfo->ExtTrigCouplingTable[i].u32Reserved = 0;

	//Impedance
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigImpedanceTable[0].strText);
	pCapsInfo->ExtTrigImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_50_OHM;
	strcpy_s( pCapsInfo->ExtTrigImpedanceTable[i].strText, u32Size, "50 Ohm" );
	pCapsInfo->ExtTrigImpedanceTable[i].u32Reserved = 0;
	i++;

	if (! m_pCardState->bLAB11G )
	{
		pCapsInfo->ExtTrigImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_1M_OHM;
		strcpy_s( pCapsInfo->ExtTrigImpedanceTable[i].strText, u32Size, "High Z" );
		pCapsInfo->ExtTrigImpedanceTable[i].u32Reserved = 0;
	}
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	CsRabbitDev::FindSrInfo( const LONGLONG llSr, PRABBIT_SAMPLE_RATE_INFO *pSrInfo )
{
	for( unsigned i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		if ( llSr == m_pCardState->SrInfoTable[i].llSampleRate )
		{
			*pSrInfo = &m_pCardState->SrInfoTable[i];
			return TRUE;
		}
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsRabbitDev::SetupHwAverageMode( BOOLEAN bFullRes, uInt32 u32ScaleFactor )
{
	uInt32		DevideSelector = 0;

	m_bExpertAvgFullRes = bFullRes;
	// HW Averaging
	// Using full resolution, all devide factor will be ignored
	if ( bFullRes )
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

		m_pCardState->AcqConfig.i32SampleRes = CSRB_DEFAULT_RES;
		switch ( u32ScaleFactor )
		{
		case 1024:
			{
			DevideSelector = 0;
			}
			break;
		case 512:
			{
			DevideSelector = 1;
			}
			break;
		case 256:
			{
			DevideSelector = 2;
			}
			break;
		case 128:
			{
			DevideSelector = 3;
			}
			break;
		case 64:
			{
			DevideSelector = 4;
			}
			break;
		case 32:
			{
			DevideSelector = 5;
			}
			break;
		case 16:
			{
			DevideSelector = 6;
			}
			break;
		case 8:
			{
			DevideSelector = 7;
			}
			break;
		case 4:
			{
			DevideSelector = 8;
			}
			break;
		case 2:
			{
			DevideSelector = 9;
			}
			break;

		default:
			{
			DevideSelector = 10;		// Scale Factor 1
			}
			break;
		}
	}

	#define	DUAL_CHAN_ON_BIT	0x10000

	uInt32	u32Data = 0;
	if (( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ) != 0 )
		u32Data |= DUAL_CHAN_ON_BIT;
	else
		u32Data &= ~DUAL_CHAN_ON_BIT;


	WriteGageRegister( GEN_COMMAND_R_REG, (DevideSelector << 8) | u32Data);
	CsSetDataFormat( formatHwAverage );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff, int64 i64TrigDelay)
{
	uInt8	u8Shift = 0;
	uInt32	u32Mode = m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE;

	switch(u32Mode)
	{
	case CS_MODE_DUAL:
		u8Shift = 3;
		break;
	case CS_MODE_SINGLE:
		u8Shift = 4;
		break;
	}

	m_u32SegmentResolution = (1 << u8Shift);
	m_Stream.bInfiniteDepth = (i64SegmentSize == -1);		// Infinite 

	// From samples, we have to convert to memory location then send this value to firmware
	// Because the firmware only support 30 bit register depth, we have to validate if the depth value exeeds 30 bit wide
	if ( ! m_Stream.bInfiniteDepth )
	{
		if ( (i64SegmentSize >> u8Shift) >= MAX_HW_DEPTH_COUNTER  )
			return CS_SEGMENTSIZE_TOO_BIG;
		if ( (i64PostDepth >> u8Shift) >= MAX_HW_DEPTH_COUNTER  ) 
			return CS_INVALID_TRIG_DEPTH;
	}

	if ( (i64HoldOff >> u8Shift) >= MAX_HW_DEPTH_COUNTER ) 
		return CS_INVALID_TRIGHOLDOFF;
	if ( (i64TrigDelay >> u8Shift) >= MAX_HW_DEPTH_COUNTER )
		return CS_INVALID_TRIGDELAY;

	uInt32	u32SegmentSize	= (uInt32) (i64SegmentSize >> u8Shift);
	uInt32	u32PostDepth	= (uInt32) (i64PostDepth >> u8Shift);
	uInt32	u32HoldOffDepth = (uInt32) (i64HoldOff >> u8Shift);
	uInt32	u32TrigDelay    = (uInt32) (i64TrigDelay >> u8Shift);

	int32 i32Ret = CS_SUCCESS;
	if ( m_bNucleon )
	{
		i32Ret = WriteRegisterThruNios( (uInt32) m_i32OneSampleResAdjust, CSPLXBASE_ONE_SAMPLE_RESOLUITON_ADJ );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	
	if ( m_bMulrecAvgTD )
	{
		// Interchange Segment count and Mulrec Acg count
		// From firmware point of view, the SegmentCount is number of averaging
		// and the number of averaging is segment count
		i32Ret = WriteRegisterThruNios( u32SegmentCount, CSPLXBASE_SET_MULREC_AVG_COUNT );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( m_bCalibActive )
			u32SegmentCount = 1;
		else
			u32SegmentCount = m_u32MulrecAvgCount;
	}

	i32Ret = WriteRegisterThruNios( u32SegmentSize, CSPLXBASE_SET_SEG_LENGTH_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios( m_Stream.bInfiniteDepth ? -1 : u32PostDepth,	CSPLXBASE_SET_SEG_POST_TRIG_CMD	);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// In regular Stream mode, we have to send a big segment count (-1 = 0xFFFFFFFF ) in order to get the highest performance
	i32Ret = WriteRegisterThruNios( ( m_Stream.bEnabled && ! m_bCalibActive && !m_bMulrecAvgTD ) ? -1 : u32SegmentCount, CSPLXBASE_SET_SEG_NUMBER_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( m_bNucleon )
	{
		// Square record
		uInt32 u32SqrPreTrig		= u32SegmentSize - u32PostDepth;
		uInt32 u32SqrTrigDelay		= (uInt32) (m_pCardState->AcqConfig.i64TriggerDelay >> u8Shift);
		uInt32 u32SqrHoldOffDepth	= 0;

		if ( u32HoldOffDepth >  u32SqrPreTrig )
			u32SqrHoldOffDepth	= u32HoldOffDepth - u32SqrPreTrig;

		i32Ret = WriteRegisterThruNios( u32SqrPreTrig, CSPLXBASE_SET_SEG_PRE_TRIG_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( u32SqrHoldOffDepth, CSPLXBASE_SET_SQUARE_THOLDOFF_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( u32SqrTrigDelay, CSPLXBASE_SET_SQUARE_TDELAY_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	else
	{
		i32Ret = WriteRegisterThruNios( u32HoldOffDepth, CSPLXBASE_SET_SEG_PRE_TRIG_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	if ( m_bMulrecAvgTD )
	{
		i32Ret = WriteRegisterThruNios( u32TrigDelay, CSPLXBASE_SET_MULREC_AVG_DELAY );
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	if ( m_bHistogram )
	{
		// Use the "real" segment size (the one that the user wants) instaed of the adjusted hw segment size
		m_u32HistogramMemDepth = (uInt32) (m_pCardState->AcqConfig.i64SegmentSize >> u8Shift);
	}

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

LONGLONG	CsRabbitDev::GetMaxSampleRate( BOOLEAN bPingpong )
{

	for( unsigned i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		if ( bPingpong == (BOOLEAN) m_pCardState->SrInfoTable[i].u16PingPong )
			return m_pCardState->SrInfoTable[i].llSampleRate;
	}

	return m_pCardState->SrInfoTable[0].llSampleRate;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

LONGLONG	CsRabbitDev::GetMaxExtClockRate( uInt32 u32Mode )
{
	UNREFERENCED_PARAMETER(u32Mode);
	// Only work in non-pingpong mode for all type of cards
	return m_pCardState->SrInfoTable[1].llSampleRate;

#if 0

	// Assume m_pCardState->SrInfoTable[0] is the highest sample rate in Single Channel mode
	if ( m_pCardState->bLAB11G )
		return m_pCardState->SrInfoTable[1].llSampleRate;		// Only work in non-pingpong mode
	else if ( u32Mode & CS_MODE_SINGLE )
	{
		// Single Channel Mode
		if ( !m_bAllowExtClkMs_pp && (0 != m_MasterIndependent->m_Next) )
			return m_pCardState->SrInfoTable[0].llSampleRate>>1;
		else
			return m_pCardState->SrInfoTable[0].llSampleRate;
	}
	else
	{
		// Dual Channel Mode
		if ( m_pCardState->SrInfoTable[0].u16PingPong )
			return m_pCardState->SrInfoTable[0].llSampleRate >> 1;
		else
			return m_pCardState->SrInfoTable[0].llSampleRate;
	}
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsRabbitDev::GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine )
{
	// A Free trigger engine is the one that is disabled and not yet assigned to any user
	// trigger settings

	if ( i32TrigSource < 0 )
	{
		if ( 0 == m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32UserTrigIndex )
		{
			*u16TriggerEngine = CSRB_TRIGENGINE_EXT;
			return CS_SUCCESS;
		}
	}
	else
	{
		uInt16	i = (i32TrigSource > 0 ) ? CSRB_TRIGENGINE_A1 : 0;

		// Validate trigger engines per channel
		int u32SameSourceCount = 0;
		for ( ; i < GetBoardNode()->NbTriggerMachine; i ++ )
		{
			if ( 0 != m_CfgTrigParams[i].u32UserTrigIndex && m_CfgTrigParams[i].i32Source == i32TrigSource )
				u32SameSourceCount++;

			if ( u32SameSourceCount >= 2 )		// Only 2 trigger engines per channel
				return CS_ALLTRIGGERENGINES_USED;
		}

		i = (i32TrigSource > 0 ) ? CSRB_TRIGENGINE_A1 : 0;
		for ( ; i < GetBoardNode()->NbTriggerMachine; i ++ )
		{
			if ( 0 == m_CfgTrigParams[i].u32UserTrigIndex )
			{
				*u16TriggerEngine = i;
				return CS_SUCCESS;
			}
		}
	}

	return CS_ALLTRIGGERENGINES_USED;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsRabbitDev::AssignBoardType()
{
	if ( 0 != GetBoardNode()->u32BoardType )
		return;

	if ( m_bCobraMaxPCIe )
	{
		if ( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL ) )
		{
			GetBoardNode()->u32BoardType			= CSX14G8_BOARDTYPE;
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_4GHZ;		// Default sampling rate
			m_pCardState->AcqConfig.u32Mode			= CS_MODE_SINGLE;
			GetBoardNode()->NbAnalogChannel			= 1;
			m_pCardState->b14G8						= TRUE;
		}
		else
		{
			GetBoardNode()->u32BoardType			= CSX24G8_BOARDTYPE;
			m_pCardState->AcqConfig.i64SampleRate	= CS_SR_2GHZ;		// Default sampling rate
			m_pCardState->AcqConfig.u32Mode			= CS_MODE_DUAL;
			GetBoardNode()->NbAnalogChannel			= 2;
		}
	}
	else if ( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_LAB11G ) )
	{
		// Lab11G card
		m_pCardState->bLAB11G					= true;
		GetBoardNode()->u32BoardType			= LAB11G_BOARDTYPE;
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_1GHZ;
		m_pCardState->AcqConfig.u32Mode = CS_MODE_SINGLE;
		GetBoardNode()->NbAnalogChannel = 2;
	}
	else if ( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL) )
	{
		// Cs11G8 card
		GetBoardNode()->u32BoardType			= CS11G8_BOARDTYPE;
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_1GHZ;		// Default sampling rate
		m_pCardState->AcqConfig.u32Mode = CS_MODE_SINGLE;
		GetBoardNode()->NbAnalogChannel = 1;
	}
	else if ( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_500OSC) )
	{
		// Cs21G8 card
		GetBoardNode()->u32BoardType			= CS21G8_BOARDTYPE;
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_500MHZ;			// Default sampling rate
		m_pCardState->AcqConfig.u32Mode = CSRB_DEFAULT_MODE;
		GetBoardNode()->NbAnalogChannel = 2;
	}
	else
	{
		// Cs22G8 card	(default)
		GetBoardNode()->u32BoardType			= CS22G8_BOARDTYPE;
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_1GHZ;
		m_pCardState->AcqConfig.u32Mode = CSRB_DEFAULT_MODE;
		GetBoardNode()->NbAnalogChannel = 2;
	}

	if ( !m_bNucleon )
	{
		m_pCardState->bHighBandwidth = ( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_HIGHBW) );
		if( m_pCardState->bHighBandwidth )
		{
			// Disable all calibrations and use Calib Table from Eeprom
			m_pCardState->bUseDacCalibFromEEprom	=
			m_bNeverCalibrateDc			= true;
			m_bNeverCalibrateMs			=
			m_bNeverCalibExtTrig		= TRUE;
		}
	}

	m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_8;
	m_pCardState->AcqConfig.i32SampleOffset = CSRB_DEFAULT_SAMPLE_OFFSET;

	m_pCardState->AcqConfig.u32ExtClk = CSRB_DEFAULT_EXT_CLOCK_EN;

	m_pCardState->AcqConfig.i32SampleRes = CSRB_DEFAULT_RES;
	m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_1;

	GetBoardNode()->NbDigitalChannel	= 0;

	if ( m_bNucleon )
		GetBoardNode()->u32BoardType |= CSNUCLEONBASE_BOARDTYPE;

	if ( m_pCardState->bLAB11G ) 
		GetBoardNode()->NbTriggerMachine	= GetBoardNode()->NbAnalogChannel + 1;
	else
		GetBoardNode()->NbTriggerMachine	= 2*GetBoardNode()->NbAnalogChannel + 1;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR )
{

	for ( uInt32 i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		if ( m_pCardState->SrInfoTable[i].llSampleRate == llSampleRate )
		{
			*u32IndexSR = i;
			return CS_SUCCESS;
		}
	}

	*u32IndexSR = 0;
	return CS_INVALID_SAMPLE_RATE;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8Index = 0;
	int64	i64MaxMemPerChan;
	int64	i64MaxSegmentSize;
	int64	i64MaxDepth;
	int64	i64MaxPreTrigDepth = 0;
	int64	i64MaxTrigDelay;
	uInt32	u32LimitSegmentCount = 0xFFFFFFFF;
	uInt32	u32MaxSegmentCount = 1;
	int32	i32MinSegmentSize = 16;
	uInt32	u32DepthRes = CSRB_TRIGGER_RES;
	bool	bXpertBackup[10];

	uInt32	u32RealSegmentCount = pAcqCfg->u32SegmentCount;
	int64	i64RealSegmentSize	= pAcqCfg->i64SegmentSize;
	int64	i64RealPostDepth	= pAcqCfg->i64Depth;

	// This function is called before ACTION_COMMIT
	// CsValidateAcquisitionConfig may change all Xpert flags
	// so that all algorithm for validation work.
	// At the end we will restore actual value of Xpert Flags 			
	bXpertBackup[0]	= m_bMulrecAvgTD;
	bXpertBackup[1]	= m_Stream.bEnabled;

	m_bMulrecAvgTD = m_Stream.bEnabled = m_Stream.bInfiniteDepth = false;

	// Validation for Expert Histogram
	// Unlike other eXpert firmwares from Base board, the expert Histogram is embedded in default addon firmware (Image 0 addon)
	bool	bHistogramEnable = false;
	if ( 0!= ( pAcqCfg->u32Mode & CS_MODE_EXPERT_HISTOGRAM ) )
	{
		bHistogramEnable = IsHistogramEnabled();
		if ( ! bHistogramEnable )
		{
			if (u32Coerce)
				pAcqCfg->u32Mode &=  ~CS_MODE_EXPERT_HISTOGRAM;
			else
				return CS_INVALID_ACQ_MODE;
		}
	}

	CsRabbitDev*	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		pDevice->m_bHistogram = bHistogramEnable;
		pDevice = pDevice->m_Next;
	}

	if ( pAcqCfg->u32Mode & CS_MODE_USER1 )
		u8Index = 1;
	else if ( pAcqCfg->u32Mode & CS_MODE_USER2 )
		u8Index = 2;
	
	i64MaxTrigDelay = CSMV_LIMIT_HWDEPTH_COUNTER;
	i64MaxTrigDelay *= 128;
	i64MaxTrigDelay /= (32*(pAcqCfg->u32Mode&CS_MASKED_MODE));

	// Samples reserved by HW at the end of each segment
	uInt32	u32TailHwReserved = m_32SegmentTailBytes;

	if ( m_bNucleon )
	{
		i64MaxPreTrigDepth = 4088;
		u32DepthRes <<= 1;
		i32MinSegmentSize = 256;
	}
	i64MaxMemPerChan = m_pSystem->SysInfo.i64MaxMemory;
	// As designed from FW, the last segment always take more space
	i64MaxMemPerChan -= (4*64);

	if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
	{
		i64MaxPreTrigDepth <<= 2;
		u32DepthRes >>= 1;
		u32TailHwReserved >>= 1;
		i64MaxMemPerChan >>= 1;
		i32MinSegmentSize >>= 1;
	}
	else	// if ( pAcqCfg->u32Mode & CS_MODE_SINGLE )
		i64MaxPreTrigDepth <<= 3;

	// Calculate the Max allowed Segment Size
	i64MaxMemPerChan -= CsGetPostTriggerExtraSamples(pAcqCfg->u32Mode);
	i64MaxSegmentSize = i64MaxMemPerChan - u32TailHwReserved;
	i64MaxDepth = i64MaxSegmentSize;
	if ( ! m_bNucleon )
		i64MaxPreTrigDepth = i64MaxDepth;

	// No pretrigger in histogram mode
	if ( bHistogramEnable )
		i64MaxPreTrigDepth = 0;

	if ( 0 != u8Index )
	{
		// Make sure that Master/Slave system has all the same optionss
		CsRabbitDev* pDevice = m_MasterIndependent->m_Next;
		while( pDevice )
		{
			if ( GetBoardNode()->u32BaseBoardOptions[u8Index] != pDevice->GetBoardNode()->u32BaseBoardOptions[u8Index] )
				return CS_MASTERSLAVE_DISCREPANCY;

			pDevice = pDevice->m_Next;
		}

		if ( GetBoardNode()->u32BaseBoardOptions[u8Index] == 0 )
		{
			if ( u32Coerce )
			{
				pAcqCfg->u32Mode &= ~CS_MODE_USER1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return (i32Status = CS_INVALID_ACQ_MODE);
		}

		if ( IsExpertAVG(u8Index) )
		{	
#define		COBRA_USER_MAXAVG_PRETRIG						64

			m_bMulrecAvgTD = TRUE;
			u32TailHwReserved = 0;		// No tail

			i64MaxSegmentSize	= m_u32MaxHwAvgDepth/(pAcqCfg->u32Mode&CS_MASKED_MODE);
			i64MaxDepth			= i64MaxSegmentSize;

			// Each sample will be 32 bits sample
			i64MaxMemPerChan /= 4;
			i64MaxPreTrigDepth = m_bNucleon ? 0 : COBRA_USER_MAXAVG_PRETRIG;

			 if ( 0 != (pAcqCfg->u32Mode & CS_MODE_DUAL) )
				 i64MaxPreTrigDepth >>= 1;
		}

		m_Stream.bEnabled = IsExpertSTREAM(u8Index);
	}
	else
	{
		if ( pAcqCfg->u32Mode & CS_MODE_SW_AVERAGING )
		{
			i64MaxDepth		= ( pAcqCfg->u32Mode & CS_MODE_SINGLE ) ? MAX_SW_AVERAGING_SEMGENTSIZE : MAX_SW_AVERAGING_SEMGENTSIZE / 2; 
			i64MaxSegmentSize  = ( pAcqCfg->u32Mode & CS_MODE_SINGLE ) ? MAX_SW_AVERAGING_SEMGENTSIZE : MAX_SW_AVERAGING_SEMGENTSIZE / 2; 
			// No pretrigger is allowed in HW averaging
			i64MaxPreTrigDepth	 = 0;
			// Trigger Delay is not supported in HW averaging
			i64MaxTrigDelay		 = 0;
			u32LimitSegmentCount = MAX_SW_AVERAGE_SEGMENTCOUNT;
		}
	}

	if ( pAcqCfg->i64TriggerDelay != 0 )
		i64MaxPreTrigDepth = 0;		// No pretrigger if TriggerDelay is enable

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

	// Validation of Segment Size
	if ( 0 >= pAcqCfg->i64SegmentSize  )
	{
		if (u32Coerce)
		{
			pAcqCfg->i64SegmentSize = CSRB_DEFAULT_SEGMENT_SIZE;
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
	if ( ! m_bOneSampleResolution )
	{
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
	}

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

   // Adjust Segemnt Parameters for Hardware
	u32RealSegmentCount = pAcqCfg->u32SegmentCount;
	i64RealSegmentSize	= pAcqCfg->i64SegmentSize;
	i64RealPostDepth	= pAcqCfg->i64Depth;

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

	if ( pAcqCfg->i64Depth > pAcqCfg->i64SegmentSize )
	{
		if (u32Coerce)
			pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;
		else
			return CS_DEPTH_SIZE_TOO_BIG;
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
	if ( m_bOneSampleResolution )
		u32DepthRes = 1;

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

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( pAcqCfg->u32Mode, &u32RealSegmentCount, &i64RealSegmentSize, &i64RealPostDepth );
	}

	// Validation of segment count
	// In Stream mode, the segment count is ignored. Otherwise, the segment count should be calculated base on available memory
	if ( !m_Stream.bEnabled )
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

	if ( CS_SUCCEEDED(i32Status) && bHistogramEnable )
		MsHistogramActivate( true );
	else
		MsHistogramActivate( false );

	return i32Status;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------	------
int32	CsRabbitDev::CsAutoUpdateFirmware()
{
	int32	i32RetStatus = CS_SUCCESS;
	int32	i32Status = CS_SUCCESS;

	if ( m_bSkipFirmwareCheck )
		return CS_FAIL;

	CsRabbitDev *pCard = (CsRabbitDev*) m_ProcessGlblVars->pFirstUnCfg;

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

		// Close the handle to kernel driver
		pCard->UpdateCardState();
		pCard->UnInitializeDevIoCtl();
		pCard = pCard->m_HWONext;
	}

	return i32RetStatus; 
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsRabbitDev::CombineCardUpdateFirmware()
{
	int32					i32Status = CS_SUCCESS;
	uInt8					*pBuffer;
	char					szText[128];
	uInt32					u32HeaderOffset;
	uInt32					u32ChecksumOffset;
	uInt32					u32Checksum;
	int32					i32FwUpdateStatus[3] = { 0 };
	char					OldFwFlVer[40];
	char					NewFwFlVer[40];


	if ( m_bSkipFirmwareCheck )
		return CS_FAIL;

	if ( (m_hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName )) == 0 )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_OPEN_FILE, m_szCsiFileName );
		return CS_FAIL;
	}

	// check if any expert firmware needs updating. If not don't bother reading the fwl file.
	// if they do need updating and we can't open the file it's an error
	if ( m_PlxData->CsBoard.u8BadBbFirmware & (BAD_EXPERT1_FW | BAD_EXPERT2_FW) )
	{
		if ( (m_hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName )) == 0 )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_OPEN_FILE, m_szFwlFileName );
			if ( (FILE_HANDLE)0 != m_hCsiFile )
				GageCloseFile( m_hCsiFile );
			return CS_FAIL;
		}
	}

	TRACE(DISP_TRACE_LEVEL, "GageAllocateMemoryX\n");
	pBuffer = (uInt8 *) GageAllocateMemoryX( Max(BB_FLASHIMAGE_SIZE, RBT_ADDONFLASH_FWIMAGESIZE) );
	if ( NULL == pBuffer )
	{
		GageCloseFile( m_hCsiFile );
		if ( (FILE_HANDLE)0 != m_hFwlFile )
			GageCloseFile( m_hFwlFile );
		return CS_INSUFFICIENT_RESOURCES;
	}

	//-------------      UPDATE BASEBOARD FIRMWARE  --------------------------

	HeaderElement	HdrElement[3];
	u32HeaderOffset = FIELD_OFFSET(BaseBoardFlashData, ImagesHeader) +
						FIELD_OFFSET(BaseBoardFlashHeader, HdrElement);
	u32ChecksumOffset = FIELD_OFFSET(BaseBoardFlashData, u32ChecksumSignature);

	TRACE(DISP_TRACE_LEVEL, "pCard->m_PlxData->u8BadBbFirmware \n");
	if ( 0 != m_PlxData->CsBoard.u8BadBbFirmware )
	{
		ResetCachedState();

		i32Status = ReadFlash( HdrElement, u32HeaderOffset, sizeof(HdrElement) );

		// Clear valid Checksum
		u32Checksum = 0;
		i32Status = WriteFlashEx( &u32Checksum, u32ChecksumOffset, sizeof(u32Checksum), AccessFirst );

		// Update the primary image
		TRACE(DISP_TRACE_LEVEL, "m_PlxData->u8BadBbFirmware & BAD_DEFAULT_FW\n");
		if ( m_PlxData->CsBoard.u8BadBbFirmware & BAD_DEFAULT_FW )
		{
			// Set the flag so that we know which firmware image we have to reload later. 
			i32FwUpdateStatus[0] = -1;

			// Load new image from file if it is not already in buffer
			RtlZeroMemory( pBuffer, BB_FLASHIMAGE_SIZE);
			GageReadFile(m_hCsiFile, pBuffer, m_BaseBoardCsiEntry.u32ImageSize, &m_BaseBoardCsiEntry.u32ImageOffset);

			// Write Base board FPGA image
			TRACE(DISP_TRACE_LEVEL, "WriteFlashEx( pBuffer, 0, m_BaseBoardCsiEntry.u32ImageSize, AccessMiddle );\n");
			i32Status = WriteFlashEx( pBuffer, 0, m_BaseBoardCsiEntry.u32ImageSize, AccessMiddle );
			if ( CS_SUCCEEDED(i32Status) )
			{
				m_PlxData->CsBoard.u8BadBbFirmware &= ~BAD_DEFAULT_FW;
				HdrElement[0].u32Version = m_BaseBoardCsiEntry.u32FwVersion;
				HdrElement[0].u32FpgaOptions = m_BaseBoardCsiEntry.u32FwOptions;
				i32FwUpdateStatus[0] = CS_SUCCESS;
			}
		}

		// Update the Expert images
		if ( m_hFwlFile )
		{
			for (int i = 0; i < 2; i++)
			{
				if ( m_PlxData->CsBoard.u8BadBbFirmware & (BAD_EXPERT1_FW << i)  )
				{
					// Set the flag so that we know which firmware image we have to reload later. 
					i32FwUpdateStatus[i+1] = -1;

					// Load new image from file if it is not already in buffer
					RtlZeroMemory( pBuffer, BB_FLASHIMAGE_SIZE);
					GageReadFile(m_hFwlFile, pBuffer, m_BaseBoardExpertEntry[i].u32ImageSize, &m_BaseBoardExpertEntry[i].u32ImageOffset);

					// Write Base board FPGA image
					i32Status = WriteFlashEx( pBuffer, BB_FLASHIMAGE_SIZE*(i+1), m_BaseBoardExpertEntry[i].u32ImageSize, AccessMiddle );
					if ( CS_SUCCEEDED(i32Status) )
					{
						m_PlxData->CsBoard.u8BadBbFirmware &= ~(BAD_EXPERT1_FW << i);
						HdrElement[i+1].u32Version = m_BaseBoardExpertEntry[i].u32Version;
						HdrElement[i+1].u32FpgaOptions = m_BaseBoardExpertEntry[i].u32Option;
						i32FwUpdateStatus[i+1] = CS_SUCCESS;
					}
				}
			}
		}

		// Update Header with the new versions
		TRACE(DISP_TRACE_LEVEL, "WriteFlashEx\n");
		i32Status = WriteFlashEx( HdrElement, u32HeaderOffset, sizeof(HdrElement), AccessMiddle );

		// Set valid Checksum
		if ( CS_SUCCEEDED(i32Status) )
		{
			TRACE(DISP_TRACE_LEVEL, "CSI_FWCHECKSUM_VALID\n");
			u32Checksum	= CSI_FWCHECKSUM_VALID;
			i32Status = WriteFlashEx( &u32Checksum, u32ChecksumOffset, sizeof(u32Checksum), AccessLast );
		}

		if ( CS_SUCCEEDED(i32Status) )
		{
			for (int i = 0; i < 3; i++)
			{
				if ( CS_SUCCESS == i32FwUpdateStatus[i] )
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
			UpdateMaxMemorySize( GetBoardNode()->u32BaseBoardHardwareOptions, CSRB_DEFAULT_SAMPLE_SIZE );
			ClearInterrupts();
			if ( !m_pCardState->bAddOnRegistersInitialized )
			{
				GetInitStatus()->Info.u32AddOnPresent = IsAddonBoardPresent();
				if ( GetInitStatus()->Info.u32AddOnPresent )
				{
					InitAddOnRegisters();
				}
			}
		}
		else
		{
			FlashReset();
			//Reset the pointer to the default image
			BaseBoardConfigReset(0);
		}

		// The firmware has been updated. Invalidate the current firmware version.
		for (int i = 0; i < 3; i++)
		{
			if ( 0 != i32FwUpdateStatus[i] )
				GetBoardNode()->u32RawFwVersionB[i] = 0;
		}
	}

	//-------------      UPDATE ADDON FIRMWARE  --------------------------
	if ( m_PlxData->CsBoard.u8BadAddonFirmware )
		i32Status = UpdateAddonFirmware( m_hCsiFile, pBuffer );

	GageCloseFile( m_hCsiFile );

	if ( m_hFwlFile )
		GageCloseFile( m_hFwlFile );

	TRACE(DISP_TRACE_LEVEL, "GageFreeMemoryX\n");
	GageFreeMemoryX( pBuffer );

	TRACE(DISP_TRACE_LEVEL, "Returns\n");
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt16	CsRabbitDev::ConvertToHwChannel( uInt16 u16UserChannel )
{
	// Rabbit cards usings Combine Base board only
	// Swap Hw Channel and User Channel
	if ( 0 != (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL) )
		return CS_CHAN_1;
	else if( u16UserChannel == CS_CHAN_1 )
		 return CS_CHAN_2;
	else
		return CS_CHAN_1;
}

//-----------------------------------------------------------------------------
//	Convert the Hw channel to User channel
//-----------------------------------------------------------------------------
uInt16	CsRabbitDev::ConvertToUserChannel( uInt16 u16HwChannel )
{
	// Rabbit cards usings Combine Base board only
	// Swap Hw Channel and User Channel
	return ConvertToHwChannel( u16HwChannel );
}

//-----------------------------------------------------------------------------
//	Convert the Hw channel of one card to the User Channel of M/S system
//-----------------------------------------------------------------------------
uInt16	CsRabbitDev::ConvertToMsUserChannel( uInt16 u16HwChannel )
{
	 uInt16 u16UserChannel = ConvertToUserChannel( u16HwChannel );
	 return (u16UserChannel + GetBoardNode()->NbAnalogChannel*(m_u8CardIndex-1));
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsRabbitDev::RestoreCalibDacInfo()
{
	if ( m_pCardState->bUseDacCalibFromEEprom )
		RtlCopyMemory( &m_pCardState->DacInfo, &m_pCardState->CalibInfoTable.ChanCalibInfo, sizeof(m_pCardState->DacInfo) );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsRabbitDev::CheckMasterSlaveFirmwareVersion()
{
	int32	i32Status = CS_SUCCESS;

	if ( m_PlxData->CsBoard.u8BadAddonFirmware || m_PlxData->CsBoard.u8BadBbFirmware )
		return CS_INVALID_FW_VERSION;

	if( this != m_pTriggerMaster )
	{
		if ( m_pCardState->VerInfo.BbCpldInfo.u8Reg      != m_pTriggerMaster->m_pCardState->VerInfo.BbCpldInfo.u8Reg       ||
			 m_pCardState->VerInfo.BbFpgaInfo.u32Reg     != m_pTriggerMaster->m_pCardState->VerInfo.BbFpgaInfo.u32Reg      ||
			 m_pCardState->VerInfo.AddonFpgaFwInfo.u32Reg!= m_pTriggerMaster->m_pCardState->VerInfo.AddonFpgaFwInfo.u32Reg ||
			 m_pCardState->VerInfo.AnCpldFwInfo.u32Reg   != m_pTriggerMaster->m_pCardState->VerInfo.AnCpldFwInfo.u32Reg    ||
			 m_pCardState->VerInfo.TrigCpldFwInfo.u32Reg != m_pTriggerMaster->m_pCardState->VerInfo.TrigCpldFwInfo.u32Reg  ||
			 m_pCardState->VerInfo.ConfCpldInfo.u32Reg   != m_pTriggerMaster->m_pCardState->VerInfo.ConfCpldInfo.u32Reg    )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_WARN_FW_DISCREPANCY, "" );
			i32Status = CS_FALSE;
		}

	}

	return i32Status;
}



#ifdef _WINDOWS_
//------------------------------------------------------------------------------
//
//	Parameters on Board initialization.
//  Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

int32	CsRabbitDev::ReadCommonRegistryOnBoardInit()
{
#ifdef _WINDOWS_
	HKEY hKey;
#ifdef _COBRAMAXPCIE_DRV_
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsEcdG8WDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
#else
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsxyG8WDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
#endif
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_bSkipFirmwareCheck ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipCheckFirmware"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipFirmwareCheck = (ul != 0);

		ul = m_bSkipMasterSlaveDetection ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipMasterSlaveDetection"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipMasterSlaveDetection = (ul != 0);

//		ul = m_bTestBridgeEnabled ? 1 : 0;
//		::RegQueryValueEx( hKey, _T("BridgeTestEnable"), NULL, NULL, (LPBYTE)&ul, &DataSize );
//		m_bTestBridgeEnabled = (ul != 0);

		ul = m_bNoConfigReset ? 1 : 0;
		::RegQueryValueEx( hKey, _T("NoConfigReset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bNoConfigReset = (ul != 0);

		ul = m_bClearConfigBootLocation ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ClearCfgBootLocation"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bClearConfigBootLocation = (ul != 0);

		::RegCloseKey( hKey );
	}	
#endif

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32 CsRabbitDev::ReadCommonRegistryOnAcqSystemInit()
{
	HKEY hKey;
#ifdef _COBRAMAXPCIE_DRV_
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsEcdG8WDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
#else
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsxyG8WDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
#endif
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_bZeroTrigAddress ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ZeroTrigAddress"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroTrigAddress = (ul != 0);


		ul = m_bZeroTrigAddrOffset ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ZeroTrigAddrOffset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroTrigAddrOffset = (ul != 0);

		ul = m_bZeroDepthAdjust ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ZeroDepthAdjust"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroDepthAdjust = (ul != 0);

		ul = 0;
		::RegQueryValueEx( hKey, _T("ZeroExtTrigDataPathAdjust"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bZeroExtTrigDataPathAdjust = (ul != 0);

		ul = m_u8TrigSensitivity;
		::RegQueryValueEx( hKey, _T("DTriggerSensitivity"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul <= 255 )
			m_u8TrigSensitivity = (uInt8) ul;
		else
			m_u8TrigSensitivity = 255;

		ul = m_u32DefaultExtClkSkip;
		::RegQueryValueEx( hKey, _T("DefaultExtClkSkip"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32DefaultExtClkSkip = ul;

		ul =  0;
		if ( m_pCardState->ermRoleMode != ermStandAlone )
			::RegQueryValueEx( hKey, _T("AllowPingPongExtClkMs"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bAllowExtClkMs_pp = ul != 0;

		if ( m_pCardState->bLAB11G )
		{
			m_bNeverCalibExtTrig = true;
		}
		else
		{
			ul = m_bNeverCalibExtTrig ? 1 : 0;
			m_u8DebugCalibExtTrig = (uInt8) ul;
			::RegQueryValueEx( hKey, _T("NeverCalibExtTrig"), NULL, NULL, (LPBYTE)&ul, &DataSize );
			m_u8DebugCalibExtTrig = (uInt8) ul;
			m_bNeverCalibExtTrig = (ul != 0);
		}

		ul = 1;
		::RegQueryValueEx( hKey, _T("HwAvg32"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bHwAvg32 = (ul != 0);
		ReadCalibrationSettings( hKey );

		::RegCloseKey( hKey );
	}	
	return CS_SUCCESS;
}

#else
#define DRV_SECTION_NAME      "Cobra"

//------------------------------------------------------------------------------
// Linux only
//	Parameters on Board initialization.
// Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

int32 CsRabbitDev::ReadCommonCfgOnBoardInit()
{
   char szSectionName[16]={0};
   char szDummy[128]={0};
   ULONG	ul(0);
	DWORD	DataSize = sizeof(ul);
   char  szIniFile[256] = {0};
     
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
   // key not support for Linux
	ul = m_bSkipFirmwareCheck ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("SkipCheckFirmware"),  ul,  szIniFile);	
	m_bSkipFirmwareCheck = (ul != 0);          

	ul = m_bSkipMasterSlaveDetection ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("SkipMasterSlaveDetection"),  ul,  szIniFile);	
	m_bSkipMasterSlaveDetection = (ul != 0);   
   
	ul = m_bClearConfigBootLocation ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ClearConfigBootLocation"),  ul,  szIniFile);	
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

int32 CsRabbitDev::ReadCommonCfgOnAcqSystemInit()
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
   
   ul = m_bZeroTrigAddress ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroTrigAddress"),  ul,  szIniFile);	
	m_bZeroTrigAddress = (ul != 0);

	ul = m_bZeroTrigAddrOffset ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroTrigAddrOffset"),  ul,  szIniFile);	
	m_bZeroTrigAddrOffset = (ul != 0);

	ul = m_bZeroDepthAdjust ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroDepthAdjust"),  ul,  szIniFile);	
	m_bZeroDepthAdjust = (ul != 0);

	ul = 0;
   ul = GetCfgKeyInt(szSectionName, _T("ZeroExtTrigDataPathAdjust"),  ul,  szIniFile);	
	m_bZeroExtTrigDataPathAdjust = (ul != 0);

	ul = m_u8TrigSensitivity;
   ul = GetCfgKeyInt(szSectionName, _T("DTriggerSensitivity"),  ul,  szIniFile);	
	if ( ul <= 255 )
		m_u8TrigSensitivity = (uInt8) ul;
	else
		m_u8TrigSensitivity = 255;

	ul = m_u32DefaultExtClkSkip;
   ul = GetCfgKeyInt(szSectionName, _T("DefaultExtClkSkip"),  ul,  szIniFile);	
	m_u32DefaultExtClkSkip = ul;

	ul =  0;
	if ( m_pCardState->ermRoleMode != ermStandAlone )
   ul = GetCfgKeyInt(szSectionName, _T("AllowPingPongExtClkMs"),  ul,  szIniFile);	
	m_bAllowExtClkMs_pp = ul != 0;

	if ( m_pCardState->bLAB11G )
		m_bNeverCalibExtTrig = true;
	else
	{
		ul = m_bNeverCalibExtTrig ? 1 : 0;
		m_u8DebugCalibExtTrig = (uInt8) ul;
      ul = GetCfgKeyInt(szSectionName, _T("NeverCalibExtTrig"),  ul,  szIniFile);	
		m_u8DebugCalibExtTrig = (uInt8) ul;
		m_bNeverCalibExtTrig = (ul != 0);
	}

	ul = 1;
   ul = GetCfgKeyInt(szSectionName, _T("HwAvg32"),  ul,  szIniFile);	
	m_bHwAvg32 = (ul != 0);
   
   /* get Calibration key values */
	ReadCalibrationSettings( szIniFile, szSectionName );
   
	return CS_SUCCESS;
}

int32	CsRabbitDev::ShowCfgKeyInfo()   
{
   printf("---------------------------------\n");
   printf("    Driver Config Setting Infos \n");
   printf("---------------------------------\n");
   printf("    SectionName: [Cobra] \n");
   printf("    List of Cfg keys supported: \n");
   printf("\t%s\n","NoConfigReset");
   printf("\n\t%s\n\n","- Acq config keys: ");
   printf("\t%s\n","ZeroTrigAddress");
   printf("\t%s\n","ZeroTrigAddrOffset");
   printf("\t%s\n","ZeroDepthAdjust");
   printf("\t%s\n","ZeroExtTrigDataPathAdjust");
   printf("\t%s\n","DTriggerSensitivity");
   printf("\t%s\n","DefaultExtClkSkip");
   printf("\t%s\n","DefaultExtClkSkip");
   printf("\t%s\n","AllowPingPongExtClkMs");
   printf("\t%s\n","NeverCalibExtTrig");
   printf("\t%s\n","HwAvg32");

   printf("\n\t%s\n\n","- Calibration config keys:");

   printf("\t%s\n","FastDcCal");
   printf("\t%s\n","NoCalibrationDelay");
   printf("\t%s\n","NeverCalibrateDc");
   printf("\t%s\n","UseDacCalibFromEeprom");
   printf("\t%s\n","CalDacSettleDelay");
   printf("\t%s\n","CalAdcOffset");
   printf("\t%s\n","IgnoreCalibError");
   printf("\t%s\n","NeverCalibrateMs");
   printf("\t%s\t\n","DebugCalSrc");
   printf("\t%s\n","NeverValidateExtClk");
   printf("\t%s\n","TraceCalib");
   printf("---------------------------------\n");
   
}   
#endif   

#ifdef USE_XML_REGISTRY

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

inline void	QueryBackDoorParams(XMLElement* pElement, char *szName, uInt32 *u32Value)
{
	XMLVariable* pDbValue = pElement->FindVariableZ(szName);
	if ( pDbValue )
		*u32Value = pDbValue->GetValueInt();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsRabbitDev::ReadCommonRegistryOnAcqSystemInit()
{
	XML		*pSystem = new XML("E:\\CsxyG8Debug.xml");

	if (!pSystem)
		return 0;

	XMLElement* pRoot = pSystem->GetRootElement();
	int nChildren = pRoot->GetChildrenNum();

	XMLElement* pElement = pRoot->GetChildren()[0];

	ULONG ul(0);
	::QueryBackDoorParams(pElement, "NeverCalibrateDc", &ul);
	m_bNeverCalibrateDc = (ul != 0);

	ul = 0;
	::QueryBackDoorParams(pElement, "ZeroTrigAddress", &ul);
	m_bZeroTrigAddress = (ul != 0);

	ul = m_bZeroTrigAddrOffset ? 1UL : 0;
	::QueryBackDoorParams(pElement, "ZeroTrigAddrOffset", &ul);
	m_bZeroTrigAddrOffset = (ul != 0);

	ul = m_bZeroDepthAdjust ? 1UL : 0;
	::QueryBackDoorParams(pElement, "ZeroDepthAdjust", &ul);
	m_bZeroDepthAdjust = (ul != 0);

	ul = 0;
	::QueryBackDoorParams(pElement, "ZeroExtTrigDataPathAdjust", &ul);
	m_bZeroExtTrigDataPathAdjust = (ul != 0);

	ul = m_pCardState->u8TrigSensitivity;
	::QueryBackDoorParams(pElement, "DTriggerSensitivity", &ul);
	if ( ul <= 100 )
		m_pCardState->u8TrigSensitivity = (uInt8) ul;
	else
		m_pCardState->u8TrigSensitivity = 100;

	ul = m_bExtClkOverClocking ? 1 : 0;
	::QueryBackDoorParams(pElement, "ExtClkOverClocking", &ul);
	m_bExtClkOverClocking = ul != 0;

	ul = m_bExtClkNeverPingpong ? 1 : 0;
	::QueryBackDoorParams(pElement, "ExtClkNoPingpong", &ul);
	m_bExtClkNeverPingpong = ul != 0;


	ul = m_bNeverCalibExtTrig ? 1 : 0;
	::QueryBackDoorParams(pElement, "NeverCalibExtTrig", &ul);
	m_bNeverCalibExtTrig = ul != 0;

	ul = (ULONG) 0;
	::QueryBackDoorParams(pElement, "NoAdjExtTrigFail", &ul);
	m_bNoAdjExtTrigFail = ul != 0;



	ul = m_bSkipCalib ? 1 : 0;
	::QueryBackDoorParams(pElement, "FastDcCal", &ul);
	m_bSkipCalib = 0 != ul;

	ul = m_u16SkipCalibDelay;
	::QueryBackDoorParams(pElement, "NoCalibrationDelay", &ul);

	if(ul > 24 * 60 ) //limitted to 1 day
		ul = 24*60;
	m_u16SkipCalibDelay = (uInt16)ul;

	ul = m_bNeverCalibrateDc ? 1 : 0;
	::QueryBackDoorParams(pElement, "NeverCalibrateDc", &ul);
	m_bNeverCalibrateDc = 0 != ul;

	ul = m_pCardState->bUseDacCalibFromEEprom ? 1 : 0;
	::QueryBackDoorParams(pElement, "UseDacCalibFromEeprom", &ul);
	m_pCardState->bUseDacCalibFromEEprom = 0 != ul;

	ul = m_pCardState->u8CalDacSettleDelay;
	::QueryBackDoorParams(pElement, "CalDacSettleDelay", &ul);
	m_pCardState->u8CalDacSettleDelay = (uInt8)ul;

	ul = (uInt32)m_i32CalAdcOffset_uV;
	::QueryBackDoorParams(pElement, "CalAdcOffset", &ul);
	m_i32CalAdcOffset_uV = (int32)ul;

	ul = m_u32IgnoreCalibError;
	::QueryBackDoorParams(pElement, "IgnoreCalibError", &ul);
	m_u32IgnoreCalibError = ul;

	ul = m_bNeverCalibrateMs ? 1 : 0;
	::QueryBackDoorParams(pElement, "NeverCalibrateMs", &ul);
	m_bNeverCalibrateMs = 0 != ul;

	ul = m_bDebugCalibSrc ? 1 : 0;
	::QueryBackDoorParams(pElement, "DebugCalSrc", &ul);
	m_bDebugCalibSrc = 0 != ul;

#if DBG
	ul = m_u32DebugCalibTrace;
	::QueryBackDoorParams(pElement, "TraceCalib", &ul);
	m_u32DebugCalibTrace = ul;
#endif

	return CS_SUCCESS;
}

#endif

//-----------------------------------------------------------------------------
//-----	SetPulseOut
//----- ADD-ON REGISTER
//----- Address	CSRB_OPTION_SET_WR_CMD
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetTrigOut( PCS_TRIG_OUT_STRUCT pTrigOut )
{
	int32 i32Ret = CS_SUCCESS;
	CS_TRIG_OUT_STRUCT	TrigOut;
	if( NULL == pTrigOut )
	{
		TrigOut.u16Value = CS_OUT_NONE;
		TrigOut.u16Mode = CS_TRIGOUT_MODE;
	}
	else
		TrigOut = *pTrigOut;

	if( CS_TRIGOUT_MODE == TrigOut.u16Mode )
	{
		m_pCardState->u16TrigOutMode = CS_TRIGOUT_MODE;
		uInt16 u16DataExtTrig = m_pCardState->AddOnReg.u16ExtTrig;
		u16DataExtTrig &= ~CSRB_SET_EXT_TRIG_TRIGOUTEN;
		m_pCardState->eteTrigEnable		 = eteAlways;
		if( CS_TRIGOUT == TrigOut.u16Value )
		{
			m_pCardState->bDisableTrigOut = false;
			u16DataExtTrig |= CSRB_SET_EXT_TRIG_TRIGOUTEN;
		}
		else
		{
			m_pCardState->bDisableTrigOut = true;
		}

		i32Ret = SendTriggerEnableSetting( m_pCardState->eteTrigEnable );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(u16DataExtTrig, CSRB_SET_EXT_TRIG);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->AddOnReg.u16ExtTrig = u16DataExtTrig;
	}
	else if( CS_EXTTRIG_EN_MODE == TrigOut.u16Mode )
	{
		m_pCardState->u16TrigOutMode = CS_EXTTRIG_EN_MODE;
		uInt16 u16DataExtTrig = m_pCardState->AddOnReg.u16ExtTrig;
		u16DataExtTrig &= ~CSRB_SET_EXT_TRIG_TRIGOUTEN;
		m_pCardState->bDisableTrigOut = true;

		m_pCardState->eteTrigEnable = eTrigEnableMode(TrigOut.u16Value);
		i32Ret = SendTriggerEnableSetting( m_pCardState->eteTrigEnable );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(u16DataExtTrig, CSRB_SET_EXT_TRIG);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->AddOnReg.u16ExtTrig = u16DataExtTrig;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsRabbitDev::GetTrigOut(void)
{
	if( CS_TRIGOUT_MODE == m_pCardState->u16TrigOutMode )
		return m_pCardState->bDisableTrigOut ? CS_OUT_NONE : CS_TRIGOUT;
	else if( CS_EXTTRIG_EN_MODE == m_pCardState->u16TrigOutMode )
		return (uInt16) m_pCardState->eteTrigEnable;
	else
		return CS_OUT_NONE;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsRabbitDev::GetValidTrigOutMode(void)
{
	if ( m_pCardState->bLAB11G )
		return CS_TRIGOUT_MODE;
	else
		return CS_TRIGOUT_MODE|CS_EXTTRIG_EN_MODE;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsRabbitDev::GetValidTrigOutValue(uInt16 u16Mode)
{
	if( u16Mode == CS_TRIGOUT_MODE )
		return CS_OUT_NONE|CS_TRIGOUT;
	else if( u16Mode == CS_EXTTRIG_EN_MODE )
		return CS_OUT_NONE|CS_EXTTRIGEN_LIVE|CS_EXTTRIGEN_POS_LATCH|CS_EXTTRIGEN_NEG_LATCH|CS_EXTTRIGEN_POS_LATCH_ONCE|CS_EXTTRIGEN_NEG_LATCH_ONCE;
	else
		return 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsRabbitDev::SetTriggerAddressOffsetAdjust( int8 TrigAddressOffset )
{
	if ( 0 != m_pCardState->AcqConfig.u32ExtClk )
	{
		SetExtClkTriggerAddressOffsetAdjust( TrigAddressOffset );
		return;
	}

	uInt8	u8IndexMode = ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0) ? 0 : 1;
	uInt32	u32IndexSR = 0;
	FindSRIndex( m_pCardState->AcqConfig.i64SampleRate, &u32IndexSR );
	m_MasterIndependent->m_pCardState->CalibInfoTable.i8TrigAddrOffsetAdjust[u32IndexSR][u8IndexMode] = TrigAddressOffset;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int8	CsRabbitDev::GetExtClkTriggerAddressOffsetAdjust()
{
	if ( m_bZeroTrigAddrOffset )
		return 0;

	int8	i8TrigAddr = 0;
	uInt8	u8Column = ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0) ? 0 : 1;
	uInt8	u8Row	= (m_pCardState->AddOnReg.u16PingPong != 0) ? 0 : 1;

	// M/S
	if ( NULL != m_MasterIndependent->m_Next )
		u8Row += 2;

	i8TrigAddr = m_MasterIndependent->m_pCardState->CalibInfoTable.i8ExtClkTrigAddrOffAdj[u8Row][u8Column];

	int8 i8Div = int8(m_pCardState->AcqConfig.u32ExtClkSampleSkip/CS_SAMPLESKIP_FACTOR);
	if( 0 == i8Div ) i8Div = 1;
	i8TrigAddr /= i8Div;

	return i8TrigAddr;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsRabbitDev::SetExtClkTriggerAddressOffsetAdjust( int8 i8TrigAddressOffset )
{
	uInt8 u8Column = ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0) ? 0 : 1;
	uInt8 u8Row	= (m_pCardState->AddOnReg.u16PingPong != 0) ? 0 : 1;
	if ( NULL !=  m_MasterIndependent->m_Next )
		u8Row += 2;
	int8 i8Div = int8(m_pCardState->AcqConfig.u32ExtClkSampleSkip/CS_SAMPLESKIP_FACTOR);
	if( 0 == i8Div ) i8Div = 1;
	i8TrigAddressOffset *= i8Div;
	m_MasterIndependent->m_pCardState->CalibInfoTable.i8ExtClkTrigAddrOffAdj[u8Row][u8Column] = i8TrigAddressOffset;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetRoleMode( eRoleMode ermSet )
{
	int32 i32Ret = SetClockOut(ermSet, m_pCardState->eclkoClockOut);
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->ermRoleMode = ermSet;
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool	CsRabbitDev::IsExtTriggerEnable( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;

	// Detect if one of trigger sources is ext trigger
	for ( uInt32 i = 0; i < pATrigCfg->u32TriggerCount; i++ )
	{
		if ( pTrigCfg[i].i32Source < 0 )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsRabbitDev::UpdateCardState(BOOLEAN bReset)
{
	m_pCardState->bCardStateValid = !bReset;

#if !defined( USE_SHARED_CARDSTATE )
	IoctlSetCardState(m_pCardState);
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsRabbitDev::SetDefaultAuxIoCfg()
{
	m_pCardState->eclkoClockOut		= CSRB_DEFAULT_CLKOUT;
	m_pCardState->eteTrigEnable		= CSRB_DEFAULT_EXTTRIGEN;
	m_pCardState->u16TrigOutMode		= CS_TRIGOUT_MODE;
	m_pCardState->bDisableTrigOut		= TRUE;
	m_pCardState->eteTrigEnable		= eteAlways;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool	CsRabbitDev::IsImageValid( uInt8 u8Index )
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

int32 	CsRabbitDev::CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSize )
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
	u32TransferSize = (u32TransferSize/CSRB_TRIGGER_RES)*CSRB_TRIGGER_RES;

	if ( m_Stream.bFifoFull )
	{
		// Fifo overflow has occured. Allow only to transfer valid data
		if ( m_Stream.i64DataInFifo > 0 )
		{
			if ( m_Stream.i64DataInFifo < u32TransferSize )
				u32TransferSize = (uInt32) m_Stream.i64DataInFifo;
		}
		else
			i32Status = CS_STM_FIFO_OVERFLOW;
	}
	else if ( ! m_Stream.bRunForever )
	{	
		if ( m_Stream.u64DataRemain < u32TransferSize )
			u32TransferSize = (uInt32) m_Stream.u64DataRemain;
	}

	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Only one DMA can be active 
	if ( ::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 1, 0 ) ) 
		return CS_INVALID_STATE;

	m_Stream.u32TransferSize = u32TransferSize;
	i32Status = IoctlStmTransferDmaBuffer( m_Stream.u32TransferSize*m_pCardState->AcqConfig.u32SampleSize, pVa );

	if ( CS_FAILED(i32Status) )
		::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 0, 1 );

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsRabbitDev::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData )
{
	if ( ! m_Stream.bEnabled )
		return CS_INVALID_REQUEST;

	int32 i32Status = CsStmGetTransferStatus( u32TimeoutMs, pu32ErrorFlag );
	if (CS_SUCCESS == i32Status)
	{
		m_Stream.u64DataRemain -= m_Stream.u32TransferSize;
		if ( pu32ActualSize )
			*pu32ActualSize		= m_Stream.u32TransferSize;		// Return actual DMA data size

		if ( pu8EndOfData )
			*pu8EndOfData	= ( m_Stream.u64DataRemain == 0 );
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsRabbitDev::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag )
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
				if ( ! m_Stream.bRunForever && m_Stream.u64DataRemain <= (ULONGLONG) m_PlxData->CsBoard.i64MaxMemory )
					m_Stream.i64DataInFifo = m_Stream.u64DataRemain;
				else
				m_Stream.i64DataInFifo = m_PlxData->CsBoard.i64MaxMemory;

				if ( m_Stream.i64DataInFifo > m_Stream.u32TransferSize )
				m_Stream.i64DataInFifo -= m_Stream.u32TransferSize;
				else
					m_Stream.i64DataInFifo = 0;
			}
		}
		else
		{
			// Fifo full has occured. Return error if there is no more valid data to transfer.
			if ( m_Stream.i64DataInFifo > m_Stream.u32TransferSize )
				m_Stream.i64DataInFifo -= m_Stream.u32TransferSize;
			else
				m_Stream.i64DataInFifo = 0;
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

void 	CsRabbitDev::MsStmResetParams()
{
	CsRabbitDev *pDev = m_MasterIndependent;
	
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

bool CsRabbitDev::IsConfigResetRequired( uInt8 u8ExpertImageId )
{
	// For Nucleon PCIe, Streaming is supported by the default firmware. We do not have to change to the expert firmware for streaming 
	// unless the expert firmware version is different to the default one.
	// For other expert, switching to expert firmware is required.
	if ( ( m_PlxData->CsBoard.u32BaseBoardOptions[u8ExpertImageId] & CS_BBOPTIONS_STREAM ) == 0 )
		return true;
	else 
		return ( m_PlxData->CsBoard.u32UserFwVersionB[0].u32Reg != m_PlxData->CsBoard.u32UserFwVersionB[u8ExpertImageId].u32Reg );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::MsInitializeDevIoCtl()
{
	int32		 i32Status = CS_SUCCESS;
	CsRabbitDev* pDevice = m_MasterIndependent;

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

bool CsRabbitDev::IsExpertAVG(uInt8 u8ImageId)
{
	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & CS_BBOPTIONS_MULREC_AVG_TD) != 0 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsRabbitDev::IsExpertSTREAM(uInt8 u8ImageId)
{
	if ( 0 == u8ImageId )
		u8ImageId = m_pCardState->u8ImageId;

	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & CS_BBOPTIONS_STREAM) != 0 );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsRabbitDev::PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg )
{
	int64 i64MaxSegment = MAX_HW_DEPTH_COUNTER;

	i64MaxSegment  *= 16;
	i64MaxSegment  /= (pCsAcqCfg->u32Mode & CS_MASKED_MODE);

	int64	i64SegmentCount = pCsAcqCfg->i32SegmentCountHigh;
	i64SegmentCount = i64SegmentCount << 32 | pCsAcqCfg->u32SegmentCount;

	m_Stream.bInfiniteSegmentCount	= ( i64SegmentCount < 0 );
	m_Stream.bRunForever			= m_Stream.bInfiniteDepth || m_Stream.bInfiniteSegmentCount;

	if ( m_Stream.bRunForever )
		m_Stream.u64TotalDataSize	= (ULONGLONG) -1;
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
void	CsRabbitDev::NucleonGetMemorySize()
{
	uInt32	u32DimmSize = ReadRegisterFromNios(0, CSPLXBASE_GET_MEMORYSIZE_CMD);
	uInt32	u32LimitRamSize = m_PlxData->u32NvRamSize;

	m_PlxData->CsBoard.i64MaxMemory = u32DimmSize;
	m_PlxData->CsBoard.i64MaxMemory = ( u32LimitRamSize < u32DimmSize ) ? u32LimitRamSize : u32DimmSize;
	m_PlxData->CsBoard.i64MaxMemory *= 1024;				//in Samples
	m_PlxData->InitStatus.Info.u32BbMemTest = m_PlxData->CsBoard.i64MaxMemory != 0 ? 1 : 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool	CsRabbitDev::IsHistogramEnabled()
{
	// Make sure that Master/Slave system supports HISTOGRAM
	CsRabbitDev*	pDevice = m_MasterIndependent;
	bool			bHistogramOk = true;

	while( pDevice )
	{
		if ( 0 == (pDevice->m_PlxData->CsBoard.i64ExpertPermissions & CS_AOPTIONS_HISTOGRAM) )
		{
			bHistogramOk = false;
			break;
		}
		pDevice = pDevice->m_Next;
	}

	return bHistogramOk;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsRabbitDev::MsHistogramActivate( bool bEnable )
{
	// Make sure that Master/Slave system supports HISTOGRAM
	CsRabbitDev*	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		pDevice->m_bHistogram = bEnable;
		pDevice = pDevice->m_Next;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsRabbitDev::ReadConfigBootLocation()
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
int32	CsRabbitDev::WriteConfigBootLocation( uInt32	u32Location )
{
	char	szText[256];

   fprintf(stdout, "WriteConfigBootLocation(%d) do nothing ... \n", u32Location);
   return CS_SUCCESS;

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

