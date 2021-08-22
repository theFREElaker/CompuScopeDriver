// CsDeereDevice.cpp
//

#include "StdAfx.h"
#include "CsDeere.h"
#include "CsDeereFlash.h"
#include "CsDeereFlashObj.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

#ifdef _WINDOWS_
	#include <process.h>			// for _beginthreadex

	// DEBUG PRF (paralelle port)
	#include <conio.h>
#endif



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsDeereDevice::CsDeereDevice(uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars):
	m_u8CardIndex(u8Index), m_ProcessGlblVars(ProcessGlblVars), m_PseudoHandle(pDevInfo->DrvHdl)
{
	// Memory has been cleared on allocation
	// Only need to initialize member variables whose default value is different than 0
	m_BusMasterSupport = TRUE;

	m_bDeviceInitialized = false;
	m_pTriggerMaster  = this;
	m_MasterIndependent = this;
	m_bAutoClockAdjust = true;
	m_u16Atten4302 = 0;

	m_u8TrigSensitivity	= DEERE_DEFAULT_TRIG_SENSITIVITY;

	m_u16AlarmMask   = DEERE_ALARM_PROTECT_CH1 | DEERE_ALARM_PROTECT_CH2 | DEERE_ALARM_CAL_PLL_UNLOCK;

	m_bLowExtTrigGain = true;

	m_u16AdcDpaMask = 0;	

	m_u16DigitalDelay = (uInt16)-1;
	m_bMsOffsetAdjust = true;
	m_u16SkipCalibDelay = 20;
	m_ecmDebugCalSrcMode = ecmCalModeNormal;

	m_u8DataCorrection = DEERE_DEFAULT_ADC_CORRECTION;

	m_bSoftwareAveraging = false;
	m_u32MulrecAvgCount = 1;

	m_u32CalSrcRelayDelay = 32; // 3.2 ms
	m_u8CalDacSettleDelay = 6;  // ~1.84 ms  = ~28.8 us * (1<<6)
	m_u16AdcRegDelay      = 30;  // 3 ms

	m_u32MatchAdcGainTolerance = 10;
	m_u32MatchAdcOffsetTolerance = 5;

	m_bVerifyDcCalSrc = true;

	m_pVirtualMaster  =	this;
	m_MasterIndependent = this;
	m_u8TrigSensitivity = DEERE_DEFAULT_TRIG_SENSITIVITY;
	m_u16ExtTrigSensitivity = DEERE_DEFAULT_EXT_TRIG_SENSITIVITY;

	m_bHwAvg32	= true;
	m_bTestBridgeEnabled = true;
	m_bDynamicSquareRec = true;		// If it is false then use "Circular Rec" on single rec mode and "Square Rec" in mulrec mode
									// if it is true then use "Square Rec" in all cases except when there is a big pretrigger data

	m_bCalibSqr = true; 			// Calibration in Square Mode

	m_u32DefaultRange			 = CS_GAIN_2_V;
	m_u32DefaultExtTrigRange	 = CS_GAIN_2_V;
	m_u32DefaultExtTrigCoupling  = CS_COUPLING_DC;
	m_u32DefaultExtTrigImpedance = CS_REAL_IMP_1M_OHM;

	m_i8OffsetHwAverage = DEERE_DEFAULT_HWAVERAGE_ADJOFFSET;
	m_pCsBoard			= GetBoardNode();
	
	CsDevIoCtl::InitializeDevIoCtl( pDevInfo );

	m_bSkipFirmwareCheck = false;

	sprintf_s(m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", DEERE_FPGA_IMAGE_FILE_NAME);
	sprintf_s(m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", DEERE_FPGA_IMAGE_FILE_NAME);
	RtlZeroMemory( &m_BaseCsiEntry, sizeof( m_BaseCsiEntry ) );


#if !defined(USE_SHARED_CARDSTATE)
	m_pCardState	= &m_CardState;
	m_PlxData		= &m_pCardState->PlxData;
	m_pCsBoard		= &m_PlxData->CsBoard;
#else
	m_pCardState	= 0;
#endif
	
	for( uInt16	i = 0; i < DEERE_CHANNELS; i++ )
		m_bUpdateCoreOrder[i] = TRUE;
}		


////////////////////////////////////////////////////////////////////////
//

int32	CsDeereDevice::CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound )
{
	int32 i32Status = CS_SUCCESS;

	CsDeereDevice* pCard	= (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
	
	// Remember the ForceInd flag for use later on Acqusition system initialization
	while (pCard)
	{
		pCard->m_pCardState->bForceIndMode = (BOOLEAN) bForceInd;
		pCard = pCard->m_HWONext;
	}

	if (( m_ProcessGlblVars->NbSystem == 0 ) && ! bForceInd )
	{
		pCard	= (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
		while (pCard)
		{
			pCard->ReadAndValidateCsiFile( pCard->m_pCsBoard->u32BaseBoardOptions[0], 0 );
			pCard->ReadAndValidateFwlFile();

#if !defined(USE_SHARED_CARDSTATE)
			// Get the latest CardState values.
			// This must be done to fixe the problem of adding or updating Expert firmware from CompuScopeManager. 
			// In this case, the firmware info (CardState) has been changed by CompuScopeManager. Then when Rm call this function,
			// Rm should get the new the firmware info by calling IoctlGetCardState.
			pCard->IoctlGetCardState(pCard->m_pCardState);
#endif
			if ( pCard->GetInitStatus()->Info.u32Nios )
			{
				pCard->ReadHardwareInfoFromEeprom();
				pCard->AssignBoardType();
				pCard->CheckRequiredFirmwareVersion();
			}

			pCard = pCard->m_HWONext;
		}

		pCard	= (CsDeereDevice*) m_ProcessGlblVars->pFirstUnCfg;
		while (pCard)
		{
			if ( pCard->m_pCardState->u8BadFirmware )
			{
				i32Status = CS_INVALID_FW_VERSION;
				break;
			}
			pCard = pCard->m_HWONext;
		}
	}

	if ( CS_FAILED( i32Status ) )
	{
		// One of cards has invalid firmware.
		// Notify CsRm for Auto firmware update
		*pu16SystemFound = 0;
	}
	else
	{
		if ( m_ProcessGlblVars->NbSystem == 0 )
		{
			*pu16SystemFound =
			m_ProcessGlblVars->NbSystem = DetectAcquisitionSystems( bForceInd );
		}
		else
			*pu16SystemFound = m_ProcessGlblVars->NbSystem;

	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsDeereDevice::CsAcquisitionSystemInitialize( uInt32 u32InitFlag )
{
	int32	i32Status = CS_SUCCESS;

	if ( ! m_pSystem )
		return CS_INVALID_HANDLE;

	if ( m_pSystem->bInitialized )
	{
		// Clean up has not been performed. Possible because application crashed.
		// Cleanup before initialize
		m_pSystem->bCrashRecovery = TRUE;
				
		i32Status = MsInitializeDevIoCtl();
		if ( CS_SUCCEEDED(i32Status) )
			CsAcquisitionSystemCleanup();
		else
			return i32Status;
	}
	
	if ( ! m_pSystem->bInitialized )
	{
		m_pSystem->bCrashRecovery = FALSE;		
		// Open link to kernel driver for all cards in system
		i32Status = MsInitializeDevIoCtl();
		if ( CS_FAILED(i32Status) )
			return i32Status;

		CsDeereDevice* pDevice = this;
		while ( pDevice )
		{
			if ( ! pDevice->m_pCardState->bCardStateValid )
			{
				i32Status = pDevice->DeviceInitialize();
				if ( CS_FAILED(i32Status) )
					break;
			}

			i32Status = pDevice->AcqSystemInitialize();
			if ( CS_FAILED(i32Status) )
				break;

			pDevice = pDevice->m_Next;
		}

		if ( CS_SUCCEEDED(i32Status) )
		{
			m_u32SystemInitFlag =  u32InitFlag;

			if ( ! m_pCardState->bForceIndMode )
			{
				// Create acquisition events
				m_pSystem->hAcqSystemCleanup	= ::CreateEvent(NULL, FALSE, FALSE, NULL );
				m_pSystem->hEventTriggered		= ::CreateEvent(NULL, FALSE, FALSE, NULL );
				m_pSystem->hEventEndOfBusy		= ::CreateEvent(NULL, FALSE, FALSE, NULL );
				m_pSystem->hEventAlarm			= ::CreateEvent(NULL, FALSE, FALSE, NULL );
				
				// Create events for asynchronous data transfer
				CsDeereDevice* pDevice = this;
				while ( pDevice )
				{
					pDevice->m_DataTransfer.hAsynEvent	= ::CreateEvent(NULL, FALSE, FALSE, NULL );
					pDevice = pDevice->m_Next;
				}

				if ( ! m_UseIntrTrigger )
				{
					i32Status = RegisterEventHandle( m_pSystem->hEventTriggered, ACQ_EVENT_TRIGGERED );
					if ( CS_FAILED(i32Status) )
					{
						::OutputDebugString("Error RegisterEvent TRIGGERED......\n");
						return i32Status;
					}
					else
						m_UseIntrTrigger = TRUE;
				}

				if ( ! m_UseIntrBusy )
				{
					i32Status = RegisterEventHandle( m_pSystem->hEventEndOfBusy, ACQ_EVENT_END_BUSY );
					if ( CS_FAILED(i32Status) )
					{
						::OutputDebugString("Error RegisterEvent END_BUSY......\n");
						return i32Status;
					}
					else
						m_UseIntrBusy = TRUE;
				}

				// Register the Alarm event to kernel driver
				i32Status = RegisterEventHandle( m_pSystem->hEventAlarm, ACQ_EVENT_ALARM );
				if ( CS_FAILED(i32Status) )
				{
					::OutputDebugString("Error RegisterEvent ALARM......\n");
					return i32Status;
				}

		#ifdef _WINDOWS_
				i32Status = CreateThreadAsynTransfer();
				if ( CS_SUCCEEDED(i32Status) )
				{
					m_pSystem->hThreadIntr = (HANDLE)::_beginthreadex(NULL, 0, ThreadFuncIntr, this, 0, (unsigned int *)&m_pSystem->u32ThreadIntrId);
					::SetThreadPriority(m_pSystem->hThreadIntr, THREAD_PRIORITY_HIGHEST);
				}
		#else
				// CreateInterruptEventThread();
				InitEventHandlers();
		#endif

				if ( 0 == m_u32SystemInitFlag )	
				{	
					// If m_u32SystemInitFlag == 0 then perform the default full initialization,
					// otherwise just take whatever it is from the current state
					MsCsResetDefaultParams();
				}
			}

			pDevice = m_MasterIndependent;
			m_pCardState->u32ProcId = GetCurrentProcessId();
			while( pDevice )
			{
				pDevice->m_pCardState->u32ProcId = m_pCardState->u32ProcId;
				pDevice->UpdateCardState();
				pDevice = pDevice->m_Next;
			}
			m_pSystem->bInitialized = TRUE;
			m_pSystem->BusyTransferCount = 0;
		}

		if ( CS_FAILED(i32Status) )
			CsAcquisitionSystemCleanup();
	}

	return i32Status;
}



////////////////////////////////////////////////////////////////////////
//

void	CsDeereDevice::CsAcquisitionSystemCleanup()
{
	// Set the reset flag in order to stop all pending processing
	m_pSystem->bReset = TRUE;

	CsAbort();

	if ( m_pAverageBuffer )
	{
		::GageFreeMemoryX( m_pAverageBuffer );
		m_pAverageBuffer = NULL;
	}

	CsDeereDevice *pDevice = m_MasterIndependent;

	if ( m_pCardState->u8ImageId != 0 )
	{
		// switch to the default image
		pDevice = m_MasterIndependent;
		while (pDevice != NULL)
		{
			pDevice->CsLoadFpgaImage( 0 );
			pDevice = pDevice->m_Next;
		}

		pDevice = m_MasterIndependent;
		while (pDevice != NULL)
		{
			pDevice->CsSetAcquisitionConfig();
			pDevice->CsSetChannelConfig();
			pDevice->CsSetTriggerConfig();
			pDevice->ForceFastCalibrationSettingsAllChannels();

			pDevice = pDevice->m_Next;
		}
	}

	// Undo register events
	// Undo register events
	if ( 0 == m_u32SystemInitFlag )
	{
		RegisterEventHandle( NULL, ACQ_EVENT_TRIGGERED );
		RegisterEventHandle( NULL, ACQ_EVENT_END_BUSY );
		RegisterEventHandle( NULL, ACQ_EVENT_ALARM );
	}

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		pDevice->ClearInterrupts();
		pDevice->m_pCardState->u32ProcId = 0;
		pDevice->UpdateCardState();
		pDevice->UnInitializeDevIoCtl();

		pDevice->m_UseIntrBusy = FALSE;
		pDevice->m_UseIntrTrigger = FALSE;

		// Close asynchronous data transfer event
		if ( pDevice->m_DataTransfer.hAsynEvent )
		{
			CloseHandle( pDevice->m_DataTransfer.hAsynEvent );
			pDevice->m_DataTransfer.hAsynEvent = 0;
		}

		pDevice = pDevice->m_Next;
	}

	if ( 0 == m_u32SystemInitFlag )
	{
#ifdef __linux__ // if linux only close handles if we not recovering from a crash
			     // otherwise we'll crash on SetEvent or CloseHandle
		if ( !m_pSystem->bCrashRecovery )
		{
			// Close acquisition events and thread handles
			if ( m_pSystem->hAcqSystemCleanup )
			{
				// Terminate the thread waiting for events
				::SetEvent(m_pSystem->hAcqSystemCleanup);
				CloseHandle(m_pSystem->hAcqSystemCleanup);
			}

			if (m_pSystem->hEventTriggered)
				CloseHandle(m_pSystem->hEventTriggered);
			if (m_pSystem->hEventEndOfBusy)
				CloseHandle(m_pSystem->hEventEndOfBusy);
			m_pSystem->bCrashRecovery = FALSE; // if it was true, set to false			
		}
#else				
		// Terminate the thread waiting for events
		::SetEvent(m_pSystem->hAcqSystemCleanup);

		// Close acquisition events
		CloseHandle(m_pSystem->hAcqSystemCleanup);
		CloseHandle(m_pSystem->hEventTriggered);
		CloseHandle(m_pSystem->hEventEndOfBusy);
#endif // __linux__

		m_pSystem->hAcqSystemCleanup	=
		m_pSystem->hEventTriggered		=
		m_pSystem->hEventEndOfBusy		= 0;
		m_UseIntrBusy = m_UseIntrTrigger = 0;
	}

	// Reset system to default state
	m_pSystem->bInitialized = FALSE;
	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;

}

////////////////////////////////////////////////////////////////////////
//

uInt32	CsDeereDevice::CsGetAcquisitionStatus()
{

	switch ( m_pSystem->u16AcqStatus )//----- start from the previous known state
	{
		case ACQ_STATUS_READY: //----- no busy, no trigger => wait for busy
			m_pSystem->u16AcqStatus = ACQ_STATUS_READY;//----- no change
			break;

		case ACQ_STATUS_WAIT_TRIGGER://----- busy

			if ( m_pTriggerMaster->ReadTriggerStatus() & TRIGGERED )
			{
				if ( ! m_pTriggerMaster->ReadBusyStatus() )
				{
					GetAlarmStatus();
					m_pSystem->u16AcqStatus = ACQ_STATUS_READY;//----- not busy anymore => wait for new cycle,
				}
				else
					m_pSystem->u16AcqStatus = ACQ_STATUS_TRIGGERED;	//----- busy & trigger => capture
			}
			else
				m_pSystem->u16AcqStatus = ACQ_STATUS_WAIT_TRIGGER;

			break;

		case ACQ_STATUS_TRIGGERED:
			if ( ! m_pTriggerMaster->ReadBusyStatus() )	//----- not busy anymore
			{
				GetAlarmStatus();
				m_pSystem->u16AcqStatus = ACQ_STATUS_READY;
			}
			else
				m_pSystem->u16AcqStatus = ACQ_STATUS_TRIGGERED;

			break;
		case ACQ_STATUS_BUSY_TX:
			break;
	}

	return m_pSystem->u16AcqStatus;
}


////////////////////////////////////////////////////////////////////////
//


int32	CsDeereDevice::CsGetParams( uInt32 u32ParamId, PVOID pOutBuffer, uInt32 u32OutSize )
{
	PVOID			pUserBuffer = pOutBuffer;
	int32			i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(u32OutSize);

	switch ( u32ParamId )
	{

	case CS_ACQUISITION:
		{
			i32Status = CsGetAcquisitionConfig( (PCSACQUISITIONCONFIG) pUserBuffer );
		}
		break;

	case CS_TRIGGER:
		{
			i32Status = CsGetTriggerConfig( (PARRAY_TRIGGERCONFIG) pUserBuffer );
		}
		break;

	case CS_CHANNEL:
		{
			i32Status = CsGetChannelConfig( (PARRAY_CHANNELCONFIG) pUserBuffer );
		}
		break;

	case CS_READ_FLASH:
	case CS_READ_EEPROM:
		{
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) pUserBuffer;
			i32Status = IoctlReadFlash(pGenericRw->u32Offset, pGenericRw->pBuffer, pGenericRw->u32BufferSize);
		}
		break;

	case CS_READ_NVRAM: //---- Read NvRam (Boot Eeprom)
		{
			i32Status = CsReadPlxNvRAM( (PPLX_NVRAM_IMAGE) pUserBuffer );
		}
		break;

	case CS_READ_VER_INFO:
		{
			*((PCS_FW_VER_INFO) pUserBuffer) = m_pCardState->VerInfo;
		}
		break;

	case CS_EXTENDED_BOARD_OPTIONS:
		{
			uInt32 *pExOptions = (uInt32 *) pUserBuffer;

			pExOptions[0] = m_pCsBoard->u32BaseBoardOptions[1];
			pExOptions[1] = m_pCsBoard->u32BaseBoardOptions[2];
		}
		break;

	case CS_SYSTEMINFO_EX:
		{
			PCSSYSTEMINFOEX pSysInfoEx = (PCSSYSTEMINFOEX) pUserBuffer;

			uInt32	i;

			RtlZeroMemory( pSysInfoEx, sizeof(CSSYSTEMINFOEX) );
			pSysInfoEx->u32Size = sizeof(CSSYSTEMINFOEX);
			pSysInfoEx->u16DeviceId = GetDeviceId();;

			CsDeereDevice *pDevice = m_MasterIndependent;

			// Permission and firmware versions from the master card
            pSysInfoEx->i64ExpertPermissions = pDevice->m_pCsBoard->i64ExpertPermissions;
			pSysInfoEx->u32BaseBoardHwOptions = pDevice->m_pCsBoard->u32BaseBoardHardwareOptions;
			pSysInfoEx->u32AddonHwOptions = pDevice->m_pCsBoard->u32AddonHardwareOptions;

			for ( i = 0; i < sizeof(m_pCsBoard->u32RawFwVersionB) /sizeof(uInt32) ; i ++)
			{
				pSysInfoEx->u32BaseBoardFwOptions[i]	= m_pCsBoard->u32BaseBoardOptions[i];
				pSysInfoEx->u32AddonFwOptions[i]		= m_pCsBoard->u32AddonOptions[i];

				pSysInfoEx->u32BaseBoardFwVersions[i].u32Reg	= m_pCsBoard->u32UserFwVersionB[i].u32Reg;
				pSysInfoEx->u32AddonFwVersions[i].u32Reg		= 0;
			}

			pDevice = pDevice->m_Next;

			// System permission will be "AND" persmissions of all Master/Slave cards
			while ( pDevice )
			{
				// i starts from 1 because the image 0 is not an expertt option
				for ( i = 1; i < sizeof(m_pCsBoard->u32RawFwVersionB) /sizeof(uInt32) ; i ++)
				{
					pSysInfoEx->u32BaseBoardFwOptions[i] &= pDevice->m_pCsBoard->u32BaseBoardOptions[i];
					pSysInfoEx->u32AddonFwOptions[i] &= pDevice->m_pCsBoard->u32AddonOptions[i];

					if ( 0 == pSysInfoEx->u32BaseBoardFwOptions[i] )
						pSysInfoEx->u32BaseBoardFwVersions[i].u32Reg = 0;

					if ( 0 == pSysInfoEx->u32AddonFwOptions[i] )
						pSysInfoEx->u32AddonFwVersions[i].u32Reg = 0;
				}

				pSysInfoEx->i64ExpertPermissions &= pDevice->m_pCsBoard->i64ExpertPermissions;
				pSysInfoEx->u32AddonHwOptions &= pDevice->m_pCsBoard->u32AddonHardwareOptions;
				pSysInfoEx->u32BaseBoardHwOptions &= pDevice->m_pCsBoard->u32BaseBoardHardwareOptions;
				pDevice = pDevice->m_Next;
			}
		}
		break;

	case CS_TIMESTAMP_TICKFREQUENCY:
		{
			PLARGE_INTEGER pTickCount = (PLARGE_INTEGER) pUserBuffer;

			if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
			{
				pTickCount->QuadPart = DEERE_MEMORY_CLOCK;
				pTickCount->QuadPart <<= 4;
			}
			else
				pTickCount->QuadPart = m_pCardState->AcqConfig.i64SampleRate;
		}
		break;


	case CS_DRV_BOARD_CAPS:
		{
			PCSDEEREBOARDCAPS pCapsInfo = (PCSDEEREBOARDCAPS) pUserBuffer;
			FillOutBoardCaps(pCapsInfo);
		}
		break;

	case CS_TRIG_OUT:
		{
			PCS_TRIG_OUT_STRUCT pTrigOut = (PCS_TRIG_OUT_STRUCT) pUserBuffer;

			pTrigOut->u16Valid = CS_OUT_NONE | CS_TRIGOUT;
			pTrigOut->u16ValidMode = CS_TRIGOUT_MODE;
			pTrigOut->u16Mode = CS_TRIGOUT_MODE;
			pTrigOut->u16Value = m_pCardState->bDisableTrigOut ? CS_OUT_NONE : CS_TRIGOUT;
		}
		break;

	case CS_CLOCK_OUT:
		{
			PCS_CLOCK_OUT_STRUCT pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;

			pClockOut->u16Value = (uInt16) GetClockOut();
			pClockOut->u16Valid = CS_OUT_NONE|CS_CLKOUT_SAMPLE|CS_CLKOUT_REF;
		}
		break;

	case CS_TRIG_ENABLE:
		i32Status = CS_FUNCTION_NOT_SUPPORTED;
		break;


	case CS_MULREC_AVG_COUNT:
		*( (uInt32*) pUserBuffer ) = m_u32MulrecAvgCount; 
		break;

	case CS_AUX_IN:
		{
			if ( 0 == (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_AUX_IO) )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PCS_AUX_IN_STRUCT pAuxIn = (PCS_AUX_IN_STRUCT) pUserBuffer;
				pAuxIn->u16ExtTrigEnVal  = m_pCardState->u16ExtTrigEnCfg;
				pAuxIn->u16ModeVal		 = (uInt16) m_pCardState->AuxIn;
			}
		}
		break;

	case CS_AUX_OUT:
		{
			if ( 0 == (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_AUX_IO) )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PCS_AUX_OUT_STRUCT pAuxOut = (PCS_AUX_OUT_STRUCT) pUserBuffer;
				pAuxOut->u16ModeVal		   = (uInt16) m_pCardState->AuxOut;
			}
		}
		break;

	case CS_READ_CALIB_A2D:
		{
			PCS_READCALIBA2D_STRUCT pReadCalibA2D = (PCS_READCALIBA2D_STRUCT) pUserBuffer;
			CsDeereDevice* pDevice = GetCardPointerFromBoardIndex( pReadCalibA2D->u8CardNumber );

			if ( NULL != pDevice )
				i32Status = pDevice->ReadCalibA2D( pReadCalibA2D->i32ExpectedVoltage, 
									   (eCalInputJD)pReadCalibA2D->u8Input,
									   &(pReadCalibA2D->i32ReadVoltage), 
									   &(pReadCalibA2D->i32NoiseLevel) );	
		}
		break;

	default:
		i32Status = CS_INVALID_PARAMS_ID;
		break;

	}

	return i32Status;

}


////////////////////////////////////////////////////////////////////////
//

int32	CsDeereDevice::CsSetParams( uInt32 u32ParamId, PVOID pInBuffer,  uInt32 u32InSize , uInt32 u32CommitFlag )
{
	PVOID			pUserBuffer = pInBuffer;
	int32			i32Status = CS_SUCCESS;
	CsDeereDevice		*pDevice = this;

	UNREFERENCED_PARAMETER(u32InSize);
	UNREFERENCED_PARAMETER(u32CommitFlag);

	switch ( u32ParamId )
	{

	case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG			pSystemCfg = NULL;
			PCSACQUISITIONCONFIG	pAcqCfg;
			PARRAY_CHANNELCONFIG	pAChanCfg;
			PARRAY_TRIGGERCONFIG	pATrigCfg;


			pSystemCfg = (PCSSYSTEMCONFIG) pUserBuffer;

			pAcqCfg		= pSystemCfg->pAcqCfg;
			pAChanCfg	= pSystemCfg->pAChannels;
			pATrigCfg	= pSystemCfg->pATriggers;

			// Disable TrigOut and BusyOut before calibration
			ConfiureTriggerBusyOut();

			i32Status = CommitNewConfiguration( pAcqCfg, pAChanCfg, pATrigCfg );
			if ( CS_SUCCEEDED(i32Status) )
			{
				// Detect if we have to call MsMatchAdcPhaseAllChannels
				bool	bCalibNeed = false;

				CsDeereDevice *pDevice = m_MasterIndependent;
				while( pDevice )
				{
					bCalibNeed = (isAdcPingPong() && !m_bNeverMatchAdcPhase);
					bCalibNeed &= (pDevice->m_bCoreMatchNeeded[0] || pDevice->m_bCoreMatchNeeded[1]);
					bCalibNeed |= pDevice->m_bAdcMatchNeeded[0] || pDevice->m_bAdcMatchNeeded[1];

					if ( bCalibNeed )
						break;
								
					pDevice = pDevice->m_Next;
				}

				if ( bCalibNeed )
				{
					i32Status = MsMatchAdcPhaseAllChannels();
					if ( CS_SUCCEEDED(i32Status) )
						i32Status = CommitNewConfiguration( pAcqCfg, pAChanCfg, pATrigCfg );
				}
			}

			// Restore M/S digital delay
			MsAdjustDigitalDelay();

			// Restore TrigOut and BusyOut
			ConfiureTriggerBusyOut( !m_pCardState->bDisableTrigOut, eAuxOutBusyOut == m_pCardState->AuxOut );
		}
		break;


	case CS_WRITE_FLASH:
	case CS_WRITE_EEPROM:
		{
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

			if ( pGenericRw->u32BufferSize !=0 && pGenericRw->pBuffer != 0 )
			{
				if ( u32Offset + u32BufferSize <= DEERE_FLASH_SIZE )
				{
					CsDeereFlash  FlashObj( GetPlxBaseObj() );

					if ( u32Offset == FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage1 ) || 
						 u32Offset == FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage2 ) ||
						 u32Offset == FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage3 ) )
						i32Status = FlashObj.FlashWriteEx(u32Offset, pAppBuffer, u32BufferSize, false );
					else
						i32Status = FlashObj.FlashWriteEx(u32Offset, pAppBuffer, u32BufferSize, true );
				}
				else
					i32Status = CS_INVALID_POINTER_BUFFER;
			}
			else
			{
				// The Hardware info from eeeprom has been updated. Reload the Hardware Info
				ResetCachedState();
				ReadHardwareInfoFromEeprom();

				ReadVersionInfo(2);
				ReadVersionInfo(1);
				ReadVersionInfo(0);
			}
		}
		break;


	case CS_WRITE_NVRAM: //---- Write NvRam (Boot Eeprom)
		{
			i32Status = CsWritePlxNvRam( (PPLX_NVRAM_IMAGE) pUserBuffer );
		}
		break;


	case CS_SELF_TEST_MODE:
		{
			PCS_SELF_TEST_STRUCT	pSelf = (PCS_SELF_TEST_STRUCT) pUserBuffer;
			i32Status = WriteToSelfTestRegister( pSelf->u16Value );
		}
		break;

	case CS_SEND_DAC: //---- Send Write Command to DAC
		{
			PCS_SENDDAC_STRUCT	pSendDAC = (PCS_SENDDAC_STRUCT) pUserBuffer;
			pDevice = GetCardPointerFromBoardIndex( pSendDAC->u8CardNumber );

			if ( NULL != pDevice ) 
			{
				if ( DEERE_ADC_BASE_0 + DEERE_ADC_COARSE_OFFSET <= pSendDAC->u8DacAddress &&  DEERE_ADC_BASE_0 + DEERE_ADC_SKEW >= pSendDAC->u8DacAddress )
					i32Status = pDevice->WriteToAdcReg(pSendDAC->u8DacId, 0, pSendDAC->u8DacAddress - DEERE_ADC_BASE_0, (uInt8) pSendDAC->u16DacValue );
				else if ( DEERE_ADC_BASE_1 + DEERE_ADC_COARSE_OFFSET <= pSendDAC->u8DacAddress &&  DEERE_ADC_BASE_1 + DEERE_ADC_SKEW >= pSendDAC->u8DacAddress )
					i32Status = pDevice->WriteToAdcReg(pSendDAC->u8DacId, 1, pSendDAC->u8DacAddress - DEERE_ADC_BASE_1, (uInt8) pSendDAC->u16DacValue );
				else if ( DEERE_ADC_BASE_2 + DEERE_ADC_COARSE_OFFSET <= pSendDAC->u8DacAddress &&  DEERE_ADC_BASE_2 + DEERE_ADC_SKEW >= pSendDAC->u8DacAddress )
					i32Status = pDevice->WriteToAdcReg(pSendDAC->u8DacId, 2, pSendDAC->u8DacAddress - DEERE_ADC_BASE_2, (uInt8) pSendDAC->u16DacValue );
				else if ( DEERE_ADC_BASE_3 + DEERE_ADC_COARSE_OFFSET <= pSendDAC->u8DacAddress &&  DEERE_ADC_BASE_3 + DEERE_ADC_SKEW >= pSendDAC->u8DacAddress )
					i32Status = pDevice->WriteToAdcReg(pSendDAC->u8DacId, 3, pSendDAC->u8DacAddress - DEERE_ADC_BASE_3, (uInt8) pSendDAC->u16DacValue );
				else
					i32Status = pDevice->WriteToDac( pSendDAC->u8DacAddress, pSendDAC->u16DacValue);
			}
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_TRIG_OUT:
		{
			PCS_TRIG_OUT_STRUCT	pTrigOutCfg = (PCS_TRIG_OUT_STRUCT) pUserBuffer;

			m_pCardState->bDisableTrigOut = pTrigOutCfg->u16Value == CS_OUT_NONE;
			ConfiureTriggerBusyOut( !m_pCardState->bDisableTrigOut, eAuxOutBusyOut == m_pCardState->AuxOut );
		}
		break;

	case CS_CLOCK_OUT:
		{
			PCS_CLOCK_OUT_STRUCT	pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;
			if ( GetClockOut() != (eClkOutMode)pClockOut->u16Value )
				i32Status = SetClockOut( (eClkOutMode)pClockOut->u16Value, true );
		}
		break;

	case CS_TRIG_ENABLE:
		i32Status = CS_FUNCTION_NOT_SUPPORTED;
		break;


#ifdef	DEBUG_MASTERSLAVE
	case CS_DEBUG_MS_PULSE:
	{
			CsDeereDevice* pCard = m_MasterIndependent;
			while( pCard )
			{
				pCard->m_u16DebugMsPulse += 1;
				pCard->m_u16DebugMsPulse = pCard->m_u16DebugMsPulse % 3;
				pCard = pCard->m_Next;
			}
			
			if ( 2 == m_u16DebugMsPulse )
			{
				pCard = m_MasterIndependent;
				
				while ( pCard )
				{
					// Switch to Ms calib mode
					pCard->SendCalibModeSetting( CS_CHAN_1, ecmCalModeMs );
					pCard = pCard->m_Next;
				}
			}
			else if ( 0 == m_u16DebugMsPulse )
			{
				pCard = m_MasterIndependent;
				
				while ( pCard )
				{
					// Switch to Mormal calib mode
					pCard->SendCalibModeSetting( CS_CHAN_1 );
					uInt32 u32Index;
					FindFeIndex( m_pCardState->ChannelParams[0].u32InputRange, &u32Index );
					WriteToDac(DEERE_USER_OFFSET_1, m_pCardState->DacInfo[0][u32Index]._UserOffset, 0);
					pCard = pCard->m_Next;
				}
			}
		}
		break;

	case CS_DEBUG_MS_OFFSET:
		{
			CsDeereDevice* pCard = m_MasterIndependent;
			while( pCard )
			{
				pCard->m_bMsOffsetAdjust = ! pCard->m_bMsOffsetAdjust;
				pCard = pCard->m_Next;
			}
			MsAdjustDigitalDelay();
		}
		break;
#endif


#ifdef	DEBUG_EXTTRIGCALIB
	case CS_DEBUG_EXTTRIGCALIB:
		{
			m_u16DebugExtTrigCalib += 1;
			m_u16DebugExtTrigCalib = m_u16DebugExtTrigCalib % 3;

			if ( 2 == m_u16DebugExtTrigCalib )
			{
				// Switch to Ext calib mode
//				m_pTriggerMaster->SendCalibModeSetting( CS_CHAN_1, ecmCalModeExtTrig );
				GageCopyMemoryX( m_OldTrigParams, m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams));

				if ( m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Source )
				{
					m_pTriggerMaster->SetupForExtTrigCalib_SourceExtTrig();
				}
				else
				{
					m_pTriggerMaster->SetupForExtTrigCalib_SourceChan1();
				}
			}
			else if ( 0 == m_u16DebugExtTrigCalib )
			{
				// Switch to Normal mode
				m_pTriggerMaster->SendCalibModeSetting( CS_CHAN_1 );
				uInt8 u8IndexTrig = INDEX_TRIGENGINE_A1;
				GageCopyMemoryX( m_CfgTrigParams, m_OldTrigParams, sizeof(m_CfgTrigParams));

				m_pTriggerMaster->SendExtTriggerSetting( m_CfgTrigParams[INDEX_TRIGENGINE_EXT].i32Source != 0 ? true : false,
									   m_CfgTrigParams[INDEX_TRIGENGINE_EXT].i32Level,
									   m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32Condition,
									   m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange,
									   m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance,
									   m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtCoupling );

				m_pTriggerMaster->SendTriggerEngineSetting( m_CfgTrigParams[u8IndexTrig].i32Source,
															   m_CfgTrigParams[u8IndexTrig].u32Condition,
															   m_CfgTrigParams[u8IndexTrig].i32Level,
															   m_CfgTrigParams[u8IndexTrig+1].i32Source,
															   m_CfgTrigParams[u8IndexTrig+1].u32Condition,
															   m_CfgTrigParams[u8IndexTrig+1].i32Level,
															   m_CfgTrigParams[u8IndexTrig+2].i32Source,
															   m_CfgTrigParams[u8IndexTrig+2].u32Condition,
															   m_CfgTrigParams[u8IndexTrig+2].i32Level,
															   m_CfgTrigParams[u8IndexTrig+3].i32Source,
															   m_CfgTrigParams[u8IndexTrig+3].u32Condition,
															   m_CfgTrigParams[u8IndexTrig+3].i32Level );
				m_pTriggerMaster->SendDigitalTriggerSensitivity( INDEX_TRIGENGINE_A1, m_u8TrigSensitivity );
			}
		}
		break;
