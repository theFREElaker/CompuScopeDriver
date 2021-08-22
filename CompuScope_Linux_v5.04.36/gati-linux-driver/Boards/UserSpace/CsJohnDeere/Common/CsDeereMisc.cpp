#include "StdAfx.h"
#include "CsDeere.h"
#include "CsDeereFlash.h"
#include "CsDeereFlashObj.h"
#include "CsPrivatePrototypes.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif
extern SHAREDDLLMEM Globals;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32	CsDeereDevice::DeviceInitialize(void)
{
	int32		i32Status = CS_SUCCESS;
	BOOLEAN		bInitSuccess = FALSE;

	i32Status = CsPlxBase::Initialized();
	if ( CS_FAILED(i32Status) )
		return i32Status;

#ifdef USE_SHARED_CARDSTATE
	if ( m_pCardState->bCardStateValid )
	{
		if (  m_pCardState->bAddOnRegistersInitialized )
		{
			InitBoardCaps();
			return CS_SUCCESS;
		}
	}
	else if ( 0 != m_pCardState->u8ImageId  )
		CsLoadFpgaImage( 0 );
#else
	DEERE_CARDSTATE			tempCardState = {0};

	i32Status = IoctlGetCardState(&tempCardState);
	
	ASSERT(CS_SUCCEEDED(i32Status));
	if ( CS_FAILED(i32Status) )
		return i32Status;

	if ( tempCardState.bCardStateValid )
	{
		GageCopyMemoryX(m_pCardState, &tempCardState,  sizeof(DEERE_CARDSTATE));
		if ( m_pCardState->bAddOnRegistersInitialized )
		{
			InitBoardCaps();
			return CS_SUCCESS;
		}
	}
	else
	{
		m_pCardState->u8ImageId		= tempCardState.u8ImageId;
		m_pCardState->u16CardNumber = tempCardState.u16CardNumber;
		if ( 0 != tempCardState.u8ImageId )
			CsLoadFpgaImage( 0 );
	}
#endif

	m_pCardState->bAddOnRegistersInitialized = false;

	RtlZeroMemory( &m_pCardState->AddOnReg, sizeof(m_pCardState->AddOnReg) );
	RtlZeroMemory( m_pCardState->bCalNeeded, sizeof(m_pCardState->bCalNeeded) );

	m_pCardState->u32Rcount		= DEERE_4360_CLK_1V1; //for 1V0 will be redefined for 1V0
	m_pCardState->u32Ncount		= DEERE_HI_CAL_FREQ;
	m_pCardState->bDpaNeeded	= true;
	m_pCardState->ermRoleMode	= ermStandAlone;
	m_pCardState->u8MsTriggMode		= DEERE_DEFAULT_MSTRIGMODE;
	m_pCardState->bAdcAlignCalibReq = true;
	m_pCardState->bLowCalFreq	= (m_pCardState->u32Ncount != DEERE_HI_CAL_FREQ);
	SetDefaultAuxIoCfg();

	m_UseIntrTrigger = m_UseIntrBusy = FALSE;
	m_BusMasterSupport = TRUE;

#define  BYPASS_READNVRAM			// Patch for Honywell Linux System

	PLX_NVRAM_IMAGE	nvRAM_Image = { 0 };
#ifdef  BYPASS_READNVRAM
	// Reading NvRAM in Linux from a new PC crashes the system
	// Simulate reading Baseboard HW info
	nvRAM_Image.Data.BaseBoardVersion = 0x110;
	nvRAM_Image.Data.BaseBoardSerialNumber = 0x125fffff;		// Default serial number
	nvRAM_Image.Data.BaseBoardHardwareOptions = 0;
	m_PlxData->u32NvRamSize = nvRAM_Image.Data.MemorySize = 256*1024;
#else
	i32Status = CsReadPlxNvRAM( &nvRAM_Image );
	if ( CS_FAILED( i32Status ) )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "Error while reading NVRAM." );
		return i32Status;
	}
