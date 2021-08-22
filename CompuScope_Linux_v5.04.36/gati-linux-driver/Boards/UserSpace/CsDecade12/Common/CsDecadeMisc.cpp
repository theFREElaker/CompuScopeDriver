

#include "StdAfx.h"
#include "CsTypes.h"
#include "CsIoctl.h"
#include "CsDrvDefines.h"
#include "CsDecade.h"
#include "CsEventLog.h"
#include "xml.h"

#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif

extern SHAREDDLLMEM Globals;
float ScaleFactor = 131071.0;    //100000000;
char szText[256]={0};
int32	CsDecade::DeviceInitialize(void)
{
	int32 i32Ret = CS_SUCCESS;

#if !defined(USE_SHARED_CARDSTATE)
      IoctlGetCardState(m_pCardState );
      m_pCardState->u16CardNumber = 1;
#endif 

	i32Ret = CsPlxBase::Initialized();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	ReadCommonRegistryOnBoardInit();

	if ( ! m_pCardState->bCardStateValid )
	{
		// Re-initalize the whole structure, clear all the previous information
		RtlZeroMemory( m_pCardState->ChannelParams, sizeof(m_pCardState->ChannelParams) );
		RtlZeroMemory(&m_PlxData->CsBoard, sizeof(m_PlxData->CsBoard));
		ResetCachedState();
    
		// Reset the firmware version variables before reading firmware version
		RtlZeroMemory( &m_pCardState->VerInfo, sizeof(m_pCardState->VerInfo) );
		m_pCardState->VerInfo.u32Size = sizeof(CS_FW_VER_INFO);

		m_PlxData->InitStatus.Info.u32Nios = 1;
		InitializeDefaultAuxIoCfg();
		ReadHardwareInfoFromFlash();
		AssignBoardType();
		InitBoardCaps();
		InitAddOnRegisters();
		SetDefaultAuxIoCfg();

		GetMemorySize();
		DetectBootLocation();

		ReadVersionInfo();

		m_pCardState->bCardStateValid = TRUE;
	}

	m_bFirmwareChannelRemap = ( m_pCardState->VerInfo.BaseBoardFwVersion.Version.uIncRev > 0x36 );

	// Gain calibration code does not work with the old firmware
	m_bNeverCalibrateGain	= ! m_bFirmwareChannelRemap;

	// Set the DMA descriptor table size depending on the frimware version
	uInt32 u32Version = ReadGageRegister( DECADE_HWR_RD_CMD );	
	if ( u32Version > 59 )
		m_pCardState->u32DmaDescriptorSize = 1024;
	else
		m_pCardState->u32DmaDescriptorSize = 256;

	UpdateCardState();
   
	// Intentional return success to allows some utilitiy programs to update firmware in case of errors
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsDecade::DetectBootLocation()
{
	char szText[256];

	// Assume that we boot by the safe Boot image
	m_pCardState->i32CurrentBootLocation = 0;
	m_pCardState->bSafeBootImageActive = TRUE;
	sprintf_s( szText, sizeof(szText), TEXT("Current boot location: Safe Boot"));

	m_pCardState->i32ConfigBootLocation = GetConfigBootLocation();

	if (0 == m_pCardState->i32ConfigBootLocation)
	{
		// If the config boot location is invalid, set the location 1 (regular firmware) by default
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
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::MsCsResetDefaultParams( BOOLEAN bReset )	
{
	int32		i32Ret = CS_SUCCESS;

	SetDataPackMode(m_ConfigDataPackMode = DataUnpack, true);
	CsSetDataFormat();
	SendCalibModeSetting();

	// On Decade 6G, many cards has problem with LVDS allignment in SendClocksetting() inside CsSetAcquisitionConfig().
	// Ignore the errors returned in within this function so that we have the all default settings.
	// With bad boards if we do not ignore the errors, the card ends up with a wierd state because
	// we return on error then some of value on the board are not set.
	// Anyway, the error return from this function cannot be reported to users.128

	// Clear all cache state and force reconfig the hardware
	if ( bReset )
		ResetCachedState();

	// Set Default configuration
	m_u16DebugExtTrigCalib = 0;
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

	i32Ret = DDCModeSelect(CS_MODE_NO_DDC, true);
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

     // Double check OCT expert here. TBD
	i32Ret = OCTModeSelect(CS_OCT_DISABLE, true);
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	i32Ret = CsSetChannelConfig();
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	i32Ret = CsSetTriggerConfig();
//	if( CS_FAILED(i32Ret) )
//		return i32Ret;

	SetTriggerSensitivity();

	// Set flag to calib in ping pong if it is a 6G card
	DetectPingpongAcquisitions(&m_pCardState->AcqConfig);
	if (m_bPingpongCapture)
	{
		// Make sure the both "ADC" is in the same range if it is 6G card
		m_pCardState->ChannelParams[0].u32InputRange = m_pCardState->ChannelParams[1].u32InputRange;
	}
	m_bCalibPingpong = (TRUE == IsCalibrationRequired());

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

int32 CsDecade::AcquireData(uInt32 u32IntrEnabled /* default params u32IntrEnabled = OVER_VOLTAGE_INTR */)
{
	int32 i32Status = CS_SUCCESS;
   CSDECADE_DEVEXT  CardStateTmp = {0};

	// check if OverVoltage protection has been occured
#ifdef USE_SHARED_CARDSTATE   
	if ( m_pCardState->bOverVoltage )
	{
//		WriteGageRegister(INT_CFG, OVER_VOLTAGE_INTR);

//		uInt32 Test = ReadRegisterFromNios( 0, DECADE_ADDON_CPLD_MISC_STATUS );
		m_pCardState->bOverVoltage = FALSE;
		RestoreCalibModeSetting();
	}
#else
 #ifdef __linux__   
   IoctlGetCardState(&CardStateTmp );

	if ( CardStateTmp.bOverVoltage )
   {
       CardStateTmp.bOverVoltage = FALSE;
       UpdateCardState(&CardStateTmp);
	   RestoreCalibModeSetting();
   }   
 #endif   
#endif   
	// Send startacquisition command
	in_STARTACQUISITION_PARAMS AcqParams = {0};

	if ( ! m_bCalibActive )
	{
		// If Stream mode is enable, we can send start acquisition in either STREAM mode or REGULAR mode .
		// If we are in calibration mode then we have to capture data in REGULAR mode.
		// Use the u32Params1 to let the kernel driver know what mode we want to capture.
		if ( m_Stream.bEnabled )
		{
			if ( m_OCT_enabled )
				AcqParams.u32Param1 = AcqCmdStreaming_OCT;
			else
				AcqParams.u32Param1 = AcqCmdStreaming;
		}
		else
		{
			if ( m_OCT_enabled )
				AcqParams.u32Param1 = AcqCmdMemory_OCT;
			else if ( m_bMulrecAvgTD )
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

void CsDecade::MsAbort()
{
	CsDecade* pCard = m_MasterIndependent;

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
//----- CsDecade::UpdateSystemInfo(PSYSTEM_NODE pSys)
//-----------------------------------------------------------------------------

void	CsDecade::UpdateSystemInfo( CSSYSTEMINFO *SysInfo )
{
	PCS_BOARD_NODE  pBdParam = &m_PlxData->CsBoard;

	SysInfo->u32Size				= sizeof(CSSYSTEMINFO);
	SysInfo->u32SampleBits			= CS_SAMPLE_BITS_12;
	SysInfo->i32SampleResolution	= -CS_SAMPLE_RES_16;
	SysInfo->u32SampleSize			= CS_SAMPLE_SIZE_2;
	SysInfo->i32SampleOffset		= CS_SAMPLE_OFFSET_12_LJ;
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

void	CsDecade::AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo )
{
	strcpy_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"CSE" );

	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & DECADE_ADDON_HW_OPTION_1CHANNEL6G) )
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"126G1" );
	else
		strcat_s(pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), (char*)"123G2" );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsDecade::WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid )
{
	UNREFERENCED_PARAMETER(u8CalibTableId);
	UNREFERENCED_PARAMETER(u32Valid);
	return CS_FUNCTION_NOT_SUPPORTED;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDecade::ReadVersionInfo()
{
	uInt32	u32BootIndex = (uInt32) m_pCardState->i32CurrentBootLocation;

	if ( 0 == m_pCardState->VerInfo.BbCpldInfo.u8Reg )
	{
		// Read base board CPLD version
		m_pCardState->VerInfo.BbCpldInfo.u8Reg = (uInt8) (ReadCpldRegister(8) >> 4);
	}

	//if ( 0 == GetBoardNode()->u32RawFwVersionB[u32BootIndex] )
	{
		// Read base board Firmware version
		PCS_FW_VER_INFO pFwVerInfo = &m_pCardState->VerInfo;
		uInt32 u32Data	= 0;

#ifdef _DEBUG
		char szText[128];

		u32Data = ReadGageRegister( DECADE_HWR_RD_CMD );	
		sprintf_s( szText, sizeof(szText), "Firmware HW version: %02d.%02d",((u32Data) >> 6) & 0xFF,(u32Data & 0x3F) );
		u32Data = 0x7FFF & ReadGageRegister( DECADE_SWR_RD_CMD );	
		sprintf_s( szText, sizeof(szText), "Firmware SW version: %02d.%02d",((u32Data) >> 9) & 0x1F,(u32Data & 0x1FF) );
#endif

		u32Data = 0x7FFF & ReadGageRegister( DECADE_HWR_RD_CMD );	
		u32Data <<= 16;
		u32Data |= (0x7FFF & ReadGageRegister( DECADE_SWR_RD_CMD ));
		pFwVerInfo->BbFpgaInfo.u32Reg = m_PlxData->CsBoard.u32RawFwVersionB[u32BootIndex] = u32Data; 
		pFwVerInfo->u32BbFpgaOptions = 0xFF & ReadGageRegister( DECADE_OPT_RD_CMD );

		// Build Csi and user version info
		pFwVerInfo->BaseBoardFwVersion.Version.uMajor	= pFwVerInfo->BbFpgaInfo.Version.uMajor;
		pFwVerInfo->BaseBoardFwVersion.Version.uMinor	= pFwVerInfo->BbFpgaInfo.Version.uMinor;
		pFwVerInfo->BaseBoardFwVersion.Version.uRev		= pFwVerInfo->u32BbFpgaOptions;
		pFwVerInfo->BaseBoardFwVersion.Version.uIncRev	= (64*pFwVerInfo->BbFpgaInfo.Version.uRev + pFwVerInfo->BbFpgaInfo.Version.uIncRev);

		GetBoardNode()->u32UserFwVersionB[u32BootIndex].u32Reg		= m_pCardState->VerInfo.BaseBoardFwVersion.u32Reg;
	}

	if ( m_pCardState->bAddOnRegistersInitialized && 0 == m_pCardState->VerInfo.ConfCpldInfo.u32Reg )
	{
		// Read Addon CPLD Version
		PCS_FW_VER_INFO				pFwVerInfo = &m_pCardState->VerInfo;
		DECADE_ADDON_CPLD_VERSION	AddonCpldVersion = {0};

		AddonCpldVersion.u16RegVal = (uInt16) ReadRegisterFromNios(0, DECADE_ADDON_CPLD_FW_VERSION);

		// Converting version number from FW structure to SW user structure
		pFwVerInfo->ConfCpldInfo.Info.uRev		= AddonCpldVersion.bits.Version;
		pFwVerInfo->ConfCpldInfo.Info.uMajor	= AddonCpldVersion.bits.Major;
		pFwVerInfo->ConfCpldInfo.Info.uMinor	= AddonCpldVersion.bits.Minor;
	}


}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsDecade::BuildIndependantSystem( CsDecade** FirstCard, BOOLEAN bForceInd )
{
	CsDecade*	pCard	= (CsDecade*) m_ProcessGlblVars->pFirstUnCfg;
	*FirstCard = NULL;

	UNREFERENCED_PARAMETER(bForceInd);

	if( NULL != pCard )
	{
		ASSERT(ermStandAlone == pCard->m_pCardState->ermRoleMode);

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

int32	CsDecade::RemoveDeviceFromUnCfgList()
{
	CsDecade	*Current	= (CsDecade	*) m_ProcessGlblVars->pFirstUnCfg;
	CsDecade	*Prev = NULL;

	if ( this  == (CsDecade	*) m_ProcessGlblVars->pFirstUnCfg )
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

	if ( this  == (CsDecade	*) m_ProcessGlblVars->pLastUnCfg )
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

uInt16	CsDecade::DetectAcquisitionSystems( BOOL bForceInd )
{
	CsDecade	*pDevice = (CsDecade *) m_ProcessGlblVars->pFirstUnCfg;
	CsDecade	*pFirstCardInSystem[20];

	while( pDevice )
	{
		// Undo M/S link
		pDevice->IoctlSetMasterSlaveLink( NULL, 0 );
		pDevice->m_pCardState->ermRoleMode = ermStandAlone;
		pDevice->m_Next =  pDevice->m_HWONext;
		if ( NULL == pDevice->m_Next )
		{
			m_ProcessGlblVars->pLastUnCfg = pDevice;
		}

		pDevice = pDevice->m_HWONext;
	}

	uInt32			i = 0;
	uInt16			NbCardInSystem;
	CsDecade		*FirstCard = NULL;
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

		CsDecade *pDev = (CsDecade *) pFirstCardInSystem[i];
		j = 0;

		memset(pSystem->u8CardIndexList, 0, sizeof(pSystem->u8CardIndexList));
		while( pDev )
		{
			// Set M/S card index list
			pSystem->u8CardIndexList[j++] = pDev->m_u8CardIndex;

			pDev->m_pSystem = pSystem;
			pDev = pDev->m_Next;
		}

		pDev = (CsDecade *) pFirstCardInSystem[i];
		SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		pSystem->u16AcqStatus = ACQ_STATUS_READY;
	}

	return m_ProcessGlblVars->NbSystem;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade:: ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg )
{
	bool bDebugRawTs = false;	// for debugging only. Return the raw timestamps from memory

	// Set Mode select 0
	WriteGageRegister(MODESEL, 0);

	for (uInt32 i = 0; i < u32NumOfSeg; i++)
	{
		pBuffer[i] = ReadRegisterFromNios(StartSegment, CSPLXBASE_READ_TIMESTAMP0_CMD);
		pBuffer[i] &= 0xFFFF;
		pBuffer[i] <<= 32;


		uInt32	u32LowTs = ReadRegisterFromNios(StartSegment++, CSPLXBASE_READ_TIMESTAMP1_CMD);
		pBuffer[i] |= u32LowTs;

		if (!bDebugRawTs)
		{	
			if (m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK)
			{
				// Timestamp in memory clock
				uInt32 u32Etb = 0;
				if (CS_MODE_DUAL == (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE))
				{
					u32Etb = u32LowTs & 0x0F;	// 4 bit Etb
				}
				else
				{
					u32Etb = u32LowTs & 0x1F;	// 5 bit Etb
				}

				// Convert the sample count from 187.MHz clock to 200MHz clock
				float fAdjustSample = (float)(1.0*u32Etb*30.0 / 32.0 / m_pCardState->AddOnReg.u32Decimation);
				uInt32 NewEtb = (uInt32)fAdjustSample;

				pBuffer[i] >>= 5;				// Removed all Etbs		
				pBuffer[i] += NewEtb;			// Adjust wiht new Etbs
			}
			else
			{
				// Timestamp in sampling clock
				if (CS_MODE_DUAL == (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE))
				{
					pBuffer[i] >>= 1;
					pBuffer[i] &= ~0xF;
					pBuffer[i] |= (u32LowTs & 0xF);
				}
			}
		}
	}
	
	// PROBLEM_FIRMWARE
	// Problem of firmware with support for Packed data
	// Reading tail destroys the setting of Data Pack register. We have to restore the value of this register here
	SetDataPackMode(m_ConfigDataPackMode, true);
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void	CsDecade:: SetDataMaskModeTransfer ( uInt32 u32TxMode )
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
uInt32	CsDecade:: GetModeSelect( uInt16 u16UserChannel )
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
uInt32	CsDecade:: GetDataMaskModeTransfer ( uInt32 u32TxMode )
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

CsDecade *CsDecade::GetCardPointerFromBoardIndex( uInt16 BoardIndex )
{
	CsDecade *pDevice = m_MasterIndependent;

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

uInt16 CsDecade::NormalizedChannelIndex( uInt16 u16ChannelIndex )
{
	if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) == CS_MODE_DUAL )
		return	( u16ChannelIndex % 2) ? CS_CHAN_1: CS_CHAN_2;
	else
		return CS_CHAN_1;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDecade::NormalizedTriggerSource( int32 i32TriggerSource )
{
	if ( i32TriggerSource ==  CS_TRIG_SOURCE_DISABLE )
		return CS_TRIG_SOURCE_DISABLE;
	else if ( i32TriggerSource < 0 )
		return CS_TRIG_SOURCE_EXT;
	else
	{
		if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) == CS_MODE_DUAL )
			return	( i32TriggerSource % 2) ? CS_TRIG_SOURCE_CHAN_1: CS_TRIG_SOURCE_CHAN_2;
		if  ( m_bSingleChannel2 )
		{
			if ( 0 == (i32TriggerSource % 2) )
				return CS_TRIG_SOURCE_CHAN_2;
			else
				return CS_TRIG_SOURCE_DISABLE;
		}
		else
		{
			if ( i32TriggerSource % 2 )
				return CS_TRIG_SOURCE_CHAN_1;
			else
				return CS_TRIG_SOURCE_DISABLE;
		}
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDecade::CsSetDataFormat( _CSDATAFORMAT csFormat )
{
	UNREFERENCED_PARAMETER(csFormat);

	if ( m_bMulrecAvgTD )
	{
		// Full resolution, 32 bit data
		m_DataFormatInfo.u32Signed = 1;
		m_DataFormatInfo.u32Packed = 0;
		m_DataFormatInfo.i32SampleOffset	= CS_SAMPLE_OFFSET_12_LJ;
		m_DataFormatInfo.i32SampleRes		= -CS_SAMPLE_RES_12;
		m_DataFormatInfo.u32SampleBits		= CS_SAMPLE_BITS_12;
		m_DataFormatInfo.u32SampleSize_Bits	= 8*CS_SAMPLE_SIZE_4;
	}
	else if ( m_bSoftwareAveraging )
	{
		m_DataFormatInfo.u32Signed = 1;
		m_DataFormatInfo.u32Packed = 0;
		m_DataFormatInfo.i32SampleOffset	= CS_SAMPLE_OFFSET_12_LJ;
		m_DataFormatInfo.i32SampleRes		= -CS_SAMPLE_RES_16;
		m_DataFormatInfo.u32SampleBits		= CS_SAMPLE_BITS_12;
		m_DataFormatInfo.u32SampleSize_Bits	= 8*CS_SAMPLE_SIZE_4;
	}
	else
	{
		switch( m_DataPackMode )
		{
		case DataPacked_8:
			m_DataFormatInfo.u32Signed = 1;
			m_DataFormatInfo.u32Packed = 1;
			m_DataFormatInfo.i32SampleOffset	= CS_SAMPLE_OFFSET_8;
			m_DataFormatInfo.i32SampleRes		= -CS_SAMPLE_RES_8;
			m_DataFormatInfo.u32SampleBits		= 
			m_DataFormatInfo.u32SampleSize_Bits	= CS_SAMPLE_BITS_8;
			break;
		case DataPacked_12:
			m_DataFormatInfo.u32Signed = 1;
			m_DataFormatInfo.u32Packed = 1;
			m_DataFormatInfo.i32SampleOffset	= CS_SAMPLE_OFFSET_12;
			m_DataFormatInfo.i32SampleRes		= -CS_SAMPLE_RES_12;
			m_DataFormatInfo.u32SampleBits		= 
			m_DataFormatInfo.u32SampleSize_Bits	= CS_SAMPLE_BITS_12;
			break;

		case DataUnpack:
		default:
			{
				// Default data format
				m_DataFormatInfo.u32Signed = 1;
				m_DataFormatInfo.u32Packed = 0;
				m_DataFormatInfo.i32SampleOffset	= CS_SAMPLE_OFFSET_12_LJ;
				m_DataFormatInfo.i32SampleRes		= -CS_SAMPLE_RES_16;
				m_DataFormatInfo.u32SampleBits		= CS_SAMPLE_BITS_12;
				m_DataFormatInfo.u32SampleSize_Bits	= 8*CS_SAMPLE_SIZE_2;
			}
			break;
		}
	}

	m_pCardState->AcqConfig.i32SampleOffset = m_DataFormatInfo.i32SampleOffset;
	m_pCardState->AcqConfig.i32SampleRes	= m_DataFormatInfo.i32SampleRes;
	m_pCardState->AcqConfig.u32SampleBits	= m_DataFormatInfo.u32SampleBits;
	m_pCardState->AcqConfig.u32SampleSize	= m_DataFormatInfo.u32SampleSize_Bits/8;
	if ( 0 != (m_DataFormatInfo.u32SampleSize_Bits % 8 ))
		m_pCardState->AcqConfig.u32SampleSize += 1;


}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDecade::CsResetDataFormat()
{
	m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_12_LJ;
	m_pCardState->AcqConfig.i32SampleRes	= -CS_SAMPLE_RES_16;
	m_pCardState->AcqConfig.u32SampleBits	= CS_SAMPLE_BITS_12;
	m_pCardState->AcqConfig.u32SampleSize	= CS_SAMPLE_SIZE_2;

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::InitAddOnRegisters(bool bForce)
{
	int32	i32Ret = CS_SUCCESS;

	if( m_pCardState->bAddOnRegistersInitialized && !bForce )
		return i32Ret;

	if( !GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	i32Ret = SendCalibModeSetting();
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Read the default settings of all "IMPORTANT" registers.
	i32Ret = SetClockControlRegister( DECADE_DEFAULT_CLOCK_REG, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Enabled All trigger engines
	i32Ret = WriteRegisterThruNios(CSRB_GLB_TRIG_ENG_ENABLE, DECADE_TRIGENGINES_EN);
	if (CS_FAILED(i32Ret))
		return i32Ret;

	if ( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
	{
		// old register still supported.
		i32Ret = WriteRegisterThruNios(DECADE_DEFAULT_TRIGIN_REG, DECADE_TRIGGER_IO_CONTROL | DECADE_SPI_WRITE);
		if (CS_FAILED(i32Ret))
			return i32Ret;

		i32Ret = SetTriggerIoRegister( DECADE_DEFAULT_TRIGOUT_PINGPONG_REG, true );					// Trigout disable
	}
	else
	{
		i32Ret = SetTriggerIoRegister( DECADE_DEFAULT_TRIGIO_REG, true );
	}
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Init the internal structures. We do not care about the return error for this moment
	CsSetChannelConfig();
	CsSetTriggerConfig();

	// addon ADC auto calib
	i32Ret = WriteRegisterThruNios( 0x40, DECADE_BB_TRIGGER_AC_CALIB_REG );

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

int32	CsDecade::AcqSystemInitialize()
{
	int32	i32Ret = CS_SUCCESS;

	m_bOneSampleResolution = false;

#ifdef _WINDOWS_
	ReadCommonRegistryOnAcqSystemInit();
#else
	ReadCommonCfgOnAcqSystemInit();
#endif   

	if( !GetInitStatus()->Info.u32Nios )
		return i32Ret;

	// Update pointers for SR tables in shared mem
	InitBoardCaps();

	i32Ret = InitAddOnRegisters();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	// Reset (Disable) kernel Stream variables
	i32Ret = IoctlStmSetStreamMode(0);
	
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDecade::ResetCachedState(void)
{
	// After ConfigReset all settings on the card is lost. By reseting these parameters, we make sure
	// that the next call of CssetAcquisitionCfg, CsSetChannelCfg ... will reconfigure HW even if the
	// configuration has not been changed.
	RtlZeroMemory( &m_pCardState->AddOnReg, sizeof(m_pCardState->AddOnReg) );

	m_pCardState->AddOnReg.u16ExtTrigEnable = (uInt16)-1;
	m_pCardState->bExtTrigCalib = true;
	m_pCardState->bAdcLvdsAligned		= 
	m_pCardState->bExtTriggerAligned	= false;
	m_pCardState->bAddOnRegistersInitialized = false;

	m_pCardState->AddOnReg.u32FeSetup[0].bits.Channel = 1;
	m_pCardState->AddOnReg.u32FeSetup[1].bits.Channel = 2;

	m_pCardState->ChannelParams[0].u32InputRange = 
	m_pCardState->ChannelParams[1].u32InputRange = 0;	

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	CsDecade::FillOutBoardCaps(PCSDECADE_BOARDCAPS pCapsInfo)
{
	uInt32		u32Temp0;
	uInt32		u32Temp1;
	uInt32		i=0;


	RtlZeroMemory( pCapsInfo, sizeof(PCSDECADE_BOARDCAPS) );

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

		if ( 0 != m_pCardState->SrInfoTable[i].u16PingPong )
			pCapsInfo->SrTable[i].u32Mode = CS_MODE_SINGLE;
		else
			pCapsInfo->SrTable[i].u32Mode = CS_MODE_DUAL | CS_MODE_SINGLE;

	}

	// Fill External Clock info
	pCapsInfo->ExtClkTable[0].i64Max = DECADE_MAX_EXT_CLK;
	pCapsInfo->ExtClkTable[0].i64Min = DECADE_MIN_EXT_CLK;
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

	if (0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & DECADE_ADDON_HW_OPTION_AC_COUPLING))
	{
		pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_AC;
		strcpy_s(pCapsInfo->CouplingTable[i].strText, u32Size, "AC");
	}
	else
	{
		pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_DC;
		strcpy_s(pCapsInfo->CouplingTable[i].strText, u32Size, "DC");
	}
	pCapsInfo->CouplingTable[i].u32Reserved = 0;

	//Fill impedance table
	u32Size =  sizeof(pCapsInfo->ImpedanceTable[0].strText);
	pCapsInfo->ImpedanceTable[0].u32Imdepdance = CS_REAL_IMP_50_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[0].strText, u32Size, "50 Ohm" );
	pCapsInfo->ImpedanceTable[0].u32Reserved = 0;

	//Fill filter table
/*	i = 0;
	pCapsInfo->FilterTable[i].u32LowCutoff  = 0;
	pCapsInfo->FilterTable[i].u32HighCutoff = DECADE_FILTER_FULL;

	i++;
	pCapsInfo->FilterTable[i].u32LowCutoff  = 0;
	pCapsInfo->FilterTable[i].u32HighCutoff = DECADE_FILTER_LIMIT;
*/

	//Fill ext trig tables
	// Ranges
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigRangeTable[0].strText);
	pCapsInfo->ExtTrigRangeTable[i].u32InputRange = DECADE_DEFAULT_EXT_GAIN;
	strcpy_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "+5 V" );
	pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM | CS_IMP_EXT_TRIG;

	//Coupling
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigCouplingTable[0].strText);
	pCapsInfo->ExtTrigCouplingTable[i].u32Coupling = DECADE_DEFAULT_EXT_COUPLING;
	strcpy_s( pCapsInfo->ExtTrigCouplingTable[i].strText,u32Size, "DC" );
	pCapsInfo->ExtTrigCouplingTable[i].u32Reserved = 0;

	//Impedance
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigImpedanceTable[0].strText);
	pCapsInfo->ExtTrigImpedanceTable[i].u32Imdepdance = DECADE_DEFAULT_EXT_IMPEDANCE;
	strcpy_s( pCapsInfo->ExtTrigImpedanceTable[i].strText, u32Size, "1 KOhm" );
	pCapsInfo->ExtTrigImpedanceTable[i].u32Reserved = 0;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	CsDecade::FindSrInfo( const LONGLONG llSr, PDECADE_SAMPLE_RATE_INFO *pSrInfo )
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

int32	CsDecade::SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff, int64 i64TrigDelay)
{
	int32	i32Ret = CS_SUCCESS;
	uInt8	u8Shift = 0;
	uInt8	u8Divider = 0;
	uInt32	u32Mode = m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE;
	uInt32	u32OneSampleTrigDelay = 0;

	switch(u32Mode)
	{
		case CS_MODE_DUAL:
			u8Shift = 4;		// 16 samples unit
			u8Divider = 16;
			break;
		case CS_MODE_SINGLE:
			u8Shift = 5;		// 32 samples unit
			u8Divider = 32;
			break;
	}

	m_Stream.bInfiniteDepth = (i64SegmentSize == -1);		// Infinite 

	// From samples, we have to convert to memory location then send this value to firmware
	// Because the firmware only support 30 bit register depth, we have to validate if the depth value exeeds 30 bit wide
	if ( ! m_Stream.bInfiniteDepth )
	{
		if ( (i64SegmentSize >> u8Shift) > DECADE_MAX_HW_DEPTH_COUNTER  )
			return CS_SEGMENTSIZE_TOO_BIG;
		if ( (i64PostDepth >> u8Shift) > DECADE_MAX_HW_DEPTH_COUNTER  ) 
			return CS_SEGMENTSIZE_TOO_BIG;
	}

	if ( (i64HoldOff >> u8Shift) > DECADE_MAX_HW_DEPTH_COUNTER ) 
		return CS_INVALID_TRIGHOLDOFF;
	if ( (i64TrigDelay >> u8Shift) > DECADE_MAX_HW_DEPTH_COUNTER )
		return CS_INVALID_TRIGDELAY;

	// Adjust pretrigger in case of one sample trigger resolution
	if (m_bOneSampleResolution)
	{
		int64 i64PreTrigger = i64SegmentSize - i64PostDepth;
		uInt32 u32OneSampleTrigDelay_PreTrig = 0;
		if (i64PreTrigger % u8Divider)
		{
			u32OneSampleTrigDelay_PreTrig = u8Divider - (i64PreTrigger % u8Divider);
			// Round down the post trigger depth into multiple of resolution
			i64PostDepth -= u32OneSampleTrigDelay_PreTrig;
		}

		u32OneSampleTrigDelay = i64TrigDelay % u8Divider;
		if (u32OneSampleTrigDelay)
		{
			// Round down the trigger delay into multiple of resolution
			i64TrigDelay -= u32OneSampleTrigDelay;
		}
		u32OneSampleTrigDelay += u32OneSampleTrigDelay_PreTrig;
	}

	// Convert to memory location unit
	uInt32	u32SegmentSize	= (uInt32) (i64SegmentSize / u8Divider);
	uInt32	u32PostDepth	= (uInt32) (i64PostDepth  / u8Divider);
	uInt32	u32HoldOffDepth = (uInt32) (i64HoldOff  / u8Divider);

	if ( m_bMulrecAvgTD )
	{
		// Interchange Segment count and Mulrec Acg count
		// From firmware point of view, the SegmentCount is number of averaging
		// and the number of averaging is segment count
		i32Ret = WriteRegisterThruNios( u32SegmentCount, DECADE_SET_MULREC_AVG_COUNT );
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

	// Square record
	uInt32 u32SqrPreTrig		= u32SegmentSize - u32PostDepth;
	//uInt32 u32SqrTrigDelay		= (uInt32) (i64TrigDelay >> u8Shift);
	uInt32 u32SqrTrigDelay		= (uInt32) (i64TrigDelay / u8Divider);
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

	i32Ret = WriteRegisterThruNios(u32OneSampleTrigDelay, CSPLXBASE_ONE_SAMPLE_TRIGDELAY);
	if (CS_FAILED(i32Ret))
		return i32Ret;

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

LONGLONG	CsDecade::GetMaxSampleRate( BOOLEAN bPingpong )
{

	for( unsigned i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		if ( bPingpong == (BOOLEAN) m_pCardState->SrInfoTable[i].u16PingPong )
			return m_pCardState->SrInfoTable[i].llSampleRate;
	}

	return m_pCardState->SrInfoTable[0].llSampleRate;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDecade::GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine )
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
void CsDecade::AssignBoardType()
{

	if ( 0 != GetBoardNode()->u32BoardType )
		return;

	// Check if Single or dual channel
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & DECADE_ADDON_HW_OPTION_1CHANNEL6G) )
		GetBoardNode()->NbAnalogChannel			= 1;
	else
		GetBoardNode()->NbAnalogChannel			= 2;

	if ( 2 == GetBoardNode()->NbAnalogChannel )
	{
		GetBoardNode()->u32BoardType			= CSDECADE123G_BOARDTYPE;
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_3GHZ;		// Default sampling rate
		m_pCardState->AcqConfig.u32Mode			= CS_MODE_DUAL;
	}
	else
	{
		GetBoardNode()->u32BoardType			= CSDECADE126G_BOARDTYPE;
		m_pCardState->AcqConfig.i64SampleRate	= CS_SR_6GHZ;		// Default sampling rate
		m_pCardState->AcqConfig.u32Mode			= CS_MODE_SINGLE;
	}
	
	m_pCardState->AcqConfig.u32SampleBits	= CS_SAMPLE_BITS_12;
	m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_12_LJ;

	m_pCardState->AcqConfig.u32ExtClk		= DECADE_DEFAULT_EXT_CLOCK_EN;

	m_pCardState->AcqConfig.i32SampleRes	= -CS_SAMPLE_RES_16;
	m_pCardState->AcqConfig.u32SampleSize	= CS_SAMPLE_SIZE_2;

	GetBoardNode()->NbDigitalChannel		= 0;
	GetBoardNode()->NbTriggerMachine		= 2*GetBoardNode()->NbAnalogChannel + 1;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR )
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

int32	CsDecade::CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8Index = 0;
	int64	i64MaxMemPerChan = 0;
	int64	i64MaxSegmentSize = 0;
	int64	i64MaxDepth = 0;
	int64	i64MaxPreTrigDepth = MAX_PRETRIGGERDEPTH_SINGLE;
	int64	i64MaxTrigDelay = 0;
	uInt32	u32LimitSegmentCount = 0xFFFFFFFF;
	uInt32	u32MaxSegmentCount = 1;
	int32	i32MinSegmentSize = DECADE_DEPTH_INCREMENT; 
	uInt32	u32DepthRes = DECADE_DEPTH_INCREMENT;
	bool	bXpertBackup[10];
    uInt32  u32TriggerDelayRes = DECADE_DELAY_INCREMENT;
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
	
	i64MaxTrigDelay = DECADE_MAX_HW_DEPTH_COUNTER;
	i64MaxTrigDelay *= 32;
	i64MaxTrigDelay /= (pAcqCfg->u32Mode&CS_MASKED_MODE);

	i64MaxPreTrigDepth = MAX_PRETRIGGERDEPTH_SINGLE;
	i64MaxMemPerChan = m_pSystem->SysInfo.i64MaxMemory;
	// As designed from FW, the last segment always take more space
	i64MaxMemPerChan -= (32*64) / sizeof(uInt16);

	if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
	{
		u32DepthRes			>>= 1;
		i32MinSegmentSize	>>= 1;
		u32TriggerDelayRes	>>= 1;
		i64MaxPreTrigDepth	>>= 1;
		u32TailHwReserved	>>= 1;
		i64MaxMemPerChan	>>= 1;
	}

	if (m_bOneSampleResolution)
		u32TriggerDelayRes = 1;

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
			i32MinSegmentSize	= 160;
			if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
			{
				i32MinSegmentSize >>= 1;
				u32TailHwReserved >>= 1;
			}

			i64MaxSegmentSize	= DECADE_MAX_HWAVG_DEPTH/(pAcqCfg->u32Mode&CS_MASKED_MODE);
			i64MaxDepth			= i64MaxSegmentSize;
		}
		else if ( IsExpertSupported(u8Index, DataStreaming) )
		{
			m_Stream.bEnabled	= true;
			if ( DataPacked_8 == m_ConfigDataPackMode )
			{
				u32DepthRes = DECADE_DEPTH_INCREMENT_PACK8;
				i32MinSegmentSize = DECADE_DEPTH_INCREMENT_PACK8;
				if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
				{
					u32DepthRes >>= 1;
					i32MinSegmentSize >>= 1;
				}
			}
			else
			{
				u32DepthRes = m_u8DepthIncrement;
				if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
				{
					u32DepthRes >>= 1;
				}
			}
		}
		else if ( IsExpertSupported(u8Index, DDC ) )
		{
			// DDC works only in SINGLE channel mode)
			if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
			{
				if ( u32Coerce )
				{
					pAcqCfg->u32Mode &= ~CS_MODE_DUAL;
					pAcqCfg->u32Mode |= CS_MODE_SINGLE;
					i32Status = CsValidateAcquisitionConfig( pAcqCfg, u32Coerce );
					if ( CS_SUCCESS == i32Status || CS_CONFIG_CHANGED == i32Status )
						i32Status = CS_CONFIG_CHANGED;
					goto ExitValidate;
				}
				else
				{
					i32Status = CS_INVALID_ACQ_MODE;
					goto ExitValidate;
				}
			}

			m_Stream.bEnabled = (StreamingMode == m_CaptureMode);
		}
		else if ( IsExpertSupported(u8Index, OCT ) )
		{
			u32DepthRes = 6; // ???? TBD

			if ( ! m_OCT_Config_set )
			{
				i32Status = CS_OCT_CORE_CONFIG_ERROR;
				goto ExitValidate;
			}

			// OCT works only in DUAL channel mode)
			if ( pAcqCfg->u32Mode & CS_MODE_SINGLE )
			{
				if ( u32Coerce )
				{
					pAcqCfg->u32Mode &= ~CS_MODE_SINGLE;
					pAcqCfg->u32Mode |= CS_MODE_DUAL;
					i32Status = CsValidateAcquisitionConfig( pAcqCfg, u32Coerce );
					if ( CS_SUCCESS == i32Status || CS_CONFIG_CHANGED == i32Status )
						i32Status = CS_CONFIG_CHANGED;
					goto ExitValidate;
				}
				else
				{
					i32Status = CS_INVALID_ACQ_MODE;
					goto ExitValidate;
				}
			}
			m_Stream.bEnabled = (StreamingMode == m_CaptureMode);
		}

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
				pAcqCfg->i64SegmentSize = DECADE_DEFAULT_SEGMENT_SIZE;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_SEGMENT_SIZE;
		}

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
					pAcqCfg->i64SegmentSize = i32MinSegmentSize; // same as u32DepthRes
				else
					pAcqCfg->i64SegmentSize = i64CoerceVal;

				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				if ( DataPacked_8 == m_ConfigDataPackMode )
					return CS_INVALID_DATA8_SEGMENT_SIZE;
				else
					return CS_INVALID_SEGMENT_SIZE;
			}
		}

		if ( !m_bOneSampleResolution && (pAcqCfg->i64Depth % u32DepthRes ) )
		{
			if (u32Coerce)
			{
				// Coerce the value by rounding down. If not possible then round up
				int64 i64CoerceVal = pAcqCfg->i64Depth - (pAcqCfg->i64Depth % u32DepthRes);
				if ( i64CoerceVal < i32MinSegmentSize )
					pAcqCfg->i64Depth = i32MinSegmentSize;		// same as u32DepthRes
				else
					pAcqCfg->i64Depth = i64CoerceVal;

				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				if ( DataPacked_8 == m_ConfigDataPackMode )
					return CS_INVALID_DATA8_TRIG_DEPTH;
				else
					return CS_INVALID_TRIG_DEPTH;
			}
		}

		if ( pAcqCfg->i64SegmentSize < pAcqCfg->i64Depth )
		{
			if (u32Coerce)
			{
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;
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

		// Decade is "square" and requires pAcqCfg->i64TriggerHoldoff >= pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth;
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
int32	CsDecade::CsAutoUpdateFirmware()
{
	return CS_FUNCTION_NOT_SUPPORTED; 
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt16	CsDecade::RemapChannel( uInt16 u16UserChannel )
{
	if ( CS_CHAN_1 == u16UserChannel )
		return CS_CHAN_2;
	else
		return CS_CHAN_1;
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
uInt16	CsDecade::ConvertToHwChannel( uInt16 u16UserChannel )
{
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & DECADE_ADDON_HW_OPTION_1CHANNEL6G) )
	{
		return CS_CHAN_2;
	}
	else
		return u16UserChannel;
}

//-----------------------------------------------------------------------------
//	Convert the Hw channel to User channel
//-----------------------------------------------------------------------------
uInt16	CsDecade::ConvertToUserChannel( uInt16 u16HwChannel )
{
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & DECADE_ADDON_HW_OPTION_1CHANNEL6G) )
	{
		return CS_CHAN_1;
	}
	else
		return u16HwChannel;
}

//-----------------------------------------------------------------------------
//	Convert the Hw channel of one card to the User Channel of M/S system
//-----------------------------------------------------------------------------
uInt16	CsDecade::ConvertToMsUserChannel( uInt16 u16HwChannel )
{
	 uInt16 u16UserChannel = ConvertToUserChannel( u16HwChannel );
	 return (u16UserChannel + GetBoardNode()->NbAnalogChannel*(m_u8CardIndex-1));
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsDecade::RestoreCalibDacInfo()
{
	//if ( m_pCardState->bUseDacCalibFromEEprom )
	//	RtlCopyMemory( &m_pCardState->DacInfo, &m_pCardState->CalibInfoTable.ChanCalibInfo, sizeof(m_pCardState->DacInfo) );
}

void CsDecade::UpdateCardState(PCSDECADE_DEVEXT pCardSt, BOOLEAN bReset)
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

int32	CsDecade::ReadCommonRegistryOnBoardInit()
{
#ifdef _WINDOWS_
	HKEY hKey;

	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsE12xGyWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);
		ul = m_bClearConfigBootLocation ? 1 : 0;
		::RegQueryValueEx( hKey, _T("ClearCfgBootLocation"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bClearConfigBootLocation = (ul != 0);

		::RegCloseKey( hKey );
	}	
#endif   

	return CS_SUCCESS;
}

#ifdef _WINDOWS_
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32 CsDecade::ReadCommonRegistryOnAcqSystemInit()
{

	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CsE12xGyWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_u8TrigSensitivity;
		::RegQueryValueEx( hKey, _T("DTriggerSensitivity"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul <= 255 )
			m_u8TrigSensitivity = (uInt8) ul;
		else
			m_u8TrigSensitivity = 255;

		ul = m_u32DefaultExtClkSkip;
		::RegQueryValueEx( hKey, _T("DefaultExtClkSkip"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul == CS_SAMPLESKIP_FACTOR/2 || ul == CS_SAMPLESKIP_FACTOR )
			m_u32DefaultExtClkSkip = ul;

		ul = m_bExtTrigLevelFix;
		::RegQueryValueEx( hKey, _T("ExtTrigLevelFix"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bExtTrigLevelFix = (0 != ul);
		
		ul = m_OverVoltageDetection;
		::RegQueryValueEx(hKey, _T("OverVoltageDetection"), NULL, NULL, (LPBYTE)&ul, &DataSize);
		m_OverVoltageDetection = ul;

		ul = m_bOneSampleResolution;
		::RegQueryValueEx(hKey, _T("OneSampleResolution"), NULL, NULL, (LPBYTE)&ul, &DataSize);
		m_bOneSampleResolution = (0 != ul);

		ReadCalibrationSettings( hKey );

		::RegCloseKey( hKey );
	}	

	return CS_SUCCESS;
}
#else
   
#define DRV_SECTION_NAME      "EonExpress"

//------------------------------------------------------------------------------
// Linux read from config file.
//------------------------------------------------------------------------------
int32 CsDecade::ReadCommonCfgOnAcqSystemInit()
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
 
	ul = m_u8TrigSensitivity;
	ul = GetCfgKeyInt(szSectionName, _T("DTriggerSensitivity"),  ul,  szIniFile);	
	if ( ul <= 255 )
		m_u8TrigSensitivity = (uInt8) ul;
	else
		m_u8TrigSensitivity = 255;

	ul = m_u32DefaultExtClkSkip;
	ul = GetCfgKeyInt(szSectionName, _T("DefaultExtClkSkip"),  ul,  szIniFile);	
	if ( ul == CS_SAMPLESKIP_FACTOR/2 || ul == CS_SAMPLESKIP_FACTOR )
		m_u32DefaultExtClkSkip = ul;

	ul = m_bExtTrigLevelFix;
	ul = GetCfgKeyInt(szSectionName, _T("ExtTrigLevelFix"),  ul,  szIniFile);	
	if ( ul != m_bExtTrigLevelFix )
		m_bExtTrigLevelFix = 1;

	ul = m_OverVoltageDetection;
	ul = GetCfgKeyInt(szSectionName, _T("OverVoltageDetection"),  ul,  szIniFile);	
	if ( ul != m_OverVoltageDetection )
		m_OverVoltageDetection = 1;

	ul = m_bOneSampleResolution;
	ul = GetCfgKeyInt(szSectionName, _T("OneSampleResolution"),  ul,  szIniFile);	
	if ( ul != m_bOneSampleResolution)
		m_bOneSampleResolution = ul;

	ReadCalibrationSettings( szIniFile, szSectionName );

	return CS_SUCCESS;
}

int32	CsDecade::ShowCfgKeyInfo()   
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
   printf("\t%s\n","BetterEnob");
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

int32	CsDecade::ReadCommonRegistryOnAcqSystemInit()
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

	ul = m_u32IgnoreCalibErrorMask;
	::QueryBackDoorParams(pElement, "IgnoreCalibError", &ul);
	m_u32IgnoreCalibErrorMask = ul;

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
bool	CsDecade::IsExtTriggerEnable( PARRAY_TRIGGERCONFIG pATrigCfg )
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
void CsDecade::InitializeDefaultAuxIoCfg()
{
	m_pCardState->eclkoClockOut		= DECADE_DEFAULT_CLKOUT;
	m_pCardState->eteTrigEnable		= DECADE_DEFAULT_EXTTRIGEN;
	m_pCardState->u16TrigOutMode	= CS_TRIGOUT_MODE;
	m_pCardState->bDisableTrigOut	= TRUE;
	m_pCardState->eteTrigEnable		= eteAlways;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsDecade::SetDefaultAuxIoCfg()
{
	SetClockOut(m_pCardState->eclkoClockOut);
	SetTrigOut(m_pCardState->bDisableTrigOut ? CS_OUT_NONE : CS_TRIGOUT);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool	CsDecade::IsImageValid( uInt8 u8Index )
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32Offset;
	CS_FIRMWARE_INFOS	FwInfo = {0};

	u32Offset = FIELD_OFFSET(DECADE_FLS_IMAGE_STRUCT, FwInfo) +
				FIELD_OFFSET(DECADE_FWINFO_SECTOR, FwInfoStruct);

	switch ( u8Index )
	{
		case 1:	u32Offset += FIELD_OFFSET(DECADE_FLASH_LAYOUT, FlashImage0); break;
		case 2:	u32Offset += FIELD_OFFSET(DECADE_FLASH_LAYOUT, FlashImage1); break;
		default:
			u32Offset += FIELD_OFFSET(DECADE_FLASH_LAYOUT, SafeBoot); break;
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
// Since we support DDC and DataPacked mode, each "sample" is no longer a 16-bit sample
// A 'sample' of data can be 64 bits (DDC) or 12 bits (Data Packed 12)...
// To avoid the confusion for the size of a 'sample', we will not refer to 'sample'
// as the unit of the TransferSize. 
// the TransferSize is now number of DWORDs (2BYTEs)
//-----------------------------------------------------------------------------

int32 	CsDecade::CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSize_DWORDs )
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
			if ( m_Stream.i64DataInFifo < u32TransferSize_DWORDs )
				u32TransferSize_DWORDs = (uInt32) m_Stream.i64DataInFifo;
		}
		else
			i32Status = CS_STM_FIFO_OVERFLOW;
	}
	else if ( ! m_Stream.bRunForever )
	{	
		if ( m_Stream.u64DataRemain < u32TransferSize_DWORDs )
			u32TransferSize_DWORDs = (uInt32) m_Stream.u64DataRemain;
	}

	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Only one DMA can be active 
	if ( ::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 1, 0 ) ) 
		return CS_INVALID_STATE;

	uInt32	u32TransferSizeBytes = 0;
//	if (m_DDC_enabled)
//	{
//		// m_Stream.u32TransferSize is in native 16-bit sample unit
//		m_Stream.u32TransferSize = u32TransferSizeSamples << 2;
//		u32TransferSizeBytes = u32TransferSizeSamples << 3; // DDC data is 64-bit, thus = 8 bytes
//	}
//	else
	{
		m_Stream.u32TransferSize = u32TransferSize_DWORDs;
		u32TransferSizeBytes = u32TransferSize_DWORDs << 1;
	}

	i32Status = IoctlStmTransferDmaBuffer( u32TransferSizeBytes, pVa );
	if ( CS_FAILED(i32Status) )
		::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 0, 1 );

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData )
{
	if ( ! m_Stream.bEnabled )
		return CS_INVALID_REQUEST;

	int32 i32Status = CsStmGetTransferStatus( u32TimeoutMs, pu32ErrorFlag );
	if (CS_SUCCESS == i32Status)
	{
		if (!m_Stream.bRunForever)
			m_Stream.u64DataRemain -= m_Stream.u32TransferSize;

		if ( pu32ActualSize )
			*pu32ActualSize		= m_Stream.u32TransferSize;

		if ( pu8EndOfData )
			*pu8EndOfData	= ( m_Stream.u64DataRemain == 0 );
	}

	return i32Status;
}
#define PATCH_FOR_12BIT_PACKED
#define MEM_ADJUST_FOR_FIRMWARE_BUG		0x8000000	// 128M DWORD = 256M Bytes
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag )
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
				uInt32	u32MemAdjustment = 0;	// Patch/work around for Firmware Bug in pack12 mode,

#ifdef	PATCH_FOR_12BIT_PACKED
				// Firmware Bug in pack12 mode,
				// in regular mode, mem record has 64 bytes (512 bits) contains 40 samples (480 bits) and 4 bytes padding.
				// Compute the padding total to be removed
				if (DataPacked_12 == m_DataPackMode)
					u32MemAdjustment = (uInt32) (m_PlxData->CsBoard.i64MaxMemory/64)*4;
#endif

				// Fifo full has occured. From this point all data in memory is still valid
				// We notify the application about the state of FIFO full only when all valid data are upload
				// to application
				ULONGLONG	ValidDataImMemory = (ULONGLONG) m_PlxData->CsBoard.i64MaxMemory - u32MemAdjustment;

				m_Stream.bFifoFull = true;
				if ( ! m_Stream.bRunForever && m_Stream.u64DataRemain <= ValidDataImMemory )
					m_Stream.i64DataInFifo = m_Stream.u64DataRemain;
				else
					m_Stream.i64DataInFifo = ValidDataImMemory;

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

void 	CsDecade::MsStmResetParams()
{
	CsDecade *pDev = m_MasterIndependent;
	
	while( pDev )
	{
		PSTREAMSTRUCT pStreamStr = &pDev->m_Stream;

		pStreamStr->bFifoFull			 = false;
		pStreamStr->u32BusyTransferCount = 0;
		pStreamStr->u32WaitTransferCount = 0;
		// Data remain in number of WORDs (2 BYTEs)
		pStreamStr->u64DataRemain		 = pStreamStr->u64TotalDataSize>>1;

		pDev = pDev->m_Next;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::MsInitializeDevIoCtl()
{
	int32		 i32Status = CS_SUCCESS;
	CsDecade* pDevice = m_MasterIndependent;

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

bool	CsDecade::IsExpertSupported( uInt8 u8ImageId, eXpertId xpId )
{
	return ((m_PlxData->CsBoard.u32BaseBoardOptions[u8ImageId] & xpId) != 0 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsDecade::PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg )
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
			// Total data size (without TAIL) in samples
			m_Stream.u64TotalDataSize = pCsAcqCfg->i64SegmentSize*(pCsAcqCfg->u32Mode & CS_MASKED_MODE) * i64SegmentCount;
			// Total data size (without TAIL) in BYTEs
			m_Stream.u64TotalDataSize = m_Stream.u64TotalDataSize*m_DataFormatInfo.u32SampleSize_Bits/8;

			if ( m_OCT_enabled )
				m_Stream.u64TotalDataSize <<= 1;  //OCT TBD

			// Total data size (included TAIL) in BYTEs
			if ( ! m_bMulrecAvgTD )
				m_Stream.u64TotalDataSize += m_32SegmentTailBytes * i64SegmentCount;
	}
}
	

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsDecade::GetMemorySize()
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
//	mode_select= (default 0:acq data, CS_DDC_ENABLE: DDC , CS_DDC_DEBUG_MUX: mux
//-----------------------------------------------------------------------------
int32	CsDecade::DDCModeSelect(uInt32 mode_select, bool bForce /* = false */ )
{
	int32	i32Status = CS_SUCCESS;
	uInt32	data = 0;

	if ( !bForce  && m_iDDC_ModeSelect == mode_select )
		return CS_SUCCESS;

#ifdef _DEBUG
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("DDCModeSelect 0x%x \n"),mode_select);
	OutputDebugString(szDbgText);
#endif
	if (mode_select==CS_MODE_NO_DDC)
	{
		i32Status = WriteRegisterThruNios(0, DECADE_DATA_SELECT);
#ifdef _DEBUG
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("write to 0x%x data 0x%x return %d \n"),DECADE_DATA_SELECT, 0, i32Status);
		OutputDebugString(szDbgText);
#endif
	}
	else if ( (mode_select>0) && (mode_select<=CS_DDC_MODE_LAST) )
	{
		if (mode_select==CS_DDC_MODE_ENABLE)
			data = 6;
		if (mode_select==CS_DDC_DEBUG_MUX)
			data = 4;
		if (mode_select==CS_DDC_DEBUG_NCO)
			data = 3;
		if (mode_select==CS_DDC_MODE_CH2)
			data = 7;
		data = (data << 1);
		i32Status = WriteRegisterThruNios(data, DECADE_DATA_SELECT);

#ifdef _DEBUG
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("mode_select=%d write to 0x%x data 0x%x return %d \n"),
			mode_select, DECADE_DATA_SELECT, data, i32Status);
		OutputDebugString(szDbgText);
#endif
	}

	m_iDDC_ModeSelect = mode_select;

	// Since DDCModeSelect() can be used to switch DDC and normal. We need to keep track the last DDC enable mode. 
	if (mode_select > CS_MODE_NO_DDC)
		m_iDDC_Enable_ModeSelect = mode_select;

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::DDCModeStop()
{
	int32	i32Status = CS_SUCCESS;

#ifdef _DEBUG
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("DDCModeStop\n"));
	OutputDebugString(szDbgText);
#endif

#ifdef DDC_FIRMWARE_AVAILABLE

	i32Status = WriteRegisterThruNios( 0, DECADE_DATA_SELECT);
	if( CS_FAILED(i32Status) )
		return i32Status;
#endif

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::SetDDCConfig(PDDC_CONFIG_PARAMS pConfig)
{
	int32	i32Status = CS_SUCCESS;
	uInt32	u32RegVal = 0;
	PDDC_CONFIG_PARAMS pCfg = pConfig;

#ifdef _DEBUG
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("SetDDCConfig NCOFreq %d NbStages %d ScaleFactors [ %d %d %d %d] \n"),
				pConfig->u32NCOFreq, 
				pConfig->NbStages, 
				pConfig->u8GainL1, 
				pConfig->u8GainL2, 
				pConfig->u8GainL3, 
				pConfig->u8GainL4);	
	
	OutputDebugString(szDbgText);
#endif

	TraceDDC(TRACE_DDC_SET_CONFIG, &pConfig->u32NCOFreq, &pConfig->NbStages, 
		&pConfig->u8GainL1, &pConfig->u8GainL2, &pConfig->u8GainL3, &pConfig->u8GainL4);

#ifdef DDC_FIRMWARE_AVAILABLE
	// NCO Delta 32bits 
	u32RegVal = (int)(((float)NCO_CONSTANT * (float)pCfg->u32NCOFreq ) / (float)187500.0);
#ifdef _DEBUG
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("write to DECADE_DDC_NCO_CTRL= 0x%x regval [%d 0x%x] \n"),
			DECADE_DDC_NCO_CTRL, u32RegVal, u32RegVal);
	OutputDebugString(szDbgText);
#endif

	WriteGageRegister( DECADE_DDC_NCO_CTRL, u32RegVal );	

	u32RegVal = (uInt32)( pCfg->NbStages ) & 0xF;
#ifdef _DEBUG
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("write to DECADE_DDC_DDS_COMMAND= 0x%x regval [%d 0x%x] \n"),
			DECADE_DDC_DDS_COMMAND, u32RegVal, u32RegVal);
	OutputDebugString(szDbgText);
