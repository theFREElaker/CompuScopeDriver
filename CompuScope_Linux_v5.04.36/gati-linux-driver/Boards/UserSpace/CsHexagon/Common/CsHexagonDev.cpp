// CsHexagon.cpp
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "CsIoctl.h"
#include "CsDrvDefines.h"
#include "CsHexagon.h"

#ifdef _WINDOWS_
	#include <process.h>			// for _beginthreadex
	#include "CsMsgLog.h"
#endif


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsHexagon::CsHexagon(uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars):
	m_u8CardIndex(u8Index), m_ProcessGlblVars(ProcessGlblVars), m_PseudoHandle(pDevInfo->DrvHdl), m_DeviceId(pDevInfo->DeviceId)
{

	// Memory has been cleared on allocation
	// Only need to initialize member variables whose default value is different than 0
	m_pTriggerMaster	= this;
	m_MasterIndependent = this;
	m_CaptureMode		= MemoryMode;

	m_u32IgnoreCalibError	= 0;
	m_u32DefaultExtClkSkip  = CS_SAMPLESKIP_FACTOR;
	m_u16SkipCalibDelay		= 20;
	m_32SegmentTailBytes	= HEXAGON_HWMULREC_TAIL_BYTES;
	m_u32CalSrcSettleDelay  = 10;		// 0.1 ms unit == 1ms
	
	CsDevIoCtl::InitializeDevIoCtl( pDevInfo );

#if !defined( USE_SHARED_CARDSTATE )
	m_pCardState	= &m_CardState;
	m_PlxData		= &m_pCardState->PlxData;
#endif

	m_i32ExtTrigSensitivity = HEXAGON_DEFAULT_EXT_TRIG_SENSE;			// VC TBC
	m_u16TrigSensitivity	= HEXAGON_DEFAULT_TRIG_SENSITIVITY;

#ifdef __linux__
        // The default params not null must be defined.
	m_u32CommitTimeDelay 	= COMMIT_TIME_DELAY_MS;
	m_u32GainTolerance_lr 	= DEFAULT_GAIN_TOLERANCE_LR;
#endif
	RtlZeroMemory( &m_BaseBoardCsiEntry, sizeof( m_BaseBoardCsiEntry ) );

	// Set the correct fpga image file name and fwl file
	sprintf_s( m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", CSHEXAGON_FPGAIMAGE_FILENAME );
	sprintf_s( m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", CSHEXAGON_FPGAIMAGE_FILENAME );

}


////////////////////////////////////////////////////////////////////////
//

int32	CsHexagon::CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound )
{
	int32 i32Status = CS_SUCCESS;

	CsHexagon* pCard	= (CsHexagon*) m_ProcessGlblVars->pFirstUnCfg;
	
	// Remember the ForceInd flag for use later on Acqusition system initialization
	while (pCard)
	{
		pCard->m_pCardState->bForceIndMode = (BOOLEAN)bForceInd;
		pCard = pCard->m_HWONext;
	}

	if (( m_ProcessGlblVars->NbSystem == 0 ) && ! bForceInd )
	{
		pCard	= (CsHexagon*) m_ProcessGlblVars->pFirstUnCfg;
		while (pCard)
		{
			if ( pCard->m_PlxData->InitStatus.Info.u32Nios && pCard->m_PlxData->InitStatus.Info.u32AddOnPresent )
			{
				pCard->ReadHardwareInfoFromFlash();
			}

			pCard = pCard->m_HWONext;
		}

		pCard	= (CsHexagon*) m_ProcessGlblVars->pFirstUnCfg;
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

int32 CsHexagon::CsAcquisitionSystemInitialize( uInt32 u32InitFlag )
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

		CsHexagon* pDevice = this;
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

			if ( ! m_pCardState->bForceIndMode )
			{
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

			if ( CS_SUCCEEDED(i32Status) )
			{
				pDevice = m_MasterIndependent;
				m_pCardState->u32ProcId = GetCurrentProcessId();
				while( pDevice )
				{
                    pDevice->GetCardState();
					pDevice->m_pCardState->u32ProcId = m_pCardState->u32ProcId;
                    pDevice->UpdateCardState();
					pDevice = pDevice->m_Next;
				}

				m_pSystem->bInitialized = TRUE;
				m_pSystem->BusyTransferCount = 0;
			}
			if ( m_pSystem->hAcqSystemCleanup )
				ResetEvent(m_pSystem->hAcqSystemCleanup); // Jan 20,2016 - because thread is now manual reset
		}

		if ( CS_FAILED(i32Status) )
			CsAcquisitionSystemCleanup();
	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

void	CsHexagon::CsAcquisitionSystemCleanup()
{
	// Set the reset flag in order to stop all pending processing
	m_pSystem->bReset = TRUE;

	CsAbort();

	if ( m_pAverageBuffer )
	{
		::GageFreeMemoryX( m_pAverageBuffer );
		m_pAverageBuffer = NULL;
	}
	
	CsHexagon *pDevice = m_MasterIndependent;

	// Undo register events
	RegisterEventHandle( NULL, ACQ_EVENT_TRIGGERED );
	RegisterEventHandle( NULL, ACQ_EVENT_END_BUSY );

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		pDevice->SetDefaultInterrutps();
  		pDevice->GetCardState();
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

   // Terminate the thread waiting for events
   ::SetEvent(m_pSystem->hAcqSystemCleanup);

   // Close acquisition events
   CloseHandle(m_pSystem->hAcqSystemCleanup);
   CloseHandle(m_pSystem->hEventTriggered);
   CloseHandle(m_pSystem->hEventEndOfBusy);
   CloseHandle(m_DataTransfer.hAsynThread); // Added Jan 20, 2016
   CloseHandle(m_pSystem->hThreadIntr);	 // ADDED Jan. 20, 2016

#endif
	m_pSystem->hAcqSystemCleanup	=
	m_pSystem->hEventTriggered		=
	m_pSystem->hEventEndOfBusy		=
	m_pSystem->hThreadIntr			= 
	m_DataTransfer.hAsynThread		= 0;
	m_UseIntrBusy = m_UseIntrTrigger = 0;

	// Reset system to default state
	m_pSystem->bInitialized = FALSE;
	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;
}


////////////////////////////////////////////////////////////////////////
//

uInt32	CsHexagon::CsGetAcquisitionStatus()
{

	switch ( m_pSystem->u16AcqStatus )//----- start from the previous known state
	{
		case ACQ_STATUS_READY: //----- no busy, no trigger => wait for busy
			m_pSystem->u16AcqStatus = ACQ_STATUS_READY;//----- no change
			break;

		case ACQ_STATUS_WAIT_TRIGGER://----- busy
			m_pSystem->u16AcqStatus = ACQ_STATUS_TRIGGERED;	//----- busy & trigger => capture

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


int32	CsHexagon::CsGetParams( uInt32 u32ParamId, PVOID pOutBuffer, uInt32 u32OutSize )
{
	PVOID			pUserBuffer = pOutBuffer;
	int32			i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(u32OutSize);

	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;

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


	case CS_READ_EEPROM:
	case CS_READ_FLASH:
		{
			PCS_GENERIC_FLASH_READWRITE pGenericRw = (PCS_GENERIC_FLASH_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

			i32Status = IoctlReadFlash( u32Offset, pAppBuffer, u32BufferSize );
		}
		break;

	case CS_READ_NVRAM: //---- Read NvRam (Boot Eeprom)
		{
			// check buffer for Read access
			PCS_PLX_NVRAM_RW_STRUCT pNVRAM_Struct = (PCS_PLX_NVRAM_RW_STRUCT) pUserBuffer;
			
			if ( pNVRAM_Struct->u32Size != sizeof( CS_PLX_NVRAM_RW_STRUCT ) )
				return CS_INVALID_STRUCT_SIZE;

			if ( IsBadWritePtr( &pNVRAM_Struct->PlxNvRam, sizeof(PLX_NVRAM_IMAGE) ) )
				return CS_INVALID_POINTER_BUFFER;

			i32Status = CsReadPlxNvRAM( &pNVRAM_Struct->PlxNvRam );
		}
		break;

	case CS_READ_VER_INFO:
		{
			// check buffer for Read access
			PCS_FW_VER_INFO pInfo_Struct = (PCS_FW_VER_INFO) pUserBuffer;

			if ( IsBadWritePtr( pUserBuffer, sizeof(CS_FW_VER_INFO) ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pInfo_Struct->u32Size != sizeof( CS_FW_VER_INFO ) )
				return CS_INVALID_STRUCT_SIZE;

			*pInfo_Struct = m_pCardState->VerInfo;
		}
		break;

	case CS_EXTENDED_BOARD_OPTIONS:
		{
			uInt32 *pExOptions = (uInt32 *) pUserBuffer;

			pExOptions[0] = pExOptions[1] = 0;
			
			// NEW_BOOT_FW
			// Do not return Expert when the FW has been changed and the system has not yet reboot
			if ( ! m_pCardState->bNeedFullShutdown )
			{
				if ( 2 == m_pCardState->i32CurrentBootLocation || 
					 3 == m_pCardState->i32CurrentBootLocation )
				{
					// We are doing this because we try to keep backward compatible with the OLD scheme of Expert firmwares
					// The card was booted with the Expert firmware. Return only the current active expert firmware and it locations
					if ( 2 == m_pCardState->i32CurrentBootLocation )
						pExOptions[0] = m_PlxData->CsBoard.u32BaseBoardOptions[2] & HEXAGON_VALID_EXPERT_FW;		// Expert 1
					else 
						pExOptions[1] = m_PlxData->CsBoard.u32BaseBoardOptions[3] & HEXAGON_VALID_EXPERT_FW;		// Expert 2
				}
			}
		}
		break;

	case CS_SYSTEMINFO_EX:
		{
			PCSSYSTEMINFOEX pSysInfoEx = (PCSSYSTEMINFOEX) pUserBuffer;

			uInt32	i, j;

			RtlZeroMemory( pSysInfoEx, sizeof(CSSYSTEMINFOEX) );
			pSysInfoEx->u32Size = sizeof(CSSYSTEMINFOEX);
			pSysInfoEx->u16DeviceId = GetDeviceId();

			CsHexagon *pDevice = m_MasterIndependent;

			// Permission and firmware versions from the master card
            pSysInfoEx->i64ExpertPermissions = pDevice->GetBoardNode()->i64ExpertPermissions;
			pSysInfoEx->u32BaseBoardHwOptions = pDevice->GetBoardNode()->u32BaseBoardHardwareOptions;
			pSysInfoEx->u32AddonHwOptions = pDevice->GetBoardNode()->u32AddonHardwareOptions;

			for ( i = REGULAR_INDEX_OFFSET, j = 0; i < sizeof(GetBoardNode()->u32RawFwVersionB) /sizeof(uInt32) ; i++, j++)
			{
				pSysInfoEx->u32BaseBoardFwOptions[j]			= GetBoardNode()->u32BaseBoardOptions[i];
				pSysInfoEx->u32AddonFwOptions[j]				= GetBoardNode()->u32AddonOptions[i];
				pSysInfoEx->u32BaseBoardFwVersions[j].u32Reg	= GetBoardNode()->u32UserFwVersionB[i].u32Reg;
				pSysInfoEx->u32AddonFwVersions[j].u32Reg		= GetBoardNode()->u32UserFwVersionA[i].u32Reg;
			}

			pDevice = pDevice->m_Next;

			// System permission will be "AND" persmissions of all Master/Slave cards
			while ( pDevice )
			{
				// i starts from 1 because the image 0 is not an expertt option
				for ( i = REGULAR_INDEX_OFFSET, j = 0; i < sizeof(GetBoardNode()->u32RawFwVersionB) /sizeof(uInt32) ; i++, j++)
				{
					pSysInfoEx->u32BaseBoardFwOptions[j]	&= pDevice->GetBoardNode()->u32BaseBoardOptions[i];
					pSysInfoEx->u32AddonFwOptions[j]		&= pDevice->GetBoardNode()->u32AddonOptions[i];

					if ( 0 == pSysInfoEx->u32BaseBoardFwOptions[j] )
						pSysInfoEx->u32BaseBoardFwVersions[j].u32Reg = 0;

					if ( 0 == pSysInfoEx->u32AddonFwOptions[j] )
						pSysInfoEx->u32AddonFwVersions[j].u32Reg = 0;
				}

				pSysInfoEx->i64ExpertPermissions	&= pDevice->GetBoardNode()->i64ExpertPermissions;
				pSysInfoEx->u32AddonHwOptions		&= pDevice->GetBoardNode()->u32AddonHardwareOptions;
				pSysInfoEx->u32BaseBoardHwOptions	&= pDevice->GetBoardNode()->u32BaseBoardHardwareOptions;
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
			PCSHEXAGON_BOARDCAPS pCapsInfo = (PCSHEXAGON_BOARDCAPS) pUserBuffer;
			FillOutBoardCaps(pCapsInfo);
		}
		break;

	case CS_CLOCK_OUT:
		{
			PCS_CLOCK_OUT_STRUCT pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;

			pClockOut->u16Value = (uInt16) GetClockOut();
			pClockOut->u16Valid = CS_OUT_NONE | CS_CLKOUT_REF;
		}
		break;

	case CS_TRIG_OUT:
		{
			PCS_TRIG_OUT_STRUCT pTrigOut = (PCS_TRIG_OUT_STRUCT) pUserBuffer;

			pTrigOut->u16Valid		= CS_OUT_NONE | CS_TRIGOUT;
			pTrigOut->u16ValidMode	= CS_TRIGOUT_MODE;
			pTrigOut->u16Mode		= CS_TRIGOUT_MODE;
			pTrigOut->u16Value		= m_pCardState->bDisableTrigOut ? CS_OUT_NONE : CS_TRIGOUT;
		}
		break;

	case CS_TRIG_ENABLE:
		{
			PCS_TRIG_ENABLE_STRUCT pExtTrigEn = (PCS_TRIG_ENABLE_STRUCT) pUserBuffer;
			pExtTrigEn->u16Value =(uInt16)  GetExtTrigEnable();
		}
		break;

	case CS_READ_PCIeLINK_INFO:
		{
			CsHexagon	*pDevice = m_MasterIndependent;
			PPCIeLINK_INFO	pPciExInfo = (PPCIeLINK_INFO) pUserBuffer;

			if ( 0 != pDevice->m_Next )
				pDevice = GetCardPointerFromBoardIndex( pPciExInfo->u16CardIndex );

			if ( NULL == pDevice )			
				i32Status = CS_INVALID_CARD;
			else
				i32Status = pDevice->IoctlGetPciExLnkInfo( pPciExInfo );
		}
		break;


	case CS_READ_GAGE_REGISTER:
		{
			PCS_RW_GAGE_REGISTER_STRUCT		pReadRegister = (PCS_RW_GAGE_REGISTER_STRUCT) pUserBuffer;
			CsHexagon*					pDevice = GetCardPointerFromBoardIndex( pReadRegister->u16CardIndex );

			if ( NULL != pDevice )
				pReadRegister->u32Data = pDevice->ReadGageRegister( pReadRegister->u32Offset );
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_READ_PCI_REGISTER:
		{
			PCS_RW_GAGE_REGISTER_STRUCT		pReadRegister = (PCS_RW_GAGE_REGISTER_STRUCT) pUserBuffer;
			CsHexagon*					pDevice = GetCardPointerFromBoardIndex( pReadRegister->u16CardIndex );

			if ( NULL != pDevice )
				pReadRegister->u32Data = pDevice->ReadPciRegister( pReadRegister->u32Offset );
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_MULREC_AVG_COUNT:
		*( (uInt32*) pUserBuffer ) = m_u32MulrecAvgCount; 
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

	case CS_USE_DACCALIBTABLE:
	case CS_DEFAULTUSE_DACCALIBTABLE:
		{
			i32Status = CS_FUNCTION_NOT_SUPPORTED;
		}
		break;

	case CS_CURRENT_BOOT_LOCATION:
		// -1 becasue we want to convert to the application point of view
		*( (int32*) pUserBuffer ) = m_pCardState->i32CurrentBootLocation-1; 
		break;

	case CS_NEXT_BOOT_LOCATION:
		// -1 becasue we want to convert to the application point of view
		*( (int32*) pUserBuffer ) = GetConfigBootLocation()-1; 
		break;

	case CS_CURRENT_FW_VERSION:
		{
			if ( IsBadWritePtr( pUserBuffer, sizeof(FIRMWARE_VERSION_STRUCT) ) )
				i32Status = CS_INVALID_POINTER_BUFFER;
			else
			{
				PFIRMWARE_VERSION_STRUCT pVer = (PFIRMWARE_VERSION_STRUCT) pUserBuffer;
				int	i	= m_pCardState->i32CurrentBootLocation;

				pVer->BaseBoard.u32Reg	= m_PlxData->CsBoard.u32UserFwVersionB[i].u32Reg;
				pVer->AddonBoard.u32Reg	= m_PlxData->CsBoard.u32UserFwVersionA[0].u32Reg;
			}			
		}
		break;

	case CS_FLASH_SECTOR_SIZE:
		if ( IsBadWritePtr( pUserBuffer, sizeof(uInt32) ) )
			i32Status = CS_INVALID_POINTER_BUFFER;
		else
		{
			*( (int32*) pUserBuffer ) = HEXAGON_FLASH_SECTOR_SIZE; 
		}			
		break;
	
	case CS_FFT_CONFIG:
		{
			if ( ! IsBadWritePtr( pUserBuffer, sizeof(CS_FFT_CONFIG_PARAMS) ) )
				memcpy( pUserBuffer, &m_FFTConfig, sizeof( CS_FFT_CONFIG_PARAMS ));
			else
				i32Status = CS_INVALID_POINTER_BUFFER;
		}
		break;

	case CS_CAPTURE_MODE:
		if ( IsBadWritePtr( pUserBuffer, sizeof(CsCaptureMode) ) )
			i32Status = CS_INVALID_POINTER_BUFFER;
		else
		{
			*( (CsCaptureMode*) pUserBuffer ) = m_CaptureMode; 
		}			
		break;

#ifdef _WINDOWS_      
   // VC temp removed 
	//!For Debug:  Flash operation READ Value
	case CS_FLASH_OP_READ_VAL:
		if ( IsBadWritePtr( pUserBuffer, sizeof(CS_RW_GAGE_REGISTER_STRUCT) ) )
			i32Status = CS_INVALID_POINTER_BUFFER;
		else
		{
			PCS_RW_GAGE_REGISTER_STRUCT	pTemp = (PCS_RW_GAGE_REGISTER_STRUCT)(pUserBuffer);
			pTemp->u32Data = IoctlFlashOpRead(pTemp->u32Offset);
		}			
		break;
#endif

	default:
		i32Status = CS_INVALID_PARAMS_ID;
		break;

	}

	return i32Status;

}


////////////////////////////////////////////////////////////////////////
//

int32	CsHexagon::CsSetParams( uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize )
{
	PVOID			pUserBuffer = pInBuffer;
	int32			i32Status = CS_SUCCESS;
	CsHexagon		*pDevice = this;

	UNREFERENCED_PARAMETER(u32InSize);
	GetCardState();

	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;

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


	case CS_WRITE_EEPROM:	//----- Write Add-On Flash
	case CS_WRITE_FLASH:	//----- Write Base Board Flash
		{
			PCS_GENERIC_FLASH_READWRITE pGenericRw = (PCS_GENERIC_FLASH_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

			if ( pGenericRw->u32BufferSize !=0 && pGenericRw->pBuffer != 0 )
			{
				if ( u32Offset + u32BufferSize <= HEXAGON_FLASH_SIZE )
					i32Status = WriteBaseBoardFlash(u32Offset, pAppBuffer, u32BufferSize, true );
				else
					i32Status = CS_INVALID_POINTER_BUFFER;
			}
			else
			{
				// The Hardware info from flash has been updated. Reload the Hardware Info
				m_PlxData->CsBoard.i64ExpertPermissions = 0;
				RtlZeroMemory( m_PlxData->CsBoard.u32RawFwVersionB, sizeof( m_PlxData->CsBoard.u32RawFwVersionB ) );
				RtlZeroMemory( m_PlxData->CsBoard.u32RawFwVersionA, sizeof( m_PlxData->CsBoard.u32RawFwVersionA ) );
				
				// Force calibration next time 
				ResetCachedState();
				DeviceInitialize(true);
			}
		}
		break;


	case CS_WRITE_NVRAM: //---- Write NvRam (Boot Eeprom)
		{
			// check buffer for Read access
			PCS_PLX_NVRAM_RW_STRUCT pNVRAM_Struct = (PCS_PLX_NVRAM_RW_STRUCT) pUserBuffer;
			
			if ( pNVRAM_Struct->u32Size != sizeof( CS_PLX_NVRAM_RW_STRUCT ) )
			{
				i32Status = CS_INVALID_STRUCT_SIZE;
				break;
			}

			if ( IsBadReadPtr( &pNVRAM_Struct->PlxNvRam, sizeof(PLX_NVRAM_IMAGE) ) )
			{
				i32Status = CS_INVALID_POINTER_BUFFER;
				break;
			}

			i32Status = CsWritePlxNvRam( &pNVRAM_Struct->PlxNvRam );
			i32Status = ReadHardwareInfoFromFlash();
		}
		break;


	case CS_SELF_TEST_MODE:
		{
			PCS_SELF_TEST_STRUCT	pSelf = (PCS_SELF_TEST_STRUCT) pUserBuffer;
			i32Status = WriteToSelfTestRegister( pSelf->u16Value );
		}
		break;

	case CS_CLOCK_OUT:
		{
			PCS_CLOCK_OUT_STRUCT	pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;
			i32Status = SetClockOut( (eClkOutMode)pClockOut->u16Value );
		}
		break;

	case CS_TRIG_OUT:
		{
			PCS_TRIG_OUT_STRUCT	pTrigOut = (PCS_TRIG_OUT_STRUCT) pUserBuffer;
			i32Status = SetTrigOut( pTrigOut->u16Value );
		}
		break;

	case CS_SEND_DAC: //---- Send Write Command to DAC
	case CS_USE_DACCALIBTABLE:
	case CS_DEFAULTUSE_DACCALIBTABLE:
		{
			i32Status = CS_FUNCTION_NOT_SUPPORTED;
		}
		break;

	case CS_TIMESTAMP_RESET:
		ResetTimeStamp();
		Sleep(500);
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

	case CS_MULREC_AVG_COUNT:
		{
			// This setting should be done before ACTION_COMMIT
			// because m_u32MulrecAvgCount will be used in SendSegmentSetting() for Expert AVG
			uInt32	u32MulrecAvgCount = *( (uInt32*) pUserBuffer ); 
			if ( u32MulrecAvgCount < 1 || u32MulrecAvgCount > HEXAGON_MAX_NUMBER_OF_AVERAGE  )
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

	case CS_WRITE_GAGE_REGISTER:
		{
			PCS_RW_GAGE_REGISTER_STRUCT		pWriteRegister = (PCS_RW_GAGE_REGISTER_STRUCT) pUserBuffer;
			pDevice = GetCardPointerFromBoardIndex( pWriteRegister->u16CardIndex );
			uInt16 u16DbgMenu = pWriteRegister->u32Offset & HEXAGON_DBG_INFOS_CMD_MASK;
			
			if ( NULL != pDevice )
			{
				// These functions supported by CsTest+ which is still used PCS_RD_GAGE_REGISTER_STRUCT
				PCS_RW_GAGE_REGISTER_STRUCT pWriteReg = (PCS_RW_GAGE_REGISTER_STRUCT) pUserBuffer;

				// for debugging only FFxx
				if (u16DbgMenu) 
				{
					uInt32 Options = pWriteReg->u32Offset & 0xFF;
					uInt32 Params = pWriteReg->u32Data;

					if (u16DbgMenu == HEXAGON_DBG_INFOS_CMD)
						DbgMain(Options, Params);
					else if (u16DbgMenu == HEXAGON_DBG_ADV_INFOS_CMD)
						DbgAdvMain(Options, Params);
				}
				else
					i32Status = pDevice->WriteGageRegister( pWriteRegister->u32Offset, pWriteRegister->u32Data );
			}
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_WRITE_PCI_REGISTER:
		{
			PCS_RW_GAGE_REGISTER_STRUCT		pWriteRegister = (PCS_RW_GAGE_REGISTER_STRUCT) pUserBuffer;
			pDevice = GetCardPointerFromBoardIndex( pWriteRegister->u16CardIndex );

			if ( NULL != pDevice )
				i32Status = pDevice->WritePciRegister( pWriteRegister->u32Offset, pWriteRegister->u32Data );
			else
				i32Status = CS_INVALID_CARD;
		}
		break;

	case CS_ADDON_RESET:
		AddonReset();
		break;

	case CS_NEXT_BOOT_LOCATION:
		{
			if ( IsBadReadPtr( pUserBuffer, sizeof(int32) ) )
				i32Status = CS_INVALID_POINTER_BUFFER;
			else
			{
				// +1 becasue we want to convert to the driver boot location
				while( pDevice )
				{
					int32 i32DrvBootLocation = *( (int32*) pUserBuffer ) + 1;
					i32Status = pDevice->SetConfigBootLocation( i32DrvBootLocation );
					if ( CS_FAILED(i32Status) )
						break;
					pDevice = pDevice->m_Next;
				}
			}
		}
		break;

	case CS_IDENTIFY_LED:
		{
			// Flash the "Identify" LED for about 5 seconds
			FlashIdentifyLED(true);			// Flash the LED
			Sleep(5000);
			FlashIdentifyLED(false);		// turn off the LED
		}
		break;

	case CS_FFT_CONFIG:
		{
			if ( ! IsBadReadPtr( pUserBuffer, sizeof(CS_FFT_CONFIG_PARAMS) ) )
			{
				CS_FFT_CONFIG_PARAMS *pFft = (PCS_FFT_CONFIG_PARAMS) pUserBuffer;
				pDevice = this;
				while (pDevice)
				{
					pDevice->CsCopyFFTparams( pFft );
					pDevice = pDevice->m_Next;
				}
			}
			else
				i32Status = CS_INVALID_POINTER_BUFFER;
		}
		break;

		case CS_FFTWINDOW_CONFIG:
		{
			if ( ! IsBadReadPtr( pUserBuffer, sizeof(uInt32) ) )
			{
				CS_FFTWINDOW_CONFIG_PARAMS	FFTwinParams = { 0 };
				uInt16	*pu16UserBuffer = (uInt16	*) pUserBuffer;
				uInt16	u16WindSize = *pu16UserBuffer;

				pu16UserBuffer++;
				if ( ! IsBadReadPtr( pu16UserBuffer, u16WindSize*sizeof(FFTwinParams.i16Coefficients[0]) ) )
				{
					pDevice = this;
					while (pDevice)
					{
						pDevice->CsCopyFFTWindowParams( u16WindSize, (int16 *)pu16UserBuffer );
						pDevice = pDevice->m_Next;
					}
				}
				else
					i32Status = CS_INVALID_POINTER_BUFFER;
			}
			else
				i32Status = CS_INVALID_POINTER_BUFFER;

		}
		break;

	case CS_CAPTURE_MODE:
		if ( IsBadReadPtr( pUserBuffer, sizeof(CsCaptureMode) ) )
			i32Status = CS_INVALID_POINTER_BUFFER;
		else
		{
			m_CaptureMode		= *( (CsCaptureMode*) pUserBuffer);
		}			
		break;

#ifdef _WINDOWS_      
   // VC temp removed 
	//!For Debug:  Flash operation WRITE Value
	case CS_FLASH_OP_WRITE_VAL:
		if ( IsBadReadPtr( pUserBuffer, sizeof(CS_RW_GAGE_REGISTER_STRUCT) ) )
			i32Status = CS_INVALID_POINTER_BUFFER;
		else
		{
			PCS_RW_GAGE_REGISTER_STRUCT	pTemp = (PCS_RW_GAGE_REGISTER_STRUCT)(pUserBuffer);
			i32Status = IoctlFlashOpWrite(pTemp->u32Offset, pTemp->u32Data );
		}			
		break;
#endif

	//!For Debug: Write a buffer to flash
	case CS_FLASH_PROG_BUFFER:
		break;

	default:
		i32Status = CS_INVALID_PARAMS_ID;
		break;
	}

	UpdateCardState();

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsHexagon::CsStartAcquisition()
{
	int32	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;

	i32Status = CheckOverVoltage();
	if ( CS_FAILED(i32Status) )
		return i32Status;

	if (m_pCardState->bMultiSegmentTransfer)
	{
		// Restore the acquisition settings if the multi segments transfer has been
		// used before
		i32Status = SetupForReadFullMemory(false);
		if (CS_FAILED(i32Status))
			return i32Status;
	}

	m_pSystem->bEndAcquisitionNotification = FALSE;
	m_pSystem->u32TriggerNotification =  0;
	m_pSystem->u32TriggeredCount = 0;

	m_pSystem->u16AcqStatus  = ACQ_STATUS_WAIT_TRIGGER;

	// Reset notification flag before acquisition
	m_pTriggerMaster->GetBoardNode()->bBusy = true;
	m_pTriggerMaster->GetBoardNode()->bTriggered = false;

	if ( m_Stream.bEnabled )
		MsStmResetParams();

	m_MasterIndependent->m_pCardState->u32ActualSegmentCount = m_pCardState->AcqConfig.u32SegmentCount;

	uInt32	u32IntBaseMask = 0;

	if ( m_UseIntrBusy )
		u32IntBaseMask |= m_bMulrecAvgTD ? MULREC_AVG_DONE_INTR : ACQUISITION_DONE_INTR;

	if ( m_UseIntrTrigger )
		u32IntBaseMask |= m_bMulrecAvgTD ? FIRST_TRIGGER_AVG_INTR : FIRST_TRIGGER_INTR;

	u32IntBaseMask |= OVER_VOLTAGE_INTR;

	AcquireData( u32IntBaseMask );

	return CS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
//

int32	CsHexagon::CsAbort()
{
	CsHexagon *pDevice = m_MasterIndependent;

	if ( pDevice->m_Stream.bEnabled )
	{
		while ( pDevice )
		{
			if ( pDevice->m_Stream.u32BusyTransferCount > 0 )
			{
				pDevice->IoctlAbortAsyncTransfer();
				// Yeild the remaining time slice to the thread that waits for DMA complete if there is any
				Sleep(0);
				// If there is no thread that waits for DMA complete then we have to call this to reset the internal state
				pDevice->CsStmGetTransferStatus( 10, NULL );
			}

			// Abort and Reset Stream
			pDevice->IoctlStmSetStreamMode(-1);
			pDevice = pDevice->m_Next;
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
					GetBoardNode()->bTriggered = true;
					GetBoardNode()->bBusy = false;
					MsAbort();
					m_MasterIndependent->m_pCardState->u32ActualSegmentCount = ReadRegisterFromNios( 0, CSPLXBASE_READ_REC_COUNT_CMD ) & PLX_REC_COUNT_MASK;	

					m_pTriggerMaster->SignalEvents();
				}
				break;

			default:
			{
				if ( m_pSystem->BusyTransferCount > 0 )
				{
					// Abort all pending asynchronous data transfers
					pDevice = this;
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
	CsHexagon* pCard = m_MasterIndependent;

	while ( pCard )
	{
		pCard->SetDefaultInterrutps();
		pCard = pCard->m_Next;
	}

	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;

	return CS_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
//

void	CsHexagon::CsForceTrigger()
{

	m_pTriggerMaster->ForceTrigger();

}

////////////////////////////////////////////////////////////////////////
//

int32	CsHexagon::CsDataTransfer( in_DATA_TRANSFER *InDataXfer )
{
	int32						i32Status = CS_SUCCESS;
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;
	
	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;



	// Validation of pointers for returned values
	if (IsBadWritePtr(pOutParams, sizeof(OUT_PARAMS_TRANSFERDATA)))
		return CS_INVALID_POINTER_BUFFER;

	// Validation of Segment
	if ((pInParams->u32Segment == 0) || (pInParams->u32Segment > m_pCardState->AcqConfig.u32SegmentCount))
		return CS_INVALID_SEGMENT;

	if (pInParams->u32Mode & TxMODE_TIMESTAMP)
		return (i32Status = CsTransferTimeStamp(InDataXfer));

	// Validation of channel Index
	if ((pInParams->u16Channel > m_PlxData->CsBoard.NbAnalogChannel) ||
		((m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE) && (pInParams->u16Channel % 2 == 0)))
			return CS_INVALID_CHANNEL;

	if (pInParams->u32Mode & TxMODE_MULTISEGMENTS)
		i32Status = CsMemoryTransfer(InDataXfer);
	else
	{
		uInt32 u32Mode = pInParams->u32Mode & ~TxMODE_SLAVE;

		if (u32Mode == TxMODE_DATA_ANALOGONLY ||
			u32Mode == TxMODE_DATA_16 ||
			u32Mode == TxMODE_DATA_32 ||
			u32Mode == TxMODE_DATA_ONLYDIGITAL ||
			u32Mode == TxMODE_DATA_FFT ||
			u32Mode == TxMODE_DEBUG_DISP)
		{

			if (TxMODE_DATA_32 & u32Mode)
			{
				if (!m_bMulrecAvgTD && !m_bSoftwareAveraging)
				{
					return CS_INVALID_TRANSFER_MODE;
				}
				// Converted to bytes count
				pInParams->i64Length *= sizeof(uInt32);
			}
			else if (TxMODE_DATA_64 & u32Mode)
			{
				// Converted to bytes count
				pInParams->i64Length *= sizeof(int64);
			}
			else
			{
				// Converted to bytes count
				pInParams->i64Length *= sizeof(uInt16);
			}

			// Validation for the Length of transfer
			// The length should be multiple of 4 bytes
			if (pInParams->i64Length < 4 || pInParams->i64Length % 4 != 0)
				return	CS_INVALID_LENGTH;

			// Validation for the StartAddress
			if (pInParams->i64StartAddress >  m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerDelay)
				return	CS_INVALID_START;

			//---- Find the card that we need to setup for transfer
			CsHexagon* pDevice = GetCardPointerFromChannelIndex(pInParams->u16Channel);

			if (pDevice)
			{
				// Check resources required for asynchronous transfer
				if (0 != pInParams->hNotifyEvent && (0 == pDevice->m_DataTransfer.hAsynEvent || 0 == m_DataTransfer.hAsynThread))
					return CS_INSUFFICIENT_RESOURCES;

				if (InterlockedCompareExchange(&pDevice->m_DataTransfer.BusyTransferCount, 1, 0))
				{
					// A previous asynchronous data transfer has not been completed yet
					// Return error. Cannot process this request now.
					return  CS_DRIVER_ASYNC_REQUEST_BUSY;
				}
				else
				{
					// Set flag for slave transfer
					pDevice->m_DataTransfer.bUseSlaveTransfer = pInParams->u32Mode & TxMODE_SLAVE;

					if (m_bSoftwareAveraging)
						return pDevice->ProcessDataTransferModeAveraging(InDataXfer);
					else if (u32Mode == TxMODE_DEBUG_DISP)
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

	return i32Status;
}

////////////////////////////////////////////////////////////////////////
//

int32	CsHexagon::CsMemoryTransfer(in_DATA_TRANSFER *InDataXfer)
{
	int32 i32Status = CsCheckPowerState();
	if (CS_FAILED(i32Status))
		return i32Status;

	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;

	pInParams = &InDataXfer->InParams;
	pOutParams = InDataXfer->OutParams;

	// Prepare for data transfer
	in_PREPARE_DATATRANSFER	 InXferEx = { 0 };
	out_PREPARE_DATATRANSFER OutXferEx;

	// Validation of startAddress
	if (pInParams->i64StartAddress % (HEXAGON_DEPTH_INCREMENT / (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE)))
		return CS_INVALID_START;

	// Remember the current segment settinngs
	m_OldAcqConfig = m_pCardState->AcqConfig;

	// Calculate the transfer start address relative to the beginning of the segment
	// The quare maximim pre trigger is small, so it is safe to use 32-bit variable here
	int64 i64PreTrigger = (m_OldAcqConfig.i64SegmentSize - m_OldAcqConfig.i64Depth);
	int64 i64Offset = i64PreTrigger - m_OldAcqConfig.i64TriggerDelay + pInParams->i64StartAddress;

	// Change the segment settings so that we can read accorss multiple segments
	// Set as one single segment
	if (!m_pCardState->bMultiSegmentTransfer)
	{
		i32Status = SetupForReadFullMemory(true);
		if (CS_FAILED(i32Status))
			return i32Status;
	}

	// Output params to be returned to the application
	pOutParams->i64ActualStart = pInParams->i64StartAddress;
	pOutParams->i64ActualLength = pInParams->i64Length;

	// Calculate the address base on the Segment number
	// Size of one segment in samples
	InXferEx.i64StartAddr = m_OldAcqConfig.i64SegmentSize;
	InXferEx.i64StartAddr += m_32SegmentTailBytes / sizeof(uInt16) / (m_OldAcqConfig.u32Mode & CS_MASKED_MODE);
	InXferEx.i64StartAddr *= (pInParams->u32Segment - 1);
	InXferEx.i64StartAddr += i64Offset;

	InXferEx.u32Segment = 1;
	InXferEx.u16Channel = pInParams->u16Channel;

	InXferEx.u32FifoMode	= GetModeSelect(InXferEx.u16Channel);
	InXferEx.bBusMasterDma	= TRUE;
	InXferEx.bIntrEnable	= TRUE;

	i32Status = IoctlPreDataTransfer(&InXferEx, &OutXferEx);
	if (CS_SUCCEEDED(i32Status))
	{
		if (pInParams->pDataBuffer != NULL && pInParams->i64Length > 0)
		{
			i32Status = TransferDataFromOnBoardMemory(pInParams->pDataBuffer, pInParams->i64Length * sizeof(uInt16), 0);
		}
	}

	return i32Status;
}

////////////////////////////////////////////////////////////////////////
//

int32 CsHexagon::CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt )
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

	//return CS_NO_INTERRUPT;
	return i32Status;
}

////////////////////////////////////////////////////////////////////////
//

void	CsHexagon::CsSystemReset(BOOL bHard_reset)
{
	CsAbort();

	CsHexagon *pCard = m_MasterIndependent;
	pCard = m_MasterIndependent;

	while (pCard)
	{
		if ( bHard_reset )
		{
			pCard->ConfigurationReset(m_pCardState->i32ConfigBootLocation);
			pCard->DetectBootLocation();
		}

		// Reset Addon board
		pCard->AddonReset();
		pCard->ReadHardwareInfoFromFlash();
		pCard->ReadVersionInfo();

		pCard = pCard->m_Next;
	}

	if ( bHard_reset )
	{
		// Needed if ConfigurationReset() is called
		Sleep ( 1000 );		// wait 1s
	}
	SetDefaultAuxIoCfg();
	UpdateCardState();

	MsCsResetDefaultParams( TRUE );

}

////////////////////////////////////////////////////////////////////////
//

int32	CsHexagon::CsForceCalibration()
{
	int32			i32Status = CS_SUCCESS;

	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;

	uInt16			u16ChanZeroBased = 0;
	CsHexagon		*pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
		{
			u16ChanZeroBased = ConvertToHwChannel(i) - 1;
			pDevice->m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
			pDevice->m_bForceCal[u16ChanZeroBased] = true;
		}
		
		i32Status = pDevice->CalibrateAllChannels();
		if ( CS_FAILED(i32Status) )
			break;

		pDevice = pDevice->m_Next;
	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsHexagon::CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait )
{
	int32 i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(bWait);

	CsHexagon *pDevice = GetCardPointerFromChannelIndex( u16Channel );

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
void * CsHexagon::operator new (size_t  size)
{
	PVOID	p = new char[size];

	if ( p )
		memset(p, 0, size);

	return p;
}

#ifdef USE_SHARED_CARDSTATE

int32	CsHexagon::InitializeDevIoCtl()
{
	int32 i32Status = CsDevIoCtl::InitializeDevIoCtl();

	if ( CS_SUCCEEDED( i32Status ) )
	{
		i32Status = IoctlGetCardStatePointer( &m_pCardState );
		if ( CS_SUCCEEDED( i32Status ) )
		{
			if ( sizeof(CSHEXAGON_DEVEXT) == m_pCardState->u32Size && HEXAGON_DEVEXT_VERSION == m_pCardState->u32Version )
			{
				m_PlxData	= &m_pCardState->PlxData;
				return CS_SUCCESS;
			}
			else
			{
				::OutputDebugString("\nError HEXAGON_CARDSTATE struct size discrepency !!");
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


/*

			/// DEBUG STREAM
					char szText[256];
					uInt32 StreamStatus = 0;
					StreamStatus = ReadGageRegister( 0x2C );
					sprintf_s( szText, sizeof(szText), "Interrupt config = 0x%x\n", StreamStatus );
					::OutputDebugString(szText);

					StreamStatus = ReadPciRegister( 0x1D4 );
					sprintf_s( szText, sizeof(szText), "StreamStatus = 0x%x\n", StreamStatus );
					::OutputDebugString(szText);

					uInt32 DmaStatus = ReadGageRegister( 0x400 );
					sprintf_s( szText, sizeof(szText), "DmaStatus = 0x%x\n", DmaStatus );
					::OutputDebugString(szText);

*/