#endif

	// Baseboard HW info
	m_pCsBoard->u32BaseBoardVersion				= nvRAM_Image.Data.BaseBoardVersion;
	m_pCsBoard->u32SerNum						= nvRAM_Image.Data.BaseBoardSerialNumber;
	m_pCsBoard->u32BaseBoardHardwareOptions		= nvRAM_Image.Data.BaseBoardHardwareOptions;

	if( IsNiosInit() )
	{
		UpdateMaxMemorySize( m_pCsBoard->u32BaseBoardHardwareOptions, CS_SAMPLE_SIZE_2 );
		ClearInterrupts();
		GetInitStatus()->Info.u32AddOnPresent = TRUE;
		ReadHardwareInfoFromEeprom(FALSE);
		AssignBoardType();
		InitBoardCaps();

		// Read all frimware version
		RtlZeroMemory( &m_pCardState->VerInfo, sizeof(m_pCardState->VerInfo) );
		m_pCardState->VerInfo.u32Size = sizeof(CS_FW_VER_INFO);
		ReadVersionInfo(2);	// expert 2
		ReadVersionInfo(1);		// expert 1
		ReadVersionInfo(0);		

		i32Status = InitRegisters();
		if ( CS_FAILED (i32Status) )
		{
			// Fail to init the addon board.
			// Possible because of invalid firmwares. Attempt to update firmware later
			ReadAndValidateCsiFile( m_pCsBoard->u32BaseBoardHardwareOptions, 0 );
			CheckRequiredFirmwareVersion();
			if( 0 == m_pCardState->u8BadFirmware )
			{
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Fail to initialize board." );
			}
		}
		bInitSuccess = true;
	}
	else
	{
		::OutputDebugString("Nios init failed.\n ");
		// Nios error. This may caused by bad base board firmware
		// Set Flag to force update firmware later
		m_pCardState->u8BadFirmware = BAD_DEFAULT_FW;
	}

	UpdateCardState(bInitSuccess);

	// Intentional return success to allows some utilitiy programs to update firmware in case of errors
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::MsCsResetDefaultParams( bool bReset )
{
	int32 i32Ret = CS_SUCCESS;
	CsDeereDevice *pDevice = m_MasterIndependent;

	// Set Default configuration
	while( pDevice )
	{
		i32Ret = pDevice->CsSetAcquisitionConfig();
		if( CS_FAILED(i32Ret) )	return i32Ret;
		pDevice = pDevice->m_Next;
	}

	// Disable TrigOut and BusyOut 
	ConfiureTriggerBusyOut();

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		RtlZeroMemory( pDevice->m_CfgTrigParams, sizeof( m_CfgTrigParams ) );

		if(!pDevice->GetInitStatus()->Info.u32Nios )
			return CS_FLASH_NOT_INIT;

		i32Ret = pDevice->InitRegisters();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( bReset )
		{
			// Reset all params and force re-calibrate front end on the next CsSetChannelConfig()
			RtlZeroMemory( &pDevice->m_pCardState->ChannelParams, sizeof(m_pCardState->ChannelParams) );
			pDevice->m_pCardState->AddOnReg.u32FeSetup[0]=pDevice->m_pCardState->AddOnReg.u32FeSetup[1]= 0;
	
			i32Ret = pDevice->CsSetChannelConfig();
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		else
		{
#ifdef _WINDOWS_
			i32Ret = pDevice->CsSetChannelConfig();
#else
			i32Ret = pDevice->CsSetChannelConfig(NULL, false);
#endif
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}

		i32Ret = pDevice->CsSetTriggerConfig();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = pDevice->SendAdcSelfCal();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		pDevice = pDevice->m_Next;
	}

	i32Ret = MsCalibrateAdcAlign();
	i32Ret = MsCalibrateSkew();

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		i32Ret = pDevice->CalibrateAllChannels();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		pDevice->ConfigureAlarmSource();

		pDevice = pDevice->m_Next;
	}

	MsMatchAdcPhaseAllChannels();
	MsAdjustDigitalDelay();

	// Restore TrigOut and BusyOut after calibration
	ConfiureTriggerBusyOut( !m_pCardState->bDisableTrigOut, eAuxOutBusyOut == m_pCardState->AuxOut );
	// Disable gate triggering
	ConfigureAuxIn( false );

	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16	CsDeereDevice::DetectAcquisitionSystems( BOOL bForceInd )
{
	CsDeereDevice	*pDevice = (CsDeereDevice *) m_ProcessGlblVars->pFirstUnCfg;
	CsDeereDevice	*pFirstCardInSystem[20];

	while( pDevice )
	{
		// Undo M/S link
		pDevice->IoctlSetMasterSlaveLink( NULL, 0 );
		pDevice->m_pCardState->ermRoleMode = ermStandAlone;
		pDevice->m_pNextInTheChain = 0;
		pDevice->m_Next =  pDevice->m_HWONext;
		pDevice = pDevice->m_HWONext;
	}

	uInt32			i = 0;
	uInt16			NbCardInSystem;
	CsDeereDevice	*FirstCard = NULL;
	DRVSYSTEM		*pSystem;
	CSSYSTEMINFO	*SysInfo;

	if( !bForceInd )
	{
		int16 i16NumMasterSlave = 0;
		int16 i16NumStanAlone = 0;

		pSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

		DetectMasterSlave( &i16NumMasterSlave, &i16NumStanAlone );
		if( i16NumMasterSlave > 0 )
		{
			while( (NbCardInSystem = BuildMasterSlaveSystem( &FirstCard ) ) != 0 )
			{
				pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];
				pFirstCardInSystem[i] = FirstCard;

				pSystem->DrvHandle	= FirstCard->m_PseudoHandle;
				pSystem->u8FirstCardIndex = FirstCard->m_u8CardIndex;

				SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
				SysInfo->u32BoardCount = NbCardInSystem;

				FirstCard->UpdateSystemInfo( SysInfo );
				i++;
			}
		}
	}


	while( ( NbCardInSystem = BuildIndependantSystem( &FirstCard, (BOOLEAN) bForceInd ) ) != 0 )
	{
		pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];
		pFirstCardInSystem[i] = FirstCard;

		pSystem->DrvHandle	= FirstCard->m_PseudoHandle;
		pSystem->u8FirstCardIndex = FirstCard->m_u8CardIndex;

		SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		SysInfo->u32BoardCount = NbCardInSystem;

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

		CsDeereDevice *pDev = (CsDeereDevice *) pFirstCardInSystem[i];
		j = 0;

		memset(pSystem->u8CardIndexList, 0, sizeof(pSystem->u8CardIndexList));
		while( pDev )
		{
			// Set M/S card index list
			pSystem->u8CardIndexList[j++] = pDev->m_u8CardIndex;

			pDev->m_pSystem = pSystem;
			pDev = pDev->m_Next;
		}

		pDev = (CsDeereDevice *) pFirstCardInSystem[i];
		SysInfo = &Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo;
		pSystem->u16AcqStatus = ACQ_STATUS_READY;
	}

	return m_ProcessGlblVars->NbSystem;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsDeereDevice::BuildMasterSlaveSystem( CsDeereDevice** ppFirstCard )
{
	CsDeereDevice*	pCurUnCfg	= (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
	CsDeereDevice*	pMaster = NULL;

	//Look for the Master
	while( NULL != pCurUnCfg )
	{
		//if ( ermMaster == pCurUnCfg->m_ShJdData->ermRoleMode )
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

	*ppFirstCard  = pMaster;
	CsDeereDevice*	pCard = pMaster;
	uInt16			u16CardInSystem = 0;
	LONGLONG		llMaxMem = pMaster->m_pCsBoard->i64MaxMemory;
	while ( pCard )
	{
		//remove card from unconfigured list
		pCard->RemoveDeviceFromUnCfgList();
		
		pCard->m_pTriggerMaster = pMaster;
		pCard->m_Next = pCard->m_pNextInTheChain;
		pCard->m_MasterIndependent = pMaster;
		pCard->m_pVirtualMaster	 = pMaster;
		pCard->m_pCardState->u16CardNumber = ++u16CardInSystem;	

		llMaxMem = llMaxMem < pCard->m_pCsBoard->i64MaxMemory ? llMaxMem : pCard->m_pCsBoard->i64MaxMemory;

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
		if ( pMaster->m_pCsBoard->u32BoardType != pCard->m_pCsBoard->u32BoardType )
		{
#if DBG
			t.Trace( TraceInfo, "Master/Slave descrepancy. Master BT = 0x%x, Card Num %d BT = 0x%x\n", 
				pMaster->m_pCsBoard->u32BoardType, pCard->m_ShJdData->u16CardNumber, pCard->m_pCsBoard->u32BoardType );
#else
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Master/Slave descrepancy." );
#endif
			bRemoveSystem = true;
			break;
		}

		if ( CS_FAILED( pCard->CheckMasterSlaveFirmwareVersion() ) )
		{
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
			pCard->m_pCsBoard->i64MaxMemory = llMaxMem;
		
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
		sprintf_s( szText, sizeof(szText), TEXT("Eon Master / Slave system of %d boards initialized\n"), u16CardInSystem);
		::OutputDebugString(szText);
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_TEXT, szText );
	}
		
	return u16CardInSystem;
}

//-----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------- 

uInt16 CsDeereDevice::BuildIndependantSystem( CsDeereDevice** FirstCard, BOOLEAN bForceInd )
{
	CsDeereDevice*	pCard = (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
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
#if DBG
				t.Trace(TraceInfo, "Zero memory\n" );
#else
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Zero memory" );
#endif
//				pCard->AddDeviceToInvalidCfgList();
				return 0;
			}
		
			if (pCard->GetInitStatus()->Info.u32Nios && pCard->m_pCardState->bAddOnRegistersInitialized)
				pCard->AddonReset();
			else
			{				
				return 0;
			}

			if ( 0 == m_pCardState->u8BadFirmware )
				bRemoveSystem = false;
			
			if(!bRemoveSystem)
			{
				char	szText[128];
				sprintf_s( szText, sizeof(szText), TEXT("Eon System %x initialized\n"), pCard->m_pCsBoard->u32SerNum);
				::OutputDebugString(szText);
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

			*FirstCard = pCard;
			return 1;
		}
	}
		
	return 0;
}

	
//-----------------------------------------------------------------------------
//----- CsDeereDevice::UpdateSystemInfo(PSYSTEM_NODE pSys)
//-----------------------------------------------------------------------------

void	CsDeereDevice::UpdateSystemInfo( CSSYSTEMINFO *SysInfo )
{
	PCS_BOARD_NODE  pBdParam = GetBoardNode();

	SysInfo->u32Size				= sizeof(CSSYSTEMINFO);
	SysInfo->u32SampleBits			= m_pCardState->AcqConfig.u32SampleBits;
	SysInfo->i32SampleResolution	= m_pCardState->AcqConfig.i32SampleRes;
	SysInfo->u32SampleSize			= m_pCardState->AcqConfig.u32SampleSize;
	SysInfo->i32SampleOffset		= m_pCardState->AcqConfig.i32SampleOffset;
	SysInfo->u32BoardType			= pBdParam->u32BoardType;
	// External trigger only avaiable from master card
	SysInfo->u32TriggerMachineCount = 1 + (pBdParam->NbTriggerMachine-1)*SysInfo->u32BoardCount;
	SysInfo->u32ChannelCount		= pBdParam->NbAnalogChannel * SysInfo->u32BoardCount;
	SysInfo->u32BaseBoardOptions	= pBdParam->u32BaseBoardHardwareOptions;
	SysInfo->u32AddonOptions		= pBdParam->u32AddonHardwareOptions;
	SysInfo->i64MaxMemory			= pBdParam->i64MaxMemory;

	AssignAcqSystemBoardName( SysInfo );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::InitRegisters0(bool bForce)
{
	int32 i32Ret = CS_SUCCESS;

	if( m_pCardState->bAddOnRegistersInitialized && !bForce )
		return i32Ret;

	if( !GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	// Wait for Addon board ready
	CSTIMER		CsTimeout;
	CsTimeout.Set( CS_WAIT_TIMEOUT );
	for(;;)
	{
		uInt32	u32Temp = ReadRegisterFromNios( 0, DEERE_FWV_RD_CMD );
		if ( u32Temp != FAILED_NIOS_DATA ) 
			break;

		if ( CsTimeout.State() )
		{
			u32Temp = ReadRegisterFromNios( 0, DEERE_FWV_RD_CMD );
			if ( u32Temp != FAILED_NIOS_DATA ) 
				break;
			else
				return CS_ADDONINIT_ERROR;	// timeout !!!
		}
	}

	// Read but skip validating Addon firmware
	// The firmware will be re-validate later when CsRm calls GetAcquisitionSystemCount()
	i32Ret = ReadHardwareInfoFromEeprom(FALSE);
	if( CS_FAILED ( i32Ret ) )	return i32Ret;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsDeereDevice::ReadHardwareInfoFromEeprom( BOOLEAN bCheckSum )
{
	int32		i32Status = CS_SUCCESS;
	uInt32		u32Offset;
	DEERE_FLASH_FIRMWARE_INFOS		FwInfo[DEERE_NUMBER_OF_FLASHIMAGES] = {0};
	CsDeereFlash FlashObj( GetPlxBaseObj() );


	for ( uInt32 i = 0; i < DEERE_NUMBER_OF_FLASHIMAGES; i ++ )
	{
		if ( 0 == i )
			u32Offset = FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage1);
		else if ( 1 == i )
			u32Offset = FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage2);
		else
			u32Offset = FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage3);

		u32Offset += FIELD_OFFSET(DEERE_FLASH_IMAGE_STRUCT, FwInfo) + FIELD_OFFSET(DEERE_FLASH_FWINFO_SECTOR, FwInfoStruct);
		i32Status = IoctlReadFlash( u32Offset, &FwInfo[i], sizeof(DEERE_FLASH_FIRMWARE_INFOS) );
		if ( CS_FAILED(i32Status) )	return i32Status;
	}

	if ( ! m_bSkipFirmwareCheck )
	{
		// Validate Checksum of the default image
		if ( bCheckSum &&  (FwInfo[0].u32ChecksumSignature != CSI_FWCHECKSUM_VALID ) ) 
		{
			CsEventLog EventLog;
			EventLog.Report( CS_FWCHECKSUM_ERROR, "" );
			m_pCardState->u8BadFirmware = BAD_DEFAULT_FW;
			return CS_INVALID_FW_VERSION;
		}
	}

	// Reading Flash Footer info ( ADDON )
	DEERE_FLASHFOOTER DeereFooter = {0};	

	u32Offset = FIELD_OFFSET(DEERE_FLASH_LAYOUT, Footer) + FIELD_OFFSET(DEERE_FLASHFOOTER, Footer);
	i32Status = IoctlReadFlash( u32Offset, &DeereFooter, sizeof(DeereFooter) );
	if ( CS_SUCCESS == i32Status )
	{
#ifdef BYPASS_READNVRAM
		m_pCsBoard->u32SerNum = DeereFooter.Footer.u32SerialNumber;			// Base board serial number
#endif
		// Unitiialized Eeprom has all bits set
		// Reset options in case of uninitilzed eeprom
		if ( DeereFooter.Footer.u32HwOptions == (uInt32)(-1) )
			DeereFooter.Footer.u32HwOptions = 0;

		if ( -1 == DeereFooter.Footer.i64ExpertPermissions  )
			DeereFooter.Footer.i64ExpertPermissions = 0;

		m_pCsBoard->u32AddonHardwareOptions = DeereFooter.Footer.u32HwOptions;

		for ( uInt32 i = 0; i < DEERE_NUMBER_OF_FLASHIMAGES; i ++ )
		{
			if ( (FwInfo[i].u32ChecksumSignature == CSI_FWCHECKSUM_VALID) && (FwInfo[i].u32FwOptions != -1) )
			{
				m_pCsBoard->u32BaseBoardOptions[i] = FwInfo[i].u32FwOptions & DeereFooter.Footer.i64ExpertPermissions;
				m_pCsBoard->u32RawFwVersionB[i] = FwInfo[i].u32FwVersion;
			}
			else
			{
				m_pCsBoard->u32BaseBoardOptions[i] = 0;
				m_pCsBoard->u32RawFwVersionB[i] = 0;
			}
		}

		if ( CS_CLKOUT_REF >= DeereFooter.u16ClkOut )
			m_pCardState->eclkoClockOut = (eClkOutMode)DeereFooter.u16ClkOut;
		if ( 0xFFFF != DeereFooter.u16TrigOutEnabled)
			m_pCardState->bDisableTrigOut = (0 == DeereFooter.u16TrigOutEnabled);

		// Aux I/O configurations from eeprom
		if ( 0 != (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_AUX_IO) )
		{
			if ( CS_EXTTRIGEN_NEG_LATCH_ONCE > DeereFooter.AuxInCfg.u16ExtTrigEnVal )
				m_pCardState->u16ExtTrigEnCfg = DeereFooter.AuxInCfg.u16ExtTrigEnVal;
			if ( 0xFFFF != DeereFooter.AuxInCfg.u16ModeVal )
				m_pCardState->AuxIn = (eAuxIn) DeereFooter.AuxInCfg.u16ModeVal;
			if ( 0xFFFF != DeereFooter.AuxOutCfg )
				m_pCardState->AuxOut = (eAuxOut) DeereFooter.AuxOutCfg;
		}

		m_pCardState->bLowCalFreq   = ( 0 != (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_LOW_CALIB_FREQ) );
		m_pCardState->u32Ncount	  = m_pCardState->bLowCalFreq  ? DEERE_LOW_CAL_FREQ : DEERE_HI_CAL_FREQ;
	}
	else
	{
		m_pCsBoard->u32BaseBoardHardwareOptions = 0;
		m_pCsBoard->u32AddonHardwareOptions = 0;
		RtlZeroMemory( m_pCsBoard->u32AddonOptions, sizeof(m_pCsBoard->u32AddonOptions) );
		m_pCardState->eclkoClockOut = (eClkOutMode)DEERE_DEFAULT_CLOCK_OUT;
	}

	m_pCsBoard->i64ExpertPermissions = DeereFooter.Footer.i64ExpertPermissions;

	return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsDeereDevice::ReadCalibTableFromEeprom(uInt8	u8CalibTableId)
{
	uInt32	u32Addr;
	uInt32	u32EepromSize;


	switch ( u8CalibTableId )
	{
		case 2:
			u32Addr = FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage2 );
			break;
		default:
			u32Addr = FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage1 );
			break;
	}

	u32Addr += FIELD_OFFSET(DEERE_FLASH_IMAGE_STRUCT, Calib);
	u32EepromSize = sizeof((((DEERE_FLASH_IMAGE_STRUCT *)0)->Calib));

	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  DEERE_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....

	uInt32 u32CalibTableSize = sizeof(m_CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	CsDeereFlash  FlashObj( GetPlxBaseObj() );
	
	uInt32	u32CalTblValid = 0;
	uInt32	u32CalTblChksum = 0;
	int32	i32Status = CS_SUCCESS;
	uInt32	u32Offset = 0;

	u32Offset = FIELD_OFFSET(DEERE_CAL_INFO, u32Valid);
	IoctlReadFlash( u32Addr + u32Offset, &u32CalTblValid, sizeof(u32CalTblValid) );
	
	u32Offset = FIELD_OFFSET(DEERE_CAL_INFO, u32Checksum);
	IoctlReadFlash( u32Addr + u32Offset, &u32CalTblChksum, sizeof(u32CalTblChksum) );

	if ( (-1 == u32CalTblValid ) || (0 == u32CalTblValid ) ||
		 (DEERECALIBINFO_CHECKSUM_VALID != u32CalTblChksum) )
	{
		// In valid caltable
		u32CalTblValid = 0;
	}
	else
	{
		// Read whole calib table
		i32Status = IoctlReadFlash( u32Addr, &m_CalibInfoTable, u32CalibTableSize );
		if ( CS_FAILED( i32Status ) )	u32CalTblValid = 0;
	}

	if ( 0 != (u32CalTblValid & DEERE_CAL_ADC_VALID) )
		RtlCopyMemory( m_pCardState->AdcReg, m_CalibInfoTable.AdcCalInfo, sizeof(m_pCardState->AdcReg) );
	
	if ( 0 != (u32CalTblValid & DEERE_CAL_DAC_VALID) )
		RtlCopyMemory( &m_pCardState->DacInfo, &m_CalibInfoTable.ChanCalibInfo, sizeof(m_pCardState->DacInfo) );
	

	// both dac and Adc portion need to be valid to use eeprom cal table
	m_bUseDacCalibFromEEprom = (DEERE_CAL_DAC_VALID|DEERE_CAL_ADC_VALID) == (u32CalTblValid & (DEERE_CAL_DAC_VALID|DEERE_CAL_ADC_VALID));

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsDeereDevice::WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid )
{
	uInt32	u32Addr;
	uInt32	u32EepromSize;

	switch ( u8CalibTableId )
	{
		case 2:
			u32Addr = FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage2 );
			break;
		default:
			u32Addr = FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage1 );
			break;
	}

	u32Addr += FIELD_OFFSET(DEERE_FLASH_IMAGE_STRUCT, Calib);
	u32EepromSize = sizeof((((DEERE_FLASH_IMAGE_STRUCT *)0)->Calib));

	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  DEERE_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....

	uInt32 u32CalibTableSize = sizeof(m_CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	// Update the Calib Table with all latest Dac values
	RtlCopyMemory( &m_CalibInfoTable.ChanCalibInfo, &m_pCardState->DacInfo, sizeof(m_pCardState->DacInfo) );

	RtlCopyMemory( m_CalibInfoTable.AdcCalInfo, m_pCardState->AdcReg, sizeof(m_pCardState->AdcReg) );

	m_CalibInfoTable.u32Size = sizeof( m_CalibInfoTable );
	m_CalibInfoTable.u32Checksum = DEERECALIBINFO_CHECKSUM_VALID;

	//Clear valid bits if invalid were set
	uInt32 u32InvalidMask = u32Valid & (DEERE_VALID_MASK >> 1);
	m_CalibInfoTable.u32Valid &= ~(u32InvalidMask << 1);

	//set valid bits from input
	m_CalibInfoTable.u32Valid |= u32Valid & DEERE_VALID_MASK;

	CsDeereFlash	FlashObj( GetPlxBaseObj() );

	ResetCachedState();
	return FlashObj.FlashWriteEx( u32Addr, &m_CalibInfoTable, sizeof(m_CalibInfoTable) );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDeereDevice::ReadVersionInfo(uInt8 u8ImageId)
{
	if ( m_pCardState->u8ImageId != u8ImageId )
	{
		switch( u8ImageId )
		{
		case 2:
		case 1:
			if ( 0 != m_pCsBoard->u32BaseBoardOptions[u8ImageId] )
			{
				BaseBoardConfigReset( u8ImageId );
			}
			else
				return;
			break;

		case 0:
		default:
			u8ImageId = 0;
			BaseBoardConfigReset(0);
			break;
		}
	}

	uInt8 u8BootCpldVer = (uInt8) ReadFlashRegister(VERSION_REG_PLX);
	//Version is split 5:3 instead of 4:4
	m_pCardState->VerInfo.BbCpldInfo.Info.uMajor = (u8BootCpldVer >> 3) & 0xF;
	m_pCardState->VerInfo.BbCpldInfo.Info.uMinor = u8BootCpldVer & 0x7;

	if ( GetInitStatus()->Info.u32Nios )
	{
		uInt32 u32Data = 0x7FFF & ReadRegisterFromNios(0,DEERE_HWR_RD_CMD);	
		u32Data <<= 16;
		u32Data |= (0x7FFF & ReadRegisterFromNios(0,DEERE_SWR_RD_CMD));
		m_pCardState->VerInfo.BbFpgaInfo.u32Reg = m_pCsBoard->u32RawFwVersionB[m_pCardState->u8ImageId] = u32Data; 

		// Build user version info
		m_pCardState->VerInfo.BaseBoardFwVersion.Version.uMajor		= m_pCardState->VerInfo.BbFpgaInfo.Version.uMajor;
		m_pCardState->VerInfo.BaseBoardFwVersion.Version.uMinor		= m_pCardState->VerInfo.BbFpgaInfo.Version.uMinor;
		m_pCardState->VerInfo.BaseBoardFwVersion.Version.uRev		= m_pCardState->VerInfo.BbFpgaInfo.Version.uRev;
		m_pCardState->VerInfo.BaseBoardFwVersion.Version.uIncRev	= m_pCardState->VerInfo.BbFpgaInfo.Version.uIncRev;
		m_pCsBoard->u32UserFwVersionB[m_pCardState->u8ImageId].u32Reg = m_pCardState->VerInfo.BaseBoardFwVersion.u32Reg;

		u32Data = ReadRegisterFromNios(0,DEERE_AN_FWR_CMD);
		///MMMMMMM:mmmm:xxxx
		m_pCardState->VerInfo.ConfCpldInfo.Info.uRev = ( u32Data >> 8 ) & 0x7F;
		m_pCardState->VerInfo.ConfCpldInfo.Info.uMajor = ( u32Data >> 4 ) & 0xF;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::CsLoadFpgaImage( uInt8 u8ImageId )
{
	BOOLEAN		bConfigReset = FALSE;
	switch( u8ImageId )
	{
		case 1:
		{
			if ( m_pCardState->u8ImageId != 1 )
			{
				if ( m_pCsBoard->u32BaseBoardOptions[u8ImageId] != 0 )
				{
					BaseBoardConfigReset(1);
					m_pCardState->AcqConfig.u32Mode &= ~(CS_MODE_USER1 | CS_MODE_USER2);
					m_pCardState->AcqConfig.u32Mode |= CS_MODE_USER1;
					bConfigReset = TRUE;
				}
				else
					return CS_INVALID_ACQ_MODE;
			}
		}
		break;

		case 2:
		{
			if ( m_pCardState->u8ImageId != 2 )
			{
				if ( m_pCsBoard->u32BaseBoardOptions[u8ImageId] != 0 )
				{
					BaseBoardConfigReset(2);
					m_pCardState->AcqConfig.u32Mode &= ~(CS_MODE_USER1 | CS_MODE_USER2);
					m_pCardState->AcqConfig.u32Mode |= CS_MODE_USER2;
					bConfigReset = TRUE;
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
				BaseBoardConfigReset(0);
				m_pCardState->AcqConfig.u32Mode &= ~(CS_MODE_USER1 | CS_MODE_USER2);
				bConfigReset = TRUE;
			}
		}
		break;

	}

	if ( 0 != u8ImageId )
		m_bMulrecAvgTD =  ( m_pCsBoard->u32BaseBoardOptions[m_pCardState->u8ImageId ] & CS_BBOPTIONS_MULREC_AVG_TD ) != 0;
	else
	{
		m_bMulrecAvgTD = FALSE;
		CsSetDataFormat();
	}

	// After ConfigReset all settings on the card is lost. By reseting these parameters, we make sure
	// that the next call of CssetAcquisitionCfg, CsSetChannelCfg ... will reconfigure HW even if the
	// configuration has not been changed.
	if ( bConfigReset )
	{
		ResetCachedState();
		ReadVersionInfo(m_pCardState->u8ImageId);
		
		UpdateCardState();
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

int32	CsDeereDevice::CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8Index = 0;
	int64	i64MaxMemPerChan;
	int64	i64MaxSegmentSize;
	int64	i64MaxDepth;
	int64	i64MaxPreTrigDepth;
	int64	i64MaxTrigDelay;
	uInt32	u32LimitSegmentCount = 0xFFFFFFFF;
	uInt32	u32MaxSegmentCount = 1;
	int32	i32MinSegmentSize = 32;
	uInt32	u32MinTrgHoldOff = 0;


	// Samples reserved by HW at the end of each segment
	uInt32	u32TailHwReserved = BYTESRESERVED_HWMULREC_TAIL / ((pAcqCfg->u32Mode & CS_MODE_DUAL) ? 2 : 1);

	// Calculate the Max allowed Segment Size
	i64MaxMemPerChan = m_pSystem->SysInfo.i64MaxMemory;
	i64MaxMemPerChan >>= ((pAcqCfg->u32Mode & CS_MODE_DUAL) ? 1: 0);
	i64MaxSegmentSize = i64MaxMemPerChan - u32TailHwReserved - GetSegmentPadding( pAcqCfg->u32Mode );
	i64MaxDepth = i64MaxPreTrigDepth = i64MaxSegmentSize;
	i64MaxTrigDelay = CSMV_LIMIT_HWDEPTH_COUNTER;
	i64MaxTrigDelay *= 128;
	i64MaxTrigDelay /= (32*(pAcqCfg->u32Mode&CS_MASKED_MODE));

	if ( pAcqCfg->u32Mode & CS_MODE_USER1 )
		u8Index = 1;
	else if ( pAcqCfg->u32Mode & CS_MODE_USER2 )
		u8Index = 2;

	if ( 0 != u8Index )
	{
		// Make sure that Master/Slave system has all the same optionss
		CsDeereDevice* pDevice = m_MasterIndependent->m_Next;
		while( pDevice )
		{
			if ( m_pCsBoard->u32BaseBoardOptions[u8Index] != pDevice->m_pCsBoard->u32BaseBoardOptions[u8Index] )
				return CS_MASTERSLAVE_DISCREPANCY;

			pDevice = pDevice->m_Next;
		}

		if ( m_pCsBoard->u32BaseBoardOptions[u8Index] == 0 )
		{
			if ( u32Coerce )
			{
				pAcqCfg->u32Mode &= ~CS_MODE_USER1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return (i32Status = CS_INVALID_ACQ_MODE);
		}

		if (( m_pCsBoard->u32BaseBoardOptions[u8Index] & CS_BBOPTIONS_MULREC_AVG_TD ) != 0 )
		{	
			m_bMulrecAvgTD = TRUE;
			m_bSquareRec   = true;
			// Each sample will be 32 bits sample
			i64MaxMemPerChan /= 2;
			u32TailHwReserved /= 2;
			i64MaxPreTrigDepth = DEERE_AVG_FIXED_PRETRIG;
			 if ( 0 != (pAcqCfg->u32Mode & CS_MODE_DUAL) )
				 i64MaxPreTrigDepth >>= 1;

			i64MaxSegmentSize	= ( pAcqCfg->u32Mode & CS_MODE_SINGLE ) ? MAX_DRIVER_HWAVERAGING_DEPTH : MAX_DRIVER_HWAVERAGING_DEPTH / 2; 
			i64MaxDepth		= i64MaxSegmentSize;
		}
	}
	else
	{
		if ( pAcqCfg->u32Mode & CS_MODE_SW_AVERAGING )
		{
			i64MaxDepth		= ( pAcqCfg->u32Mode & CS_MODE_SINGLE ) ? MAX_SW_AVERAGING_SEMGENTSIZE : MAX_SW_AVERAGING_SEMGENTSIZE / 2; 
			i64MaxSegmentSize  = ( pAcqCfg->u32Mode & CS_MODE_SINGLE ) ? MAX_SW_AVERAGING_SEMGENTSIZE : MAX_SW_AVERAGING_SEMGENTSIZE / 2; 
			// No pretrigger is allowed in HW averaging
			i64MaxPreTrigDepth = 0;
			// Trigger Delay is not supported in HW averaging
			i64MaxTrigDelay = 0;
			u32LimitSegmentCount = MAX_SW_AVERAGE_SEGMENTCOUNT;
		}
		else
		{
			// Detect if square tec mode will be used
			m_bSquareRec = IsSquareRec( pAcqCfg->u32Mode, pAcqCfg->u32SegmentCount, (pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth) ); 

			if ( m_bSquareRec )
				i64MaxPreTrigDepth = DEERE_MAX_SQUARE_PRETRIGDEPTH / (pAcqCfg->u32Mode & CS_MASKED_MODE);

			if ( pAcqCfg->i64TriggerDelay != 0 )
				i64MaxPreTrigDepth = 0;		// No pretrigger if TriggerDelay is enable
		}
	}

	// Validation of Segment Size
	if ( 0 >= pAcqCfg->i64SegmentSize  )
	{
		if (u32Coerce)
		{
			pAcqCfg->i64SegmentSize = DEERE_DEFAULT_SEGMENT_SIZE;
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


	uInt32	u32RealSegmentCount = pAcqCfg->u32SegmentCount;
	int64	i64RealSegmentSize	= pAcqCfg->i64SegmentSize;
	int64	i64RealPostDepth	= pAcqCfg->i64Depth;

    // Adjust Segemnt Parameters for Hardware
	AdjustedHwSegmentParameters( pAcqCfg->u32Mode, &u32RealSegmentCount, &i64RealSegmentSize, &i64RealPostDepth );

	if (i64RealPostDepth > i64MaxDepth )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64Depth -= (i64RealPostDepth - i64MaxDepth);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return (i32Status = CS_INVALID_SEGMENT_SIZE);
	}

	if ( i64RealSegmentSize > i64MaxSegmentSize  )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize -= (i64RealSegmentSize - i64MaxSegmentSize);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return (i32Status = CS_INVALID_PRETRIGGER_DEPTH);
	}

	if ( pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth > i64MaxPreTrigDepth )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize = pAcqCfg->i64Depth + i64MaxPreTrigDepth;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return (i32Status = CS_INVALID_PRETRIGGER_DEPTH);
	}


	if ( pAcqCfg->i64TriggerDelay > i64MaxTrigDelay )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64TriggerDelay = i64MaxTrigDelay;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return (i32Status = CS_INVALID_TRIGDELAY);
	}

	if ( m_bSquareRec )
	{
		u32MinTrgHoldOff = 	(uInt32) (pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth);
		if ( pAcqCfg->i64TriggerHoldoff < u32MinTrgHoldOff )
		{
			if ( u32Coerce )
			{
				pAcqCfg->i64TriggerHoldoff = u32MinTrgHoldOff;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return (i32Status = CS_INVALID_TRIGHOLDOFF);
		}
	}

	// Limit the max segment size 
	if ( pAcqCfg->i64SegmentSize > i64MaxSegmentSize )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize = i64MaxSegmentSize;
			if ( pAcqCfg->i64Depth > pAcqCfg->i64SegmentSize )
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;

			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return (i32Status = CS_INVALID_SEGMENT_SIZE);
	}

	// Limit the min segment size 
	if ( pAcqCfg->i64SegmentSize < i32MinSegmentSize  )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize = i32MinSegmentSize;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return (i32Status = CS_INVALID_SEGMENT_SIZE);
	}
	
	if ( CS_CONFIG_CHANGED == i32Status )
	{
		u32RealSegmentCount = pAcqCfg->u32SegmentCount;
		i64RealSegmentSize	= pAcqCfg->i64SegmentSize;
		i64RealPostDepth	= pAcqCfg->i64Depth;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( pAcqCfg->u32Mode, &u32RealSegmentCount, &i64RealSegmentSize, &i64RealPostDepth );
	}

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
			return (i32Status = CS_INVALID_SEGMENT_COUNT);
	}	

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDeereDevice::SetupHwAverageMode( bool bFullRes, uInt32 u32ScaleFactor )
{
	uInt32		DevideSelector = 0;

	m_bAvgFullRes = bFullRes;

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

		m_pCardState->AcqConfig.i32SampleRes = DEERE_DEFAULT_RES;
		switch ( u32ScaleFactor )
		{
			case 1024:	DevideSelector =  0; break;
			case 512:	DevideSelector =  1; break;
			case 256:	DevideSelector =  2; break;
			case 128:	DevideSelector =  3; break;
			case 64:	DevideSelector =  4; break;
			case 32:	DevideSelector =  5; break;
			case 16:	DevideSelector =  6; break;
			case 8:		DevideSelector =  7; break;
			case 4:		DevideSelector =  8; break;
			case 2:		DevideSelector =  9; break;
			default:	DevideSelector = 10; break;
		}
	}

	uInt32	u32Data = 0;
	if (( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ) != 0 )
		u32Data |= DEERE_DUAL_CHAN_ON_BIT;

	WriteGageRegister( GEN_COMMAND_R_REG, (DevideSelector << 8) | u32Data);
	CsSetDataFormat( formatHwAverage );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::AcquireData(uInt32 u32IntrEnabled)
{
	int32 i32Ret = CS_SUCCESS;

	if( m_bDebugCalSettlingTime && (ecmCalModeDc == m_CalibMode) && ( NULL == m_pi16CalibBuffer ) )
	{
		i32Ret = SendCalibDC(m_pCardState->ChannelParams[0].u32Impedance, m_pCardState->ChannelParams[0].u32InputRange, m_pCardState->TriggerParams[1].u32Condition != CS_TRIG_COND_POS_SLOPE);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	// Reset alarm record count
	m_MasterIndependent->m_pCardState->u32ActualSegmentCount = m_u32HwSegmentCount;

	// Send StartAcquisition command
	in_STARTACQUISITION_PARAMS AcqParams = {0};

	AcqParams.bOverload		= true;		// Do not use kernel generic StartAcquisition()
	AcqParams.u32IntrMask	= u32IntrEnabled;
	AcqParams.u32Param1		= 0;//m_u16AlarmSource;

	i32Ret = IoctlDrvStartAcquisition( &AcqParams );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if( m_bDebugCalSettlingTime && (ecmCalModeDc == m_CalibMode) && ( NULL == m_pi16CalibBuffer ) )
	{
		BBtiming( uInt32((10000000 * m_pCardState->AcqConfig.i64TriggerHoldoff) / m_pCardState->AcqConfig.i64SampleRate) );
		i32Ret = SendCalibDC(m_pCardState->ChannelParams[0].u32Impedance, m_pCardState->ChannelParams[0].u32InputRange, m_pCardState->TriggerParams[1].u32Condition == CS_TRIG_COND_POS_SLOPE );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
		
	if ( (DEERE_PULSE_DISABLED & m_pCardState->AddOnReg.u16CalibPulse) != DEERE_PULSE_DISABLED )
		i32Ret = GeneratePulseCalib();

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsDeereDevice::MsAbort()
{
	CsDeereDevice* pCard = m_MasterIndependent;
	while (pCard)
	{
		if( pCard != m_pTriggerMaster )
			pCard->AbortAcquisition();
		pCard = pCard->m_Next;
	}
	m_pTriggerMaster->AbortAcquisition();
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

int32	CsDeereDevice::CheckRequiredFirmwareVersion()
{
	int32	i32Status = CS_SUCCESS;
	int32	i32Error = CS_SUCCESS;


	if ( m_bSkipFirmwareCheck )
		return CS_SUCCESS;

	if ( m_bCsiFileValid )
	{
		FPGA_FWVERSION	CsiVer;

		CsiVer.u32Reg = m_BaseCsiEntry.u32FwVersion;

		if ( CompareFwVersion( m_pCardState->VerInfo.BbFpgaInfo, CsiVer ) ) 
			m_pCardState->u8BadFirmware = BAD_DEFAULT_FW;
	}

	// added March 22, 2011 because of problem when reinitialing from CsExpertUpdate
	// we would failinitialization below with BAD_EXPERT1_FW because the version of 
	// CsiVer.u32Reg and ExpertVer.u32Reg.  Now, if we don't detect expert firmware
	// we don't even check the version of the expert firmware

	BOOLEAN bExpertFwDetected = FALSE;
	for ( int i = 1; i <= 2; i ++ )
	{
		if ( m_pCsBoard->u32BaseBoardOptions[i] != 0 )
		{
			bExpertFwDetected = TRUE;
			break;
		}
	}

	// Check the Expert image firmwares
	if ( m_bFwlFileValid  && bExpertFwDetected )
	{
		FPGA_FWVERSION		CsiVer;
		FPGA_FWVERSION		ExpertVer;

		for ( int i = 0; i < sizeof(m_BaseExpertEntry) / sizeof(m_BaseExpertEntry[0]); i ++ )
		{
			CsiVer.u32Reg = m_BaseExpertEntry[i].u32Version;
			ExpertVer.u32Reg = m_pCsBoard->u32RawFwVersionB[i+1];

			if ( CompareFwVersion( ExpertVer, CsiVer ) ) 
				m_pCardState->u8BadFirmware |= BAD_EXPERT1_FW << i;
		}
	}

	if ( CS_FAILED( i32Error ) )
		return i32Error;
	else
		return i32Status;
}

//------------------------------------------------------------------------------
//
//	Parameters on Board initialization.
//  Reboot or reload drivers is nessesary 
//
//------------------------------------------------------------------------------

int32	CsDeereDevice::ReadCommonRegistryOnBoardInit()
{
#ifdef _WINDOWS_
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CscdG12WDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		DWORD	DataSize;
		ULONG	ul(0);

		ul = m_bSkipFirmwareCheck ? 1 : 0;
		::RegQueryValueEx( hKey, _T("SkipCheckFirmware"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipFirmwareCheck = (ul != 0);

//		ul = m_bSkipMasterSlaveDetection ? 1 : 0;
//		::RegQueryValueEx( hKey, _T("SkipMasterSlaveDetection"), NULL, NULL, (LPBYTE)&ul, &DataSize );
//		m_bSkipMasterSlaveDetection = (ul != 0);

		ul = m_bTestBridgeEnabled ? 1 : 0;
		::RegQueryValueEx( hKey, _T("BridgeTestEnable"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bTestBridgeEnabled = (ul != 0);


		::RegCloseKey( hKey );
	}	
#endif

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32 CsDeereDevice::ReadCommonRegistryOnAcqSystemInit()
{
#ifdef _WINDOWS_
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\CscdG12WDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
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


		ul = m_u8TrigSensitivity;
		::RegQueryValueEx( hKey, _T("DTriggerSensitivity"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul <= 255 )
			m_u8TrigSensitivity = (uInt8) ul;
		else
			m_u8TrigSensitivity = 255;

		ul = m_u16ExtTrigSensitivity;
		::RegQueryValueEx( hKey, _T("ExtTrigSensitivity"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if ( ul <= ULONG(200) ) // 20%
			m_u16ExtTrigSensitivity = (uInt16) ul;
		else
			m_u16ExtTrigSensitivity = 200;


		ul = m_u8DebugCalibExtTrig;
		::RegQueryValueEx( hKey, _T("NeverCalibExtTrig"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u8DebugCalibExtTrig = (uInt8) ul;

		ul = m_bNoAdjExtTrigFail ? 1 : 0;
		::RegQueryValueEx( hKey, _T("NoAdjExtTrigFail"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bNoAdjExtTrigFail = ul != 0;

		ul = m_bLowExtTrigGain ? 1 : 0;
		::RegQueryValueEx( hKey, _T("LowExtTrigGain"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bLowExtTrigGain = ul != 0;

		if( !m_bLowExtTrigGain )
			m_u32DefaultExtTrigRange = CS_GAIN_10_V;

		ul = m_BusMasterSupport? 0UL : 1UL;
		::RegQueryValueEx( hKey, _T("NoDma"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_BusMasterSupport = ul == 0;

		ul = m_bDynamicSquareRec ? 1 : 0;
		::RegQueryValueEx( hKey, _T("DynamicSquareRec"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bDynamicSquareRec =  ul != 0;

		ul = (ULONG) m_bHwAvg32;
		::RegQueryValueEx( hKey, _T("HwAvg32"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bHwAvg32 = (ul != 0);

		ul = (ULONG) m_bCh2Single;
		::RegQueryValueEx( hKey, _T("Ch2Single"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bCh2Single = (ul != 0);

		ReadCalibrationSettings( hKey );

		::RegCloseKey( hKey );
	}	
#endif

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsDeereDevice::ReadCalibrationSettings(void *Params)
{

#ifdef _WINDOWS_
	ULONG	ul(0);
	DWORD	DataSize = sizeof(ul);
	HKEY	hKey = (HKEY) Params;


	ul = m_bSkipCalib ? 1 : 0;
	::RegQueryValueEx( hKey, _T("FastDcCal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bSkipCalib = 0 != ul;

	ul = m_u16SkipCalibDelay;
	::RegQueryValueEx( hKey, _T("NoCalibrationDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );

	if(ul > 24 * 60 ) //limitted to 1 day
		ul = 24*60;
	m_u16SkipCalibDelay = (uInt16) ul;

	ul = m_bNeverCalibrateDc ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverCalibrateDc"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverCalibrateDc = 0 != ul;

	ul = m_bNeverMatchAdcOffset ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverMatchAdcOffset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverMatchAdcOffset = 0 != ul;

	ul = m_bNeverMatchAdcGain ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverMatchAdcGain"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverMatchAdcGain = 0 != ul;

	ul = m_bNeverMatchAdcPhase ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverMatchAdcPhase"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverMatchAdcPhase = 0 != ul;

	ul = m_bSkipUserCal ? 1 : 0;
	::RegQueryValueEx( hKey, _T("SkipUserCal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bSkipUserCal = 0 != ul;

	ul = m_u16AdcDpaMask;
	::RegQueryValueEx( hKey, _T("AdcDpaMask"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u16AdcDpaMask = (uInt16)ul;

	ul = m_bSkipAdcSelfCal ? 1 : 0;
	::RegQueryValueEx( hKey, _T("SkipAdcSelfCal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bSkipAdcSelfCal = 0 != ul;

	ul = m_u8CalDacSettleDelay;
	::RegQueryValueEx( hKey, _T("CalDacSettleDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u8CalDacSettleDelay = (uInt8)ul;

	ul = m_u32IgnoreCalibError;
	::RegQueryValueEx( hKey, _T("IgnoreCalibError"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32IgnoreCalibError = ul;

	ul = m_u8NeverCalibrateMs;
	::RegQueryValueEx( hKey, _T("NeverCalibrateMs"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u8NeverCalibrateMs = (uInt8) ul;

	ul = m_bDebugCalibSrc ? 1 : 0;
	::RegQueryValueEx( hKey, _T("DebugCalSrc"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bDebugCalibSrc = 0 != ul;

	ul = m_bDebugCalSettlingTime ? 1 : 0;
	::RegQueryValueEx( hKey, _T("DebugCalSrcSettleTime"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bDebugCalSettlingTime = 0 != ul;

	ul = (uInt32)m_ecmDebugCalSrcMode;
	::RegQueryValueEx( hKey, _T("DebugCalSrcMode"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	if( ul >  (ULONG)ecmCalModeTAO)
		ul = (ULONG)ecmCalModeTAO;
	m_ecmDebugCalSrcMode = (eCalMode)ul;

	ul = m_bNeverCalibAdcAlign ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverCalibAdcAlign"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverCalibAdcAlign = ul != 0;

	ul = m_bSkipCalibAdcAlign ? 1 : 0;
	::RegQueryValueEx( hKey, _T("SkipCalibAdcAlign"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bSkipCalibAdcAlign = ul != 0;

	ul = m_u16AlarmMask;
	::RegQueryValueEx( hKey, _T("AlarmMask"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u16AlarmMask = (uInt16)ul;

	ul = m_bGrayCode ? 1 : 0;
	::RegQueryValueEx( hKey, _T("UseGrayCode"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bGrayCode = ul != 0;
	if( m_bGrayCode )
		m_BusMasterSupport = FALSE;
	
	ul = m_bUseUserCalSignal ? 1 : 0;
	::RegQueryValueEx( hKey, _T("UseUserCalSignal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bUseUserCalSignal = ul != 0;

	ul = m_u32DebugCalibTrace;
	::RegQueryValueEx( hKey, _T("TraceCalib"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32DebugCalibTrace = ul;

	ul = m_u16AdcRegDelay;
	::RegQueryValueEx( hKey, _T("AdcRegDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u16AdcRegDelay = (uInt16)ul;


#else

	UNREFERENCED_PARAMETER(Params);

#endif
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsDeereDevice::CsAutoUpdateFirmware()
{
	int32 i32Ret = CS_SUCCESS;

	FILE_HANDLE hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName );
	if ( 0 == hCsiFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_OPEN_FILE, m_szCsiFileName );
		return CS_FAIL;
	}

	// check if any expert firmware needs updating. If not don't bother reading the fwl file.
	// if they do need updating and we can't open the file it's an error
	BOOL bUpdateFwl = FALSE;
	CsDeereDevice* pCard = (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
	while ( pCard )
	{
		// Open handle to kernel driver
		pCard->InitializeDevIoCtl();

		if ( pCard->m_pCardState->u8BadFirmware & (BAD_EXPERT1_FW | BAD_EXPERT2_FW) )
			bUpdateFwl = TRUE;

		pCard = pCard->m_HWONext;
	}

	FILE_HANDLE hFwlFile = (FILE_HANDLE)0;
	if ( bUpdateFwl )
	{
		hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName );
		if ( 0 == hFwlFile )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_OPEN_FILE, m_szCsiFileName );
			GageCloseFile( hCsiFile );
			return CS_FAIL;
		}	
	}

	uInt8* pBuffer = (uInt8 *) GageAllocateMemoryX( DEERE_FLASH_FWIMAGESIZE );
	if ( NULL == pBuffer )
	{
		GageCloseFile( hCsiFile );
		if ( (FILE_HANDLE)0 != hFwlFile )
			GageCloseFile( hFwlFile );
		return CS_INSUFFICIENT_RESOURCES;
	}
	//-------------      UPDATE EXPERT FIRMWARE  --------------------------
	pCard = (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
	uInt32 u32OffsetOfImage = 0;
	DEERE_FLASH_FIRMWARE_INFOS FwInfo = {0};

	uInt32	u32FwImageOffset	= 0;
	uInt32	u32FwInfoOffset		= 0;

	if ( hFwlFile ) // if file doesn't exist then don't update firmware
	{
		for ( int i = 0; i < sizeof(m_BaseExpertEntry) / sizeof(m_BaseExpertEntry[0]); i ++ )
		{
			if ( 0 == i )
			{
				u32FwImageOffset	= FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage2);
				u32FwInfoOffset		= FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage2) + 	FIELD_OFFSET(DEERE_FLASH_IMAGE_STRUCT, FwInfo) +
									  FIELD_OFFSET(DEERE_FLASH_FWINFO_SECTOR, FwInfoStruct);
			}
			else
			{
				u32FwImageOffset	= FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage3);
				u32FwInfoOffset		= FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage3) + 	FIELD_OFFSET(DEERE_FLASH_IMAGE_STRUCT, FwInfo) +
									  FIELD_OFFSET(DEERE_FLASH_FWINFO_SECTOR, FwInfoStruct);
			}

			pCard = (CsDeereDevice*)  m_ProcessGlblVars->pFirstUnCfg;

			while( pCard )
			{
				CsDeereFlash FlashObj( pCard->GetPlxBaseObj() );
				if ( 0 != (pCard->m_pCardState->u8BadFirmware & (BAD_EXPERT1_FW<<i) ) && pCard->m_BaseExpertEntry[i].u32Version != 0 )
				{
					// Remember the old version number before update
					uInt32 u32OldVer = pCard->m_pCsBoard->u32RawFwVersionB[i+1];

					if ( pCard->m_BaseExpertEntry[i].u32ImageOffset != u32OffsetOfImage )
					{
						// Load new image from file if it is not already in buffer
						u32OffsetOfImage = m_BaseExpertEntry[i].u32ImageOffset;
						RtlZeroMemory( pBuffer, DEERE_FLASH_FWIMAGESIZE );
						GageReadFile(hFwlFile, pBuffer, m_BaseExpertEntry[i].u32ImageSize, &m_BaseExpertEntry[i].u32ImageOffset);
					}

					// Clear valid Checksum
					i32Ret = pCard->IoctlReadFlash( u32FwInfoOffset, &FwInfo, sizeof(FwInfo) );
					if(CS_FAILED(i32Ret)) return i32Ret;
					FwInfo.u32ChecksumSignature		= 0;
					i32Ret = FlashObj.FlashWriteEx( u32FwInfoOffset, &FwInfo, sizeof(FwInfo), true );
					if(CS_FAILED(i32Ret)) return i32Ret;

					// Write Addon FPGA image
					i32Ret = FlashObj.FlashWriteEx( u32FwImageOffset, pBuffer,  pCard->m_BaseExpertEntry[i].u32ImageSize, false );
					// Set valid Checksum
					if ( CS_SUCCEEDED(i32Ret) )
					{
						FPGA_FWVERSION	ExpertVer;
						ExpertVer.u32Reg = pCard->m_BaseExpertEntry[i].u32Version ;

						pCard->ReadVersionInfo((uInt8)(i+1));
						if ( 0 == CompareFwVersion( pCard->m_pCardState->VerInfo.BbFpgaInfo, ExpertVer ) )
						{
							FwInfo.u32ChecksumSignature		= CSI_FWCHECKSUM_VALID;
							FwInfo.u32FwOptions				= pCard->m_BaseExpertEntry[i].u32Option;
							FwInfo.u32FwVersion				= ExpertVer.u32Reg;
							i32Ret = FlashObj.FlashWriteEx( u32FwInfoOffset, &FwInfo, sizeof(FwInfo), true );
						}
					}

					if ( CS_SUCCEEDED(i32Ret) )
					{
						char szText[128], OldFwFlVer[40], NewFwFlVer[40];

						pCard->m_pCardState->u8BadFirmware &= ~(BAD_EXPERT1_FW<<i);
						FriendlyFwVersion( pCard->m_BaseExpertEntry[i].u32Version, NewFwFlVer, sizeof(NewFwFlVer));
						FriendlyFwVersion( u32OldVer, OldFwFlVer, sizeof(OldFwFlVer));
						sprintf_s( szText, sizeof(szText), TEXT("Updated firmware Image%i from v%s to v%s"), i+1, OldFwFlVer, NewFwFlVer );
						::OutputDebugString( szText );
						CsEventLog EventLog;
						EventLog.Report( CS_ADDONFIRMWARE_UPDATED, szText );
					}

					// Close the handle to kernel driver
					pCard->UpdateCardState();
				}
				pCard = pCard->m_HWONext;
			}
		}
	}
	//-------------      UPDATE BASE FIRMWARE  --------------------------
	u32FwInfoOffset = FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage1) + 	FIELD_OFFSET(DEERE_FLASH_IMAGE_STRUCT, FwInfo) +
							  FIELD_OFFSET(DEERE_FLASH_FWINFO_SECTOR, FwInfoStruct);

	pCard = (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
	while( pCard )
	{
		CsDeereFlash FlashObj( pCard->GetPlxBaseObj() );
		if ( 0 != pCard->m_pCardState->u8BadFirmware && pCard->m_BaseCsiEntry.u32FwVersion != 0 )
		{
			// Remember the old version number before update
			uInt32 u32OldVer = pCard->m_pCsBoard->u32RawFwVersionB[0];

			if ( pCard->m_BaseCsiEntry.u32ImageOffset != u32OffsetOfImage )
			{
				// Load new image from file if it is not already in buffer
				u32OffsetOfImage = m_BaseCsiEntry.u32ImageOffset;
				RtlZeroMemory( pBuffer, DEERE_FLASH_FWIMAGESIZE );

				uInt32 	u32ImageOffset = m_BaseCsiEntry.u32ImageOffset;
				GageReadFile(hCsiFile, pBuffer, m_BaseCsiEntry.u32ImageSize, &u32ImageOffset);
			}

			// Clear valid Checksum
			i32Ret = pCard->IoctlReadFlash( u32FwInfoOffset, &FwInfo, sizeof(FwInfo) );
			if(CS_FAILED(i32Ret)) return i32Ret;
			FwInfo.u32ChecksumSignature		= 0;
			i32Ret = FlashObj.FlashWriteEx( u32FwInfoOffset, &FwInfo, sizeof(FwInfo), true );
			if(CS_FAILED(i32Ret)) return i32Ret;

			// Write Base FPGA image
			i32Ret = FlashObj.FlashWriteEx( FIELD_OFFSET(DEERE_FLASH_LAYOUT, FlashImage1), pBuffer,  pCard->m_BaseCsiEntry.u32ImageSize, false );
			// Set valid Checksum
			if ( CS_SUCCEEDED(i32Ret) )
			{
				FPGA_FWVERSION		FwVer;
				FwVer.u32Reg = pCard->m_BaseCsiEntry.u32FwVersion;

				pCard->ReadVersionInfo();
				if ( 0 == CompareFwVersion( pCard->m_pCardState->VerInfo.BbFpgaInfo, FwVer ) )
				{
					FwInfo.u32ChecksumSignature		= CSI_FWCHECKSUM_VALID;
					FwInfo.u32FwOptions				= pCard->m_BaseCsiEntry.u32FwOptions;
					FwInfo.u32FwVersion				= pCard->m_BaseCsiEntry.u32FwVersion;
					i32Ret = FlashObj.FlashWriteEx( u32FwInfoOffset, &FwInfo, sizeof(FwInfo), true );
				}
			}

			if ( CS_SUCCEEDED(i32Ret) )
			{
				pCard->m_pCardState->u8BadFirmware = 0;

				char szText[128], OldFwFlVer[40], NewFwFlVer[40];
				FriendlyFwVersion( pCard->m_BaseCsiEntry.u32FwVersion, NewFwFlVer, sizeof(NewFwFlVer));
				FriendlyFwVersion( u32OldVer, OldFwFlVer, sizeof(OldFwFlVer));
				sprintf_s( szText, sizeof(szText), TEXT("Updated firmware Image0 from v%s to v%s"), OldFwFlVer, NewFwFlVer );
				::OutputDebugString(szText);
				CsEventLog EventLog;
				EventLog.Report( CS_ADDONFIRMWARE_UPDATED, szText );
			}
			else
			{
				::OutputDebugString( "Writing firmware error.\n" );
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, "Fail to update firmware." );
			}
	
			// Full renitialization is required for this card
			pCard->BaseBoardConfigReset(0);
			pCard->ResetCachedState();
			pCard->m_pCardState->bAddOnRegistersInitialized = false;
			pCard->m_pCardState->bCardStateValid = FALSE;
			pCard->UpdateCardState();

			pCard->DeviceInitialize();
		}

		pCard = pCard->m_HWONext;
	}

	pCard = (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
	while( pCard )
	{
		// Close the handle to kernel driver
		pCard->UnInitializeDevIoCtl();
		pCard = pCard->m_HWONext;
	}

	GageCloseFile( hCsiFile );
	if ( hFwlFile )
		GageCloseFile( hFwlFile );
	GageFreeMemoryX( pBuffer );

	return CS_SUCCESS;
}


#define CSISEPARATOR_LENGTH			16
const uInt8 CsiSeparator[] = "-- SEPARATOR --|";		// 16 character long + NULL character

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsDeereDevice::ReadAndValidateCsiFile( uInt32 u32BbHwOptions, uInt32 u32AoHwOptions )
{
	int32			i32RetCode = CS_SUCCESS;
	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry;


	UNREFERENCED_PARAMETER(u32AoHwOptions);

	// Attempt to open Csi file
	FILE_HANDLE hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName );

	if ( ! hCsiFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_NOTFOUND, m_szCsiFileName );
		return CS_MISC_ERROR;
	}

	GageReadFile( hCsiFile, &header, sizeof(header) );
	if ( ( header.u32Size == sizeof(CSI_FILE_HEADER) ) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) &&
		 (( header.u32Version == 0 ) || ( header.u32Version == OLD_CSIFILE_VERSION )) )
	{
		for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
		{
			if ( 0 == GageReadFile( hCsiFile, &CsiEntry, sizeof(CsiEntry) ) )
			{
				i32RetCode = CS_FAIL;
				break;
			}

			if ( CSI_DEERE_TARGET == CsiEntry.u32Target )
			{
				// Found Add-on board firmware info
				if ( (CsiEntry.u32HwOptions & DEERE_BBHW_OPT_FWREQUIRED_MASK) == 
					 (u32BbHwOptions & DEERE_BBHW_OPT_FWREQUIRED_MASK ) )
				{
					GageCopyMemoryX( &m_BaseCsiEntry, &CsiEntry, sizeof(CSI_ENTRY) );
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
		if (m_BaseCsiEntry.u32ImageOffset == 0  )
		{
			i32RetCode = CS_FAIL;
		}
		else
		{
			uInt8	u8Buffer[CSISEPARATOR_LENGTH];
			uInt32	u32ReadPosition = m_BaseCsiEntry.u32ImageOffset - CSISEPARATOR_LENGTH;

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
		m_bCsiFileValid = false;
		CsEventLog EventLog;
		EventLog.Report( CS_CSIFILE_INVALID, m_szCsiFileName );		
	}
	else
		m_bCsiFileValid = true;
	
	return i32RetCode;
}



//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsDeereDevice::ReadAndValidateFwlFile()
{
	int32				i32RetCode = CS_SUCCESS;
	FWL_FILE_HEADER		header = {0};
	FPGA_HEADER_INFO	FwlEntry = {0};
	BOOLEAN				bExpertFwDetected = FALSE;


	for ( int i = 1; i <= 2; i ++ )
	{
		if ( m_pCsBoard->u32BaseBoardOptions[i] != 0 )
		{
			bExpertFwDetected = TRUE;
			break;
		}
	}

	// Attempt to open Fwl file
	FILE_HANDLE hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName );

	// If we can't open the file, report it to the event log whether or not we
	// actually will be using it.
	if ( ! hFwlFile )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_NOTFOUND, m_szFwlFileName );
		return CS_MISC_ERROR;
	}

	// If expert firmware doe not present on the current card then do not bother reading the Fwl file,
	// just close it and return
	if ( ! bExpertFwDetected )
	{
		GageCloseFile( hFwlFile );
		return CS_SUCCESS;
	}

	GageReadFile( hFwlFile, &header, sizeof(header) );
	if ( ( header.u32Size == sizeof(FWL_FILE_HEADER) ) && ( CsIsDeereBoard(header.u32BoardType) ) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) && (header.u32Version == OLD_FWLFILE_VERSION ) )
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
				for ( int j = 0; j < sizeof(m_BaseExpertEntry) / sizeof(m_BaseExpertEntry[0]); j ++ )
				{
					if ( m_pCsBoard->u32BaseBoardOptions[j+1] == FwlEntry.u32Option )
					{
						RtlCopyMemory( &m_BaseExpertEntry[j], &FwlEntry, sizeof(FwlEntry) );
					}
				}
			}	
		}
	}
	else
	{
		i32RetCode = CS_FAIL;
	}

	GageCloseFile ( hFwlFile );

	if ( CS_FAIL == i32RetCode )
	{
		m_bFwlFileValid = false;
		CsEventLog EventLog;
		EventLog.Report( CS_FWLFILE_INVALID, m_szFwlFileName );
	}
	else
		m_bFwlFileValid = true;
	
	return i32RetCode;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDeereDevice::CsSetDataFormat( _CSDATAFORMAT csFormat )
{
	switch ( csFormat )
	{
		case formatHwAverage:
			if ( m_bAvgFullRes )
			{
				m_pCardState->AcqConfig.u32SampleBits = m_pCardState->b14Bits ? CS_SAMPLE_BITS_14 : CS_SAMPLE_BITS_12;
				m_pCardState->AcqConfig.i32SampleOffset = m_pCardState->b14Bits ? CS_SAMPLE_OFFSET_14_LJ : CS_SAMPLE_OFFSET_12_LJ;
				m_pCardState->AcqConfig.i32SampleOffset <<= 6;
				m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
				m_pCardState->AcqConfig.i32SampleRes	= (-CS_SAMPLE_RES_LJ) << 6;
			}
			else
			{
				m_pCardState->AcqConfig.u32SampleBits = m_pCardState->b14Bits ? CS_SAMPLE_BITS_14 : CS_SAMPLE_BITS_12;
				m_pCardState->AcqConfig.i32SampleOffset = m_pCardState->b14Bits ? CS_SAMPLE_OFFSET_14_LJ : CS_SAMPLE_OFFSET_12_LJ;
				m_pCardState->AcqConfig.i32SampleOffset >>= 2;
				m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_2;
				m_pCardState->AcqConfig.i32SampleRes = -CS_SAMPLE_RES_LJ >> 2;
			}
			break;

		case formatSoftwareAverage:

			m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_4;
			m_pCardState->AcqConfig.u32SampleBits = m_pCardState->b14Bits ? CS_SAMPLE_BITS_14 : CS_SAMPLE_BITS_12;
			m_pCardState->AcqConfig.i32SampleOffset = m_pCardState->b14Bits ? CS_SAMPLE_OFFSET_14_LJ : CS_SAMPLE_OFFSET_12_LJ;
			m_pCardState->AcqConfig.i32SampleRes = DEERE_DEFAULT_RES;
			break;

		default:
			m_pCardState->AcqConfig.u32SampleBits = m_pCardState->b14Bits ? CS_SAMPLE_BITS_14 : CS_SAMPLE_BITS_12;
			m_pCardState->AcqConfig.i32SampleOffset = m_pCardState->b14Bits ? CS_SAMPLE_OFFSET_14_LJ : CS_SAMPLE_OFFSET_12_LJ;
			m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_2;
			m_pCardState->AcqConfig.i32SampleRes = -CS_SAMPLE_RES_LJ;
			break;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsDeereDevice::AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32* pu32SegmentCount, int64* pi64SegmentSize, int64* pi64Depth )
{
	uInt32 u32ExtraPreTrig = DEERE_DEFAULT_XTRAPRETRIGGER << ((u32Mode & CS_MODE_SINGLE) ? 1 : 0);
	
	if ( m_bZeroDepthAdjust && m_bZeroTrigAddrOffset )
		u32ExtraPreTrig = 0;

	m_u32TailExtraData = CsGetPostTriggerExtraSamples( u32Mode );


	if( m_bMulrecAvgTD )
	{ 
		m_i64HwTriggerHoldoff = m_pCardState->AcqConfig.i64TriggerHoldoff;

		// Ususally the effect of trigger delay is being set by PostTrig depth > Segment size but
		// in Expert Mulrec AVG TD, the trigger delay is being set by a specific register.
		// Make sure PostTrig depth = Segment size
		*pi64Depth			+= m_u32TailExtraData;
		*pi64SegmentSize	+= m_u32TailExtraData;
	}
	else if ( m_bSquareRec )
	{
		*pi64SegmentSize = *pi64SegmentSize + m_u32TailExtraData;
		*pi64Depth = *pi64Depth + m_u32TailExtraData;

		LONGLONG llPreTrig = *pi64SegmentSize - *pi64Depth;
		if ( m_pCardState->AcqConfig.i64TriggerHoldoff < llPreTrig )
			m_i64HwTriggerHoldoff = llPreTrig;
		else
			m_i64HwTriggerHoldoff = m_pCardState->AcqConfig.i64TriggerHoldoff;
	}
	else 
	{
		*pi64SegmentSize = *pi64SegmentSize + m_u32TailExtraData + u32ExtraPreTrig;
		*pi64Depth = *pi64Depth + m_pCardState->AcqConfig.i64TriggerDelay + m_u32TailExtraData;
		m_i64HwTriggerHoldoff = m_pCardState->AcqConfig.i64TriggerHoldoff + u32ExtraPreTrig;
	}

	m_u32HwSegmentCount	= *pu32SegmentCount;
	m_i64HwSegmentSize	= *pi64SegmentSize;
	m_i64HwPostDepth	= *pi64Depth;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDeereDevice::GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine )
{
	// A Free trigger engine is the one that is disabled and not yet assigned to any user
	// trigger settings

	if ( i32TrigSource < 0 )
	{
		if ( 0 == m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32UserTrigIndex )
		{
			*u16TriggerEngine = INDEX_TRIGENGINE_EXT;
			return CS_SUCCESS;
		}
	}
	else
	{
		uInt16	i = 0;
		
		if ( i32TrigSource > 0 )
			i = INDEX_TRIGENGINE_A1;
		else
			i = 0;

		for ( ; i < m_pCsBoard->NbTriggerMachine; i ++ )
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

//--------------------------------------------------------------------------------------------------
//	Internal extra samples to be added in the post trigger depth and segment size in order to
// 	ensure post trigger depth.
//  Withou ensure post trigger depth, the lost is because of trigger address offset and invalid
//  samples at the end of each record.
//---------------------------------------------------------------------------------------------------

uInt32	CsDeereDevice::CsGetPostTriggerExtraSamples( uInt32 u32Mode )
{
	if ( m_bZeroDepthAdjust )
		return 0;

	uInt32	u32TailExtraData = 0;

	if ( m_bMulrecAvgTD )
	{
		u32TailExtraData = DEERE_AVG_ENSURE_POSTTRIG + DEERE_AVG_FIXED_PRETRIG;
		if ( 0 != (u32Mode & CS_MODE_DUAL) )
			u32TailExtraData >>= 1;
	}
	else if ( m_bSquareRec )
		u32TailExtraData = 0;
	else
	{
		if ( u32Mode & CS_MODE_SINGLE )
			u32TailExtraData = (DEERE_MAX_TRIGGER_ADDRESS_OFFSET << 1) + DEERE_MAX_TAIL_INVALID_SAMPLES_SINGLE;
		else
			u32TailExtraData = DEERE_MAX_TRIGGER_ADDRESS_OFFSET + DEERE_MAX_TAIL_INVALID_SAMPLES_DUAL;
	}

	return u32TailExtraData;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SetClockOut( eClkOutMode ecomSet, bool bForce )
{
	uInt16 u16Data = 0;
	if( 2 == m_pCsBoard->NbAnalogChannel )
		u16Data |= DEERE_PING_PONG;
	uInt32 u32PLL9511_OUT1 = 0x08;
		
	switch( ecomSet )
	{
		default: ecomSet = eclkoNone;
		case eclkoNone:      u32PLL9511_OUT1 = 0x0A; break;
		case eclkoSampleClk: u16Data |= DEERE_ADC_CLK_OUT; 
		case eclkoRefClk:    u16Data |= DEERE_CLK_OUT_EN; break;
	}

	if( bForce || m_pCardState->AddOnReg.u16ClkOut != u16Data )
	{
		int32 i32Ret = WriteRegisterThruNios( u32PLL9511_OUT1, DEERE_9511_WR_CMD | DEERE_9511_OUT1);
		if( CS_FAILED(i32Ret) )		return i32Ret;

		i32Ret = WriteRegisterThruNios(u16Data, DEERE_ECXTCLK_OUTPUT);
		if( CS_FAILED(i32Ret) )		return i32Ret;
		
		m_pCardState->AddOnReg.u16ClkOut = u16Data;
		m_pCardState->eclkoClockOut = ecomSet;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SetAdcDllRange( )
{
	int32 i32Res = CS_SUCCESS;
	uInt8 u8Value = m_pCardState->u32AdcClock > DEERE_ADC_DLL_FAST_CLK ? 0 : DEERE_ADC_DLL_SLOW; 
	for( uInt16 u16Chan = 0; u16Chan < m_pCsBoard->NbAnalogChannel; u16Chan++ )
	{
		for( uInt8 u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		{
			uInt16 u16Adc = AdcForChannel(u16Chan, u8);
			uInt8 u8ModeB = ReadAdcRegDirect( u16Adc, DEERE_ADCREG_OUTPUTMODEB);
			uInt8 u8CfgStat = ReadAdcRegDirect( u16Adc, DEERE_ADCREG_CFG_STATUS);
			u8ModeB ^= u8CfgStat;
			u8ModeB ^= u8Value;
			int32 i32Ret = WriteToAdcRegDirect( u16Adc, DEERE_ADCREG_OUTPUTMODEB, u8ModeB);   
			if( CS_FAILED(i32Ret) )	i32Res = i32Ret;
		}
	}
	return i32Res;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::SelectADC(uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt16* pNiosAddr )
{
	bool bIndexed = u16Addr != DEERE_ADC_SKEW;
	if( u16Adc >= DEERE_ADC_COUNT )
		return CS_INVALID_ADC_ADDR;
	
	// Select the internal ADC
	if ( bIndexed )
	{
		int32 i32Ret = WriteToAdcRegDirect( u16Adc, DEERE_ADCREG_INDEX,  0 == u16Core ? 1 : 2 );
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	
	uInt16 u16NiosAddr (0);
	// Select the function
	switch( u16Addr )
	{
		case DEERE_ADC_COARSE_OFFSET: u16NiosAddr = DEERE_ADCREG_OFF_COARSE; break;
		case DEERE_ADC_FINE_OFFSET:	  u16NiosAddr = DEERE_ADCREG_OFF_FINE; break;
		case DEERE_ADC_COARSE_GAIN:	  u16NiosAddr = DEERE_ADCREG_GAIN_COARSE; break;
		case DEERE_ADC_MEDIUM_GAIN:   u16NiosAddr = DEERE_ADCREG_GAIN_MEDIUM; break;
		case DEERE_ADC_FINE_GAIN:	  u16NiosAddr = DEERE_ADCREG_GAIN_FINE;	break;
		case DEERE_ADC_SKEW:          u16NiosAddr = DEERE_ADCREG_SKEW; break;
		default: return CS_INVALID_DAC_ADDR;
	}
	if( NULL != pNiosAddr )
		*pNiosAddr = u16NiosAddr;
	
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
uInt8 CsDeereDevice::ReadAdcRegDirect( uInt16 u16Adc, uInt16 u16NiosAddr)
{
	uInt32 u32Read = ReadRegisterFromNios( 0, DEERE_ADC_RD_CMD | u16NiosAddr | u16Adc);
	TraceCalib(TRACE_ADC_REG_DIR, u32Read, u16Adc, (int32)u16NiosAddr, 0 );
	return (uInt8)(u32Read & 0xff);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::ReadAdcReg(uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt8* pu8Data, uInt32 u32InputRange )
{
	uInt16 u16NiosAddr;
	int32 i32Ret = SelectADC( u16Core, u16Adc, u16Addr, &u16NiosAddr);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt32 u32Read = ReadRegisterFromNios( 0, DEERE_ADC_RD_CMD | u16NiosAddr | u16Adc );
	if( FAILED_NIOS_DATA == u32Read )	return CS_ADC_ACCESS_ERROR;
	
	uInt8 u8ValRead = (uInt8)(u32Read & 0xff);

	if ( DEERE_ADC_COARSE_GAIN == u16Addr )
		u8ValRead = KenetCoarseGainConvert( u8ValRead, false );

	TraceCalib(TRACE_ADC_REG_DIR, u8ValRead, u16Adc, (int32)u16NiosAddr, 0 );

	if ( 0 == u32InputRange )
		u32InputRange = m_pCardState->ChannelParams[ChannelForAdc( u16Adc )].u32InputRange;

	UpdateAdcCalibInfoStructure( u32InputRange, u16Core, u16Adc, u16Addr, u8ValRead );
	if( NULL != pu8Data  )
		*pu8Data = u8ValRead;
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::WriteToAdcRegDirect( uInt16 u16Adc, uInt16 u16NiosAddr, uInt8 u8Data )
{
	int32 i32 = WriteRegisterThruNios( u8Data, DEERE_ADC_WR_CMD | u16NiosAddr | u16Adc);
	TraceCalib(TRACE_ADC_REG_DIR, u8Data, u16Adc, (int32)u16NiosAddr, 1 );
	return i32;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::WriteToAdcReg(uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt8 u8Data, uInt16 u16Delay)
{
	if( 0xFFFF == u16Delay )
		u16Delay = m_u16AdcRegDelay;
	uInt8	u8ConvertData = u8Data;
	if ( DEERE_ADC_COARSE_GAIN == u16Addr )
		 u8ConvertData = KenetCoarseGainConvert( u8Data, true );

	uInt16 u16NiosAddr;
	int32	i32Ret = SelectADC( u16Core, u16Adc, u16Addr, &u16NiosAddr);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToAdcRegDirect( u16Adc, u16NiosAddr, u8ConvertData );
	UpdateAdcCalibInfoStructure( m_pCardState->ChannelParams[ChannelForAdc( u16Adc )].u32InputRange, u16Core, u16Adc, u16Addr, u8ConvertData );
	BBtiming(u16Delay);
#if _DEBUG
	uInt8 u8ReadData(u8Data);
	ReadAdcReg(u16Core, u16Adc, u16Addr, &u8ReadData );
	if( u8ReadData != u8Data )
	{
		char szTrace[256];
		sprintf_s(szTrace, sizeof(szTrace),  " ! ! ! WriteToAdcReg failed. Adc %d Core %d Addr %d, NiosAddr 0x%02x, Written 0x%02x Read 0x%02x  Diff 0x%02x\n",
			u16Adc, u16Core, u16Addr, u16NiosAddr, u8Data, u8ReadData, u8Data^u8ReadData );
		::OutputDebugString(szTrace);
	}
#endif
	TraceCalib(TRACE_ADC_REG, u16Core, u16Adc, (int32)u16Addr, (int32)u8Data );
	return CS_SUCCEEDED(i32Ret)?CS_SUCCESS:CS_ADC_ACCESS_ERROR;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::WriteToAdcRegAllChannelCores(uInt16 u16HwChannel, uInt16 u16Addr, uInt8 u8Data, uInt16 u16Delay)
{
	if( (0 == u16HwChannel) || (u16HwChannel > DEERE_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	if( 0xFFFF == u16Delay )
		u16Delay = m_u16AdcRegDelay;

	bool bError = false;
	for( uInt8 u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
	{
		for( uInt16 u16Core = 0; u16Core < DEERE_ADC_CORES; u16Core++ )
		{
			int32 i32Ret = WriteToAdcReg(u16Core, AdcForChannel(u16ChanZeroBased, u8), u16Addr, u8Data, 0 );
			if( CS_FAILED(i32Ret) )	bError = true;
		}
	}
	BBtiming(u16Delay);
	return bError?CS_ADC_ACCESS_ERROR:CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32    CsDeereDevice::WriteDeltaToAdcRegAllChannelCores(uInt16 u16HwChannel, uInt16 u16Addr, int16 i16Delta, uInt16 u16Delay)
{
	if( (0 == u16HwChannel) || (u16HwChannel > DEERE_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	if( 0 == i16Delta ) 	return CS_SUCCESS;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	if( 0xFFFF == u16Delay ) u16Delay = m_u16AdcRegDelay;
	
	uInt8 u8Old, u8;
	uInt16 u16Core; int16 i16New;
	bool bError = false;
	int16 i16MaxSpan = u16Addr == DEERE_ADC_COARSE_GAIN ? c_u8AdcCoarseGainSpan : c_u8AdcCalRegSpan;
	for( u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		for( u16Core = 0; u16Core < DEERE_ADC_CORES; u16Core++ )
		{
			int32 i32Ret = ReadAdcReg(u16Core, AdcForChannel(u16ChanZeroBased, u8), u16Addr, &u8Old );
			if( CS_FAILED(i32Ret) )	
			{
				i16Delta = 0;bError = true;
			}
			else
			{
				i16New = int16(u8Old) + i16Delta;
				i16New = Max( 0, Min(i16MaxSpan, i16New));
				i16Delta = i16New - u8Old;
			}
		}

	if( 0 == i16Delta )	return CS_FALSE;

	for( u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		for( u16Core = 0; u16Core < DEERE_ADC_CORES; u16Core++ )
		{
			int32 i32Ret = ReadAdcReg(u16Core, AdcForChannel(u16ChanZeroBased, u8), u16Addr, &u8Old );
			if( CS_FAILED(i32Ret) )	
			{
				bError = true;break;
			}
			else
			{
				i16New = int16(u8Old) + i16Delta;
				i32Ret = WriteToAdcReg(u16Core, AdcForChannel(u16ChanZeroBased, u8), u16Addr, uInt8(i16New), 0);
				if( CS_FAILED(i32Ret) )	bError = true;
			}
		}

	BBtiming(u16Delay);
	return bError?CS_ADC_ACCESS_ERROR:CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::WriteToDac(uInt16 u16Addr, uInt16 u16Data, uInt8 u8Delay )
{
	uInt32	u32DacAddr = u16Addr;
	if( 0xff == u8Delay )
		u8Delay = m_u8CalDacSettleDelay;

	bool bPhaseLimit = false;
	switch ( u16Addr )
	{
		case DEERE_EXTRIG_LEVEL: u32DacAddr =  (0x0 | DEERE_DAC7552_OUTA); break;
		case DEERE_EXTRIG_OFFSET: u32DacAddr = (0x0 | DEERE_DAC7552_OUTB); break;
		
		case DEERE_OFFSET_NULL_1: u32DacAddr = (0x4 | DEERE_DAC7552_OUTA); break;
		case DEERE_OFFSET_COMP_1: u32DacAddr = (0x4 | DEERE_DAC7552_OUTB); break;
		case DEERE_USER_OFFSET_1: u32DacAddr = (0x2 | DEERE_DAC7552_OUTA); break;
		case DEERE_USER_OFFSET_2: u32DacAddr = (0x2 | DEERE_DAC7552_OUTB); break;
		case DEERE_OFFSET_NULL_2: u32DacAddr = (0x1 | DEERE_DAC7552_OUTB); break;
		case DEERE_OFFSET_COMP_2: u32DacAddr = (0x1 | DEERE_DAC7552_OUTA); break;
		case DEERE_CLKPHASE_0:    u32DacAddr = (0x6 | DEERE_DAC7552_OUTA); bPhaseLimit = true; break; // DCDRV0
		case DEERE_CLKPHASE_2:    u32DacAddr = (0x6 | DEERE_DAC7552_OUTB); bPhaseLimit = true; break; // DCDRV180
		case DEERE_CLKPHASE_1:    u32DacAddr = (0x5 | DEERE_DAC7552_OUTA); bPhaseLimit = true; break; // DCDRV0
		case DEERE_CLKPHASE_3:    u32DacAddr = (0x5 | DEERE_DAC7552_OUTB); bPhaseLimit = true; break; // DCDRV180
		case DEERE_MS_CLK_PHASE_ADJ: u32DacAddr = (0x3 | DEERE_DAC7552_OUTA); bPhaseLimit = true; break;

		case DEERE_R_COUNT:  m_pCardState->u32Rcount = u16Data; Configure4360(); break;
		case DEERE_N_COUNT:  m_pCardState->u32Ncount = u16Data; Configure4360(); break;
		case DEERE_AC_ATTEN: m_u16Atten4302 = u16Data; Configure4302(); break;
		
		case DEERE_EXT_TRIG_SHIFT: SetDigitalDelay (u16Data); break;
		
		case DEERE_ADC_CORRECTION:  m_u8DataCorrection = (uInt8)u16Data; for(uInt16 i = 1; i < m_pCsBoard->NbAnalogChannel; i++ )SendAcdCorrection(i, m_u8DataCorrection); break;

		default: return CS_INVALID_DAC_ADDR;
	}
	
	uInt32	u32Data = u8Delay;
	u32Data <<= 16;

	if( bPhaseLimit )
		u16Data = Min( c_u16CalDacCount/2 + c_u16DacPhaseLimit, Max(c_u16CalDacCount/2 - c_u16DacPhaseLimit, u16Data) );

	u32Data |= u16Data;

	TraceCalib(TRACE_CAL_DAC, u8Delay, u32DacAddr, (int32)u16Data, (int32)u16Addr );
	
	uInt16* pu16DacInfo = GetDacInfoPointer( u16Addr );
	if ( pu16DacInfo )
		*pu16DacInfo = u16Data;
	
	return WriteRegisterThruNios(u32Data, DEERE_7552_WR_CMD | u32DacAddr);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::ReadDac(uInt16 u16DacAddr, uInt16* pu16Data)
{
	if( NULL == pu16Data )
		return CS_NULL_POINTER;

	if( DEERE_R_COUNT == u16DacAddr )
		*pu16Data = (uInt16)m_pCardState->u32Rcount;
	else if( DEERE_N_COUNT == u16DacAddr )
		*pu16Data = (uInt16)m_pCardState->u32Ncount;
	else if( DEERE_AC_ATTEN == u16DacAddr )
		*pu16Data = m_u16Atten4302;
	else if( DEERE_ADC_CORRECTION == u16DacAddr )
		*pu16Data = m_u8DataCorrection;
	else if( DEERE_EXT_TRIG_SHIFT == u16DacAddr )
		*pu16Data = m_u16DigitalDelay;
	else
	{
		uInt16* pu16DacInfo = GetDacInfoPointer( u16DacAddr );
		if ( NULL == pu16DacInfo )
			return CS_INVALID_DAC_ADDR;
		*pu16Data = *pu16DacInfo;
	}
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize , int64 i64HoldOff )
{
	
	AbortAcquisition(); //----- Abort the previous acquisition, i.e. reset the counters, it's weird but should be like that

	// While Nios Interface does not support 64 bit address,
	ASSERT( i64SegmentSize < 0x100000000 );
	ASSERT( i64PostDepth < 0x100000000 );
	ASSERT( i64HoldOff < 0x100000000 );


	uInt8	u8Shift = 0;
	if ( 0 != ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ))
		u8Shift = SHIFT_DUAL;
	else
		u8Shift = SHIFT_SINGLE;	

	uInt32	u32SegmentSize	= (uInt32) i64SegmentSize;
	uInt32	u32PostDepth	= (uInt32) i64PostDepth;
	uInt32	u32HoldOffDepth = (uInt32) i64HoldOff;
	int32	i32Ret = CS_SUCCESS;

	if ( m_bMulrecAvgTD )
	{
		// Interchange Segment count and Mulrec Acg count
		// From firmware point of view, the SegmentCount is number of averaging
		// and the number of averaging is segment count
		i32Ret = WriteRegisterThruNios( u32SegmentCount, CSPLXBASE_SET_MULREC_AVG_COUNT_FB );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		u32SegmentCount = m_u32MulrecAvgCount;
		u32SegmentSize = u32PostDepth;
	}

	i32Ret = WriteRegisterThruNios( u32SegmentSize >> u8Shift , CSPLXBASE_SET_SEG_LENGTH_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios( u32PostDepth >> u8Shift,	CSPLXBASE_SET_SEG_POST_TRIG_CMD	);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios( m_bCalibActive ? 1 : u32SegmentCount, CSPLXBASE_SET_SEG_NUMBER_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	uInt32	u32SqrPreTrig = u32HoldOffDepth;
	uInt32	u32SqrHoldOffDepth = 0;
	uInt32  u32SqrTrigDelay = 0;

	// Configure Square Rec mode
	i32Ret = WriteRegisterThruNios( m_bSquareRec ? 0: 1, CSPLXBASE_SET_CIRCULAR_REC_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( m_bSquareRec )
	{
		u32SqrPreTrig		= u32SegmentSize - u32PostDepth;
		u32SqrTrigDelay		= (uInt32) m_pCardState->AcqConfig.i64TriggerDelay;
		u32SqrHoldOffDepth	= 0;

		if ( u32HoldOffDepth >  u32SqrPreTrig )
			u32SqrHoldOffDepth	= u32HoldOffDepth - u32SqrPreTrig;

		i32Ret = WriteRegisterThruNios( u32SqrPreTrig>> u8Shift, CSPLXBASE_SET_SEG_PRE_TRIG_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( u32SqrHoldOffDepth >> u8Shift, CSPLXBASE_SET_SQUARE_THOLDOFF_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( u32SqrTrigDelay >> u8Shift, CSPLXBASE_SET_SQUARE_TDELAY_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	else
	{
		// Patch for Firmware Bug !!!!
		// This register setting, tt suppose to work only with square mode but it has effects also in circular mode
		// Clear square delay in circular mode !!!
		i32Ret = WriteRegisterThruNios( 0, CSPLXBASE_SET_SQUARE_TDELAY_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( u32HoldOffDepth >> u8Shift, CSPLXBASE_SET_SEG_PRE_TRIG_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	if ( m_bMulrecAvgTD )
	{
		i32Ret = WriteRegisterThruNios( (uInt32)(m_pCardState->AcqConfig.i64TriggerDelay >> u8Shift), CSPLXBASE_SET_MULREC_AVG_DELAY_FB );
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}	
/*
	char szTrace[128];
	if ( m_bSquareRec )
		sprintf_s(szTrace, sizeof(szTrace), "#### SQUARE / SegmentSize = %d, PosDepth = %d, HoldOff = %d\n",
							u32SegmentSize, u32PostDepth, u32HoldOffDepth );
	else
		sprintf_s(szTrace, sizeof(szTrace), "#### CIRC / SegmentSize = %d, PosDepth = %d, HoldOff = %d\n",
							u32SegmentSize, u32PostDepth, u32HoldOffDepth );
	::OutputDebugString(szTrace);
*/
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN CsDeereDevice::ReadMasterSlaveResponse()
{
	uInt32	u32Data = ReadRegisterFromNios(0, DEERE_MS_REG_RD);

	return (BOOLEAN) (u32Data & DEERE_MS_DETECT);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	CsDeereDevice::GetSegmentPadding(uInt32 u32Mode)
{
	uInt32	u32RealSegmentCount = 1;
	int64	i64RealSegmentSize;
	int64	i64SegmentSize;
	int64	i64RealPostDepth;

	// Calculate the segment padding (in samples)
	i64SegmentSize =
	i64RealSegmentSize =
	i64RealPostDepth = DEERE_DEFAULT_DEPTH;
	AdjustedHwSegmentParameters( u32Mode, &u32RealSegmentCount, &i64RealSegmentSize, &i64RealPostDepth );


	return (uInt32)(i64RealSegmentSize - i64SegmentSize );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN CsDeereDevice::IsFeCalibrationRequired()
{
	if ( m_pCsBoard->NbAnalogChannel > 1 )
		return ( m_pCardState->bCalNeeded[0] || m_pCardState->bCalNeeded[1] );
	else
		return ( m_pCardState->bCalNeeded[0] );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	CsDeereDevice::FillOutBoardCaps(PCSDEEREBOARDCAPS pCapsInfo)
{
	uInt32		u32Temp0;
	uInt32		u32Temp1;
	uInt32		i;


	RtlZeroMemory( pCapsInfo, sizeof(CSDEEREBOARDCAPS) );

	//Fill SampleRate
	for(  i = 0; i < m_u32SrInfoSize; i++ )
	{
		pCapsInfo->SrTable[i].PublicPart.i64SampleRate = m_SrInfoTable[i].llSampleRate;
		
		if( m_SrInfoTable[i].llSampleRate >= 1000000000 )
		{
			u32Temp0 = (uInt32) m_SrInfoTable[i].llSampleRate/1000000000;
			u32Temp1 = (uInt32) (m_SrInfoTable[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000000000) /100000000;
			if( u32Temp1 )
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), 
												"%d.%d GS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), 
												"%d GS/s", u32Temp0 );
		}
		else if( m_SrInfoTable[i].llSampleRate >= 1000000 )
		{
			u32Temp0 = (uInt32) m_SrInfoTable[i].llSampleRate/1000000;
			u32Temp1 = (uInt32) (m_SrInfoTable[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000000) /100000;
			if( u32Temp1 )
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), 
												"%d.%d MS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), 
												"%d MS/s", u32Temp0 );
		}
		else if( m_SrInfoTable[i].llSampleRate >= 1000 )
		{
			u32Temp0 = (uInt32) m_SrInfoTable[i].llSampleRate/1000;
			u32Temp1 = (uInt32) (m_SrInfoTable[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000) /100;
			if( u32Temp1 )
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), 
												"%d.%d kS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), 
												"%d kS/s", u32Temp0 );
		}
		else
		{
			sprintf_s ( pCapsInfo->SrTable[i].PublicPart.strText, sizeof(pCapsInfo->SrTable[i].PublicPart.strText), 
												"%lld S/s", m_SrInfoTable[i].llSampleRate );
		}
	
		if ( 1 == m_pCsBoard->NbAnalogChannel )
			pCapsInfo->SrTable[i].u32Mode = CS_MODE_SINGLE;
		else
			pCapsInfo->SrTable[i].u32Mode = CS_MODE_DUAL | CS_MODE_SINGLE;

	}

	// Fill External Clock info 
	if( 1 == m_pCardState->u8NumOfAdc )
	{
		pCapsInfo->ExtClkTable[0].i64Max = m_SrInfoTable[0].llSampleRate;
		pCapsInfo->ExtClkTable[0].i64Min = DEERE_MIN_EXT_CLK;
		pCapsInfo->ExtClkTable[0].u32Mode = CS_MODE_SINGLE;
		if ( m_pCsBoard->NbAnalogChannel > 1)
			pCapsInfo->ExtClkTable[0].u32Mode |= CS_MODE_DUAL;
	}
	//Fill Range table
	// sorting is done on user level
	for( i = 0; i < m_u32FeInfoSize; i++ )
	{
		pCapsInfo->RangeTable[i].u32InputRange = m_FeInfoTable[i].u32Swing_mV;
		if( pCapsInfo->RangeTable[i].u32InputRange >= 2000 &&  0 == (pCapsInfo->RangeTable[i].u32InputRange % 2000))
			sprintf_s ( pCapsInfo->RangeTable[i].strText, sizeof(pCapsInfo->RangeTable[i].strText), 
										"�%d V", pCapsInfo->RangeTable[i].u32InputRange / 2000 );
		else
			sprintf_s ( pCapsInfo->RangeTable[i].strText, sizeof(pCapsInfo->RangeTable[i].strText), 
										"�%d mV", pCapsInfo->RangeTable[i].u32InputRange / 2 );

		pCapsInfo->RangeTable[i].u32Reserved =	CS_IMP_50_OHM;
	}

	//Fill coupling table
	i = 0;
	pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_DC;
	strcpy_s( pCapsInfo->CouplingTable[i].strText, sizeof(pCapsInfo->CouplingTable[i].strText), "DC" );
	pCapsInfo->CouplingTable[i].u32Reserved = 0;

	i++;
	pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_AC;
	strcpy_s( pCapsInfo->CouplingTable[i].strText, sizeof(pCapsInfo->CouplingTable[i].strText), "AC" );
	pCapsInfo->CouplingTable[i].u32Reserved = 0;
	
	//Fill impedance table
	i = 0;
	pCapsInfo->ImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_50_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[i].strText, sizeof(pCapsInfo->ImpedanceTable[i].strText), "50 Ohm" );
	pCapsInfo->ImpedanceTable[i].u32Reserved = 0;

	//Fill filter table
	i = 0;
	pCapsInfo->FilterTable[i].u32LowCutoff  = CS_NO_FILTER;
	switch( m_pCardState->u8NumOfAdc )
	{
		default:
		case 1: pCapsInfo->FilterTable[i].u32HighCutoff = DEERE_FILTER_FULL_1; break;
		case 2: pCapsInfo->FilterTable[i].u32HighCutoff = DEERE_FILTER_FULL_2; break;
		case 4: pCapsInfo->FilterTable[i].u32HighCutoff = DEERE_FILTER_FULL_4; break;
	}
	

	i++;
	pCapsInfo->FilterTable[i].u32LowCutoff  = 0;
	switch( m_pCardState->u8NumOfAdc )
	{
		default:
		case 1: pCapsInfo->FilterTable[i].u32HighCutoff = DEERE_FILTER_LIMIT_1; break;
		case 2: pCapsInfo->FilterTable[i].u32HighCutoff = DEERE_FILTER_LIMIT_2; break;
		case 4: pCapsInfo->FilterTable[i].u32HighCutoff = DEERE_FILTER_LIMIT_4; break;
	}
	
	//Fill ext trig tables
	// Ranges
	i = 0;
	if( !m_pCardState->bV10 || m_bLowExtTrigGain )
	{
		pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_2_V;
		sprintf_s( pCapsInfo->ExtTrigRangeTable[i].strText, sizeof(pCapsInfo->ExtTrigRangeTable[i].strText), "%s", "�1 V" );
		pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM|CS_IMP_50_OHM|CS_IMP_EXT_TRIG;
		i++;
	}
	if( !m_pCardState->bV10 || !m_bLowExtTrigGain )
	{
		pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_10_V;
		sprintf_s( pCapsInfo->ExtTrigRangeTable[i].strText, sizeof(pCapsInfo->ExtTrigRangeTable[i].strText), "%s", "�5 V" );
		pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM|CS_IMP_50_OHM|CS_IMP_EXT_TRIG;
	}

	//Coupling
	i = 0;
	pCapsInfo->ExtTrigCouplingTable[i].u32Coupling = CS_COUPLING_DC;
	strcpy_s( pCapsInfo->ExtTrigCouplingTable[i].strText, sizeof(pCapsInfo->ExtTrigCouplingTable[i].strText), "DC" );
	pCapsInfo->ExtTrigCouplingTable[i].u32Reserved = 0;
	i++;
	pCapsInfo->ExtTrigCouplingTable[i].u32Coupling = CS_COUPLING_AC;
	strcpy_s( pCapsInfo->ExtTrigCouplingTable[i].strText, sizeof(pCapsInfo->ExtTrigCouplingTable[i].strText), "AC" );
	pCapsInfo->ExtTrigCouplingTable[i].u32Reserved = 0;

	//Impedance
	i = 0;
	pCapsInfo->ExtTrigImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_50_OHM;
	strcpy_s( pCapsInfo->ExtTrigImpedanceTable[i].strText, sizeof(pCapsInfo->ExtTrigImpedanceTable[i].strText), "50 Ohm" );
	pCapsInfo->ExtTrigImpedanceTable[i].u32Reserved = 0;
	i++;
	pCapsInfo->ExtTrigImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_1M_OHM;
	strcpy_s( pCapsInfo->ExtTrigImpedanceTable[i].strText, sizeof(pCapsInfo->ExtTrigImpedanceTable[i].strText), "High Z" );
	pCapsInfo->ExtTrigImpedanceTable[i].u32Reserved = 0;

}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	CsDeereDevice::AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo )
{
	switch( pSysInfo->u32BoardType )
	{
		default:
		case CS12500_BOARDTYPE: strcpy_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CS12500"); break;
		case CS121G_BOARDTYPE:  strcpy_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CS121G "); break;
		case CS122G_BOARDTYPE:  strcpy_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CS122G "); break;
		case CS14250_BOARDTYPE: strcpy_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CS14250"); break;
		case CS14500_BOARDTYPE: strcpy_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CS14500"); break;
		case CS141G_BOARDTYPE:  strcpy_s( pSysInfo->strBoardName, sizeof(pSysInfo->strBoardName), "CS141G "); break;
	}
	pSysInfo->strBoardName[6] = ( 0 != (pSysInfo->u32AddonOptions & DEERE_OPT_DUAL_CHANNEL) ) ? '2':'1';

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
int32 CsDeereDevice::FindFeIndex( const uInt32 u32Swing, uInt32 *u32Index )
{
	for( uInt32 i = 0; i < m_u32FeInfoSize; i++ )
	{
		if ( u32Swing == m_FeInfoTable[i].u32Swing_mV )
		{
			*u32Index = i;
			return CS_SUCCESS;
		}
	}
	return CS_INVALID_GAIN;
}


#if FORLATER

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsDeereDevice::SetTriggerAddressOffsetAdjust( int8 TrigAddressOffset )
{
	m_i32Tao = TrigAddressOffset;
}

#endif


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR )
{

	for ( uInt32 i = 0; i < m_u32SrInfoSize; i++ )
	{
		if ( m_SrInfoTable[i].llSampleRate == llSampleRate )
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
void CsDeereDevice::AssignBoardType()
{
	if ( 0 != (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_DUAL_CHANNEL) )
	{
		m_pCardState->AcqConfig.u32Mode = CS_MODE_DUAL;
		m_pCsBoard->NbAnalogChannel = 2;
	}
	else
	{
		m_pCardState->AcqConfig.u32Mode = CS_MODE_SINGLE;
		m_pCsBoard->NbAnalogChannel = 1;
	}

	if ( 0 != (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_14BIT) )
	{
		m_pCardState->b14Bits = true;
		m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_14;
		m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_14_LJ;
		m_pCsBoard->u32BoardType |= 0x10;
	}
	else
	{
		m_pCardState->b14Bits = false;
		m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_12;
		m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_12_LJ;
	}

	if ( 0 != (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_2ADC) )
	{
		m_pCsBoard->u32BoardType = m_pCardState->b14Bits ? CS14500_BOARDTYPE : CS121G_BOARDTYPE;;
		m_pCardState->u8NumOfAdc = 2;
	}
	else if ( 0 != (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_4ADC) )
	{
		m_pCsBoard->u32BoardType = m_pCardState->b14Bits ? CS141G_BOARDTYPE : CS122G_BOARDTYPE;;
		m_pCardState->u8NumOfAdc = 4;
	}
	else
	{
		m_pCsBoard->u32BoardType = m_pCardState->b14Bits ? CS14250_BOARDTYPE : CS12500_BOARDTYPE;;
		m_pCardState->u8NumOfAdc = 1;
	}

	if( m_pCsBoard->u32BaseBoardVersion <= 0x0100 )
		m_pCardState->bV10 = true;
	else
		m_pCardState->bV10 = false;
	
	m_pCardState->AcqConfig.u32ExtClk = DEERE_DEFAULT_EXT_CLOCK_EN;

	m_pCardState->AcqConfig.i32SampleRes = -CS_SAMPLE_RES_LJ;
	m_pCardState->AcqConfig.u32SampleSize = CS_SAMPLE_SIZE_2;

	m_pCsBoard->NbDigitalChannel	= 0;
	m_pCsBoard->NbTriggerMachine	= 2*m_pCsBoard->NbAnalogChannel + 1;
}


//-----------------------------------------------------------------------------
//	Initialization upon received IOCTL_ACQUISITION_SYSTEM_INIT
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::AcqSystemInitialize()
{
	if( !GetInitStatus()->Info.u32Nios )
		return CS_FALSE;

	ReadCommonRegistryOnAcqSystemInit();
	return ConfigureAlarmSource();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsDeereDevice::UpdateMaxMemorySize( uInt32	u32BaseBoardHardwareOptions, uInt32 u32SampleSize )
{
	UNREFERENCED_PARAMETER(u32BaseBoardHardwareOptions);
	uInt32 u32DimmSize = ReadRegisterFromNios(0, CSPLXBASE_GET_MEMORYSIZE_CMD);
	m_pCsBoard->i64MaxMemory = ( m_PlxData->u32NvRamSize < u32DimmSize ) ? m_PlxData->u32NvRamSize : u32DimmSize;
	m_pCsBoard->i64MaxMemory = (m_pCsBoard->i64MaxMemory << 10)/u32SampleSize;
	GetInitStatus()->Info.u32BbMemTest = m_pCsBoard->i64MaxMemory != 0 ? 1 : 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsDeereDevice::UpdateAdcCalibInfoStructure(uInt32 u32InputRange, uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt8 u8Data)
{
	if( u16Core > 1 || u16Adc >= DEERE_ADC_COUNT ||  u16Addr > DEERE_ADC_SKEW )
		return;

	uInt32	u32Index = 0;

	FindFeIndex( u32InputRange, &u32Index );
	switch ( u16Addr )
	{ 
		case DEERE_ADC_COARSE_OFFSET: if( 0 == u16Core ) m_pCardState->AdcReg[u16Adc][u32Index]._CoarseOffsetA = u8Data; else m_pCardState->AdcReg[u16Adc][u32Index]._CoarseOffsetB = u8Data; break;
		case DEERE_ADC_FINE_OFFSET:   if( 0 == u16Core ) m_pCardState->AdcReg[u16Adc][u32Index]._FineOffsetA = u8Data;   else m_pCardState->AdcReg[u16Adc][u32Index]._FineOffsetB = u8Data; break;
		case DEERE_ADC_COARSE_GAIN:   if( 0 == u16Core ) m_pCardState->AdcReg[u16Adc][u32Index]._CoarseGainA = u8Data;   else m_pCardState->AdcReg[u16Adc][u32Index]._CoarseGainB = u8Data; break;
		case DEERE_ADC_MEDIUM_GAIN:   if( 0 == u16Core ) m_pCardState->AdcReg[u16Adc][u32Index]._MedGainA = u8Data;      else m_pCardState->AdcReg[u16Adc][u32Index]._MedGainB = u8Data; break;
		case DEERE_ADC_FINE_GAIN:     if( 0 == u16Core ) m_pCardState->AdcReg[u16Adc][u32Index]._FineGainA = u8Data;     else m_pCardState->AdcReg[u16Adc][u32Index]._FineGainB = u8Data; break;
		
		// _Skew is constant accross all input ranges
		case DEERE_ADC_SKEW: m_pCardState->AdcReg[u16Adc][0]._Skew = u8Data;	break;
	}

	return;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt16* CsDeereDevice::GetDacInfoPointer( uInt16 u16DacAddr )
{
	uInt16*	pDacInfoPtr = NULL;
	uInt16	u16ChanZeroBased = 0;
	uInt32	u32Index;
	int32	i32Ret = CS_SUCCESS;

	if ( u16DacAddr < DEERE_CHAN2_DAC_CTRL || u16DacAddr == DEERE_U_OFF_SCALE_1 || u16DacAddr == DEERE_U_OFF_CENTR_1 || u16DacAddr == DEERE_IR_SCALE_1 )
		u16ChanZeroBased = 0;
	else
		u16ChanZeroBased = 1;
	
	i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, &u32Index );
	if ( CS_FAILED(i32Ret) )return NULL;

	switch ( u16DacAddr )
	{
		// DAC controls
		case DEERE_OFFSET_NULL_1: pDacInfoPtr = &m_pCardState->DacInfo[0][u32Index]._OffsetNull; break;
		case DEERE_OFFSET_COMP_1: pDacInfoPtr = &m_pCardState->DacInfo[0][u32Index]._OffsetComp; break;
		case DEERE_USER_OFFSET_1: pDacInfoPtr = &m_pCardState->DacInfo[0][u32Index]._UserOffset; break;
		case DEERE_U_OFF_SCALE_1: pDacInfoPtr = (uInt16*)(&m_pCardState->DacInfo[0][u32Index]._UserOffsetScale); break;
		case DEERE_U_OFF_CENTR_1: pDacInfoPtr = &m_pCardState->DacInfo[0][u32Index]._UserOffsetCentre; break;
		case DEERE_IR_SCALE_1:    pDacInfoPtr = &m_pCardState->DacInfo[0][u32Index]._InputRangeScale; break;

		case DEERE_OFFSET_NULL_2: pDacInfoPtr = &m_pCardState->DacInfo[1][u32Index]._OffsetNull; break;
		case DEERE_OFFSET_COMP_2: pDacInfoPtr = &m_pCardState->DacInfo[1][u32Index]._OffsetComp; break;
		case DEERE_USER_OFFSET_2: pDacInfoPtr = &m_pCardState->DacInfo[1][u32Index]._UserOffset; break;
		case DEERE_U_OFF_SCALE_2: pDacInfoPtr = (uInt16*)(&m_pCardState->DacInfo[1][u32Index]._UserOffsetScale); break;
		case DEERE_U_OFF_CENTR_2: pDacInfoPtr = &m_pCardState->DacInfo[1][u32Index]._UserOffsetCentre; break;
		case DEERE_IR_SCALE_2:    pDacInfoPtr = &m_pCardState->DacInfo[1][u32Index]._InputRangeScale; break;
		
		case DEERE_CLKPHASE_1:    pDacInfoPtr = &m_pCardState->DacInfo[0][u32Index]._ClkPhaseOff_LoP; break;
		case DEERE_CLKPHASE_3:    pDacInfoPtr = &m_pCardState->DacInfo[0][u32Index]._ClkPhaseOff_HiP;  break;
		case DEERE_CLKPHASE_0:	  pDacInfoPtr = &m_pCardState->DacInfo[1][u32Index]._ClkPhaseOff_LoP; break;
		case DEERE_CLKPHASE_2:    pDacInfoPtr = &m_pCardState->DacInfo[1][u32Index]._ClkPhaseOff_HiP;  break;

		case DEERE_MS_CLK_PHASE_ADJ: pDacInfoPtr = &m_pCardState->DacInfo[0][0]._RefPhaseOff;  break;
	}
	return pDacInfoPtr;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CsDeereDevice::DefaultAdcCalibInfoRange( uInt16 u16HwChan, uInt32 u32InputRange )
{
	uInt8	u8Data;
	if( u16HwChan > m_pCsBoard->NbAnalogChannel )
		return;
	uInt16 u16ZeroBasedChan = u16HwChan - 1;
	if ( 0 == u32InputRange )
		u32InputRange = m_pCardState->ChannelParams[u16ZeroBasedChan].u32InputRange;

	for (uInt8 u8Adc = 0; u8Adc < DEERE_ADC_COUNT; u8Adc++ )
		if( isAdcForChannel( u8Adc, u16ZeroBasedChan ) )
			for (uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
				for( uInt16 u16Addr = DEERE_ADC_COARSE_OFFSET; u16Addr <= DEERE_ADC_SKEW; u16Addr++ )
					ReadAdcReg( u8Core, u8Adc, u16Addr, &u8Data, u32InputRange );
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CsDeereDevice::DefaultAdcCalibInfoStructure()
{
	uInt32	u32InputRange = 0;

	for( uInt32 i = 0; i < m_u32FeInfoSize; i++ )
	{
		u32InputRange = m_FeInfoTable[i].u32Swing_mV;
		DefaultAdcCalibInfoRange( CS_CHAN_1,u32InputRange );
		DefaultAdcCalibInfoRange( CS_CHAN_2,u32InputRange );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	CsDeereDevice::ConvertToHwTimeout( uInt32 u32SwTimeout, uInt32	u32SampleRate )
{
	uInt32 u32HwTimeout = u32SwTimeout;

	// Convert TriggerTimeout from 100 ns to number of memory locations
	// Each memory location contains 16 Samples (Single) or 2x8 Samples (Dual)
	if( CS_TIMEOUT_DISABLE != u32HwTimeout && 0 != u32HwTimeout)
	{
		LONGLONG llTimeout = (LONGLONG)u32HwTimeout * u32SampleRate;
		llTimeout /= (m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) ? 16: 32;
		//round up 100 ns
		llTimeout += 9999999;
		llTimeout /= 10000000; 

		if ( llTimeout > (uInt32) CS_TIMEOUT_DISABLE )
			m_pCardState->AcqConfig.i64TriggerTimeout = u32HwTimeout = (uInt32) CS_TIMEOUT_DISABLE;
		else
			u32HwTimeout = (uInt32)llTimeout;
	}

	return u32HwTimeout;
}

bool CsDeereDevice::isAdcForChannel( uInt8 u8AdcNum, uInt16 u16ZeroBasedChannel )
{
	if ( u8AdcNum >= DEERE_ADC_COUNT || u16ZeroBasedChannel >= DEERE_CHANNELS )
		return false;
	
	if( 4 == m_pCardState->u8NumOfAdc )
		return 0 == u16ZeroBasedChannel;
	else if( 2 == m_pCardState->u8NumOfAdc )
	{
		if( 0 == u16ZeroBasedChannel ) 
			return 0 == u8AdcNum%2; // 0 & 180 => 0 & 2
		else
			return 1 == u8AdcNum%2; // 90 & 270 => 1 & 3
	}
	else if( 0 == u16ZeroBasedChannel ) 
		return 2 == u8AdcNum;  //180
	else 
		return 3 == u8AdcNum; //270
}


uInt8 CsDeereDevice::AdcForChannel( uInt16 u16ZeroBasedChannel, uInt8 u8Parity )
{
	if ( u16ZeroBasedChannel >= DEERE_CHANNELS )
		return (uInt8)(-1);
	
	if( 4 == m_pCardState->u8NumOfAdc ) 
	{
		return (u8Parity + 0)%4;
	}
	else if( 2 == m_pCardState->u8NumOfAdc )
	{
		if( 0 == u16ZeroBasedChannel ) 
			return 0 != u8Parity%2 ? 2 : 0; //180:0
		else
			return 0 != u8Parity%2 ? 3 : 1; //270:90
	}
	else if( 1 == m_pCardState->u8NumOfAdc )
	{
		if( 0 == u16ZeroBasedChannel ) 
			return 2; //180
		else 
			return 3; //270
	}
	else
		return (uInt8)(-1);
}

uInt16 CsDeereDevice::ChannelForAdc( uInt16 u16Adc )
{
	if( 4 == m_pCardState->u8NumOfAdc ) 
		return 0;
	else if ( 2 == m_pCardState->u8NumOfAdc ) 
		return (u16Adc%2); //0 and 2 -> ch 1; 1 and 3 -> ch 2
	else if( 2 == u16Adc )
		return 0;
	else 
		return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::ConfigureAlarmSource(bool bIncludeAcSrc)
{
	uInt16	u16AlarmSource = 0;

	if ( ecmCalModeNormal == m_CalibMode )
	{
		if ( 0 != (m_pCardState->AddOnReg.u16AdcCtrl & DEERE_CHAN_A_ONLY) )
			u16AlarmSource = DEERE_ALARM_PROTECT_CH1;
		else if ( 0 != (m_pCardState->AddOnReg.u16AdcCtrl & DEERE_CHAN_B_ONLY) )
			u16AlarmSource = DEERE_ALARM_PROTECT_CH2;
		else	// Dual channel mode
			u16AlarmSource = DEERE_ALARM_PROTECT_CH1 | DEERE_ALARM_PROTECT_CH2;
	}
	else if( bIncludeAcSrc )
		u16AlarmSource = DEERE_ALARM_CAL_PLL_UNLOCK;

	u16AlarmSource &= m_u16AlarmMask;

	UpdateCardState();

	return ClearAlarmStatus( u16AlarmSource );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void	CsDeereDevice::GetAlarmStatus()
{
	m_u16AlarmStatus = (uInt16)ReadRegisterFromNios( 0, DEERE_ALARM_CMD );	
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::ClearAlarmStatus( uInt16 u16AlarmSource )
{
	int32 i32Ret = CS_SUCCESS;

	if (( u16AlarmSource != m_u16AlarmSource ) ||			// Alarm Source changed
		(0 != (m_u16AlarmSource & m_u16AlarmStatus )) )		// Got an alarm before
	{
		// Clear alarm source may generate an alarm interrupt
		m_u16AlarmSource = 0;
		i32Ret = WriteRegisterThruNios( DEERE_ALARM_CLEAR, DEERE_ALARM_CMD | SPI_WRITE_CMD_MASK );
		i32Ret = WriteRegisterThruNios( u16AlarmSource, DEERE_ALARM_CMD | SPI_WRITE_CMD_MASK );
		m_u16AlarmSource = u16AlarmSource;
		m_u16AlarmStatus = 0;
	}

	if (0 != (m_u16AlarmSource & m_u16AlarmStatus )) 
	{
		// Got an alarm before
		// The front end has been switched into protection mode. By clearing alarm status (code aboved)
		// the front end will be reset into normal acqusition mode but it will take some time.
		// Waiting for the front end relay stable.
		// In theory, 7ms second is required. For safe, we use 10ms
		BBtiming(100);
	}

	return i32Ret;}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::ReportAlarm()
{
	int32 i32Ret = CS_SUCCESS;
	//MAsked read value by allowed alarm sources
	uInt16 u16ReportedStatus = m_u16AlarmSource & m_u16AlarmStatus;
	if( 0 == u16ReportedStatus )
		return i32Ret;

	char ChannelText[50] = {0};
	char TracelChar = ' ';
	int i = 0;
	if( u16ReportedStatus & DEERE_ALARM_PROTECT_CH1 )
	{
		TracelChar = '1';
		ChannelText[i++] = '#';
		ChannelText[i++] = '1';
		i32Ret = CS_CHANNEL_PROTECT_FAULT;
	}
	
	if( u16ReportedStatus & DEERE_ALARM_PROTECT_CH2 )
	{
		TracelChar = '2';
		if( 0 != i )
		{
			ChannelText[i++] = ',';
			TracelChar++;
		}
		ChannelText[i++] = '#';
		ChannelText[i++] = '2';
		if ( 0 != ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) )
			i32Ret = CS_CHANNEL_PROTECT_FAULT;
	}
	
	if( u16ReportedStatus & DEERE_ALARM_CAL_PLL_UNLOCK )
	{
		if( CS_SUCCEEDED(i32Ret) ) i32Ret = CS_FALSE;
		::OutputDebugString(" ! # ! AC Cal source lost lock.\n");
	}
	else
	{
		ChannelText[i] = 0;
#if DEBUG
		t.Trace(TraceInfo, " ! %c - %02x (%02x) ! Protection fault on channel %s Return value %d.\n", TracelChar, m_u16AlarmStatus, u16ReportedStatus, ChannelText, i32Ret );
#else
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_OVERVOLTAGE,  ChannelText);
#endif		
	}
	return i32Ret;
}	

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool	CsDeereDevice::IsSquareRec( uInt32 u32Mode, uInt32	u32SegmentCount, LONGLONG i64PreTrigDepth )
{
	// Detect if square rec mode will be used
	if ( m_pCardState->u8ImageId != 0 )
		return true;		// Expert AVG is always in Square mode

	int32 i32MaxPreTrigSquare = ((u32Mode & CS_MODE_SINGLE) != 0) ? DEERE_MAX_SQUARE_PRETRIGDEPTH : DEERE_MAX_SQUARE_PRETRIGDEPTH>>1;

	if ( 1 == u32SegmentCount )
	{
		if ( ! m_bDynamicSquareRec )
			return false;
		else if ( i64PreTrigDepth > i32MaxPreTrigSquare )
			return false;
		else 
			return true;
	}
	else 
		return true;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CsDeereDevice::ResetCachedState()
{
	RtlZeroMemory( &m_pCardState->AddOnReg, sizeof(m_pCardState->AddOnReg) );
	m_pCardState->AddOnReg.u32FeSetup[0] = DEERE_SET_USER_INPUT;
	m_pCardState->AddOnReg.u32FeSetup[1] = DEERE_SET_USER_INPUT;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CsDeereDevice::CompareFwVersion(FPGA_FWVERSION Ver1, FPGA_FWVERSION Ver2)
{
	if ( Ver1.Version.uMajor	!= Ver2.Version.uMajor ||
		 Ver1.Version.uMinor	!= Ver2.Version.uMinor ||
		 Ver1.Version.uRev		!=	Ver2.Version.uRev  ||
		 Ver1.Version.uIncRev	!= Ver2.Version.uIncRev )
		 return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SetRoleMode( eRoleMode ermSet )
{
	int32 i32Ret = CS_SUCCESS;
	uInt16	u16MsCtrl	= (m_pCardState->AddOnReg.u16MsCtrl & ~DEERE_MSCLKBUFFER_ENABLE) | DEERE_MSTRANSCEIVEROUT_DISABLE;

	uInt16 u16ClkSelect = m_pCardState->AddOnReg.u16ClkSelect & ~DEERE_CLKR1_LOCALCLK;

	if ( ermStandAlone == ermSet )
		u16ClkSelect |= DEERE_CLKR1_LOCALCLK;
	else
	{
		u16MsCtrl &= ~DEERE_MSTRANSCEIVEROUT_DISABLE;
		u16MsCtrl |= DEERE_MSCLKBUFFER_ENABLE;
	}

	i32Ret = WriteRegisterThruNios(u16MsCtrl, DEERE_MS_CTRL);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteRegisterThruNios(u16ClkSelect, DEERE_CLOCK_CTRL);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->AddOnReg.u16ClkSelect = u16ClkSelect;
	m_pCardState->AddOnReg.u16MsCtrl	= u16MsCtrl;

	m_pCardState->ermRoleMode = ermSet;
	return i32Ret;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsDeereDevice::RemoveDeviceFromUnCfgList()
{
	CsDeereDevice	*Current	= (CsDeereDevice	*) m_ProcessGlblVars->pFirstUnCfg;
	CsDeereDevice	*Prev = NULL;

	if ( this  == (CsDeereDevice	*) m_ProcessGlblVars->pFirstUnCfg )
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

	if ( this  == (CsDeereDevice	*) m_ProcessGlblVars->pLastUnCfg )
	{
		if (Prev)
			Prev->m_Next = NULL;
		m_ProcessGlblVars->pLastUnCfg = (PVOID) Prev;
	}

	return CS_SUCCESS;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
uInt32	CsDeereDevice:: GetFifoModeTransfer( uInt16 u16UserChannel, uInt32 u32TxMode )
{
	uInt32	u32ModeSel = 0;

	 if ( (0 != (m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL)) && (0 == (u32TxMode & TxMODE_SEGMENT_TAIL)) && (0 == (u32TxMode & TxMODE_DATA_INTERLEAVED)) )
	 {
		u32ModeSel = (u16UserChannel - 1) << 4;
		u32ModeSel |= MODESEL1;
	}

	return u32ModeSel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

uInt32	CsDeereDevice::GetDataMaskModeTransfer ( uInt32 u32TxMode )
{
	uInt32	u32Data = 0;
	uInt32	u32MaskAnalog = 0;

	if ( m_pCardState->b14Bits )
		u32MaskAnalog = 0x30003;
	else
		u32MaskAnalog = 0xF000F;

	// Clear Slave mode bit
	u32TxMode &=  ~TxMODE_SLAVE;

	switch( u32TxMode )
	{
		case TxMODE_SEGMENT_TAIL:
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
		
		case TxMODE_DATA_ONLYDIGITAL:
		{
			// Masking everything but 2 bits for digital channel
			u32Data = ~u32MaskAnalog;
		}
		break;

		case TxMODE_DATA_32:
		case TxMODE_DATA_16:
		{
			// Seting for full 16 bit data transfer
			u32Data = 0x00000;
		}
		break;
		
		default:		// default mode binary
		case TxMODE_DATA_ANALOGONLY:
		{
			u32Data = u32MaskAnalog;
		}

		break;
	}
	return u32Data;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
uInt32	CsDeereDevice::GetSquareEtb()
{
	uInt32	u32SqrAdjust = 0;

	// Get the Etb
	if ( m_bSquareRec )
	{
		uInt32 u32TriggerAddress = 0;
		if ( m_bMulrecAvgTD )
			u32TriggerAddress = m_MasterIndependent->ReadRegisterFromNios(0, CSPLXBASE_READ_AVG_TRIGADDR_CMD);
		else
			u32TriggerAddress = m_MasterIndependent->ReadRegisterFromNios(0, CSPLXBASE_READ_TRIG_ADD_CMD);
		
		uInt32 Etb = (u32TriggerAddress >> 4) & 0xF;
		// Convert Etb into the offset to the next memory location
		if ( Etb != 0 )	
		{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
				u32SqrAdjust = (1<<SHIFT_DUAL) - Etb;
			else
				u32SqrAdjust = (1<<SHIFT_SINGLE) - Etb;
		}
	}

	return u32SqrAdjust;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CsDeereDevice::UpdateCardState(BOOLEAN bSuccess)
{
	m_pCardState->bCardStateValid = bSuccess;

#if !defined(USE_SHARED_CARDSTATE)
	IoctlSetCardState(m_pCardState);
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CsDeereDevice::SetDefaultAuxIoCfg()
{
	m_pCardState->bDisableTrigOut = TRUE;
	m_pCardState->u16ExtTrigEnCfg = 0;
	m_pCardState->eclkoClockOut	= eclkoNone;
	m_pCardState->AuxIn			= eAuxInNone;
	m_pCardState->AuxOut			= eAuxOutNone;

}