#endif

	WriteGageRegister( DECADE_DDC_DDS_COMMAND, u32RegVal );	

	u32RegVal = ((pCfg->u8GainL4 & 7) << 12)| ((pCfg->u8GainL3 & 7) <<8) | 
		( (pCfg->u8GainL2 & 7)  << 4) | pCfg->u8GainL1 & 0x7;
#ifdef _DEBUG
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("write to DECADE_DDC_SCALE_FACTOR= 0x%x regval [%d 0x%x] \n"), 
			DECADE_DDC_SCALE_FACTOR | NIOS_WRTIE_BIT, u32RegVal, u32RegVal);
	OutputDebugString(szDbgText);
#endif

	WriteRegisterThruNios(u32RegVal, DECADE_DDC_SCALE_FACTOR | NIOS_WRTIE_BIT);

#ifdef _DEBUG
	u32RegVal = ReadGageRegister(DECADE_DDC_DDS_COMMAND);
	if (u32RegVal) 
	{
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Scale Factor overflow 0x%x \n"), u32RegVal);
		OutputDebugString(szDbgText);
	}
#endif


#endif


	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::GetDDCConfig(PDDC_CONFIG_PARAMS pConfig)
{
	int32	i32Status = CS_SUCCESS;
	uInt32	u32RegVal=0;
	float	fFrequency = 0.0;

#ifdef DDC_FIRMWARE_AVAILABLE
	if (!pConfig)
		return -1;

	u32RegVal = ReadGageRegister(DECADE_DDC_NCO_CTRL);
	/// convert back to Khz
	if (u32RegVal)
		fFrequency = (((float)187500.0 * u32RegVal)/(float)NCO_CONSTANT);
	pConfig->u32NCOFreq	= (uInt32)fFrequency;

	//memcpy(pConfig, m_pCardState->pDDCConfig, sizeof(DDC_CONFIG_PARAMS));
	pConfig->NbStages = (uInt16) ReadGageRegister(DECADE_DDC_DDS_COMMAND);

	u32RegVal = ReadRegisterFromNios( 0, DECADE_DDC_SCALE_FACTOR );

	pConfig->u8GainL1 = u32RegVal & 0x7;
	pConfig->u8GainL2 = (u32RegVal >> 4) & 0x7;
	pConfig->u8GainL3 = (u32RegVal >> 8) & 0x7;
	pConfig->u8GainL4 = (u32RegVal >> 12) & 0x7;

	TraceDDC(TRACE_DDC_GET_CONFIG, &pConfig->u32NCOFreq, &pConfig->NbStages, 
		&pConfig->u8GainL1, &pConfig->u8GainL2, &pConfig->u8GainL3, &pConfig->u8GainL4);
#endif
	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::SendDDCControl(uInt32 Cmd)
{
	int32		 i32Ret = CS_SUCCESS;
	uInt32		 u32Val = Cmd;

	TraceDDC(TRACE_DDC_SEND_CONTROL, &Cmd, NULL);
	i32Ret = DDCModeSelect(u32Val);
	return i32Ret;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::GetDDCScaleStatus(PDDC_SCALE_OVERFLOW_STATUS pConfig)
{
	int32	i32Status = CS_SUCCESS;
	uInt32	u32RegVal=0;

#ifdef DDC_FIRMWARE_AVAILABLE
	if (!pConfig)
		return -1;

	u32RegVal = ReadGageRegister(DECADE_DDC_DDS_COMMAND);
#ifdef _DEBUG
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("GetDDCScaleStatus 0x%x \n"), u32RegVal);
	OutputDebugString(szDbgText);
#endif

	if (u32RegVal & 0xFF0)
	{
		pConfig->bGainL1_OverFlow = (u32RegVal >> 4) & 0x1;
		pConfig->bGainL2_OverFlow = (u32RegVal >> 5) & 0x1;
		pConfig->bGainL3_OverFlow = (u32RegVal >> 6) & 0x1;
		pConfig->bGainL4_OverFlow = (u32RegVal >> 8) & 0x1;
	}

	TraceDDC(TRACE_DDC_GET_SCALE_STATUS, &pConfig->bGainL1_OverFlow, &pConfig->bGainL2_OverFlow, 
		&pConfig->bGainL3_OverFlow, &pConfig->bGainL4_OverFlow);
#endif
	return i32Status;
}

//------------------------------------------------------------------------------
// Enable DDC trace by modify registry key
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CsE12xGyWDF\Parameters\TraceDDC"
//------------------------------------------------------------------------------
void CsDecade::TraceDDC(uInt32 u32TraceFlag, void *param1, void *param2, void *param3, void *param4, void *param5, void *param6)
{
	char	szText[256]={0};

	szText[0] = 0;
	uInt32 u32TraceType = (u32TraceFlag &  m_u32DDCTraceDebug);

	UNREFERENCED_PARAMETER(param4);
	//sprintf_s( szText, sizeof(szText), TEXT("TraceDDC: u32TraceType 0x%x param1 %p \n"),u32TraceType, param1);
	//OutputDebugString(szText);
	//szText[0] = 0;

	switch (u32TraceType)
	{
		case TRACE_DDC_INFO:
			if ( param1!=NULL)
				sprintf_s( szText, sizeof(szText), TEXT("DDC> Info: %s \n"), ((char*)param1));
			break;

		case TRACE_DDC_SEND_CONTROL:
			if ( param1!=NULL)
				sprintf_s( szText, sizeof(szText), TEXT("DDC> SendDDCControl Command= %d \n"), *((uInt32*)param1));
			break;
			
		case TRACE_DDC_SET_CONFIG:
			//if ( ( param1!=NULL) & ( param2!=NULL) & ( param3!=NULL) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("DDC> SetConfig: NCOFreq %d NbStages %d GainL1 %d GainL2 %d GainL3 %d GainL4 %d\n"),
					*((uInt32*)param1 ), *((uInt8*)param2 ), *((uInt8*)param3 ), *((uInt8*)param4 ),
					*((uInt8*)param5 ), *((uInt8*)param6 ) );
			}
			break;

		case TRACE_DDC_GET_CONFIG:
			//if ( ( param1!=NULL) & ( param2!=NULL) & ( param3!=NULL) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("DDC> GetConfig: NCOFreq %d NbStages %d GainL1 %d GainL2 %d GainL3 %d GainL4 %d\n"),
					*((uInt32*)param1 ), *((uInt8*)param2 ), *((uInt8*)param3 ), *((uInt8*)param4 ),
					*((uInt8*)param5 ), *((uInt8*)param6 ) );
			}
			break;

		case TRACE_DDC_GET_SCALE_STATUS:
			//if ( ( param1!=NULL) & ( param2!=NULL) & ( param3!=NULL) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("DDC> Get Scale Status: GainL1 overf %d GainL2 overf %d GainL3 overf %d GainL4 overf %d\n"),
					*((BOOL*)param1 ), *((BOOL*)param2 ), *((BOOL*)param3 ), *((BOOL*)param4 ));
			}
			break;
			
		default:
			break;
			
	}

	OutputDebugString(szText);

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::SetOCTConfig(PCSOCT_CONFIG_PARAMS pConfig)
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32RegVal = 0;
	float		fLevel = 0.0;

	TraceOCT(TRACE_OCT_SET_CONFIG, NULL, pConfig->u8Slope, pConfig->i8Level, pConfig->u8Hysteresis, pConfig->u8PhyMode);
		
