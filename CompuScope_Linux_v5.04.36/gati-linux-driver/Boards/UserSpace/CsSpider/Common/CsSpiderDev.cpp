// CsSpiderDev.cpp
//

#include "StdAfx.h"
#include "CsSpider.h"
#include "PeevAdaptor.h"

#ifdef __linux__
//	#include "CsEvtThread.h"
#endif

#ifdef _WINDOWS_
	#include <process.h>			// for _beginthreadex
	// DEBUG PRF (paralelle port)
	#include <conio.h>
#endif

#define CSPDR_DAC_SETTLE_DELAY  5
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsSpiderDev::CsSpiderDev(uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars):
	m_u8CardIndex(u8Index), m_ProcessGlblVars(ProcessGlblVars), m_PseudoHandle(pDevInfo->DrvHdl), m_bNucleon(PCIE_BUS == pDevInfo->BusType),
	m_DeviceId(pDevInfo->DeviceId), m_u32HwTrigTimeout((uInt32)CS_TIMEOUT_DISABLE)
{

	// Memory has been cleared on allocation
	// Only need to initialize member variables whose default value is different than 0
	m_BusMasterSupport = TRUE;
	m_bNoConfigReset	= true;

	m_u8CalDacSettleDelay = CSPDR_DAC_SETTLE_DELAY;

	m_pTriggerMaster  = this;
	m_MasterIndependent = this;
	m_u16SkipCalibDelay = 20;
	m_u32IgnoreCalibErrorMask = 0xff;
	m_u32MaxHwAvgDepth		= m_bNucleon ? SPDR_NUCLEON_MAX_MEM_HWAVG_DEPTH : SPDR_PCI_MAX_MEM_HWAVG_DEPTH;
	m_32SegmentTailBytes	= m_bNucleon ? 2*BYTESRESERVED_HWMULREC_TAIL : BYTESRESERVED_HWMULREC_TAIL;

	m_IntPresent		= FALSE;
	m_pNextInTheChain	= NULL;
	m_pTriggerMaster	= this;
	m_MasterIndependent = this;
	m_u32ExtClkStop = 0;

	CsDevIoCtl::InitializeDevIoCtl( pDevInfo );

	m_bDisableUserOffet = false;
	m_bSkipFirmwareCheck = false;

#if !defined(USE_SHARED_CARDSTATE)
	m_pCardState	= &m_CardState;
	m_PlxData		= &m_pCardState->PlxData;
#else
	m_pCardState	= 0;
#endif

}


////////////////////////////////////////////////////////////////////////
//