#endif

		case CS_SEND_NIOS_CMD:
		{
			PCS_SEND_NIOS_CMD_STRUCT NiosCmd = (PCS_SEND_NIOS_CMD_STRUCT) pUserBuffer;

			while( pDevice )
			{
				if ( (0 == NiosCmd->u16CardIndex) || ( NiosCmd->u16CardIndex == pDevice->m_pCardState->u16CardNumber ) )
				{
					if ( NiosCmd->bRead )
						i32Status = pDevice->WriteRegisterThruNios( NiosCmd->u32DataIn, NiosCmd->u32Command, NiosCmd->u32Timeout, &NiosCmd->u32DataOut );
					else
						i32Status = pDevice->WriteRegisterThruNios( NiosCmd->u32DataIn, NiosCmd->u32Command, NiosCmd->u32Timeout );
				}

				pDevice = pDevice->m_Next;
			}
		}
		break;

		case CS_SEND_CALIB_MODE:
		{
			PCS_CALIBMODE_STRUCT pCalibMode = (PCS_CALIBMODE_STRUCT) pUserBuffer;

			if ( NULL != pDevice )
			{
				i32Status = pDevice->SendCalibModeSetting( pCalibMode->u16Channel, (eCalMode)pCalibMode->u8CalMode);
			}
		}
		break;

		case CS_SEND_CALIB_DC:
		{
			PCS_CALIBDC_STRUCT pCalibDC = (PCS_CALIBDC_STRUCT) pUserBuffer;

			if ( NULL != pDevice )
			{
				int32 i32SetLevel_uV;
				i32Status = pDevice->SendCalibDC( pCalibDC->u32Impedance, 
									  0==pCalibDC->i32Level?0:pCalibDC->u32Range, 
									  pCalibDC->i32Level > 0, 
									  &i32SetLevel_uV );
				i32SetLevel_uV *= int32(CS_REAL_IMP_50_OHM + pCalibDC->u32Impedance); 
				i32SetLevel_uV /= CS_REAL_IMP_50_OHM;
				pCalibDC->i32SetLevel = i32SetLevel_uV;
			}
		}
		break;

		case CS_CALIBMODE_CONFIG:
		{
			PCS_CALIBMODE_PARAMS pStruct = (PCS_CALIBMODE_PARAMS) pUserBuffer;
			pDevice = GetCardPointerFromChannelIndex( pStruct->u16ChannelIndex );
			uInt16 u16ChIx = CS_CHAN_1;
			if ( 2 == m_PlxData->CsBoard.NbAnalogChannel )
				u16ChIx = 0 != (pStruct->u16ChannelIndex % 2) ? CS_CHAN_1: CS_CHAN_2;
			if ( NULL != pDevice )
				i32Status = pDevice->SendCalibModeSetting( u16ChIx, pStruct->CalibMode );
		}
		break;

		case CS_MULREC_AVG_COUNT:
		{
			// This setting should be done before ACTION_COMMIT
			// because m_u32MulrecAvgCount will be used in SendSegmentSetting() for Expert AVG
			uInt32	u32MulrecAvgCount = *( (uInt32*) pUserBuffer ); 
			if ( u32MulrecAvgCount < 1 || u32MulrecAvgCount > 1024  )
				i32Status = CS_INVALID_NUM_OF_AVERAGE;
			else
			{
				while ( pDevice )
				{
					pDevice->m_u32MulrecAvgCount = u32MulrecAvgCount;
					pDevice = pDevice->m_Next;
				}
			}
		}
		break;

	case CS_AUX_IN:
		{
			if ( 0 == (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_AUX_IO) )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PCS_AUX_IN_STRUCT pAuxIn = (PCS_AUX_IN_STRUCT) pUserBuffer;
				
				m_pCardState->u16ExtTrigEnCfg = pAuxIn->u16ExtTrigEnVal;
				m_pCardState->AuxIn			= (eAuxIn) pAuxIn->u16ModeVal;
			}
		}
		break;

	case CS_AUX_OUT:
		{
			if ( 0 == (m_pCsBoard->u32AddonHardwareOptions & DEERE_OPT_AUX_IO) )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PCS_AUX_OUT_STRUCT pAuxOut = (PCS_AUX_OUT_STRUCT) pUserBuffer;
				m_pCardState->AuxOut  = (eAuxOut) pAuxOut->u16ModeVal;
				i32Status = ConfiureTriggerBusyOut( !m_pCardState->bDisableTrigOut, eAuxOutBusyOut == m_pCardState->AuxOut );
			}
		}
		break;


	case CS_SAVE_OUT_CFG:
		{
			DEERE_FLASHFOOTER	DeereFooter;
			CsDeereFlash		FlashObj( GetPlxBaseObj() );

			uInt32 u32Offset = FIELD_OFFSET(DEERE_FLASH_LAYOUT, Footer) + FIELD_OFFSET(DEERE_FLASHFOOTER, Footer);
			i32Status = IoctlReadFlash( u32Offset, &DeereFooter, sizeof(DeereFooter) );
			if ( CS_FAILED(i32Status) )	break;

			if ( 0 != *((uInt32 *) pUserBuffer)  )
			{
				// Save out config
				DeereFooter.u16TrigOutEnabled		 = m_pCardState->bDisableTrigOut ? 0 : 1;
				DeereFooter.u16ClkOut				 = (uInt16) m_pCardState->eclkoClockOut;
				DeereFooter.AuxInCfg.u16ExtTrigEnVal = (uInt16) m_pCardState->u16ExtTrigEnCfg;
				DeereFooter.AuxInCfg.u16ModeVal		 = (uInt16) m_pCardState->AuxIn;
				DeereFooter.AuxOutCfg				 = (uInt16) m_pCardState->AuxOut;
			}
			else
			{
				// Clear out config
				DeereFooter.u16TrigOutEnabled	=
				DeereFooter.u16ClkOut			= 
				DeereFooter.AuxInCfg.u16ExtTrigEnVal =
				DeereFooter.AuxInCfg.u16ModeVal	=
				DeereFooter.AuxOutCfg			= 0xFFFF;

				SetDefaultAuxIoCfg();
			}
			i32Status = FlashObj.FlashWriteEx(u32Offset, &DeereFooter, sizeof(DeereFooter) );
		}
		break;

	case CS_TIMESTAMP_RESET:
		ResetTimeStamp();
		break;

	default:
		i32Status = CS_INVALID_PARAMS_ID;
		break;

	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsDeereDevice::CsStartAcquisition()
{

	m_pSystem->bEndAcquisitionNotification = FALSE;
	m_pSystem->u32TriggerNotification =  0;
	m_pSystem->u32TriggeredCount = 0;

	m_pSystem->u16AcqStatus  = ACQ_STATUS_WAIT_TRIGGER;

	// Reset notification flag before acquisition
	m_pTriggerMaster->m_pCsBoard->bBusy = true;
	m_pTriggerMaster->m_pCsBoard->bTriggered = false;

	uInt32	u32IntBaseMask = MAOINT;		// Alarm enabled by default
	
	if ( m_bMulrecAvgTD )
		u32IntBaseMask |= MULREC_AVG_DONE;
	else if ( m_UseIntrBusy )
		u32IntBaseMask |= N_MBUSY_REC_DONE;

	if ( m_UseIntrTrigger )
		u32IntBaseMask |= MTRIG;

	AcquireData( u32IntBaseMask );

	return CS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
//

int32	CsDeereDevice::CsAbort()
{
	switch( m_pSystem->u16AcqStatus )
	{
		case ACQ_STATUS_WAIT_TRIGGER:
		case ACQ_STATUS_TRIGGERED:
			{
				// Arbort action will clear the acquisition status, thus there is no interrupt generated.
				// We have to keep track where we were before ABORT so that we can simulate triggerred and
				// end of acquisition interrutps.
				m_pCsBoard->bTriggered = true;
				m_pCsBoard->bBusy = false;

				MsAbort();
				m_pTriggerMaster->SignalEvents();
			}
			break;

		default:
		{
			if ( m_pSystem->BusyTransferCount > 0 )
			{
				// Abort all pending asynchronous data transfers
				CsDeereDevice *pDevice = this;
				while ( pDevice )
				{
					if ( pDevice->m_DataTransfer.BusyTransferCount )
					{
						pDevice->AbortCardReadMemory();
						pDevice->m_DataTransfer.BusyTransferCount = 0;
					}
					pDevice = pDevice->m_Next;
				}
				m_pSystem->BusyTransferCount = 0;
			}
		}
		break;
	}

	// Clear the interrupt register
	CsDeereDevice* pCard = m_MasterIndependent;

	while ( pCard )
	{
		pCard->ClearInterrupts();
		pCard = pCard->m_Next;
	}

	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;

	return CS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
//

void	CsDeereDevice::CsForceTrigger()
{

	m_pTriggerMaster->ForceTrigger();

}

////////////////////////////////////////////////////////////////////////
//

int32	CsDeereDevice::CsDataTransfer( in_DATA_TRANSFER *InDataXfer )
{
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	if ( pInParams->u32Mode & TxMODE_TIMESTAMP )
	{
		// Convert the size of user Buffer from sizeof(char) to sizeof(int64)
		pInParams->i64Length = pInParams->i64Length / sizeof(int64);

		if ( pInParams->i64Length + pInParams->u32Segment - 1 > m_pCardState->AcqConfig.u32SegmentCount )
			pInParams->i64Length = m_pCardState->AcqConfig.u32SegmentCount - pInParams->u32Segment + 1;

		int32 i32Status = m_pTriggerMaster->ReadTriggerTimeStampEtb( pInParams->u32Segment - 1, (int64 *)pInParams->pDataBuffer, (uInt32) pInParams->i64Length );
		if ( CS_SUCCEEDED(i32Status) )
		{
			pOutParams->i64ActualStart = pInParams->u32Segment;
			pOutParams->i64ActualLength = pInParams->i64Length;
			pOutParams->i32HighPart = 0;

			if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
				pOutParams->i32LowPart = DEERE_MEMORY_CLOCK << 4;
			else
				pOutParams->i32LowPart = (uInt32) m_pCardState->AcqConfig.i64SampleRate;
		}
		return i32Status;
	}
	else
	{
		uInt32 u32Mode = pInParams->u32Mode & ~TxMODE_SLAVE;

		if ( u32Mode == TxMODE_DATA_ANALOGONLY ||
			 u32Mode == TxMODE_DATA_16         ||
			 u32Mode == TxMODE_DATA_32         ||
			 u32Mode == TxMODE_DATA_ONLYDIGITAL )
		{
			//---- Find the card that we need to setup for transfer
			CsDeereDevice* pDevice = GetCardPointerFromChannelIndex( pInParams->u16Channel );

			if ( pDevice )
			{
				// Check resources required for asynchronous transfer
				if ( 0 != pInParams->hNotifyEvent && ( 0 == pDevice->m_DataTransfer.hAsynEvent || 0 == m_DataTransfer.hAsynThread ) )
					return CS_INSUFFICIENT_RESOURCES;

				if ( InterlockedCompareExchange(&pDevice->m_DataTransfer.BusyTransferCount, 1, 0 ) )
				{
					// A previous asynchronous data transfer has not been completed yet
					// Return error. Cannot process this request now.
					return  CS_DRIVER_ASYNC_REQUEST_BUSY;
				}
				else
				{
					if ( pInParams->u32Mode & TxMODE_SLAVE  || ! pDevice->m_BusMasterSupport )
					{
						// Set flag for slave transfer
						pDevice->m_DataTransfer.bUseSlaveTransfer = TRUE;
					}
					else
					{
						// Clear flag for slave transfer
						pDevice->m_DataTransfer.bUseSlaveTransfer = FALSE;
					}

					if ( m_bSoftwareAveraging )
						return pDevice->ProcessDataTransferModeAveraging(InDataXfer);
					else
						return pDevice->ProcessDataTransfer(InDataXfer);
				}
			}
			else
				return CS_INVALID_CHANNEL;
		}
		else
		{
			return CS_INVALID_TRANSFER_MODE;
		}
	}
}


////////////////////////////////////////////////////////////////////////
//

int32	CsDeereDevice::CsDataTransferEx( io_DATA_TRANSFER_EX *InDataXfer )
{
	PIN_PARAMS_TRANSFERDATA_EX		pInParams;
	POUT_PARAMS_TRANSFERDATA_EX		pOutParams;


	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	uInt32 u32Mode = pInParams->u32Mode & ~TxMODE_SLAVE;

	if ( u32Mode == TxMODE_DATA_ANALOGONLY || 
		 u32Mode == TxMODE_DATA_16         || 
		 u32Mode == TxMODE_DATA_32         || 
		 u32Mode == TxMODE_DATA_ONLYDIGITAL ||
		 u32Mode == TxMODE_SEGMENT_TAIL ||
		 u32Mode == TxMODE_DATA_INTERLEAVED )
	{

		if ( ( (TxMODE_DATA_32 == u32Mode) && (!m_bHardwareAveraging && !m_bSoftwareAveraging) ) ||
		     ( (TxMODE_DATA_32 != u32Mode) && (m_bHardwareAveraging || m_bSoftwareAveraging) ) )
			return CS_INVALID_TRANSFER_MODE;

		//---- Find the card that we need to setup for transfer
		CsDeereDevice* pDevice = GetCardPointerFromChannelIndex( pInParams->u16Channel );

		if ( pDevice )
		{
			// Check resources required for asynchronous transfer
			if ( 0 != pInParams->hNotifyEvent && ( 0 == pDevice->m_DataTransfer.hAsynEvent || 0 == m_DataTransfer.hAsynThread ) )
				return CS_INSUFFICIENT_RESOURCES;

			if ( InterlockedCompareExchange(&pDevice->m_DataTransfer.BusyTransferCount, 1, 0 ) )
			{
				// A previous asynchronous data transfer has not been completed yet
				// Return error. Cannot process this request now.
				return  CS_DRIVER_ASYNC_REQUEST_BUSY;
			}
			else
			{
					// Set flag for slave transfer
				pDevice->m_DataTransfer.bUseSlaveTransfer = ( pInParams->u32Mode & TxMODE_SLAVE  || ! pDevice->m_BusMasterSupport );

				return pDevice->ProcessDataTransferEx(InDataXfer);
			}
		}
		else
			return CS_INVALID_CHANNEL;
	}
	else
	{
		return CS_INVALID_TRANSFER_MODE;
	}
}





////////////////////////////////////////////////////////////////////////
//

int32 CsDeereDevice::CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt )
{
	EVENT_HANDLE	hUserEventHandle = InRegEvnt.hUserEventHandle;
	int32			i32Status = CS_SUCCESS;

	switch ( InRegEvnt.u32EventType )
	{
	case ACQ_EVENT_TRIGGERED:
		m_pSystem->hUserEventTriggered = hUserEventHandle;
		break;

	case ACQ_EVENT_END_BUSY:
		m_pSystem->hUserEventEndOfBusy = hUserEventHandle;
		break;
	}

	return i32Status;
}

////////////////////////////////////////////////////////////////////////
//

void	CsDeereDevice::CsSystemReset(BOOL bHard_reset)
{
	UNREFERENCED_PARAMETER(bHard_reset);
	CsAbort();

	CsDeereDevice *pCard = m_MasterIndependent;
	pCard = m_MasterIndependent;
	while (pCard)
	{
		if ( pCard->m_pCardState->u8ImageId != 0 )
		{
			pCard->CsLoadFpgaImage(0);
		}
		else
		{
			// Reset base board
			pCard->BaseBoardConfigReset(0);
		}

		pCard = pCard->m_Next;
	}

	pCard = m_MasterIndependent;
	while (pCard)
	{
		// Reset Addon board
		pCard->AddonReset();
		pCard->InitRegisters(true);

		pCard->ResetCachedState();
		pCard = pCard->m_Next;
	}

	// Waiting for PLL locked
	Sleep(1000);	// 1s

	MsCsResetDefaultParams( TRUE );

}

////////////////////////////////////////////////////////////////////////
//

int32	CsDeereDevice::CsForceCalibration()
{
	int32			i32Status = CS_SUCCESS;
	CsDeereDevice	*pDevice = m_MasterIndependent;
	uInt16			u16ChanZeroBased = 0;

	while ( NULL != pDevice )
	{
		for( uInt16 i = 1; i <= m_pCsBoard->NbAnalogChannel; i++ )
		{
			u16ChanZeroBased = i - 1;
			pDevice->m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
			pDevice->m_bForceCal[u16ChanZeroBased] = true;
			pDevice->m_bVerifyDcCalSrc = true;
		}
		pDevice->m_pCardState->bDpaNeeded = true;
		i32Status = pDevice->CalibrateAllChannels();
		if ( CS_FAILED(i32Status) )
			break;

		pDevice = pDevice->m_Next;
	}

	if ( CS_SUCCEEDED(i32Status) )
		i32Status = MsCalibrateAdcAlign();

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsDeereDevice::CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait )
{
	int32 i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(bWait);

	CsDeereDevice *pDevice = GetCardPointerFromChannelIndex( u16Channel );

	if ( pDevice )
	{
		i32Status = pDevice->IoctlGetAsynDataTransferResult( pTxDataResult );
	}
	else
		i32Status = CS_INVALID_CHANNEL;

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//
void * CsDeereDevice::operator new (size_t  size)
{
	PVOID	p = new char[size];

	if ( p )
		memset(p, 0, size);

	return p;
}



void CsDeereDevice::AddonReset()
{
	CsNiosApi::AddonReset();
	m_pCardState->bDualChanAligned = false;
	m_pCardState->bAdcAlignCalibReq = true;
}

#ifdef USE_SHARED_CARDSTATE

int32	CsDeereDevice::InitializeDevIoCtl()
{
	int32 i32Status = CsDevIoCtl::InitializeDevIoCtl();

	if ( CS_SUCCEEDED( i32Status ) )
	{
		i32Status = IoctlGetCardStatePointer( &m_pCardState );
		if ( CS_SUCCEEDED( i32Status ) )
		{
			if ( sizeof(DEERE_CARDSTATE) == m_pCardState->u32Size  )
			{
				m_PlxData	= &m_pCardState->PlxData;
				m_pCsBoard	= &m_PlxData->CsBoard;
				return CS_SUCCESS;
			}
			else
			{
				::OutputDebugString("\nError DEERE_CARDSTATE struct size discrepency !!");
				CsDevIoCtl::UnInitializeDevIoCtl();
				return CS_MISC_ERROR;
			}
		}
		else
			CsDevIoCtl::UnInitializeDevIoCtl();
	}
	return i32Status;
}

#endif