#ifdef OCT_FIRMWARE_AVAILABLE

	if ( (pConfig->i8Level >100) || (pConfig->i8Level <-100) )
		return CS_OCT_INVALID_CONFIG_PARAMS;

	//Convert to register value 12 bit
	fLevel = (float)( (pConfig->i8Level * 2047/100) + 2047 );
	u32RegVal = ( (uInt32)fLevel & 0xFFF) | ((pConfig->u8Slope & 0x1) <<12)| ((pConfig->u8Hysteresis & 0xFF)<< 16); 

#ifndef _DEBUG
	char	szText[256];
	sprintf_s( szText, sizeof(szText), TEXT("SetOCTConfig u32RegVal 0x%x fLevel %06f i8Level %d 0x%x \n"),
		u32RegVal, fLevel, pConfig->i8Level, pConfig->i8Level);
	OutputDebugString(szText);
#endif

	WriteRegisterThruNios( u32RegVal, DECADE_OCT_TRIGGER_REG | NIOS_WRTIE_BIT);
	//WriteRegisterThruNios( pConfig->u32OutDepth, DECADE_OCT_RECORD_REG ); // It's would be nice to set it separately.
	if (!pConfig->u8PhyMode)
		u32RegVal = 0;
	else
		u32RegVal = 6;		// bit1&bit2 set
	WriteRegisterThruNios( u32RegVal, DECADE_OCT_MODE_REG | NIOS_WRTIE_BIT ); 

