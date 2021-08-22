// CsSplendaDev.cpp
//

#include "StdAfx.h"
#include "CsSplenda.h"

#ifdef _WINDOWS_
	#include <process.h>			// for _beginthreadex
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsSplendaDev::CsSplendaDev(uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars):
	m_u8CardIndex(u8Index), m_ProcessGlblVars(ProcessGlblVars), m_PseudoHandle(pDevInfo->DrvHdl), m_bNucleon(PCIE_BUS == pDevInfo->BusType),
	m_u32HwTrigTimeout((uInt32)CS_TIMEOUT_DISABLE), m_DeviceId(pDevInfo->DeviceId)
{

	// Memory has been cleared on allocation
	// Only need to initialize member variables whose default value is different than 0
#ifndef _WINDOWS_
	m_bSkipFirmwareCheck = true;
#endif
	m_bNoConfigReset	= true;
	m_BusMasterSupport	= TRUE;

	m_pTriggerMaster  = this;
	m_MasterIndependent = this;

	m_u8CalDacSettleDelay	= c_u8DacSettleDelay;
	m_u32CalSrcRelayDelay	= c_u32CalSrcDelay;
	m_u32CalSrcSettleDelay	= c_u32CalSrcSettleDelay;
	m_u32CalSrcSettleDelay_1MOhm = c_u32CalSrcSettleDelay_1MOhm;
	m_u32MaxHwAvgDepth		= m_bNucleon ? SPLNDA_NUCLEON_MAX_MEM_HWAVG_DEPTH : SPLNDA_PCI_MAX_MEM_HWAVG_DEPTH;
	m_32SegmentTailBytes	= m_bNucleon ? 2*BYTESRESERVED_HWMULREC_TAIL : BYTESRESERVED_HWMULREC_TAIL;

	m_u16ChanProtectStatus = 0;

#if !defined(USE_SHARED_CARDSTATE)
	m_pCardState = &m_CardState;
	m_PlxData	= &m_pCardState->PlxData;
#else
	m_pCardState = 0;
#endif

#ifdef _DEBUG
	m_bSkipCalib = false;
#else
	m_bSkipCalib = true;
#endif
	m_bUseEepromCal = false;
	m_bUseEepromCalBackup = false;
	m_u16SkipCalibDelay = 20;
	m_bSkipSpiTest = false;
	m_bCalibActive = false;

	m_u32IgnoreCalibErrorMask = 0xff;
	m_u8TrigSensitivity    = CSPLNDA_DEFAULT_TRIG_SENSITIVITY;
	m_bOscar =  (CS_DEVID_OSCAR_PCIE == m_DeviceId);

	// Set the correct fpga image file name and fwl file
	if ( m_bNucleon )
	{
		if ( m_bOscar )
		{
			sprintf_s( m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", CSOSCAR_PCIEX_FPGAIMAGE_FILENAME );
			sprintf_s( m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", CSOSCAR_PCIEX_FPGAIMAGE_FILENAME );
		}
		else
		{
			sprintf_s( m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", CSSPLENDA_PCIEX_FPGAIMAGE_FILENAME );
			sprintf_s( m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", CSSPLENDA_PCIEX_FPGAIMAGE_FILENAME );
		}
	}
	else
	{
		sprintf_s( m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", CSSPLENDA_FPGA_IMAGE_FILE_NAME );
		sprintf_s( m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", CSSPLENDA_FPGA_IMAGE_FILE_NAME );
	}

	CsDevIoCtl::InitializeDevIoCtl( pDevInfo );

	m_u16AdcOffsetAdjust= CSPLNDA_DEFAULT_ADCOFFSET_ADJ;

	m_u16TrigOutWidth = 3;

}

////////////////////////////////////////////////////////////////////////
//

int32	CsSplendaDev::CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound )
{
	int32 i32Status = CS_SUCCESS;

	CsSplendaDev* pCard	= (CsSplendaDev*) m_ProcessGlblVars->pFirstUnCfg;

	// Remember the ForceInd flag for use later on Acqusition system initialization
	while (pCard)
	{
		pCard->m_pCardState->bForceIndMode = (BOOLEAN) bForceInd;
		pCard = pCard->m_HWONext;
	}

	if (( m_ProcessGlblVars->NbSystem == 0 ) && ! bForceInd )
	{
		pCard	= (CsSplendaDev*) m_ProcessGlblVars->pFirstUnCfg;
		while (pCard)
		{
			pCard->ReadAndValidateCsiFile( pCard->m_PlxData->CsBoard.u32BaseBoardHardwareOptions );
			pCard->ReadAndValidateFwlFile();
	
#if !defined(USE_SHARED_CARDSTATE)
	
			// Get the latest CardState values.
			// This must be done to fixe the problem of adding or updating Expert firmware from CompuScopeManager. 
			// In this case, the firmware info (CardState) has been changed by CompuScopeManager. Then when Rm call this function,
			// Rm should get the new the firmware info by calling IoctlGetCardState.
			pCard->IoctlGetCardState(pCard->m_pCardState);
#endif
			if ( pCard->m_PlxData->InitStatus.Info.u32Nios )
			{
				pCard->ReadBaseBoardHardwareInfoFromFlash();
				pCard->ReadAddOnHardwareInfoFromEeprom();
				pCard->CheckRequiredFirmwareVersion();
			}

			pCard = pCard->m_HWONext;
		}

		pCard	= (CsSplendaDev*) m_ProcessGlblVars->pFirstUnCfg;
		while (pCard)
		{
			if ( pCard->m_PlxData->CsBoard.u8BadBbFirmware || pCard->m_PlxData->CsBoard.u8BadAddonFirmware )
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

int32 CsSplendaDev::CsAcquisitionSystemInitialize( uInt32 u32InitFlag )
{
	int32	i32Status = CS_SUCCESS;

	if ( ! m_pSystem )
		return CS_INVALID_HANDLE;

	if ( m_pSystem->bInitialized )
	{
		// Clean up has not been performed. Possible because application crashed.
		// Cleanup before initialize

#ifdef __linux__ // if linux only close handles if we not recovering from a crash
		m_pSystem->bCrashRecovery = TRUE;
#endif

		i32Status = MsInitializeDevIoCtl();
		if ( CS_SUCCEEDED(i32Status) )
			CsAcquisitionSystemCleanup();
		else
			return i32Status;
	}

	CsSplendaDev* pDevice = this;
	if ( ! m_pSystem->bInitialized )
	{
#ifdef __linux__ // if linux only close handles if we not recovering from a crash
		m_pSystem->bCrashRecovery = FALSE;
#endif
		// Open link to kernel driver for all cards in system
		i32Status = MsInitializeDevIoCtl();
		if ( CS_FAILED(i32Status) )
			return i32Status;

		if ( ! m_pCardState->bForceIndMode )
		{
			while ( pDevice )
			{
				if ( ! pDevice->m_pCardState->bCardStateValid )
				{
					i32Status = pDevice->DeviceInitialize();
					if ( CS_FAILED(i32Status) )
						return i32Status;
				}

				i32Status = pDevice->AcqSystemInitialize();
				if ( CS_FAILED(i32Status) )
					break;

				pDevice = pDevice->m_Next;
			}

			if ( CS_SUCCEEDED(i32Status) )
			{
    			m_u32SystemInitFlag =  u32InitFlag;

				// Create acquisition events
//				m_pSystem->hAcqSystemCleanup	= ::CreateEvent(NULL, FALSE, FALSE, NULL );
				m_pSystem->hAcqSystemCleanup	= ::CreateEvent(NULL, TRUE, FALSE, NULL );	// Jan 20, 2016 changed to manual reset because 2 threads are waiting
				m_pSystem->hEventTriggered		= ::CreateEvent(NULL, FALSE, FALSE, NULL );
				m_pSystem->hEventEndOfBusy		= ::CreateEvent(NULL, FALSE, FALSE, NULL );
				m_pSystem->hEventAlarm			= ::CreateEvent(NULL, FALSE, FALSE, NULL );

				// Create events for asynchronous data transfer
				pDevice = this;
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
				InitEventHandlers();
	#endif

				if ( 0 == m_u32SystemInitFlag )	
				{	
					// If m_u32SystemInitFlag == 0 then perform the default full initialization,
					// otherwise just take whatever it is from the current state
					MsCsResetDefaultParams();
				}
			}
		}
		if ( m_pSystem->hAcqSystemCleanup )
			ResetEvent(m_pSystem->hAcqSystemCleanup); // Jan 20,2016 - because thread is now manual reset
		m_pCardState->u32ProcId = GetCurrentProcessId();
		pDevice = m_MasterIndependent;
		while( pDevice )
		{
			pDevice->m_pCardState->u32ProcId = m_pCardState->u32ProcId;
			pDevice->UpdateCardState();
			pDevice = pDevice->m_Next;
		}

		m_pSystem->bInitialized = TRUE;
		m_pSystem->BusyTransferCount = 0;

		if ( CS_FAILED(i32Status) )
			CsAcquisitionSystemCleanup();
	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

void	CsSplendaDev::CsAcquisitionSystemCleanup()
{
	// Set the reset flag in order to stop all pending processing
	m_pSystem->bReset = TRUE;

	CsAbort();

	if ( m_pAverageBuffer )
	{
		::GageFreeMemoryX( m_pAverageBuffer );
		m_pAverageBuffer = NULL;
	}
	if ( m_pi32CalibA2DBuffer )
	{
		::GageFreeMemoryX( m_pi32CalibA2DBuffer );
		m_pi32CalibA2DBuffer = NULL;
	}

	CsSplendaDev *pDevice = m_MasterIndependent;

	if ( m_pCardState->u8ImageId != 0 )
	{
		// switch to the default image
		pDevice = m_MasterIndependent;
		while (pDevice != NULL)
		{
			pDevice->CsLoadBaseBoardFpgaImage( 0 );
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
	if ( 0 == m_u32SystemInitFlag )
	{
		RegisterEventHandle( NULL, ACQ_EVENT_TRIGGERED );
		RegisterEventHandle( NULL, ACQ_EVENT_END_BUSY );
	}

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		// Idle state for WatchDog Timer
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
//	SetEvent(m_pSystem->hAcqSystemCleanup); // JAN 20, 2016 is now manual so it terminates both threads
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
			
			if ( m_pSystem->hEventTriggered )
				CloseHandle(m_pSystem->hEventTriggered);
			if ( m_pSystem->hEventEndOfBusy )
				CloseHandle(m_pSystem->hEventEndOfBusy);
			if ( m_pSystem->hEventAlarm )
				CloseHandle(m_pSystem->hEventAlarm);			
			if ( m_pSystem->hThreadIntr )
				CloseHandle(m_pSystem->hThreadIntr);  // RG JUST ADDED Jan. 6, 2016
			if ( m_DataTransfer.hAsynThread )
				CloseHandle(m_DataTransfer.hAsynThread);
			m_pSystem->bCrashRecovery = FALSE; // if it was true, set to false			
		}
#else		
		SetEvent(m_pSystem->hAcqSystemCleanup); // JAN 20, 2016 is now manual so it terminates both threads
		// Close acquisition events and thread handles
		if ( m_pSystem->hAcqSystemCleanup )
			CloseHandle(m_pSystem->hAcqSystemCleanup);
		if ( m_pSystem->hEventTriggered )
			CloseHandle(m_pSystem->hEventTriggered);
		if ( m_pSystem->hEventEndOfBusy )
			CloseHandle(m_pSystem->hEventEndOfBusy);
		if ( m_pSystem->hEventAlarm )
			CloseHandle(m_pSystem->hEventAlarm);
		if ( m_DataTransfer.hAsynThread )
			CloseHandle(m_DataTransfer.hAsynThread); // Added Jan 20, 2016
		if ( m_pSystem->hThreadIntr )
			CloseHandle(m_pSystem->hThreadIntr);	 // ADDED Jan. 20, 2016
#endif // __linux__

		m_pSystem->hAcqSystemCleanup	=
		m_pSystem->hEventTriggered		=
		m_pSystem->hEventEndOfBusy		=
		m_pSystem->hEventAlarm			= 
		m_pSystem->hThreadIntr			= 
		m_DataTransfer.hAsynThread		= 0;
		m_UseIntrBusy = m_UseIntrTrigger = 0;
	}

	// Reset system to default state
	m_pSystem->bInitialized = FALSE;
	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;
}

////////////////////////////////////////////////////////////////////////
//

uInt32	CsSplendaDev::CsGetAcquisitionStatus()
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
					// SPI clocks is disabled at Acquire(), turn it back on
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
				// SPI clocks is disabled at Acquire(), turn it back on
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


int32	CsSplendaDev::CsGetParams( uInt32 u32ParamId, PVOID pOutBuffer, uInt32 u32OutSize )
{
	PVOID			pUserBuffer = pOutBuffer;
	int32			i32Status = CS_SUCCESS;
	CsSplendaDev	*pDevice = m_MasterIndependent;

	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
		return CS_POWERSTATE_ERROR;

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

	case CS_READ_EEPROM://----- Read Add-On Flash
		{
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

			if ( ! m_pCardState->bAddOnRegistersInitialized )
				i32Status = CS_ADDONINIT_ERROR;
			else
				i32Status = ReadEeprom(pAppBuffer, u32Offset, u32BufferSize);
		}
		break;

	case CS_READ_FLASH://----- Read Base Board Flash
		{
			PCS_GENERIC_FLASH_READWRITE pGenericRw = (PCS_GENERIC_FLASH_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

			if ( m_bNucleon )
				i32Status = IoctlReadFlash( u32Offset, pAppBuffer, u32BufferSize );
			else
				i32Status = ReadFlash(pAppBuffer, u32Offset, u32BufferSize);
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

			pExOptions[0] = m_PlxData->CsBoard.u32BaseBoardOptions[1];
			pExOptions[1] = m_PlxData->CsBoard.u32BaseBoardOptions[2];
		}
		break;

	case CS_SYSTEMINFO_EX:
		{
			PCSSYSTEMINFOEX pSysInfoEx = (PCSSYSTEMINFOEX) pUserBuffer;

			uInt32	i;

			RtlZeroMemory( pSysInfoEx, sizeof(CSSYSTEMINFOEX) );
			pSysInfoEx->u32Size = sizeof(CSSYSTEMINFOEX);
			pSysInfoEx->u16DeviceId = m_DeviceId; //GetDeviceId();

			// Permission and firmware versions from the master card
            pSysInfoEx->i64ExpertPermissions = pDevice->m_PlxData->CsBoard.i64ExpertPermissions;
			pSysInfoEx->u32BaseBoardHwOptions = pDevice->m_PlxData->CsBoard.u32BaseBoardHardwareOptions;
			pSysInfoEx->u32AddonHwOptions = pDevice->m_PlxData->CsBoard.u32AddonHardwareOptions;

			for ( i = 0; i < sizeof(m_PlxData->CsBoard.u32RawFwVersionB) /sizeof(uInt32) ; i ++)
			{
				pSysInfoEx->u32BaseBoardFwOptions[i]	= m_PlxData->CsBoard.u32BaseBoardOptions[i];
				pSysInfoEx->u32AddonFwOptions[i]		= m_PlxData->CsBoard.u32AddonOptions[i];

				pSysInfoEx->u32BaseBoardFwVersions[i].u32Reg	= GetBoardNode()->u32UserFwVersionB[i].u32Reg;
				pSysInfoEx->u32AddonFwVersions[i].u32Reg		= GetBoardNode()->u32UserFwVersionA[i].u32Reg;
			}

			pDevice = pDevice->m_Next;

			// System permission will be "AND" persmissions of all Master/Slave cards
			while ( pDevice )
			{
				// i starts from 1 because the image 0 is not an expertt option
				for ( i = 1; i < sizeof(m_PlxData->CsBoard.u32RawFwVersionB) /sizeof(uInt32) ; i ++)
				{
					pSysInfoEx->u32BaseBoardFwOptions[i] &= pDevice->m_PlxData->CsBoard.u32BaseBoardOptions[i];
					pSysInfoEx->u32AddonFwOptions[i] &= pDevice->m_PlxData->CsBoard.u32AddonOptions[i];

					if ( 0 == pSysInfoEx->u32BaseBoardFwOptions[i] )
						pSysInfoEx->u32BaseBoardFwVersions[i].u32Reg = 0;

					if ( 0 == pSysInfoEx->u32AddonFwOptions[i] )
						pSysInfoEx->u32AddonFwVersions[i].u32Reg = 0;
				}

				pSysInfoEx->i64ExpertPermissions &= pDevice->m_PlxData->CsBoard.i64ExpertPermissions;
				pSysInfoEx->u32AddonHwOptions &= pDevice->m_PlxData->CsBoard.u32AddonHardwareOptions;
				pSysInfoEx->u32BaseBoardHwOptions &= pDevice->m_PlxData->CsBoard.u32BaseBoardHardwareOptions;
				pDevice = pDevice->m_Next;
			}
		}
		break;

	case CS_TIMESTAMP_TICKFREQUENCY:
		{
			PLARGE_INTEGER pTickCount = (PLARGE_INTEGER) pUserBuffer;
			pTickCount->QuadPart = GetTimeStampFrequency();
		}
		break;


	case CS_DRV_BOARD_CAPS:
		{
			PCSSPLENDABOARDCAPS pCapsInfo = (PCSSPLENDABOARDCAPS) pUserBuffer;
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

			pClockOut->u16Value = (uInt16) m_pCardState->eclkoClockOut;
			pClockOut->u16Valid = CS_OUT_NONE|CS_CLKOUT_SAMPLE|CS_CLKOUT_REF;
		}
		break;

	case CS_MULREC_AVG_COUNT:
		*( (uInt32*) pUserBuffer ) = m_u32MulrecAvgCount; 
		break;

	case CS_GAINTARGET_FACTOR:
		{
			PRAZOR_GAINTARGET_FACTOR pGainTgFactor = (PRAZOR_GAINTARGET_FACTOR) pUserBuffer;
			uInt32	u32RangeIndex;
			uInt32	u32ImpedIndex;
			uInt32	u32FilterIndex;

			CsSplendaDev *pDevice = GetCardPointerFromChannelIndex( pGainTgFactor->u16Channel );
			if ( pDevice )
			{
				uInt16 u16Channel = NormalizedChannelIndex(pGainTgFactor->u16Channel);

				u16Channel = ConvertToHwChannel( u16Channel );
				i32Status = FindGainFactorIndex( u16Channel, pGainTgFactor->u32InputRange, pGainTgFactor->u32Impedance, pGainTgFactor->u32Filter,
												&u32RangeIndex, &u32ImpedIndex, &u32FilterIndex );
				if( CS_SUCCEEDED(i32Status) )
					pDevice->m_u16GainTarFactor[u16Channel-1][u32RangeIndex][u32ImpedIndex][u32FilterIndex] = pGainTgFactor->u16Factor;
			}
			else
				i32Status = CS_INVALID_CHANNEL;
		}
		break;


	case CS_FRONTPORT_RESISTANCE:
		{
			PRAZOR_FROTNPORT_RESISTANCE_HiZ pStruct = (PRAZOR_FROTNPORT_RESISTANCE_HiZ) pUserBuffer;

			CsSplendaDev *pDevice = GetCardPointerFromChannelIndex( pStruct->u16Channel );
			if ( pDevice )
				pDevice->m_u32FrontPortResistance_mOhm[NormalizedChannelIndex(pStruct->u16Channel)-1] = pStruct->u32FrPortRes;
			else
				i32Status = CS_INVALID_CHANNEL;
		}
		break;


	case CS_USE_DACCALIBTABLE:
		{
			uInt32	*pu32Val  = (uInt32	*) pUserBuffer;

			if( (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
				( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_VALID)   ) ||
	            ( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_INVALID) ) )
			{
				i32Status = CS_INVALID_DACCALIBTABLE;
			}
			else
				*pu32Val = m_bUseEepromCal?1:0;
		}
		break;

		case CS_DEFAULTUSE_DACCALIBTABLE:
		{
			uInt32	*pu32Val  = (uInt32	*) pUserBuffer;

			if( (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
				( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_VALID)   ) ||
	            ( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_INVALID) ) )
			{
				i32Status = CS_INVALID_DACCALIBTABLE;
			}
			else
				*pu32Val = (0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_USAGE_VALID))?1:0;
		}
		break;

		case CS_READ_CALIB_A2D:
		{
			PCS_READCALIBA2D_STRUCT pReadCalibA2D = (PCS_READCALIBA2D_STRUCT) pUserBuffer;
			CsSplendaDev* pDevice = GetCardPointerFromBoardIndex( pReadCalibA2D->u8CardNumber );

			if ( NULL != pDevice )
				i32Status = pDevice->ReadCalibA2D( pReadCalibA2D->i32ExpectedVoltage, 
									   (eCalInput)pReadCalibA2D->u8Input,
									   &(pReadCalibA2D->i32ReadVoltage), 
									   &(pReadCalibA2D->i32NoiseLevel) );	
		}
		break;

		case CS_READ_DAC:
		{
			PCS_SENDDAC_STRUCT pSendDAC = (PCS_SENDDAC_STRUCT) pUserBuffer;
			CsSplendaDev* pDevice = GetCardPointerFromBoardIndex( pSendDAC->u8CardNumber );

			if ( NULL != pDevice )
			{
				if ( CSPLNDA_TRIM_CAP_50_CHAN1 <= pSendDAC->u8DacAddress && CSPLNDA_TRIM_CAP_10_CHAN4 >= pSendDAC->u8DacAddress )
				{
					pSendDAC->u16DacValue = pDevice->m_u16TrimCap[pSendDAC->u8DacAddress % 4][CSPLNDA_TRIM_CAP_10_CHAN1 <= pSendDAC->u8DacAddress ?1:0];
				}
				else
				{
					i32Status = CS_FUNCTION_NOT_SUPPORTED;
				}
			}
			else
			{
				i32Status = CS_INVALID_CARD;
			}
		}
		break;

	case CS_READ_PCIeLINK_INFO:
		{
			if ( ! m_bNucleon )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				CsSplendaDev	*pDevice = m_MasterIndependent;
				PPCIeLINK_INFO	pPciExInfo = (PPCIeLINK_INFO) pUserBuffer;

				if ( 0 != pDevice->m_Next )
					pDevice = GetCardPointerFromBoardIndex( pPciExInfo->u16CardIndex );

				if ( NULL == pDevice )			
					i32Status = CS_INVALID_CARD;
				else
					i32Status = pDevice->IoctlGetPciExLnkInfo( pPciExInfo );
			}
		}
		break;

	case CS_READ_GAGE_REGISTER:
		{
#if 0
			char szText[128];

			uInt32 RegStatus = ReadGageRegister( NIOS_GEN_STATUS );
			sprintf_s( szText, sizeof(szText), "RegStatus = 0x%x\n", RegStatus );
			::OutputDebugString(szText);

			uInt32 StreamStatus = ReadGageRegister( 0x60 );
			StreamStatus = ReadPciRegister( 0xE0 );
			sprintf_s( szText, sizeof(szText), "StreamStatus = 0x%x\n", StreamStatus );
			::OutputDebugString(szText);

			uInt32 DmaStatus = ReadPciRegister( 0 );
			sprintf_s( szText, sizeof(szText), "DmaStatus = 0x%x\n", DmaStatus );
			::OutputDebugString(szText);

			uInt32 IntrCounter = (0x3f & ReadGageRegister( 0x24 ));
			sprintf_s( szText, sizeof(szText), "IntrCounter = 0x%x\n", IntrCounter );
			::OutputDebugString(szText);
#else
			PCS_RD_GAGE_REGISTER_STRUCT		pReadRegister = (PCS_RD_GAGE_REGISTER_STRUCT) pUserBuffer;
			CsSplendaDev*					pDevice = GetCardPointerFromBoardIndex( pReadRegister->u16CardIndex );

			if ( NULL != pDevice )
				pReadRegister->u32Data = pDevice->ReadGageRegister( pReadRegister->u16Offset );
			else
				i32Status = CS_INVALID_CARD;

#endif
		}
		break;

	case CS_READ_PCI_REGISTER:
		{
			PCS_RD_GAGE_REGISTER_STRUCT		pReadRegister = (PCS_RD_GAGE_REGISTER_STRUCT) pUserBuffer;
			CsSplendaDev*					pDevice = GetCardPointerFromBoardIndex( pReadRegister->u16CardIndex );

			if ( NULL != pDevice )
				pReadRegister->u32Data = pDevice->ReadPciRegister( pReadRegister->u16Offset );
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_STREAM_SEGMENTDATA_SIZE_SAMPLES:
		if ( m_Stream.bEnabled )
			*( (LONGLONG*) pUserBuffer ) = m_pCardState->AcqConfig.i64SegmentSize*(m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE); 
		else
			i32Status = CS_INVALID_REQUEST;
			break;

	case CS_STREAM_TOTALDATA_SIZE_BYTES:
		if ( m_Stream.bEnabled )
			*( (LONGLONG*) pUserBuffer ) = m_Stream.bRunForever ? -1 : m_Stream.u64TotalDataSize;
		else
			i32Status = CS_INVALID_REQUEST;
			break;

	case CS_SEGMENTTAIL_SIZE_BYTES:
		if ( m_bMulrecAvgTD )
			*( (uInt32*) pUserBuffer ) = 0;
		else
			*( (uInt32*) pUserBuffer ) = m_32SegmentTailBytes; 
		break;

	default:
		i32Status = CS_INVALID_PARAMS_ID;
		break;

	}

	return i32Status;

}


////////////////////////////////////////////////////////////////////////
//

int32	CsSplendaDev::CsSetParams( uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize )
{
	PVOID			pUserBuffer = pInBuffer;
	int32			i32Status = CS_SUCCESS;
	CsSplendaDev	*pDevice = m_MasterIndependent;

	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
		return CS_POWERSTATE_ERROR;

	UNREFERENCED_PARAMETER(u32InSize);

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

			i32Status = CommitNewConfiguration( pAcqCfg, pAChanCfg, pATrigCfg );

		}
		break;


	case CS_WRITE_EEPROM: //----- Write Add-On Flash
		{
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;
			
			if ( ! m_pCardState->bAddOnRegistersInitialized )
				i32Status = CS_ADDONINIT_ERROR; 
			else if ( u32Offset + u32BufferSize <= SPLNDA_FLASH_SIZE )
			{
				i32Status = WriteEepromEx(pAppBuffer, u32Offset, u32BufferSize);
				if ( CS_SUCCEEDED(i32Status) )
				{
					i32Status = ReadAddOnHardwareInfoFromEeprom();
					if ( CS_SUCCEEDED(i32Status) )
						m_pSystem->SysInfo.u32AddonOptions = m_PlxData->CsBoard.u32ActiveAddonOptions;
				}
			}
			else
				i32Status = CS_INVALID_POINTER_BUFFER;
		}
		break;

	case CS_WRITE_FLASH: //----- Write Base Board Flash
		{
			PCS_GENERIC_FLASH_READWRITE pGenericRw = (PCS_GENERIC_FLASH_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

			if ( pGenericRw->u32BufferSize !=0 && pGenericRw->pBuffer != 0 )
			{
				if ( m_bNucleon && (u32Offset + u32BufferSize <= NUCLEON_FLASH_SIZE) )
					i32Status = NucleonWriteFlash(u32Offset, pAppBuffer, u32BufferSize, true );
				else if ( u32Offset + u32BufferSize <= BBFLASH32Mbit_SIZE )
					i32Status = WriteFlashEx(pAppBuffer, u32Offset, u32BufferSize);
				else
					i32Status = CS_INVALID_POINTER_BUFFER;
			}
			else
			{
				// The Hardware info from flash has been updated. Reload the Hardware Info
				// The Hardware info from flash has been updated. Reload the Hardware Info
				RtlZeroMemory( m_PlxData->CsBoard.u32RawFwVersionB, sizeof( m_PlxData->CsBoard.u32RawFwVersionB ) );
				RtlZeroMemory( m_PlxData->CsBoard.u32RawFwVersionA, sizeof( m_PlxData->CsBoard.u32RawFwVersionA ) );

				i32Status = ReadBaseBoardHardwareInfoFromFlash();
				ReadVersionInfo();
				ReadVersionInfoEx();
			}
		}
		break;


	case CS_WRITE_NVRAM: //---- Write NvRam (Boot Eeprom)
		{
			i32Status = CsWritePlxNvRam( (PPLX_NVRAM_IMAGE) pUserBuffer );
		}
		break;

	case CS_SEND_DAC: //---- Send Write Command to DAC
		{
			PCS_SENDDAC_STRUCT	pSendDAC = (PCS_SENDDAC_STRUCT) pUserBuffer;

			if ( NULL != pDevice )
				pDevice->WriteToCalibDac( pSendDAC->u8DacAddress, pSendDAC->u16DacValue );
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_SAVE_CALIBTABLE_TO_EEPROM:
		{
			uInt32 u32Valid = *((uInt32 *)	pUserBuffer);
			i32Status = WriteCalibTableToEeprom( u32Valid );
		}
		break;

	case CS_TRIG_OUT:
		{
			PCS_TRIG_OUT_STRUCT	pTrigOut = (PCS_TRIG_OUT_STRUCT) pUserBuffer;
			i32Status = SetTrigOut( pTrigOut->u16Value );
		}
		break;

	case CS_CLOCK_OUT:
		{
			PCS_CLOCK_OUT_STRUCT	pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;
			// check if it has changed
			if ( m_pCardState->eclkoClockOut != (eClkOutMode)pClockOut->u16Value )
				i32Status = SetClockOut( (eClkOutMode)pClockOut->u16Value );
		}
		break;

	case CS_SAVE_OUT_CFG:
		{
			SPLENDA_FLASHFOOTER 	SplendaFshFooter;

			uInt32 u32Offset = FIELD_OFFSET(SPLNDA_FLASH_LAYOUT, Footer);
			i32Status = ReadEeprom( &SplendaFshFooter, u32Offset, sizeof(SplendaFshFooter) );
			if ( CS_FAILED(i32Status) )
				break;

			if ( 0 != *((uInt32 *) pUserBuffer)  )
			{
				// Save out config
				SplendaFshFooter.u16ClkOut			= (uInt16) m_pCardState->eclkoClockOut;
				SplendaFshFooter.u16TrigOutEnabled  = !m_pCardState->bDisableTrigOut;
			}
			else
			{
				// Clear out config
				SplendaFshFooter.u16ClkOut			= (uInt16)-1;
				SplendaFshFooter.u16TrigOutEnabled  = (uInt16)-1;

				m_pCardState->eclkoClockOut		= CSPLNDA_DEFAULT_CLOCK_OUT;
				m_pCardState->bDisableTrigOut		= TRUE;
			}
			i32Status = WriteEepromEx( &SplendaFshFooter, u32Offset, sizeof(SplendaFshFooter) );
		}
		break;

	case CS_SELF_TEST_MODE:
		{
			PCS_SELF_TEST_STRUCT	pSelf = (PCS_SELF_TEST_STRUCT) pUserBuffer;
			i32Status = WriteToSelfTestRegister( pSelf->u16Value );
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

	case CS_USE_DACCALIBTABLE:
		{
				uInt32* pValue = (uInt32*)pUserBuffer;
				if( (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
					( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_VALID)   ) ||
					( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_INVALID) ) )
				{
					i32Status = CS_INVALID_DACCALIBTABLE;
				}
				else if ( 0 != *pValue )
					m_bUseEepromCal = true;
				else
					m_bUseEepromCal = false;
		}
		break;

	case CS_DEFAULTUSE_DACCALIBTABLE:
		{
			if( (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
				( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_VALID)   ) ||
	            ( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_INVALID) ) )
			{
				i32Status = CS_INVALID_DACCALIBTABLE;
			}
			else 
			{
				if ( *((uInt32 *) pUserBuffer) )
					m_pCardState->CalibInfoTable.u32Valid |= CSPLNDA_CAL_USAGE_VALID;
				else
				{
					m_pCardState->CalibInfoTable.u32Valid &= ~CSPLNDA_CAL_USAGE_VALID;
					m_bUseEepromCal = false;
				}
				i32Status = WriteCalibTableToEeprom( m_pCardState->CalibInfoTable.u32Valid );
			}
		}
		break;

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
				pDevice->SendCalibModeSetting( ConvertToHwChannel(pCalibMode->u16Channel), (eCalMode)pCalibMode->u8CalMode );
			}
		}
		break;

	case CS_SEND_CALIB_DC:
		{
			PCS_CALIBDC_STRUCT pCalibDC = (PCS_CALIBDC_STRUCT) pUserBuffer;

			if ( NULL != pDevice )
			{
				pDevice->SendCalibDC( pCalibDC->u32Impedance, pCalibDC->u32Range, pCalibDC->i32Level, &(pCalibDC->i32SetLevel) );
			}
		}
		break;

	case CS_TIMESTAMP_RESET:
		ResetTimeStamp();
		break;

	case CS_WRITE_GAGE_REGISTER:
		{
			PCS_RD_GAGE_REGISTER_STRUCT		pWriteRegister = (PCS_RD_GAGE_REGISTER_STRUCT) pUserBuffer;
			CsSplendaDev*					pDevice = GetCardPointerFromBoardIndex( pWriteRegister->u16CardIndex );

			if ( NULL != pDevice )
				i32Status = pDevice->WriteGageRegister( pWriteRegister->u16Offset, pWriteRegister->u32Data );
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_WRITE_PCI_REGISTER:
		{
			PCS_RD_GAGE_REGISTER_STRUCT		pWriteRegister = (PCS_RD_GAGE_REGISTER_STRUCT) pUserBuffer;
			CsSplendaDev*					pDevice = GetCardPointerFromBoardIndex( pWriteRegister->u16CardIndex );

			if ( NULL != pDevice )
				i32Status = pDevice->WritePciRegister( pWriteRegister->u16Offset, pWriteRegister->u32Data );
			else
				i32Status = CS_INVALID_CARD;
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

int32 CsSplendaDev::CsStartAcquisition()
{
	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
		return CS_POWERSTATE_ERROR;

	m_pSystem->bEndAcquisitionNotification = FALSE;
	m_pSystem->u32TriggerNotification =  0;
	m_pSystem->u32TriggeredCount = 0;

	m_pSystem->u16AcqStatus  = ACQ_STATUS_WAIT_TRIGGER;

	// Reset notification flag before acquisition
	m_pTriggerMaster->m_PlxData->CsBoard.bBusy = true;
	m_pTriggerMaster->m_PlxData->CsBoard.bTriggered = false;

	m_PlxData->IntBaseMask = MAOINT;		// for Channels protection 

	if ( m_bMulrecAvgTD )
		m_PlxData->IntBaseMask |= MULREC_AVG_DONE;
	else if ( m_UseIntrBusy )
		m_PlxData->IntBaseMask |= N_MBUSY_REC_DONE;

	if ( m_UseIntrTrigger )
		m_PlxData->IntBaseMask |= MTRIG;

#if DBG
	m_pTriggerMaster->m_u32IntrCount = 0;
#endif

	if ( m_Stream.bEnabled )
		MsStmResetParams();

	m_MasterIndependent->m_pCardState->u32ActualSegmentCount = m_pCardState->AcqConfig.u32SegmentCount;
	MsClearChannelProtectionStatus();

	in_STARTACQUISITION_PARAMS AcqParams = {0};

	AcqParams.u32IntrMask		= m_PlxData->IntBaseMask;		
	if ( m_Stream.bEnabled )
	{
		// If Stream mode is enable, we can send start acquisition in either STREAM mode or REGULAR mode .
		// If we are in calibration mode then we have to capture data in REGULAR mode.
		// Use the u32Params1 to let the kernel driver know what mode we want to capture.
 		AcqParams.u32Param1 = 1;
	}

	// Send startacquisition command
	return IoctlDrvStartAcquisition( &AcqParams );
}


////////////////////////////////////////////////////////////////////////
//

int32	CsSplendaDev::CsAbort()
{
	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
		return CS_POWERSTATE_ERROR;

	// Clear the interrupt register
	CsSplendaDev* pCard = m_MasterIndependent;

	// Abort all pending stream asynchronous data transfers
	if ( pCard->m_Stream.bEnabled )
	{
		while ( pCard )
		{
			if ( pCard->m_Stream.u32BusyTransferCount > 0 )
			{
				pCard->IoctlAbortAsyncTransfer();

				// Yeild the remaining time slice to the thread that waits for DMA complete if there is any
				Sleep(0);
				// If there is no thread that waits for DMA complete then we have to call this to reset the internal state
				pCard->CsStmGetTransferStatus( 10, NULL );
			}

			// Abort and Reset Stream
			pCard->IoctlStmSetStreamMode(-1);
			pCard = pCard->m_Next;
		}

		// In stream mode, the HW will not generate the end of acquisition interrupt
		// Satisfy SSM state machine by simulate the end of acquisition event
		GetBoardNode()->bTriggered = true;
		GetBoardNode()->bBusy = false;
		m_pTriggerMaster->SignalEvents();
	}
	else
	{
		switch( m_pSystem->u16AcqStatus )
		{
			case ACQ_STATUS_WAIT_TRIGGER:
			case ACQ_STATUS_TRIGGERED:
				{
					// Arbort action will clear the acquisition status, thus there is no interrupt generated.
					// We have to keep track where we were before ABORT so that we can simulate triggerred and
					// end of acquisition interrutps.
					m_PlxData->CsBoard.bTriggered = true;
					m_PlxData->CsBoard.bBusy = false;
					Abort();

					m_pTriggerMaster->SignalEvents();
				}
				break;

			default:
			{
				while ( pCard )
				{
					if ( pCard->m_DataTransfer.BusyTransferCount )
					{
						pCard->AbortCardReadMemory();
						pCard->m_DataTransfer.BusyTransferCount = 0;
					}
					pCard = pCard->m_Next;
				}
				m_pSystem->BusyTransferCount = 0;
			}
			break;
		}
	}

	// Clear the interrupt register
	pCard = m_MasterIndependent;

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

void	CsSplendaDev::CsForceTrigger()
{
	m_pTriggerMaster->ForceTrigger();

}

////////////////////////////////////////////////////////////////////////
//

int32	CsSplendaDev::CsDataTransfer( in_DATA_TRANSFER *InDataXfer )
{
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
		return CS_POWERSTATE_ERROR;

	if ( ! IsChannelIndexValid ( pInParams->u16Channel ) )
		return CS_INVALID_CHANNEL;

	if ( pInParams->u32Mode & TxMODE_TIMESTAMP )
	{
		// Convert the size of user Buffer from sizeof(char) to sizeof(int64)
		pInParams->i64Length = pInParams->i64Length / sizeof(int64);

		if ( pInParams->i64Length + pInParams->u32Segment - 1 > m_pCardState->AcqConfig.u32SegmentCount )
			pInParams->i64Length = m_pCardState->AcqConfig.u32SegmentCount - pInParams->u32Segment + 1;

		int32 i32Status = m_pTriggerMaster->ReadTriggerTimeStampEtb( pInParams->u32Segment - 1, (int64 *)pInParams->pDataBuffer, (uInt32) pInParams->i64Length );
		if ( CS_SUCCEEDED(i32Status) )
		{
			pOutParams->i64ActualStart	= pInParams->u32Segment;
			pOutParams->i64ActualLength = pInParams->i64Length;
			pOutParams->i32HighPart		= 0;
			pOutParams->i32LowPart		= (uInt32)  GetTimeStampFrequency();
		}
		return i32Status;
	}
	else
	{
		uInt32 u32Mode = pInParams->u32Mode & ~TxMODE_SLAVE;

		if ( u32Mode == TxMODE_DATA_ANALOGONLY ||
			 u32Mode == TxMODE_DATA_16         ||
			 u32Mode == TxMODE_DATA_32         ||
			 u32Mode == TxMODE_DATA_ONLYDIGITAL ||
			u32Mode == TxMODE_DATA_FFT			||
			u32Mode == TxMODE_DEBUG_DISP		)
		{
			if ( TxMODE_DATA_32 & u32Mode )
			{
				if (!m_bMulrecAvgTD && !m_bSoftwareAveraging )
				{
					return CS_INVALID_TRANSFER_MODE;
				}
			}

			//---- Find the card that we need to setup for transfer
			CsSplendaDev* pDevice = GetCardPointerFromChannelIndex( pInParams->u16Channel );

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
					else if ( u32Mode == TxMODE_DEBUG_DISP)
						return pDevice->ProcessDataTransferDebug(InDataXfer);
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

int32 CsSplendaDev::CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt )
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

//	return CS_NO_INTERRUPT;
	return i32Status;
}

////////////////////////////////////////////////////////////////////////
//

void	CsSplendaDev::CsSystemReset(BOOL bHard_reset)
{
	UNREFERENCED_PARAMETER(bHard_reset);
	CsAbort();

	CsSplendaDev *pCard = m_MasterIndependent;
	pCard = m_MasterIndependent;
	while (pCard)
	{
		if ( pCard->m_pCardState->u8ImageId != 0 )
			pCard->CsLoadBaseBoardFpgaImage(0);
		else
			pCard->BaseBoardConfigurationReset(0);
		
		// Reset Addon board
		pCard->ReadCsiFileAndProgramFpga();
		pCard = pCard->m_Next;
	}

	MsCsResetDefaultParams( TRUE );

}

////////////////////////////////////////////////////////////////////////
//

int32	CsSplendaDev::CsForceCalibration()
{
	int32			i32Status = CS_SUCCESS;

	CsSplendaDev	 *pDevice = m_MasterIndependent;
	uInt16			u16ChanZeroBased = 0;
	while ( NULL != pDevice )
	{
		// Force calibration for all activated channels depending on the current mode (single, dual , quad ...)
		for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
		{
			u16ChanZeroBased = ConvertToHwChannel(i) - 1;
			pDevice->m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
			pDevice->m_bForceCal[u16ChanZeroBased] = true;
		}

		pDevice = pDevice->m_Next;
	}

	i32Status = MsCalibrateAllChannels();

	m_MasterIndependent->ResyncClock();

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsSplendaDev::CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait )
{
	int32 i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(bWait);

	CsSplendaDev *pDevice = GetCardPointerFromChannelIndex( u16Channel );

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
void * CsSplendaDev::operator new (size_t  size)
{
	PVOID	p = new char[size];

	if ( p )
		memset(p, 0, size);

	return p;
}

#ifdef USE_SHARED_CARDSTATE

int32	CsSplendaDev::InitializeDevIoCtl()
{
	int32 i32Status = CsDevIoCtl::InitializeDevIoCtl();

	if ( CS_SUCCEEDED( i32Status ) )
	{
		i32Status = IoctlGetCardStatePointer( &m_pCardState );
		if ( CS_SUCCEEDED( i32Status ) )
		{
			if ( sizeof(SPLENDA_CARDSTATE) == m_pCardState->u32Size  )
			{
				m_PlxData	= &m_pCardState->PlxData;
				return CS_SUCCESS;
			}
			else
			{
				::OutputDebugString("\nError SPLENDA_CARDSTATE struct size discrepency !!");
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
