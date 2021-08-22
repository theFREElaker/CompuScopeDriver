

#include "StdAfx.h"
#include "CsTypes.h"
#include "CsIoctl.h"
#include "CsDrvDefines.h"
#include "CsHexagon.h"
#include "xml.h"

#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif

extern SHAREDDLLMEM Globals;

int32	CsHexagon::DeviceInitialize(bool bForce)
{
	int32 i32Ret = CS_SUCCESS;

#if !defined(USE_SHARED_CARDSTATE)
	GetCardState();
#endif 

	i32Ret = CsPlxBase::Initialized();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	ReadCommonRegistryOnBoardInit();

	if ( ! m_pCardState->bCardStateValid || bForce )
	{
		// Re-initalize the whole structure, clear all the previous information
		RtlZeroMemory(&m_PlxData->CsBoard, sizeof(m_PlxData->CsBoard));
		// Reset the firmware version variables before reading firmware version
		RtlZeroMemory( &m_pCardState->VerInfo, sizeof(m_pCardState->VerInfo) );
		m_pCardState->VerInfo.u32Size = sizeof(CS_FW_VER_INFO);

		m_pCardState->u16CardNumber = 1;
		ResetCachedState();

  		// Detect Virgin board
		// Simplified method but not 100% reliable.
		m_pCardState->bVirginBoard = ((-1 == ReadConfigBootLocation()) && (false == IsBootLocationValid(0)));
		//if ( m_pCardState->bVirginBoard )
		//	m_EventLog.Report( CS_WARNING_TEXT, "Virgin board detected." );    
  
		DetectBootLocation();

		m_PlxData->InitStatus.Info.u32Nios = 1;

		// Update pointers for SR tables in shared mem
		SetDefaultInterrutps();
		SetDefaultAuxIoCfg();
		ReadHardwareInfoFromFlash();
		ReadIoConfigFromFlash();
		AssignBoardType();
		InitBoardCaps();
		InitAddOnRegisters();

		GetMemorySize();

		ReadVersionInfo();
		m_pCardState->bCardStateValid = TRUE;
		m_pCardState->bPresetDCLevelFreeze = TRUE;
		
	}

	UpdateCardState();
   
	// Intentional return success to allows some utility programs to update firmware in case of errors
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsHexagon::DetectBootLocation()
{
	char szText[256];

	// Assume that we boot by the safe Boot image
	m_pCardState->i32CurrentBootLocation = 0;
	m_pCardState->bSafeBootImageActive = TRUE;
	sprintf_s( szText, sizeof(szText), TEXT("Current boot location: Safe Boot"));

	m_pCardState->i32ConfigBootLocation = GetConfigBootLocation();

	if (0 == m_pCardState->i32ConfigBootLocation)
	{
		// Auto repair. If the config boot location is invalid, set the location 1 (regular firmware) by default
		SetConfigBootLocation(1);
	}
	else
	{
		// Check the last Firmware page loaded
		uInt32 u32RegVal = ReadGioCpld(6);
		if ( (u32RegVal & 0x40) == 0 )
		{
			// Succeeded boot at the configrued location 
			m_pCardState->i32CurrentBootLocation = m_pCardState->i32ConfigBootLocation;
			m_pCardState->bSafeBootImageActive = FALSE;
			sprintf_s( szText, sizeof(szText), TEXT("Current boot location: %d"),  m_pCardState->i32CurrentBootLocation-1 );
		}
	}

	m_EventLog.Report( CS_INFO_TEXT, szText );
	OutputDebugString( szText );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsHexagon::MsCsResetDefaultParams( BOOLEAN bReset )	
{
	int32		i32Ret = CS_SUCCESS;

	GetCardState();
	SetDefaultInterrutps();

	// Clear all cache state and force reconfig the hardware
	if ( bReset )
		ResetCachedState();

	// Set Default configuration
	m_bMulrecAvgTD		= false;
	m_Stream.bEnabled	= false;

	i32Ret = CsSetAcquisitionConfig();
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;
	
	RtlZeroMemory( m_CfgTrigParams, sizeof( m_CfgTrigParams ) );

	// Restore Calibration DacInfo in case they have been altered by calibration program
	RestoreCalibDacInfo();

	if(!GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	i32Ret = InitAddOnRegisters(bReset==TRUE);
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	i32Ret = CsSetChannelConfig();
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	i32Ret = CsSetTriggerConfig();
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	CsSetDataFormat();

	i32Ret = CalibrateAllChannels();
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;

	UpdateCardState();
   
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsHexagon::AcquireData(uInt32 u32IntrEnabled /* default params u32IntrEnabled = OVER_VOLTAGE_INTR */)
{
	int32 i32Status = CS_SUCCESS;

	// Send startacquisition command
	in_STARTACQUISITION_PARAMS AcqParams = {0};

	if ( ! m_bCalibActive )
	{
		// If Stream mode is enable, we can send start acquisition in either STREAM mode or REGULAR mode .
		// If we are in calibration mode then we have to capture data in REGULAR mode.
		// Use the u32Params1 to let the kernel driver know what mode we want to capture.
		if ( m_Stream.bEnabled )
		{
			AcqParams.u32Param1 = AcqCmdStreaming;
		}
		else
		{
			if ( m_bMulrecAvgTD )
				AcqParams.u32Param1 = AcqCmdMemory_Average;
			else
				AcqParams.u32Param1 = AcqCmdMemory;
		}
	}
	else
		AcqParams.u32Param1 = AcqCmdMemory;

	AcqParams.u32IntrMask = u32IntrEnabled;
	i32Status = IoctlDrvStartAcquisition( &AcqParams );

	return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsHexagon::MsAbort()
{
	CsHexagon* pCard = m_MasterIndependent;

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
//----- CsHexagon::UpdateSystemInfo(PSYSTEM_NODE pSys)
//-----------------------------------------------------------------------------

void	CsHexagon::UpdateSystemInfo( CSSYSTEMINFO *SysInfo )
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
	SysInfo->u32BaseBoardOptions	= pBdParam->u32BaseBoardOptions[m_pCardState->i32ConfigBootLocation];
	SysInfo->u32AddonOptions		= pBdParam->u32ActiveAddonOptions;

	SysInfo->u32ChannelCount = pBdParam->NbAnalogChannel * SysInfo->u32BoardCount;

	AssignAcqSystemBoardName( SysInfo );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsHexagon::AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo )
{
	if ( CS_DEVID_HEXAGON_PXI == GetDeviceId() || CS_DEVID_HEXAGON_A3_PXI == GetDeviceId())
		strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"CSX16" );
	else
		strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"CSE16" );

	switch( ~(CSHEXAGON_PXI_BOARDTYPE_MASK | CSHEXAGON_LOWRANGE_BOARDTYPE_MASK) & GetBoardNode()->u32BoardType )
	{
		case CSHEXAGON_16504_BOARDTYPE:
			strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"504" );
			break;
		case CSHEXAGON_16502_BOARDTYPE:
			strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"502" );
			break;
		case CSHEXAGON_161G2_BOARDTYPE:
			strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"1G2" );
			break;
		default:
			strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"1G4" );
	}

	// m_bLowRange_mV
	if (IsBoardLowRange())
    {
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"-LR");					//HEXAGON_LOWRANGE_EXT_NAME
     }

}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsHexagon::WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid )
{
	UNREFERENCED_PARAMETER(u8CalibTableId);
	UNREFERENCED_PARAMETER(u32Valid);
	return CS_FUNCTION_NOT_SUPPORTED;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsHexagon::ReadVersionInfo()
{
	uInt32	u32BootIndex = (uInt32) m_pCardState->i32CurrentBootLocation;

	//if ( 0 == m_pCardState->VerInfo.BbCpldInfo.u8Reg )
	{
		// Read base board CPLD version
		uInt16 u16CpldVer = (uInt16) ReadCpldRegister(8);

		// Patch OLD structure for CPLD version is only 8-bit,
		// Hexagon NEW CPLD version is 16 bit 
		// Convert NEW to OLD .
		m_pCardState->VerInfo.BbCpldInfo.u8Reg = (u16CpldVer & 0xF) | (u16CpldVer >> 4 & 0xF0);
	}

//	if ( 0 == GetBoardNode()->u32RawFwVersionB[u32BootIndex] )
	{
		// Read base board Firmware version
		PCS_FW_VER_INFO pFwVerInfo = &m_pCardState->VerInfo;
		uInt32 u32Data	= 0;

#ifdef _DEBUG
		char szText[128];

		u32Data = ReadGageRegister( HEXAGON_HWR_RD_CMD );	
		sprintf_s( szText, sizeof(szText), "Firmware HW version: %02d.%02d",((u32Data) >> 6) & 0xFF,(u32Data & 0x3F) );
		u32Data = 0x7FFF & ReadGageRegister( HEXAGON_SWR_RD_CMD );	
		sprintf_s( szText, sizeof(szText), "Firmware SW version: %02d.%02d",((u32Data) >> 9) & 0x1F,(u32Data & 0x1FF) );
#endif

		u32Data = 0x7FFF & ReadGageRegister( HEXAGON_HWR_RD_CMD );	
		u32Data <<= 16;
		u32Data |= (0x7FFF & ReadGageRegister( HEXAGON_SWR_RD_CMD ));
		pFwVerInfo->BbFpgaInfo.u32Reg = m_PlxData->CsBoard.u32RawFwVersionB[u32BootIndex] = u32Data; 
		pFwVerInfo->u32BbFpgaOptions = 0xFF & ReadGageRegister( HEXAGON_OPT_RD_CMD );

		// Build Csi and user version info
		pFwVerInfo->BaseBoardFwVersion.Version.uMajor	= pFwVerInfo->BbFpgaInfo.Version.uMajor;
		pFwVerInfo->BaseBoardFwVersion.Version.uMinor	= pFwVerInfo->BbFpgaInfo.Version.uMinor;
		pFwVerInfo->BaseBoardFwVersion.Version.uRev		= pFwVerInfo->u32BbFpgaOptions;
		pFwVerInfo->BaseBoardFwVersion.Version.uIncRev	= (64*pFwVerInfo->BbFpgaInfo.Version.uRev + pFwVerInfo->BbFpgaInfo.Version.uIncRev);

		GetBoardNode()->u32UserFwVersionB[u32BootIndex].u32Reg		= m_pCardState->VerInfo.BaseBoardFwVersion.u32Reg;
	}

//	if ( m_pCardState->bAddOnRegistersInitialized && 0 == m_pCardState->VerInfo.ConfCpldInfo.u32Reg )
	{
		// Read Addon CPLD Version
		PCS_FW_VER_INFO				pFwVerInfo = &m_pCardState->VerInfo;
		HEXAGON_ADDON_CPLD_VERSION	AddonCpldVersion = {0};

		AddonCpldVersion.u16RegVal = (uInt16) ReadRegisterFromNios(0, HEXAGON_ADDON_CPLD_FW_VERSION);

		// Converting version number from FW structure to SW user structure
		pFwVerInfo->ConfCpldInfo.Info.uMajor	= AddonCpldVersion.bits.Version;
		pFwVerInfo->ConfCpldInfo.Info.uMinor	= AddonCpldVersion.bits.Major;
		pFwVerInfo->ConfCpldInfo.Info.uRev		= AddonCpldVersion.bits.Minor;
	}

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsHexagon::BuildIndependantSystem( CsHexagon** FirstCard, BOOLEAN bForceInd )
{
	CsHexagon*	pCard	= (CsHexagon*) m_ProcessGlblVars->pFirstUnCfg;
	*FirstCard = NULL;

	UNREFERENCED_PARAMETER(bForceInd);

	if( NULL != pCard )
	{
		pCard->GetCardState();

		pCard->m_MasterIndependent = pCard;
		pCard->m_pTriggerMaster = pCard;

		//remove card from unconfigured list
		pCard->RemoveDeviceFromUnCfgList();
		
	
		pCard->m_pCardState->u16CardNumber = 1;
		pCard->m_Next = NULL;

		*FirstCard = pCard;
		pCard->UpdateCardState();  
		return 1;
	}

	return 0;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsHexagon::RemoveDeviceFromUnCfgList()
{
	CsHexagon	*Current	= (CsHexagon	*) m_ProcessGlblVars->pFirstUnCfg;
	CsHexagon	*Prev = NULL;

	if ( this  == (CsHexagon	*) m_ProcessGlblVars->pFirstUnCfg )
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

	if ( this  == (CsHexagon	*) m_ProcessGlblVars->pLastUnCfg )
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

uInt16	CsHexagon::DetectAcquisitionSystems( BOOL bForceInd )
{
	CsHexagon	*pDevice = (CsHexagon *) m_ProcessGlblVars->pFirstUnCfg;
	CsHexagon	*pFirstCardInSystem[20];

	while( pDevice )
	{
		// Undo M/S link
		pDevice->m_Next =  pDevice->m_HWONext;
		if ( NULL == pDevice->m_Next )
		{
			m_ProcessGlblVars->pLastUnCfg = pDevice;
		}

		pDevice = pDevice->m_HWONext;
	}

	uInt32			i = 0;
	uInt16			NbCardInSystem;
	CsHexagon		*FirstCard = NULL;
	DRVSYSTEM		*pSystem;
	CSSYSTEMINFO	*SysInfo;

	PCS_BOARD_NODE  pBdParam = &m_PlxData->CsBoard;

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

		pDevice = (CsHexagon *) pFirstCardInSystem[i];
		j = 0;

		memset(pSystem->u8CardIndexList, 0, sizeof(pSystem->u8CardIndexList));
		while( pDevice )
		{
			// Set M/S card index list
			pSystem->u8CardIndexList[j++] = pDevice->m_u8CardIndex;

			pDevice->m_pSystem = pSystem;
			pDevice = pDevice->m_Next;
		}

		pDevice = (CsHexagon *) pFirstCardInSystem[i];
		SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		pSystem->u16AcqStatus = ACQ_STATUS_READY;
	}

	return m_ProcessGlblVars->NbSystem;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
uInt32	CsHexagon:: GetModeSelect( uInt16 u16UserChannel )
{
	uInt32	u32ModeSel = 0;

	uInt16 u16Channel = ConvertToHwChannel( u16UserChannel );

	u32ModeSel = (u16Channel - 1) << 4;

	switch (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE)
	{
	case CS_MODE_SINGLE:
		break;
	case CS_MODE_DUAL:
		u32ModeSel |= MODESEL1;
		break;
	case CS_MODE_QUAD:
	default:
		u32ModeSel |= MODESEL2;
	}

	return u32ModeSel;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
uInt32	CsHexagon:: GetDataMaskModeTransfer ( uInt32 u32TxMode )
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
//
//-----------------------------------------------------------------------------

CsHexagon *CsHexagon::GetCardPointerFromBoardIndex( uInt16 BoardIndex )
{
	CsHexagon *pDevice = m_MasterIndependent;

	if ( 0 != pDevice->m_Next )
	{
		while ( pDevice )
		{
			if (pDevice->m_pCardState->u16CardNumber == BoardIndex)
				break;
			pDevice = pDevice->m_Next;
		}
	}

	return pDevice;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsHexagon::NormalizedChannelIndex( uInt16 u16ChannelIndex )
{
	ASSERT( u16ChannelIndex != 0 );

	uInt16 Temp = (u16ChannelIndex - 1)  / m_PlxData->CsBoard.NbAnalogChannel;
	return (u16ChannelIndex - Temp * m_PlxData->CsBoard.NbAnalogChannel);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsHexagon::NormalizedTriggerSource( int32 i32TriggerSource )
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

void CsHexagon::CsSetDataFormat( _CSDATAFORMAT csFormat )
{
	switch ( csFormat )
	{
		case formatSoftwareAverage:
			m_pCardState->AcqConfig.u32SampleSize	= CS_SAMPLE_SIZE_4;
			m_pCardState->AcqConfig.i32SampleOffset	= CS_SAMPLE_OFFSET_16_LJ;
			m_pCardState->AcqConfig.i32SampleRes	= -CS_SAMPLE_RES_16;
			break;

		case formatHwAverage:
			// Full resolution, 32 bit data
			m_pCardState->AcqConfig.u32SampleSize	= CS_SAMPLE_SIZE_4;
			m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_16_LJ;
			m_pCardState->AcqConfig.i32SampleRes	= -CS_SAMPLE_RES_16;
			break;

		default:
			m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_16_LJ;
			m_pCardState->AcqConfig.u32SampleSize	= CS_SAMPLE_SIZE_2;
			m_pCardState->AcqConfig.i32SampleRes	= -CS_SAMPLE_RES_16;
			break;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsHexagon::InitAddOnRegisters(bool bForce)
{
	int32	i32Ret = CS_SUCCESS;
	uInt16 u16TrigOutValue = m_pCardState->bDisableTrigOut ? CS_OUT_NONE : CS_TRIGOUT;

	if( m_pCardState->bAddOnRegistersInitialized && !bForce )
		return i32Ret;

	if( !GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	i32Ret = SendCalibModeSetting();
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Read the default settings of all "IMPORTANT" registers.
	i32Ret = SetClockOut((eClkOutMode)m_pCardState->eclkoClockOut , true);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = SetTrigOut(u16TrigOutValue);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = SetTriggerIoRegister( HEXAGON_DEFAULT_TRIGIO_REG, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Init the internal structures. We do not care about the return error for this moment
	CsSetChannelConfig();
	CsSetTriggerConfig();

	// addon ADC auto calib
	// Force default values. do not care about the return status for naow
	WriteRegisterThruNios( 0x40, HEXAGON_BB_TRIGGER_AC_CALIB_REG );
	WriteRegisterThruNios( 0, HEXAGON_ADDON_CPLD_EXTCLOCK_REF );

	WriteToAdcRegister(CS_CHAN_1, 0, 0x80A0);
	WriteToAdcRegister(CS_CHAN_1, 0, 0x00A0);
	WriteToAdcRegister(CS_CHAN_2, 0, 0x80A0);
	WriteToAdcRegister(CS_CHAN_2, 0, 0x00A0);
	BBtiming( 350 );

	m_pCardState->bAddOnRegistersInitialized = true;
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//	Initialization upon received IOCTL_ACQUISITION_SYSTEM_INIT
//
//-----------------------------------------------------------------------------

int32	CsHexagon::AcqSystemInitialize()
{
	int32	i32Ret = CS_SUCCESS;

#ifdef _WINDOWS_
	ReadCommonRegistryOnAcqSystemInit();
#else
	ReadCommonCfgOnAcqSystemInit();
#endif   

	if( !GetInitStatus()->Info.u32Nios )
		return i32Ret;

	if (IsBoardLowRange())
		m_bLowRange_mV = true;

	i32Ret = InitAddOnRegisters();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsHexagon::ResetCachedState(void)
{
	// After ConfigReset all settings on the card is lost. By reseting these parameters, we make sure
	// that the next call of CssetAcquisitionCfg, CsSetChannelCfg ... will reconfigure HW even if the
	// configuration has not been changed.
	RtlZeroMemory( &m_pCardState->AddOnReg, sizeof(m_pCardState->AddOnReg) );
	RtlZeroMemory( &m_pCardState->BaseBoardRegs, sizeof(m_pCardState->BaseBoardRegs) );

	m_pCardState->AddOnReg.u16ExtTrigEnable		= INFINITE_U16;

	m_pCardState->BaseBoardRegs.u32PreTrigDepth = 
	m_pCardState->BaseBoardRegs.u32TrigDelay	= 
	m_pCardState->BaseBoardRegs.u32TrigHoldoff	= INFINITE_U32;

	m_pCardState->bExtTrigCalib					= true;
	m_pCardState->bExtTriggerAligned			= false;
	m_pCardState->bAdcDcLevelFreezed			= false;
	m_pCardState->bAddOnRegistersInitialized	= false;

	for (int i = 0 ; i < HEXAGON_CHANNELS; i++ )
		m_pCardState->ChannelParams[i].u32InputRange = 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	CsHexagon::FillOutBoardCaps(PCSHEXAGON_BOARDCAPS pCapsInfo)
{
	uInt32		u32Temp0;
	uInt32		u32Temp1;
	uInt32		i=0;


	RtlZeroMemory( pCapsInfo, sizeof(PCSHEXAGON_BOARDCAPS) );

	//Fill SampleRate
	for( i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		pCapsInfo->SrTable[i].PublicPart.i64SampleRate = m_pCardState->SrInfoTable[i].llSampleRate;

		if( m_pCardState->SrInfoTable[i].llSampleRate >= 1000000000 )
		{
			u32Temp0 =  (uInt32) (m_pCardState->SrInfoTable[i].llSampleRate/1000000000);
			u32Temp1 = (m_pCardState->SrInfoTable[i].llSampleRate % 1000000000) /100000000;
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

		pCapsInfo->SrTable[i].u32Mode = CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE;
	}

	// Fill External Clock info
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_500MHZ) )
		pCapsInfo->ExtClkTable[0].i64Max = HEXAGON_500MHZ_MAX_EXT_CLK;
	else
		pCapsInfo->ExtClkTable[0].i64Max = HEXAGON_MAX_EXT_CLK;
	pCapsInfo->ExtClkTable[0].i64Min = HEXAGON_MIN_EXT_CLK;
	pCapsInfo->ExtClkTable[0].u32Mode = CS_MODE_SINGLE;
	pCapsInfo->ExtClkTable[0].u32SkipCount = m_u32DefaultExtClkSkip	;
	if( GetBoardNode()->NbAnalogChannel > 3 )
		pCapsInfo->ExtClkTable[0].u32Mode = CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE;
	else if ( GetBoardNode()->NbAnalogChannel > 1 )
		pCapsInfo->ExtClkTable[0].u32Mode = CS_MODE_DUAL | CS_MODE_SINGLE;

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

	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_AC_COUPLING) )
	{
		pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_AC;
		strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "AC" );
	}
	else
	{
		pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_DC;
		strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "DC" );
	}
	pCapsInfo->CouplingTable[i].u32Reserved = 0;

	//Fill impedance table
	u32Size =  sizeof(pCapsInfo->ImpedanceTable[0].strText);
	pCapsInfo->ImpedanceTable[0].u32Imdepdance = CS_REAL_IMP_50_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[0].strText, u32Size, "50 Ohm" );
	pCapsInfo->ImpedanceTable[0].u32Reserved = 0;


	//Fill ext trig tables
	// Ranges
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigRangeTable[0].strText);
	pCapsInfo->ExtTrigRangeTable[i].u32InputRange = 3300;
	strcpy_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "+3.3 V" );
	pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_HiZ | CS_IMP_EXT_TRIG;

	//Coupling
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigCouplingTable[0].strText);
	pCapsInfo->ExtTrigCouplingTable[i].u32Coupling = CS_COUPLING_DC;
	strcpy_s( pCapsInfo->ExtTrigCouplingTable[i].strText,u32Size, "DC" );
	pCapsInfo->ExtTrigCouplingTable[i].u32Reserved = 0;

	//Impedance
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigImpedanceTable[0].strText);
	pCapsInfo->ExtTrigImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_1K_OHM;
	strcpy_s( pCapsInfo->ExtTrigImpedanceTable[i].strText, u32Size, "High Z" );
	pCapsInfo->ExtTrigImpedanceTable[i].u32Reserved = 0;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	CsHexagon::FindSrInfo( const LONGLONG llSr, PHEXAGON_SAMPLE_RATE_INFO *pSrInfo )
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

int32	CsHexagon::SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff, int64 i64TrigDelay)
{
	uInt8	u8Shift = 0;
	uInt8	u8Divider = 0;
	uInt32	u32Mode = m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE;

	switch(u32Mode)
	{
		case CS_MODE_QUAD:
			u8Shift = 3;		// 16 samples unit
			u8Divider = 8;
			break;
		case CS_MODE_DUAL:
			u8Shift = 4;		// 16 samples unit
			u8Divider = 16;
			break;
		case CS_MODE_SINGLE:
			u8Shift = 5;		// 32 samples uint
			u8Divider = 32;
			break;
	}

	m_Stream.bInfiniteDepth = (i64SegmentSize == -1);		// Infinite 

	// From samples, we have to convert to memory location then send this value to firmware
	// Because the firmware only support 30 bit register depth, we have to validate if the depth value exeeds 30 bit wide
	if ( ! m_Stream.bInfiniteDepth )
	{
		if ( (i64SegmentSize >> u8Shift) > HEXAGON_MAX_HW_DEPTH_COUNTER  )
			return CS_INVALID_SEGMENT_SIZE;
		if ( (i64PostDepth >> u8Shift) > HEXAGON_MAX_HW_DEPTH_COUNTER  ) 
			return CS_INVALID_TRIG_DEPTH;
	}

	if ( (i64HoldOff >> u8Shift) > HEXAGON_MAX_HW_DEPTH_COUNTER ) 
		return CS_INVALID_TRIGHOLDOFF;
	if ( (i64TrigDelay >> u8Shift) > HEXAGON_MAX_HW_DEPTH_COUNTER )
		return CS_INVALID_TRIGDELAY;

	// Convert to memory location unit
//	uInt32	u32SegmentSize	= (uInt32) (i64SegmentSize >> u8Shift);
//	uInt32	u32PostDepth	= (uInt32) (i64PostDepth >> u8Shift);
//	uInt32	u32HoldOffDepth = (uInt32) (i64HoldOff >> u8Shift);
	uInt32	u32SegmentSize	= (uInt32) (i64SegmentSize / u8Divider);
	uInt32	u32PostDepth	= (uInt32) (i64PostDepth  / u8Divider);
	uInt32	u32HoldOffDepth = (uInt32) (i64HoldOff  / u8Divider);


	int32	i32Ret = CS_SUCCESS;

	if ( m_bMulrecAvgTD )
	{
		// Interchange Segment count and Mulrec Acg count
		// From firmware point of view, the SegmentCount is number of averaging
		// and the number of averaging is segment count
		if ( m_pCardState->BaseBoardRegs.u32AverageCount != u32SegmentCount )
		{
			i32Ret = WriteRegisterThruNios( u32SegmentCount, HEXAGON_SET_MULREC_AVG_COUNT );
			if( CS_FAILED(i32Ret) )	return i32Ret;

			m_pCardState->BaseBoardRegs.u32AverageCount = u32SegmentCount;
		}

		if ( m_bCalibActive )
			u32SegmentCount = 1;
		else
			u32SegmentCount = m_u32MulrecAvgCount;
	}

	if ( m_pCardState->BaseBoardRegs.u32SegmentLength != u32SegmentSize )
	{
		i32Ret = WriteRegisterThruNios( u32SegmentSize, CSPLXBASE_SET_SEG_LENGTH_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		m_pCardState->BaseBoardRegs.u32SegmentLength = u32SegmentSize;
	}

	if ( m_pCardState->BaseBoardRegs.u32PostTrigDepth != u32PostDepth )
	{
		i32Ret = WriteRegisterThruNios( m_Stream.bInfiniteDepth ? -1 : u32PostDepth,	CSPLXBASE_SET_SEG_POST_TRIG_CMD	);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		m_pCardState->BaseBoardRegs.u32PostTrigDepth = u32PostDepth;
	}

	// In regular Stream mode, we have to send a big segment count (-1 = 0xFFFFFFFF ) in order to get the highest performance
	if ( m_Stream.bEnabled && ! m_bCalibActive && !m_bMulrecAvgTD )
		u32SegmentCount = INFINITE_U32;

	if ( m_pCardState->BaseBoardRegs.u32RecordCount != u32SegmentCount )
	{
		i32Ret = WriteRegisterThruNios( u32SegmentCount, CSPLXBASE_SET_SEG_NUMBER_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		m_pCardState->BaseBoardRegs.u32RecordCount = u32SegmentCount;
	}
	// Square record
	uInt32 u32SqrPreTrig		= u32SegmentSize - u32PostDepth;
	uInt32 u32SqrTrigDelay		= (uInt32) (i64TrigDelay / u8Divider);
	uInt32 u32SqrHoldOffDepth	= 0;

	// Hold off is extra counts after pretrigger
	if ( u32HoldOffDepth >  u32SqrPreTrig )
		u32SqrHoldOffDepth	= u32HoldOffDepth - u32SqrPreTrig;

	if ( m_pCardState->BaseBoardRegs.u32PreTrigDepth != u32SqrPreTrig )
	{
		i32Ret = WriteRegisterThruNios( u32SqrPreTrig, CSPLXBASE_SET_SEG_PRE_TRIG_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		m_pCardState->BaseBoardRegs.u32PreTrigDepth = u32SqrPreTrig;
	}

	if ( m_pCardState->BaseBoardRegs.u32TrigHoldoff != u32SqrHoldOffDepth )
	{
		i32Ret = WriteRegisterThruNios( u32SqrHoldOffDepth, CSPLXBASE_SET_SQUARE_THOLDOFF_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		m_pCardState->BaseBoardRegs.u32TrigHoldoff = u32SqrHoldOffDepth;
	}

	if ( m_pCardState->BaseBoardRegs.u32TrigDelay != u32SqrTrigDelay )
	{
		i32Ret = WriteRegisterThruNios( u32SqrTrigDelay, CSPLXBASE_SET_SQUARE_TDELAY_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		m_pCardState->BaseBoardRegs.u32TrigDelay = u32SqrTrigDelay;
	}
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
// Trigger Engine mapping
// channel 1 map to Engine 1 & Engine 5
// channel 2 map to Engine 2 & Engine 6
// channel 3 map to Engine 3 & Engine 7
// channel 4 map to Engine 4 & Engine 8
//-----------------------------------------------------------------------------
int32   CsHexagon::GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine )
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
		uInt16	u16Engine1 = (uInt16) (CSRB_TRIGENGINE_A1 + i32TrigSource -1);
		uInt16	u16Engine2 = (uInt16) (CSRB_TRIGENGINE_A3 + i32TrigSource -1);

		if ( i32TrigSource > 0 ) // disable may be in any position for channel trigger need to define appropriate engine
		{
			if ( m_CfgTrigParams[u16Engine1].i32Source == i32TrigSource && m_CfgTrigParams[u16Engine2].i32Source == i32TrigSource )
				return CS_ALLTRIGGERENGINES_USED;

			if ( 0 == m_CfgTrigParams[u16Engine1].u32UserTrigIndex )
			{
				*u16TriggerEngine = u16Engine1;
				return CS_SUCCESS;
			}
			else if ( 0 == m_CfgTrigParams[u16Engine2].u32UserTrigIndex )
			{
				*u16TriggerEngine = u16Engine2;
				return CS_SUCCESS;
			}
		}
		else
		{
			uInt16 i = (i32TrigSource > 0 ) ? CSRB_TRIGENGINE_A1 : 0;
			for ( ; i < GetBoardNode()->NbTriggerMachine; i ++ )
			{
				if ( 0 == m_CfgTrigParams[i].u32UserTrigIndex )
				{
					*u16TriggerEngine = i;
					return CS_SUCCESS;
				}
			}

		}
	}

	return CS_ALLTRIGGERENGINES_USED;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsHexagon::AssignBoardType()
{
	if ( 0 != GetBoardNode()->u32BoardType )
		return;

	// Check if Single or dual channel
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_2CHANNELS) )
	{
		GetBoardNode()->NbAnalogChannel			= 2;
		m_pCardState->AcqConfig.u32Mode			= CS_MODE_DUAL;
	}
	else
	{
		GetBoardNode()->NbAnalogChannel			= 4;
		m_pCardState->AcqConfig.u32Mode			= CS_MODE_QUAD;
	}

	// Default sampling rate
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_500MHZ) )
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_500MHZ;
	else
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_1GHZ;

	// Assign board type
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_500MHZ) )
	{
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_2CHANNELS) )
			GetBoardNode()->u32BoardType	= CSHEXAGON_16502_BOARDTYPE;
		else
			GetBoardNode()->u32BoardType	= CSHEXAGON_16504_BOARDTYPE;
	}
	else
	{
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_2CHANNELS) )
			GetBoardNode()->u32BoardType	= CSHEXAGON_161G2_BOARDTYPE;
		else
			GetBoardNode()->u32BoardType	= CSHEXAGON_161G4_BOARDTYPE;
	}

	if ( CS_DEVID_HEXAGON_PXI == GetDeviceId() )
		GetBoardNode()->u32BoardType |= CSHEXAGON_PXI_BOARDTYPE_MASK;

	if (IsBoardLowRange())
	{
		GetBoardNode()->u32BoardType |= CSHEXAGON_LOWRANGE_BOARDTYPE_MASK;
		m_bLowRange_mV = true;
	}

	m_pCardState->AcqConfig.u32SampleBits	= CS_SAMPLE_BITS_16;
	m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_16_LJ;
	m_pCardState->AcqConfig.i32SampleRes	= -CS_SAMPLE_RES_16;
	m_pCardState->AcqConfig.u32SampleSize	= CS_SAMPLE_SIZE_2;
	m_pCardState->AcqConfig.u32ExtClk		= HEXAGON_DEFAULT_EXT_CLOCK_EN;

	GetBoardNode()->NbDigitalChannel		= 0;
	GetBoardNode()->NbTriggerMachine		= 2*GetBoardNode()->NbAnalogChannel + 1;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsHexagon::FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR )
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