int32	CsSpiderDev::CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound )
{
	int32 i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(bForceInd);

	CsSpiderDev* pCard	= (CsSpiderDev*) m_ProcessGlblVars->pFirstUnCfg;

	// Remember the ForceInd flag for use later on Acqusition system initialization
	while (pCard)
	{
		pCard->m_pCardState->bForceIndMode = (BOOLEAN)bForceInd;
		pCard = pCard->m_HWONext;
	}

	if (( m_ProcessGlblVars->NbSystem == 0 ) && ! bForceInd )
	{
		pCard	= (CsSpiderDev*) m_ProcessGlblVars->pFirstUnCfg;
		while (pCard)
		{
			pCard->ReadAndValidateCsiFile( pCard->m_PlxData->CsBoard.u32BaseBoardOptions[0], 0 );
			pCard->ReadAndValidateFwlFile();

#if !defined(USE_SHARED_CARDSTATE)
			// Get the latest CardState values.
			// This must be done to fixe the problem of adding or updating Expert firmware from CompuScopeManager. 
			// In this case, the firmware info (CardState) has been changed by CompuScopeManager. Then when Rm call this function,
			// Rm should get the new the firmware info by calling IoctlGetCardState.
			pCard->IoctlGetCardState(pCard->m_pCardState);
#endif
			if ( pCard->m_PlxData->InitStatus.Info.u32Nios && pCard->m_PlxData->InitStatus.Info.u32AddOnPresent )
			{
				pCard->ReadBaseBoardHardwareInfoFromFlash();
				pCard->ReadAddOnHardwareInfoFromEeprom();
				pCard->CheckRequiredFirmwareVersion();
			}

			pCard = pCard->m_HWONext;
		}

		pCard	= (CsSpiderDev*) m_ProcessGlblVars->pFirstUnCfg;
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

int32 CsSpiderDev::CsAcquisitionSystemInitialize( uInt32 u32InitFlag )
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

	CsSpiderDev* pDevice = this;
	if ( ! m_pSystem->bInitialized )
	{
		m_pSystem->bCrashRecovery = FALSE;
		// Open link to kernel driver for all cards in system
		i32Status = MsInitializeDevIoCtl();
		if ( CS_FAILED(i32Status) )
			return i32Status;

		if ( ! m_pCardState->bForceIndMode )
		{
			while ( pDevice )
			{
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
				CreateThreadAsynTransfer();

				m_pSystem->hThreadIntr = (HANDLE)::_beginthreadex(NULL, 0, ThreadFuncIntr, this, 0, (unsigned int *)&m_pSystem->u32ThreadIntrId);
				::SetThreadPriority(m_pSystem->hThreadIntr, THREAD_PRIORITY_HIGHEST);
		#else
				InitEventHandlers();
		#endif

				if ( 0 == m_u32SystemInitFlag )	
				{	
					// If m_u32SystemInitFlag == 0 then perform the default full initialization,
					// otherwise just take whatever it is from the current state
					MsResetDefaultParams();
				}
			}
		}

		pDevice = m_MasterIndependent;
		if ( m_pSystem->hAcqSystemCleanup )
			ResetEvent(m_pSystem->hAcqSystemCleanup); // Jan 20,2016 - because thread is now manual reset
		m_pCardState->u32ProcId = GetCurrentProcessId();
		while( pDevice )
		{
			pDevice->m_pCardState->u32ProcId = m_pCardState->u32ProcId;
			pDevice->UpdateCardState();
			pDevice = pDevice->m_Next;
		}

		m_PeevAdaptor.Initialize( GetIoctlHandle() );
		m_pSystem->bInitialized = TRUE;
		m_pSystem->BusyTransferCount = 0;
	}

	if ( CS_FAILED(i32Status) )
		CsAcquisitionSystemCleanup();

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

void	CsSpiderDev::CsAcquisitionSystemCleanup()
{
	// Set the reset flag in order to stop all pending processing
	m_pSystem->bReset = TRUE;

	CsAbort();

	if ( m_pAverageBuffer )
	{
		::GageFreeMemoryX( m_pAverageBuffer );
		m_pAverageBuffer = NULL;
	}

	if ( m_pCardState->bSpiderLP && m_i16LpXferBuffer )
	{
		::GageFreeMemoryX( m_i16LpXferBuffer );
		m_i16LpXferBuffer = NULL;
	}

	CsSpiderDev *pDevice = m_MasterIndependent;

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
	
	// Close Peev Adapter
	m_PeevAdaptor.Invalidate();

	// Undo register events
	if ( 0 == m_u32SystemInitFlag )
	{
		RegisterEventHandle( NULL, ACQ_EVENT_TRIGGERED );
		RegisterEventHandle( NULL, ACQ_EVENT_END_BUSY );
	}

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		// Close Handle to kernel driver
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
			if ( m_DataTransfer.hAsynThread )
				CloseHandle(m_DataTransfer.hAsynThread); // Added Jan 20, 2016
			if ( m_pSystem->hThreadIntr )
				CloseHandle(m_pSystem->hThreadIntr);	 // ADDED Jan. 20, 2016
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
		if ( m_DataTransfer.hAsynThread )
			CloseHandle(m_DataTransfer.hAsynThread); // Added Jan 20, 2016
		if ( m_pSystem->hThreadIntr )
			CloseHandle(m_pSystem->hThreadIntr);	 // ADDED Jan. 20, 2016
#endif 	//__linux__

		m_pSystem->hAcqSystemCleanup	=
		m_pSystem->hEventTriggered		=
		m_pSystem->hEventEndOfBusy		=
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

uInt32	CsSpiderDev::CsGetAcquisitionStatus()
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
					ControlSpiClock(true);
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
				ControlSpiClock(true);
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


int32	CsSpiderDev::CsGetParams( uInt32 u32ParamId, PVOID pOutBuffer, uInt32 u32OutSize )
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

	case CS_READ_EEPROM://----- Read Add-On Flash
		{
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

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
			GetBoardNode()->u32RawFwVersionB[0] = 0;
			ReadVersionInfo();
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

			GageZeroMemoryX( pSysInfoEx, sizeof(CSSYSTEMINFOEX) );
			pSysInfoEx->u32Size = sizeof(CSSYSTEMINFOEX);
			pSysInfoEx->u16DeviceId = GetDeviceId();

			CsSpiderDev *pDevice = m_MasterIndependent;

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
			PCS8XXXBOARDCAPS pCapsInfo = (PCS8XXXBOARDCAPS) pUserBuffer;
			FillOutBoardCaps(pCapsInfo);
		}
		break;

	case CS_MULREC_AVG_COUNT:
		*( (uInt32*) pUserBuffer ) = m_u32MulrecAvgCount; 
		break;

	case CS_TRIG_OUT:
		{
			if ( m_pCardState->bSpiderLP )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PCS_TRIG_OUT_STRUCT pTrigOut = (PCS_TRIG_OUT_STRUCT) pUserBuffer;

				pTrigOut->u16Valid = CS_OUT_NONE | CS_TRIGOUT;
				pTrigOut->u16ValidMode = CS_TRIGOUT_MODE;
				pTrigOut->u16Mode = CS_TRIGOUT_MODE;
				pTrigOut->u16Value = m_pCardState->bDisableTrigOut ? CS_OUT_NONE : CS_TRIGOUT;
			}
		}
		break;

	case CS_CLOCK_OUT:
		{
			if ( m_pCardState->bSpiderLP )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PCS_CLOCK_OUT_STRUCT pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;

				pClockOut->u16Value = (uInt16) GetClockOut();
				pClockOut->u16Valid = CS_OUT_NONE|CS_CLKOUT_SAMPLE|CS_CLKOUT_REF;
			}
		}
		break;

	case CS_READ_PCIeLINK_INFO:
		{
			if ( ! m_bNucleon )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PPCIeLINK_INFO pPciExInfo = (PPCIeLINK_INFO) pUserBuffer;
				i32Status = IoctlGetPciExLnkInfo( pPciExInfo );
			}
		}
		break;

	case CS_PEEVADAPTOR_CONFIG:
		{
			PPVADAPTORCONFIG pPvAdaptorConfig = (PPVADAPTORCONFIG) pUserBuffer;
			if ( 0 != pPvAdaptorConfig )
				pPvAdaptorConfig->u32RegVal = m_PeevAdaptor.GetSwConfig();
		}
		break;

	case CS_READ_GAGE_REGISTER:
		{
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
		}
		break;

	case CS_TRIGGERED_INFO:
		{
			PTRIGGERED_INFO_STRUCT pTrigInfo = (PTRIGGERED_INFO_STRUCT) pUserBuffer;

			if ( pTrigInfo->u32Size != sizeof(TRIGGERED_INFO_STRUCT) )
				return CS_INVALID_STRUCT_SIZE;

			if ( IsBadWritePtr( pTrigInfo->pBuffer, pTrigInfo->u32BufferSize ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pTrigInfo->u32NumOfSegments*sizeof(int16) > pTrigInfo->u32BufferSize )
				return CS_BUFFER_TOO_SMALL;
			else
			{
				i32Status = GetTriggerInfo( pTrigInfo->u32StartSegment, pTrigInfo->u32NumOfSegments, pTrigInfo->pBuffer );
				i32Status = i32Status;
			}
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

int32	CsSpiderDev::CsSetParams( uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize )
{
	PVOID			pUserBuffer = pInBuffer;
	int32			i32Status = CS_SUCCESS;
	CsSpiderDev		*pDevice = this;

	UNREFERENCED_PARAMETER(u32InSize);

	if ( ! m_pSystem->bInitialized )
		return  CS_SYSTEM_NOT_INITIALIZED;

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

			if ( u32Offset + u32BufferSize <= SPDR_FLASH_SIZE )
			{
				i32Status = WriteEepromEx(pAppBuffer, u32Offset, u32BufferSize);
				if ( CS_SUCCEEDED(i32Status) )
				{
					i32Status = ReadAddOnHardwareInfoFromEeprom(FALSE);
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
				RtlZeroMemory( GetBoardNode()->u32RawFwVersionB, sizeof(GetBoardNode()->u32RawFwVersionB) );
				RtlZeroMemory( GetBoardNode()->u32RawFwVersionA, sizeof(GetBoardNode()->u32RawFwVersionA) );

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
			pDevice = GetCardPointerFromBoardIndex( pSendDAC->u8CardNumber );

			if ( NULL != pDevice )
				pDevice->WriteToCalibDac( pSendDAC->u8DacAddress, pSendDAC->u16DacValue );
			else
				i32Status = CS_INVALID_CARD;
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
				m_u32MulrecAvgCount = u32MulrecAvgCount;
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
			if ( m_pCardState->bSpiderLP )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				PCS_CLOCK_OUT_STRUCT	pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;
				if ( m_pCardState->eclkoClockOut != (eClkOutMode)pClockOut->u16Value )
					i32Status = SetClockOut( (eClkOutMode)pClockOut->u16Value );
			}
		}
		break;

	case CS_SAVE_OUT_CFG:
		{
			SPDR_GENERAL_INFO_STRUCTURE	GeneralInfoStr = {0};

			uInt32	u32Offset = SPDR_FLASH_INFO_ADDR  + FIELD_OFFSET(SPDR_FLASH_STRUCT, Info);
			i32Status = ReadEeprom( &GeneralInfoStr, u32Offset, sizeof(GeneralInfoStr) );
			if ( CS_FAILED(i32Status) )
				break;

			if ( 0 != *((uInt32 *) pUserBuffer)  )
			{
				// Save out config
				GeneralInfoStr.u32ClkOut			= m_pCardState->eclkoClockOut;
				GeneralInfoStr.u32TrigOutEnabled	= ! m_pCardState->bDisableTrigOut;
			}
			else
			{
				// Clear out config
				GeneralInfoStr.u32ClkOut			= (uInt32)-1;
				GeneralInfoStr.u32TrigOutEnabled	= (uInt32)-1;

				m_pCardState->eclkoClockOut			= CSPDR_DEFAULT_CLOCK_OUT;
				m_pCardState->bDisableTrigOut			= TRUE;
			}

			i32Status = WriteEepromEx( &GeneralInfoStr, u32Offset, sizeof(GeneralInfoStr) );
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
						i32Status = pDevice->WriteRegisterThruNios( NiosCmd->u32DataIn, NiosCmd->u32Command, -1, &NiosCmd->u32DataOut );
					else
						i32Status = pDevice->WriteRegisterThruNios( NiosCmd->u32DataIn, NiosCmd->u32Command );
				}

				pDevice = pDevice->m_Next;
			}
		}
		break;

	case CS_TIMESTAMP_RESET:
		ResetTimeStamp();
		break;

	case CS_PEEVADAPTOR_CONFIG:
		{
			PPVADAPTORCONFIG pPvAdaptorConfig = (PPVADAPTORCONFIG) pUserBuffer;
			i32Status = m_PeevAdaptor.SetConfig(pPvAdaptorConfig->u32RegVal);
		}
		break;

	case CS_SPDR12_TRIM_TCAPS:
		{
			PCS_SPDR12_TRIM_TCAP_STRUCT pTcapStruct = (PCS_SPDR12_TRIM_TCAP_STRUCT) pUserBuffer;
			i32Status = Trim1M_TCAP( pTcapStruct->u16ChannelIndex, pTcapStruct->bTCap2Sel, pTcapStruct->u8TrimCount );
		}
		break;

	case CS_ADDON_RESET:
		if ( m_Next )
			ResetMasterSlave(true);
		else
			AddonReset();
		break;

	default:
		i32Status = CS_INVALID_PARAMS_ID;
		break;

	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsSpiderDev::CsStartAcquisition()
{
	m_pSystem->bEndAcquisitionNotification = FALSE;
	m_pSystem->u32TriggerNotification =  0;
	m_pSystem->u32TriggeredCount = 0;

	m_pSystem->u16AcqStatus  = ACQ_STATUS_WAIT_TRIGGER;

	// Reset notification flag before acquisition
	m_pTriggerMaster->m_PlxData->CsBoard.bBusy = true;
	m_pTriggerMaster->m_PlxData->CsBoard.bTriggered = false;

	m_PlxData->IntBaseMask = 0;

	if ( m_bMulrecAvgTD )
		m_PlxData->IntBaseMask |= MULREC_AVG_DONE;
	else if ( m_UseIntrBusy )
		m_PlxData->IntBaseMask |= N_MBUSY_REC_DONE;

	if ( m_UseIntrTrigger )
		m_PlxData->IntBaseMask |= MTRIG;

#if DBG
	m_pTriggerMaster->m_u32IntrCount = 0;
#endif

	in_STARTACQUISITION_PARAMS AcqParams = {0};

	AcqParams.u32IntrMask		= m_PlxData->IntBaseMask;		
	if ( m_Stream.bEnabled )
	{
		MsStmResetParams();

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

int32	CsSpiderDev::CsAbort()
{
	CsSpiderDev* pCard = m_MasterIndependent;

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
				if ( m_pSystem->BusyTransferCount > 0 )
				{
					// Abort all pending asynchronous data transfers
					CsSpiderDev *pDevice = this;
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
	}

	// Clear the interrupt register
	pCard = m_MasterIndependent;
	while ( pCard )
	{
		// Make sure to set back the SPI clock to ON
		pCard->ControlSpiClock(true);
		pCard->ClearInterrupts();
		pCard = pCard->m_Next;
	}

	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;

	return CS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
//

void	CsSpiderDev::CsForceTrigger()
{

	m_pTriggerMaster->ForceTrigger();

}

////////////////////////////////////////////////////////////////////////
//

int32	CsSpiderDev::CsDataTransfer( in_DATA_TRANSFER *InDataXfer )
{
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	if ( ! IsChannelIndexValid ( pInParams->u16Channel ) )
		return CS_INVALID_CHANNEL;

	if ( pInParams->u32Mode & TxMODE_TIMESTAMP )
	{
		// Convert the size of user Buffer from sizeof(char) to sizeof(int64)
		pInParams->i64Length = pInParams->i64Length / sizeof(int64);

		if ( pInParams->i64Length + pInParams->u32Segment - 1 > m_pCardState->AcqConfig.u32SegmentCount )
			pInParams->i64Length = m_pCardState->AcqConfig.u32SegmentCount - pInParams->u32Segment + 1;

		int32 i32Status = m_pTriggerMaster->ReadTriggerTimeStampEtb( pInParams->u32Segment - 1, (int64 *)pInParams->pDataBuffer, (uInt32) pInParams->i64Length );
		if ( CS_SUCCEEDED( i32Status) )
		{
			pOutParams->i64ActualStart	= pInParams->u32Segment;
			pOutParams->i64ActualLength = pInParams->i64Length;
			pOutParams->i32HighPart		= 0;
			pOutParams->i32LowPart		= (uInt32) GetTimeStampFrequency();
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
 			 u32Mode == TxMODE_DATA_INTERLEAVED )
		{
			if ( TxMODE_DATA_32 & u32Mode )
			{
				if ( !m_bMulrecAvgTD && !m_bSoftwareAveraging )
				{
					return CS_INVALID_TRANSFER_MODE;
				}
			}

			//---- Find the card that we need to setup for transfer
			CsSpiderDev* pDevice = GetCardPointerFromChannelIndex( pInParams->u16Channel );

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

int32 CsSpiderDev::CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt )
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

void	CsSpiderDev::CsSystemReset(BOOL bHard_reset)
{
	UNREFERENCED_PARAMETER(bHard_reset);
	CsAbort();

	CsSpiderDev *pCard = m_MasterIndependent;
	pCard = m_MasterIndependent;
	while (pCard)
	{
		if ( pCard->m_pCardState->u8ImageId != 0 )
		{
			pCard->CsLoadBaseBoardFpgaImage(0);
		}
		else
		{
			// Reset base board
			pCard->BaseBoardConfigurationReset(0);
		}

		// Reset Addon board
		pCard->AddonConfigReset();
		pCard->AddonReset();

		if ( m_bNucleon )
			pCard->UpdateBaseBoardMsStatus();

		pCard = pCard->m_Next;
	}

	MsResetDefaultParams( TRUE );

}

////////////////////////////////////////////////////////////////////////
//

int32	CsSpiderDev::CsForceCalibration()
{
	int32			i32Status = CS_SUCCESS;

	CsSpiderDev		*pDevice = m_MasterIndependent;
	uInt16			u16ChanZeroBased = 0;
	while ( pDevice )
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
	return i32Status;
}



////////////////////////////////////////////////////////////////////////
//

int32 CsSpiderDev::CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait )
{
	int32 i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(bWait);

	CsSpiderDev *pDevice = GetCardPointerFromChannelIndex( u16Channel );

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
void * CsSpiderDev::operator new (size_t  size)
{
	PVOID	p = new char[size];

	if ( p )
		memset(p, 0, size);

	return p;
}



#ifdef USE_SHARED_CARDSTATE

int32	CsSpiderDev::InitializeDevIoCtl()
{
	int32 i32Status = CsDevIoCtl::InitializeDevIoCtl();

	if ( CS_SUCCEEDED( i32Status ) )
	{
		i32Status = IoctlGetCardStatePointer( &m_pCardState );
		if ( CS_SUCCEEDED( i32Status ) )
		{
			if ( sizeof(SPIDER_CARDSTATE) == m_pCardState->u32Size  )
			{
				m_PlxData	= &m_pCardState->PlxData;
				return CS_SUCCESS;
			}
			else
			{
				::OutputDebugString("\nError SPIDER_CARDSTATE struct size discrepency !!");
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


#ifdef __linux__
#if 0
void CsSpiderDev::CreateInterruptEventThread()
{
	pthread_t 		thread_IntrEvent;
	pthread_attr_t	threadAttr;

	// Initialize the thread attribute
	::pthread_attr_init(&threadAttr);

	// Setup stacksize of the thread
	::pthread_attr_setstacksize(&threadAttr, 128*1024);

	// set thread to detached state
	::pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

	// create the thread
	::pthread_create(&thread_IntrEvent, &threadAttr, gThreadFuncIntr, this);

}
#endif
#endif