#endif

	m_OCT_Config_set = true;
	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::GetOCTConfig(PCSOCT_CONFIG_PARAMS pConfig)
{
	int32		i32Status = CS_SUCCESS;
	float		fLevel = 0.0;
	if (pConfig==NULL)
		return(-1);
#ifdef OCT_FIRMWARE_AVAILABLE
	uInt32	u32RegVal = ReadRegisterFromNios( 0, DECADE_OCT_TRIGGER_REG );
	if ((u32RegVal & 0xFFF)>2047)
		fLevel = float( (u32RegVal & 0xFFF) - 2047)*100/2047;
	else
		fLevel = float( 2047 - (u32RegVal & 0xFFF) )*(-100)/2047;
	pConfig->i8Level = (int8)(fLevel);
	pConfig->u8Hysteresis = (u32RegVal >> 16) & 0xFF;
	pConfig->u8Slope = (u32RegVal >> 12) & 0x1;

#ifdef _DEBUG
	char	szText[128];
	sprintf_s( szText, sizeof(szText), TEXT("GetOCTConfig u32RegVal 0x%x fLevel %06f \n"),u32RegVal, fLevel);
	OutputDebugString(szText);
#endif

	u32RegVal = ReadRegisterFromNios( 0, DECADE_OCT_MODE_REG );
	if (u32RegVal & 0xFF)
		pConfig->u8PhyMode = 1;
	else
		pConfig->u8PhyMode = 0;

#endif
	TraceOCT(TRACE_OCT_GET_CONFIG, NULL, pConfig->u8Slope, pConfig->i8Level, pConfig->u8Hysteresis, pConfig->u8PhyMode);

	return i32Status;
}