int32	CsHexagon::CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8Index = 0;
	int64	i64MaxMemPerChan = 0;
	int64	i64MaxSegmentSize = 0;
	int64	i64MaxDepth = 0;
	int64	i64MaxPreTrigDepth = 0;
	int64	i64MaxTrigDelay = 0;
	uInt32	u32LimitSegmentCount = 0xFFFFFFFF;
	uInt32	u32MaxSegmentCount	= 1;
	int32	i32MinSegmentSize	= m_bXS_Depth ? HEXAGON_MIN_SEGMENTSIZE : 128; 
	uInt32	u32DepthRes			= HEXAGON_DEPTH_INCREMENT;
	bool	bXpertBackup[10];
    uInt32  u32TriggerDelayRes = HEXAGON_DELAY_INCREMENT;
    uInt32	u32TailHwReserved = m_32SegmentTailBytes/sizeof(uInt16); // Samples reserved by HW at the end of each segment


	// This function is called before ACTION_COMMIT
	// CsValidateAcquisitionConfig may change all Xpert flags
	// so that all algorithm for validation work.
	// At the end we will restore actual value of Xpert Flags 			
	bXpertBackup[0]	= m_bMulrecAvgTD;
	bXpertBackup[1]	= m_Stream.bEnabled;

	m_bMulrecAvgTD = m_Stream.bEnabled = m_Stream.bInfiniteDepth = false;


	// Validate Sample Rate
	if ( 0 == pAcqCfg->u32ExtClk )
	{
		uInt32 i = 0;
		for (i = 0; i < m_pCardState->u32SrInfoSize; i++ )
		{
			if ( pAcqCfg->i64SampleRate == m_pCardState->SrInfoTable[i].llSampleRate )
				break;
		}

		if ( i >= m_pCardState->u32SrInfoSize )
		{	
			if ( u32Coerce )
			{
				pAcqCfg->i64SampleRate = m_pCardState->SrInfoTable[0].llSampleRate;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				i32Status = CS_INVALID_SAMPLE_RATE;
				goto ExitValidate;
			}
		}
	}

	if ( pAcqCfg->u32Mode & CS_MODE_USER1 )
		u8Index = 1;
	else if ( pAcqCfg->u32Mode & CS_MODE_USER2 )
		u8Index = 2;
	
	// Trigger delay use 32 bits counter & x 8/16/32 samples for quad/dual/single mode 
	i64MaxTrigDelay = HEXAGON_MAX_HW_TRIGGER_DELAY;
	i64MaxTrigDelay *= ( 32 >> ( (pAcqCfg->u32Mode&CS_MASKED_MODE)>>1 ) );
	i64MaxPreTrigDepth = MAX_PRETRIGGERDEPTH_SINGLE;

	i64MaxMemPerChan = m_pSystem->SysInfo.i64MaxMemory;
	// As a bug from FW, the last segment always takes more space
	i64MaxMemPerChan -= (512*4);

	if ( pAcqCfg->u32Mode & CS_MODE_QUAD )
	{
		u32DepthRes			>>= 2;
		i32MinSegmentSize	>>= 2;
		u32TriggerDelayRes	>>= 2;
		i64MaxPreTrigDepth	>>= 2;
		u32TailHwReserved	>>= 2;
		i64MaxMemPerChan	>>= 2;
	}
	else if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
	{
		u32DepthRes			>>= 1;
		i32MinSegmentSize	>>= 1;
		u32TriggerDelayRes	>>= 1;
		i64MaxPreTrigDepth	>>= 1;
		u32TailHwReserved	>>= 1;
		i64MaxMemPerChan	>>= 1;
	}

	// Calculate the Max allowed Segment Size
	i64MaxSegmentSize = i64MaxMemPerChan - u32TailHwReserved;
	i64MaxDepth = i64MaxSegmentSize;

	if ( 0 != u8Index )
	{
		// Convert from user index to the one we use in the driver
		u8Index += REGULAR_INDEX_OFFSET;

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

		if ( IsExpertSupported(u8Index, Averaging ) )
		{	
			m_bMulrecAvgTD		= TRUE;
			u32TailHwReserved	= 32;

			// Each sample will be 32 bits sample
			i64MaxMemPerChan	/= 2;
			i64MaxPreTrigDepth	= 0;

			i64MaxSegmentSize	= HEXAGON_MAX_HWAVG_DEPTH/(pAcqCfg->u32Mode&CS_MASKED_MODE);
			i64MaxDepth			= i64MaxSegmentSize;
		}
		else if ( IsExpertSupported(u8Index, DataStreaming) )
				m_Stream.bEnabled	= true;
	}
	else
	{
		m_Stream.bEnabled = false;

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


	// Validation of Segment Size
	if ( m_Stream.bEnabled && 0 > pAcqCfg->i64SegmentSize && 0 > pAcqCfg->i64Depth )
		m_Stream.bInfiniteDepth = true;
	else if ( 0 > pAcqCfg->i64SegmentSize )
		return CS_INVALID_SEGMENT_SIZE;
	else if ( 0 > pAcqCfg->i64Depth )
		return CS_PRETRIG_DEPTH_TOO_BIG;

	if ( ! m_Stream.bInfiniteDepth )
	{
		if (!m_Stream.bEnabled)
		{
			// Valdate the max post trigger depth if it is not in stream mode
			if (pAcqCfg->i64Depth > i64MaxDepth)
			{
				if (u32Coerce)
				{
					pAcqCfg->i64Depth = i64MaxDepth;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
				{
					i32Status = CS_INVALID_TRIG_DEPTH;
					goto ExitValidate;
				}
			}

			// Valdate the max segment size if it is not in stream mode
			if (pAcqCfg->i64SegmentSize > i64MaxSegmentSize)
			{
				if (u32Coerce)
				{
					pAcqCfg->i64SegmentSize = i64MaxSegmentSize;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
				{
					i32Status = CS_INVALID_SEGMENT_SIZE;
					goto ExitValidate;
				}
			}
		}

		if (0 >= pAcqCfg->i64SegmentSize)
		{
			if (u32Coerce)
			{
				pAcqCfg->i64SegmentSize = HEXAGON_DEFAULT_SEGMENT_SIZE;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_SEGMENT_SIZE;
		}

		// Validate triggger depth
		if (0 >= pAcqCfg->i64Depth)
		{
			if (u32Coerce)
			{
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_DEPTH;
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

		if ( pAcqCfg->i64SegmentSize % u32DepthRes )
		{
			if (u32Coerce)
			{
				// Coerce the value by rounding down. If not possible then round up
				int64 i64CoerceVal = pAcqCfg->i64SegmentSize - (pAcqCfg->i64SegmentSize % u32DepthRes);
				if ( i64CoerceVal < i32MinSegmentSize )
					pAcqCfg->i64SegmentSize += (pAcqCfg->i64SegmentSize % u32DepthRes);
				else
					pAcqCfg->i64SegmentSize = i64CoerceVal;

				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_SEGMENT_SIZE;
		}

		if ( pAcqCfg->i64Depth % u32DepthRes )
		{
			if (u32Coerce)
			{
				// Coerce the value by rounding down. If not possible then round up
				int64 i64CoerceVal = pAcqCfg->i64Depth - (pAcqCfg->i64Depth % u32DepthRes);
				if ( i64CoerceVal < i32MinSegmentSize )
					pAcqCfg->i64Depth += (pAcqCfg->i64Depth % u32DepthRes);
				else
					pAcqCfg->i64Depth = i64CoerceVal;

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

		// Hexagon is "square" and requires pAcqCfg->i64TriggerHoldoff >= pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth;
		if ( pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth > pAcqCfg->i64TriggerHoldoff )
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

	if ( pAcqCfg->i64TriggerDelay > i64MaxTrigDelay || 0 != (pAcqCfg->i64TriggerDelay % u32TriggerDelayRes) )
	{
		if ( u32Coerce )
		{
			if ( pAcqCfg->i64TriggerDelay > i64MaxTrigDelay )
				pAcqCfg->i64TriggerDelay = i64MaxTrigDelay;
			else
				pAcqCfg->i64TriggerDelay -= (pAcqCfg->i64TriggerDelay % u32TriggerDelayRes);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
		{
			i32Status = CS_INVALID_TRIGDELAY;
			goto ExitValidate;
		}
	}

	// Validation of segment count
	// In Stream mode, the segment count is ignored. Otherwise, the segment count should be calculated base on available memory
	if ( !m_Stream.bEnabled )
	{
		u32MaxSegmentCount = (uInt32)( i64MaxMemPerChan / (pAcqCfg->i64SegmentSize + u32TailHwReserved ));
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

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------	------
int32	CsHexagon::CsAutoUpdateFirmware()
{
	return CS_FUNCTION_NOT_SUPPORTED; 
}



//
// User channel vs Hardware Channel
// On the PCB, Channel 1 is labeled as the bottom Channel (CH1 INPUT), the Channel 2 is the top one (CH2 INPUT)
// From the SW driver and Firmware point of view, the channel 1 is always the top one.
//     on 3G card, channel 1 is the top input (CH2 INPUT), channel 2 is the bottom one (CH1 INPUT)
//     on 6G card, the top channel input is removed, thus only the channel 2 exist
//
// From the user poin of view, 
//     on 3G card, channel 1 is the top input (PCP label CH2 INPUT), channel 2 is the bottom one (CH1 INPUT)
//     on 6G card, channel 1 is the bottom input (CH1 INPUT). The top one is removed
// ConvertToHwChannel() is more about from the user point of view vs SW-FW point of view. Do not confuse with the PCB label.

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt16	CsHexagon::ConvertToHwChannel( uInt16 u16UserChannel )
{
	return u16UserChannel;
}

//-----------------------------------------------------------------------------
//	Convert the Hw channel to User channel
//-----------------------------------------------------------------------------
uInt16	CsHexagon::ConvertToUserChannel( uInt16 u16HwChannel )
{
	return u16HwChannel;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsHexagon::RestoreCalibDacInfo()
{
	//if ( m_pCardState->bUseDacCalibFromEEprom )
	//	RtlCopyMemory( &m_pCardState->DacInfo, &m_pCardState->CalibInfoTable.ChanCalibInfo, sizeof(m_pCardState->DacInfo) );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsHexagon::GetCardState()
{
#if !defined(USE_SHARED_CARDSTATE)
      IoctlGetCardState(m_pCardState);      
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsHexagon::UpdateCardState(PCSHEXAGON_DEVEXT pCardSt)
{
#if !defined(USE_SHARED_CARDSTATE)
	//m_pCardState->bCardStateValid = !bReset;
   if (!pCardSt)
   {
	m_pCardState->bCardStateValid &= m_PlxData->InitStatus.Info.u32Nios;
	IoctlSetCardState(m_pCardState);
   }
   else
      IoctlSetCardState(pCardSt);      
#endif
}

//------------------------------------------------------------------------------
//
//	Parameters on Board initialization.
//  Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

int32	CsHexagon::ReadCommonRegistryOnBoardInit()
{
#ifdef _WINDOWS_
	HKEY hKey;

	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsE16bcdWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_bXS_Depth ? 1 : 0;
		::RegQueryValueEx( hKey, _T("XS_Segment"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bXS_Depth = (ul != 0);

		::RegCloseKey( hKey );
	}	
#endif

	return CS_SUCCESS;
}

#ifdef _WINDOWS_
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32 CsHexagon::ReadCommonRegistryOnAcqSystemInit()
{

	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsE16bcdWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_u16TrigSensitivity;
		::RegQueryValueEx( hKey, _T("DTriggerSensitivity"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul <= (HEXAGON_DEFAULT_TRIG_SENSITIVITY*2 -1) )
			m_u16TrigSensitivity = (uInt16) ul;
		else
			m_u16TrigSensitivity = (HEXAGON_DEFAULT_TRIG_SENSITIVITY*2 -1);

		ul = m_u32DefaultExtClkSkip;
		::RegQueryValueEx( hKey, _T("DefaultExtClkSkip"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul == CS_SAMPLESKIP_FACTOR/2 || ul == CS_SAMPLESKIP_FACTOR )
			m_u32DefaultExtClkSkip = ul;

		ReadCalibrationSettings( hKey );

		::RegCloseKey( hKey );
	}	

	return CS_SUCCESS;
}
#else
   
#define DRV_SECTION_NAME      "RazorMax"

//------------------------------------------------------------------------------
// Linux read from config file.
//------------------------------------------------------------------------------
int32 CsHexagon::ReadCommonCfgOnAcqSystemInit()
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
   ul = GetCfgKeyInt(szSectionName, DISPLAY_CFG_KEYINFO, ul, szIniFile);	
   if (ul)
      ShowCfgKeyInfo();
 
	ul = m_u16TrigSensitivity;
   ul = GetCfgKeyInt(szSectionName, _T("DTriggerSensitivity"),  ul,  szIniFile);	
	if ( ul <= (HEXAGON_DEFAULT_TRIG_SENSITIVITY*2 -1) )
		m_u16TrigSensitivity = (uInt16) ul;
	else
		m_u16TrigSensitivity = (HEXAGON_DEFAULT_TRIG_SENSITIVITY*2 -1);

	ul = m_u32DefaultExtClkSkip;
   ul = GetCfgKeyInt(szSectionName, _T("DefaultExtClkSkip"),  ul,  szIniFile);	
	if ( ul == CS_SAMPLESKIP_FACTOR/2 || ul == CS_SAMPLESKIP_FACTOR )
		m_u32DefaultExtClkSkip = ul;

	ReadCalibrationSettings( szIniFile, szSectionName );

	return CS_SUCCESS;
}

int32	CsHexagon::ShowCfgKeyInfo()   
{
   printf("---------------------------------\n");
   printf("    Driver Config Setting Infos \n");
   printf("---------------------------------\n");
   printf("    SectionName: [%s] \n", DRV_SECTION_NAME);
   printf("    List of Cfg keys supported: \n");
   printf("\t%s\n","DTriggerSensitivity");
   printf("\t%s\n","DefaultExtClkSkip");
  
   printf("\t%s\n","FastDcCal");
   printf("\t%s\n","NoCalibrationDelay");
   printf("\t%s\n","NeverCalibrateDc");
   printf("\t%s\n","NeverCalibExtTrig");
   printf("\t%s\n","NeverCalibrateGain");
   printf("\t%s\n","CalDacSettleDelay");
   printf("\t%s\n","IgnoreCalibError");
   printf("\t%s\n","TraceCalib");
   printf("\t%s\n","LogFaluireOnly");
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

int32	CsHexagon::ReadCommonRegistryOnAcqSystemInit()
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

	ul = m_u32IgnoreCalibError;
	::QueryBackDoorParams(pElement, "IgnoreCalibError", &ul);
	m_u32IgnoreCalibError = ul;

#if DBG
	ul = m_u32DebugCalibTrace;
	::QueryBackDoorParams(pElement, "TraceCalib", &ul);
	m_u32DebugCalibTrace = ul;
#endif

	return CS_SUCCESS;
}

#endif


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool	CsHexagon::IsExtTriggerEnable( PARRAY_TRIGGERCONFIG pATrigCfg )
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
void CsHexagon::SetDefaultAuxIoCfg()
{
	m_pCardState->eclkoClockOut		= HEXAGON_DEFAULT_CLKOUT;
	m_pCardState->bDisableTrigOut	= TRUE;
}

//-----------------------------------------------------------------------------
// Check the Image header in the eeprom to see if it is properly configured
//-----------------------------------------------------------------------------

bool	CsHexagon::IsBootLocationValid( uInt8 u8Index )
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32Offset;
	CS_FIRMWARE_INFOS	FwInfo = {0};

	u32Offset = FIELD_OFFSET(HEXAGON_FLS_IMAGE_STRUCT, FwInfo) +
				FIELD_OFFSET(HEXAGON_FWINFO_SECTOR, FwInfoStruct);

	switch ( u8Index )
	{
		case 1:	u32Offset += FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, FlashImage0); break;
		case 2:	u32Offset += FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, FlashImage1); break;
		default:
			u32Offset += FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, SafeBoot); break;
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

int32 	CsHexagon::CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSizeSamples )
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

	uInt32	u32TransferSizeBytes = 0;
	m_Stream.u32TransferSize = u32TransferSizeSamples;
	u32TransferSizeBytes = u32TransferSizeSamples << 1;

	i32Status = IoctlStmTransferDmaBuffer( u32TransferSizeBytes, pVa );
	if ( CS_FAILED(i32Status) )
		::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 0, 1 );

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData )
{
	if ( ! m_Stream.bEnabled )
		return CS_INVALID_REQUEST;

	int32 i32Status = CsStmGetTransferStatus( u32TimeoutMs, pu32ErrorFlag );
	if (CS_SUCCESS == i32Status)
	{
		if (!m_Stream.bRunForever)
			m_Stream.u64DataRemain -= m_Stream.u32TransferSize;

		if ( pu32ActualSize )
		{
			// Return actual DMA data size
			*pu32ActualSize		= m_Stream.u32TransferSize;
		}

		if ( pu8EndOfData )
			*pu8EndOfData	= ( m_Stream.u64DataRemain == 0 );
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag )
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

void 	CsHexagon::MsStmResetParams()
{
	CsHexagon *pDev = m_MasterIndependent;
	
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
int32 CsHexagon::MsInitializeDevIoCtl()
{
	int32		 i32Status = CS_SUCCESS;
	CsHexagon* pDevice = m_MasterIndependent;

	while ( pDevice )
	{
		// Open handle to kernel driver
		i32Status = pDevice->InitializeDevIoCtl();
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

bool	CsHexagon::IsExpertSupported( uInt8 u8ImageId, eXpertId xpId )
{
	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & xpId) != 0 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsHexagon::PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg )
{
	int64	i64SegmentCount = pCsAcqCfg->i32SegmentCountHigh;
	i64SegmentCount = i64SegmentCount << 32 | pCsAcqCfg->u32SegmentCount;

	m_Stream.bInfiniteSegmentCount	= ( i64SegmentCount < 0 );
	m_Stream.bRunForever			= m_Stream.bInfiniteDepth || m_Stream.bInfiniteSegmentCount;

	if ( m_Stream.bRunForever )
		m_Stream.u64TotalDataSize	=(ULONGLONG)-1;
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
void	CsHexagon::GetMemorySize()
{
	uInt32	u32DimmSize = ReadRegisterFromNios(0, CSPLXBASE_GET_MEMORYSIZE_CMD);
	uInt32	u32LimitRamSize = m_PlxData->u32NvRamSize;

	m_PlxData->CsBoard.i64MaxMemory = u32DimmSize;
	if ( u32LimitRamSize > 0 )
		m_PlxData->CsBoard.i64MaxMemory = ( u32LimitRamSize < u32DimmSize ) ? u32LimitRamSize : u32DimmSize;
	m_PlxData->CsBoard.i64MaxMemory *= 1024 / sizeof(uInt16); //in Samples
	m_PlxData->InitStatus.Info.u32BbMemTest = m_PlxData->CsBoard.i64MaxMemory != 0 ? 1 : 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::CheckOverVoltage()
{
	int32 i32Status = CS_SUCCESS;
	// Check if OverVoltage protection has been occured
    IoctlGetMiscState( &m_VolatileState );
	if ( m_VolatileState.Bits.OverVoltage )
	{
		m_VolatileState.Bits.OverVoltage = 0;
		IoctlSetMiscState( &m_VolatileState );
		// Enabled back the Overvoltage interrupt
		WriteGageRegister( INT_CFG, OVER_VOLTAGE_INTR );
		RestoreCalibModeSetting();
		// Check again if OverVoltage protection has been occured
		IoctlGetMiscState( &m_VolatileState );
		if ( m_VolatileState.Bits.OverVoltage )
			i32Status = CS_CHANNEL_PROTECT_FAULT;
	}
	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsHexagon::SetDefaultInterrutps()
{
	// Clear interrupt fifo and counts. 
	ClearInterrupts();	
	
	// Enabled the Overvoltage interrupt
	WriteGageRegister( INT_CFG, OVER_VOLTAGE_INTR );
}

//-----------------------------------------------------------------------------
// CR6924: ADC DC level freeze may affect dc level (channel 1 only) so we need  
// to set deafault dc level  and gain before calibration.
// remove this function togerther with the key m_bNoDCLevelFreezePreset when
// hardware fix 0x3300000 command.
//-----------------------------------------------------------------------------

int32 CsHexagon::PresetDCLevelFreeze()
{
	int32	i32Ret = CS_SUCCESS;
	uInt16 u16DcGain = HEXAGON_DC_GAIN_CODE_1V;

//#ifdef _DEBUG
//	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("\n=== PresetDCLevelFreeze: m_bNoDCLevelF %d bPresetDCLevelF %d m_bNoDCLevelFPreset %d \n"), 
//				m_bNoDCLevelFreeze, m_pCardState->bPresetDCLevelFreeze, m_bNoDCLevelFreezePreset );
//	OutputDebugString(szDbgText);
//	m_EventLog.Report( CS_INFO_TEXT, szDbgText );
//#endif

	if ( m_bNoDCLevelFreeze || m_bNoDCLevelFreezePreset || !m_pCardState->bPresetDCLevelFreeze ) 
		return i32Ret;

	if (m_bLowRange_mV)
		u16DcGain = HEXAGON_DC_GAIN_CODE_LR;

	for (uInt16 u16Channel = 1; u16Channel<= m_PlxData->CsBoard.NbAnalogChannel; u16Channel++)
	{
		// Initial middle values for DACs
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, HEXAGON_DC_OFFSET_DEFAULT_VALUE, false );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = UpdateCalibDac(u16Channel, ecdOffset, HEXAGON_DC_OFFSET_DEFAULT_VALUE, false );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// Update calib gain
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16DcGain, false );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	
	i32Ret = SendADC_DC_Level_Freeze( 1,true );
	m_pCardState->bPresetDCLevelFreeze = false;
	return i32Ret;
}
//-----------------------------------------------------------------------------
// Dump Driver Trigger Config 
//-----------------------------------------------------------------------------
void CsHexagon::DumpTriggerParams(char *Str)
{
	if (Str)
		::OutputDebugString( Str );

	for ( uInt16 i = 0; i < HEXAGON_TOTAL_TRIGENGINES; i++ )
	{
		sprintf_s( szDbgText, sizeof(szDbgText),  "===TriggerParams[%d]  Source %d Condition %d Level %d u32UserTrigIndex %d \n",
				i,
				m_pCardState->TriggerParams[i].i32Source,
				m_pCardState->TriggerParams[i].u32Condition,
				m_pCardState->TriggerParams[i].i32Level,
				m_pCardState->TriggerParams[i].u32UserTrigIndex);
		::OutputDebugString( szDbgText );
	}
}

//-----------------------------------------------------------------------------
// Dump Driver Trigger Config
//-----------------------------------------------------------------------------
void CsHexagon::DumpCfgTrigParams(char *Str)
{
	CsHexagon			*pDevice = m_MasterIndependent;
	if (Str)
		::OutputDebugString( Str );

	for ( uInt16 i = 0; i < HEXAGON_TOTAL_TRIGENGINES; i++ )
	{
		sprintf_s( szDbgText, sizeof(szDbgText),  "===CfgTrigParams[%d]  Source %d Condition %d Level %d u32UserTrigIndex %d \n",
				i,
				pDevice->m_CfgTrigParams[i].i32Source,
				pDevice->m_CfgTrigParams[i].u32Condition,
				pDevice->m_CfgTrigParams[i].i32Level,
				pDevice->m_CfgTrigParams[i].u32UserTrigIndex);
		::OutputDebugString( szDbgText );
	}
}

//-----------------------------------------------------------------------------
// Dump App Trigger Config
//-----------------------------------------------------------------------------
void CsHexagon::DumpTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	PCSTRIGGERCONFIG pTrigCfg = NULL;
	if (!pATrigCfg)
		return;

	pTrigCfg = pATrigCfg->aTrigger;

	for ( uInt16 i = 0; i < pATrigCfg->u32TriggerCount; i++ )
	{
		sprintf_s( szDbgText, sizeof(szDbgText),  "===TriggerCfg[%d]  Source %d Condition %d Level %d u32UserTrigIndex %d \n",
				i,
				pTrigCfg[i].i32Source,
				pTrigCfg[i].u32Condition,
				pTrigCfg[i].i32Level,
				pTrigCfg[i].u32TriggerIndex);
		::OutputDebugString( szDbgText );
	}
}

//-----------------------------------------------------------------------------
// Is Low Range Board ?
//-----------------------------------------------------------------------------
bool CsHexagon::IsBoardLowRange( )
{
	if (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_LOWRANGE_MV)
		return true;
	else
		return false;
}