int32 CsDecade::SetOCTRecordLength(uInt32 size)
{
	int32		i32Status = CS_SUCCESS;
	TraceOCT(TRACE_OCT_SET_RECORD_LENGTH, NULL, 0, 0, 0, size);
#ifdef OCT_FIRMWARE_AVAILABLE
	i32Status = WriteRegisterThruNios( size, DECADE_OCT_RECORD_REG | NIOS_WRTIE_BIT); 
#endif
	return i32Status;
}

int32 CsDecade::GetOCTRecordLength(uInt32 *size)
{
	int32		i32Status = CS_SUCCESS;
#ifdef OCT_FIRMWARE_AVAILABLE
	uInt32	u32RegVal = ReadRegisterFromNios( 0, DECADE_OCT_RECORD_REG );
	if (size)
		*size = u32RegVal;
#endif
	TraceOCT(TRACE_OCT_GET_RECORD_LENGTH, NULL, 0, 0, 0, *size);

	return i32Status;
}

//-----------------------------------------------------------------------------
//	mode_select = (default 0=CS_OCT_DISABLE:acq data, CS_OCT_ENABLE: OCT)
//-----------------------------------------------------------------------------
int32	CsDecade::OCTModeSelect(uInt32 mode_select, bool bForce /* = false */ )
{
	int32	i32Status = CS_SUCCESS;

	if ( !bForce  && m_iOCT_ModeSelect == mode_select )
		return CS_SUCCESS;

#ifdef _DEBUG
	char	szText[128];
	sprintf_s( szText, sizeof(szText), TEXT("OCTModeSelect 0x%x \n"),mode_select);
	OutputDebugString(szText);
#endif

#ifdef OCT_FIRMWARE_AVAILABLE
	if ( mode_select==CS_OCT_ENABLE)
	{
		i32Status = WriteRegisterThruNios(1, DECADE_OCT_COMMAND_REG | NIOS_WRTIE_BIT);
	}
	else
	{
		i32Status = WriteRegisterThruNios(0, DECADE_OCT_COMMAND_REG | NIOS_WRTIE_BIT);
	}
#ifdef _DEBUG
		sprintf_s( szText, sizeof(szText), TEXT("write mode_select = %d to 0x%x return %d \n"), 
			mode_select, DECADE_DATA_SELECT,i32Status);
		OutputDebugString(szText);
#endif
#endif

	m_iOCT_ModeSelect = mode_select;
	return i32Status;
}

//------------------------------------------------------------------------------
// Enable OCT trace by modify registry key
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CsE12xGyWDF\Parameters\TraceOCT"
//------------------------------------------------------------------------------
//void CsDecade::TraceOCT(uInt32 u32TraceFlag, void *param1, void *param2, void *param3, void *param4)
void CsDecade::TraceOCT(uInt32 u32TraceFlag, char *szDisplay, uInt8 u8Slope, uInt16 u16Level, uInt16 u16Hysteresis, uInt32 u32Value)
{
	char	szText[256]={0};

	szText[0] = 0;
	uInt32 u32TraceType = (u32TraceFlag &  m_u32OCTTraceDebug);

	//sprintf_s( szText, sizeof(szText), TEXT("TraceOCT: u32TraceType 0x%x param1 %p \n"),u32TraceType, param1);
	//OutputDebugString(szText);
	//szText[0] = 0;

	switch (u32TraceType)
	{
		case TRACE_OCT_INFO:
			if ( szDisplay!=NULL)
				sprintf_s( szText, sizeof(szText), TEXT("OCT> Info: %s \n"), ((char*)szDisplay));
			break;

		case TRACE_OCT_SET_CONFIG:
			{
				sprintf_s( szText, sizeof(szText), TEXT("OCT> SetConfig: Slope %d Level %d Hysteresis %d PhyMode %d \n"),
					(uInt8)(u8Slope) , (uInt16)(u16Level), (uInt16)(u16Hysteresis), (uInt8)(u32Value) );
			}
			break;

		case TRACE_OCT_GET_CONFIG:
			{
				sprintf_s( szText, sizeof(szText), TEXT("OCT> GetConfig: Slope %d Level %d Hysteresis %d PhyMode %d \n"),
					(uInt8)(u8Slope) , (uInt16)(u16Level), (uInt16)(u16Hysteresis), (uInt8)(u32Value) );
			}
			break;

		case TRACE_OCT_SET_RECORD_LENGTH:
			sprintf_s( szText, sizeof(szText), TEXT("OCT> SetRecordLen: RecordLen %d \n"), (uInt32)(u32Value) );
			break;

		case TRACE_OCT_GET_RECORD_LENGTH:
			sprintf_s( szText, sizeof(szText), TEXT("OCT> GetRecordLen: RecordLen %d \n"), (uInt32)(u32Value) );
			break;

		default:
			break;
	}

	OutputDebugString(szText);

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDecade::WriteRegisterThruNios(uInt32 u32Data, uInt32 u32Command, int32 i32Timeout_100us, uInt32 *pu32NiosReturn, bool bTracing)
{
	char	szText[256] = { 0 };

	int32 i32Ret = CsDevIoCtl::WriteRegisterThruNios(u32Data, u32Command, i32Timeout_100us, pu32NiosReturn);
	if ( (CS_FRM_NO_RESPONSE == i32Ret) && (bTracing == true) )
	{
		sprintf_s(szText, sizeof(szText), "NIOS Timeout on the Command 0x%x, Data= 0x%x\n", u32Command, u32Data);

		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szText);
	}

	return i32Ret;
}

void CsDecade::BBtiming(uInt32 u32Delay_100us)
{
	if( 0 == u32Delay_100us )
		return;
	uInt32 u32Delay = u32Delay_100us * DECADE_WD_TIMER_FREQ_100us;

	WriteGageRegister(WD_TIMER, u32Delay);

	WriteGageRegister(GEN_COMMAND, WATCH_DOG_STOP_MASK);
	WriteGageRegister(GEN_COMMAND, WATCH_DOG_RUN_MASK);
	WriteGageRegister(GEN_COMMAND, 0);

	do
	{
	}while( 0 == (ReadGageRegister( GEN_COMMAND ) & WD_TIMER_EXPIRED) );
}

void CsDecade::DetectPingpongAcquisitions( PCSACQUISITIONCONFIG pAcqCfg )
{
	if (0 == pAcqCfg->u32ExtClk)
		m_bPingpongCapture = ((6000000000 == pAcqCfg->i64SampleRate) || (2000000000 == pAcqCfg->i64SampleRate));
	else
		m_bPingpongCapture = (500 == pAcqCfg->u32ExtClkSampleSkip);
}
