// CsRabbitDev.cpp
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "CsIoctl.h"
#include "CsDrvDefines.h"
#include "CsRabbit.h"

#ifdef _WINDOWS_
	#include <process.h>			// for _beginthreadex
#endif



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsRabbitDev::CsRabbitDev(uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars):
	m_u8CardIndex(u8Index), m_ProcessGlblVars(ProcessGlblVars), m_PseudoHandle(pDevInfo->DrvHdl), m_bNucleon(PCIE_BUS == pDevInfo->BusType),
	m_DeviceId(pDevInfo->DeviceId)
{

	// Memory has been cleared on allocation
	// Only need to initialize member variables whose default value is different than 0
	m_BusMasterSupport = TRUE;

	m_pTriggerMaster	= this;
	m_MasterIndependent = this;
	m_bAutoClockAdjust	= true;
	m_bAllowExtClkMs_pp = false;
	m_bHwAvg32			= true;
	m_bNoConfigReset	= true;

	m_u32IgnoreCalibErrorMask = 0xff;
	m_i32MsCalibTrigLevel	= MSCALIB_LEVEL_CHAN;
	m_u32DefaultExtClkSkip  = CS_SAMPLESKIP_FACTOR;
	m_u16SkipCalibDelay		= 30;		// 30 minutes
	m_u32MaxHwAvgDepth		= m_bNucleon ? CSRB_NUCLEON_MAX_MEM_HWAVG_DEPTH : CSRB_PCI_MAX_MEM_HWAVG_DEPTH;
	m_32SegmentTailBytes	= m_bNucleon ? 2*BYTESRESERVED_HWMULREC_TAIL : BYTESRESERVED_HWMULREC_TAIL;
	m_bCobraMaxPCIe			= (CS_DEVID_COBRA_MAX_PCIE == m_DeviceId);

	CsDevIoCtl::InitializeDevIoCtl( pDevInfo );

#if !defined( USE_SHARED_CARDSTATE )
	m_pCardState	= &m_CardState;
	m_PlxData		= &m_pCardState->PlxData;
#endif

	m_bSkipFirmwareCheck = false;
	m_i32ExtTrigSensitivity = CSRB_DEFAULT_EXT_TRIG_SENSE;
	m_u8TrigSensitivity	= CSRB_DEFAULT_TRIG_SENSITIVITY;

#ifndef _WINDOWS_
	m_bSkipFirmwareCheck = true;
#endif   

	RtlZeroMemory( &m_AddonCsiEntry, sizeof( m_AddonCsiEntry ) );
	RtlZeroMemory( &m_BaseBoardCsiEntry, sizeof( m_AddonCsiEntry ) );

	// Set the correct fpga image file name and fwl file
	if ( m_bNucleon )
	{
		if ( m_bCobraMaxPCIe )
		{
			sprintf_s( m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", CSCDG8_PCIEX_FPGAIMAGE_FILENAME );
			sprintf_s( m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", CSCDG8_PCIEX_FPGAIMAGE_FILENAME );
		}
		else
		{
			sprintf_s( m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", CSXYG8_PCIEX_FPGAIMAGE_FILENAME );
			sprintf_s( m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", CSXYG8_PCIEX_FPGAIMAGE_FILENAME );
		}
	}
	else
	{
		sprintf_s(m_szCsiFileName, sizeof(m_szCsiFileName), "%s.csi", CSXYG8_FPGA_IMAGE_FILE_NAME);
		sprintf_s(m_szFwlFileName, sizeof(m_szFwlFileName), "%s.fwl", CSXYG8_FPGA_IMAGE_FILE_NAME);
	}
}


////////////////////////////////////////////////////////////////////////
//

int32	CsRabbitDev::CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound )
{
	int32 i32Status = CS_SUCCESS;

	CsRabbitDev* pCard	= (CsRabbitDev*) m_ProcessGlblVars->pFirstUnCfg;
	
	// Remember the ForceInd flag for use later on Acqusition system initialization
	while (pCard)
	{
		pCard->m_pCardState->bForceIndMode = (BOOLEAN)bForceInd;
		pCard = pCard->m_HWONext;
	}

	if (( m_ProcessGlblVars->NbSystem == 0 ) && ! bForceInd )
	{
		pCard	= (CsRabbitDev*) m_ProcessGlblVars->pFirstUnCfg;
		while (pCard)
		{
#ifdef _WINDOWS_         
			pCard->ReadAndValidateCsiFile( pCard->m_PlxData->CsBoard.u32BaseBoardHardwareOptions, pCard->m_PlxData->CsBoard.u32AddonHardwareOptions );
			pCard->ReadAndValidateFwlFile();
#endif         

#if !defined( USE_SHARED_CARDSTATE )
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

		pCard	= (CsRabbitDev*) m_ProcessGlblVars->pFirstUnCfg;
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

int32 CsRabbitDev::CsAcquisitionSystemInitialize( uInt32 u32InitFlag )
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

		CsRabbitDev* pDevice = this;
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
				m_pSystem->hAcqSystemCleanup	= ::CreateEvent(NULL, FALSE, FALSE, NULL );
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
					pDevice->m_pCardState->u32ProcId = m_pCardState->u32ProcId;
					pDevice->UpdateCardState();
					pDevice = pDevice->m_Next;
				}

				m_PeevAdaptor.Initialize( GetIoctlHandle(), (char*)"CsxyG8WDF" );
				m_pSystem->bInitialized = TRUE;
				m_pSystem->BusyTransferCount = 0;
			}
		}

		if ( CS_FAILED(i32Status) )
			CsAcquisitionSystemCleanup();
	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

void	CsRabbitDev::CsAcquisitionSystemCleanup()
{
	// Set the reset flag in order to stop all pending processing
	m_pSystem->bReset = TRUE;

	CsAbort();

	if ( m_pAverageBuffer )
	{
		::GageFreeMemoryX( m_pAverageBuffer );
		m_pAverageBuffer = NULL;
	}

	CsRabbitDev *pDevice = m_MasterIndependent;

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
	RegisterEventHandle( NULL, ACQ_EVENT_TRIGGERED );
	RegisterEventHandle( NULL, ACQ_EVENT_END_BUSY );

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

#ifdef __linux__ // if linux only close handles if we not recovering from a crash
			     // otherwise we'll crash on SetEvent or CloseHandle
	if ( !m_pSystem->bCrashRecovery )
	{
		// Close acquisition events
		if (m_pSystem->hAcqSystemCleanup)
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

	// Reset system to default state
	m_pSystem->bInitialized = FALSE;
	m_pSystem->u16AcqStatus = ACQ_STATUS_READY;
}


////////////////////////////////////////////////////////////////////////
//

uInt32	CsRabbitDev::CsGetAcquisitionStatus()
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


int32	CsRabbitDev::CsGetParams( uInt32 u32ParamId, PVOID pOutBuffer, uInt32 u32OutSize )
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
			*((PCS_FW_VER_INFO) pUserBuffer) = m_pCardState->VerInfo;
		}
		break;

	case CS_EXTENDED_BOARD_OPTIONS:
		{
			uInt32 *pExOptions = (uInt32 *) pUserBuffer;

			pExOptions[0] = GetBoardNode()->u32BaseBoardOptions[1];
			pExOptions[1] = GetBoardNode()->u32BaseBoardOptions[2];
		}
		break;

	case CS_SYSTEMINFO_EX:
		{
			PCSSYSTEMINFOEX pSysInfoEx = (PCSSYSTEMINFOEX) pUserBuffer;

			uInt32	i;

			RtlZeroMemory( pSysInfoEx, sizeof(CSSYSTEMINFOEX) );
			pSysInfoEx->u32Size = sizeof(CSSYSTEMINFOEX);
			pSysInfoEx->u16DeviceId = GetDeviceId();

			CsRabbitDev *pDevice = m_MasterIndependent;

			// Permission and firmware versions from the master card
            pSysInfoEx->i64ExpertPermissions = pDevice->GetBoardNode()->i64ExpertPermissions;
			pSysInfoEx->u32BaseBoardHwOptions = pDevice->GetBoardNode()->u32BaseBoardHardwareOptions;
			pSysInfoEx->u32AddonHwOptions = pDevice->GetBoardNode()->u32AddonHardwareOptions;

			for ( i = 0; i < sizeof(GetBoardNode()->u32RawFwVersionB) /sizeof(uInt32) ; i ++)
			{
				pSysInfoEx->u32BaseBoardFwOptions[i]	= GetBoardNode()->u32BaseBoardOptions[i];
				pSysInfoEx->u32AddonFwOptions[i]		= GetBoardNode()->u32AddonOptions[i];

				pSysInfoEx->u32BaseBoardFwVersions[i].u32Reg	= GetBoardNode()->u32UserFwVersionB[i].u32Reg;
				pSysInfoEx->u32AddonFwVersions[i].u32Reg		= GetBoardNode()->u32UserFwVersionA[i].u32Reg;
			}

			pDevice = pDevice->m_Next;

			// System permission will be "AND" persmissions of all Master/Slave cards
			while ( pDevice )
			{
				// i starts from 1 because the image 0 is not an expertt option
				for ( i = 1; i < sizeof(GetBoardNode()->u32RawFwVersionB) /sizeof(uInt32) ; i ++)
				{
					pSysInfoEx->u32BaseBoardFwOptions[i] &= pDevice->GetBoardNode()->u32BaseBoardOptions[i];
					pSysInfoEx->u32AddonFwOptions[i] &= pDevice->GetBoardNode()->u32AddonOptions[i];

					if ( 0 == pSysInfoEx->u32BaseBoardFwOptions[i] )
						pSysInfoEx->u32BaseBoardFwVersions[i].u32Reg = 0;

					if ( 0 == pSysInfoEx->u32AddonFwOptions[i] )
						pSysInfoEx->u32AddonFwVersions[i].u32Reg = 0;
				}

				pSysInfoEx->i64ExpertPermissions &= pDevice->GetBoardNode()->i64ExpertPermissions;
				pSysInfoEx->u32AddonHwOptions &= pDevice->GetBoardNode()->u32AddonHardwareOptions;
				pSysInfoEx->u32BaseBoardHwOptions &= pDevice->GetBoardNode()->u32BaseBoardHardwareOptions;
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
			PCSXYG8BOARDCAPS pCapsInfo = (PCSXYG8BOARDCAPS) pUserBuffer;
			FillOutBoardCaps(pCapsInfo);
		}
		break;

	case CS_TRIG_OUT:
		{
			PCS_TRIG_OUT_STRUCT pTrigOut = (PCS_TRIG_OUT_STRUCT) pUserBuffer;

			pTrigOut->u16ValidMode = GetValidTrigOutMode();
			pTrigOut->u16Valid = GetValidTrigOutValue(pTrigOut->u16Mode);
			pTrigOut->u16Mode = GetTrigOutMode();
			pTrigOut->u16Value = GetTrigOut();
		}
		break;

	case CS_CLOCK_OUT:
		{
			PCS_CLOCK_OUT_STRUCT pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;

			pClockOut->u16Value = (uInt16) GetClockOut();
			pClockOut->u16Valid = CS_OUT_NONE|CS_CLKOUT_SAMPLE|CS_CLKOUT_REF|CS_CLKOUT_GBUS;
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
			if ( ! m_bNucleon )
				i32Status = CS_INVALID_REQUEST;
			else
			{
				CsRabbitDev	*pDevice = m_MasterIndependent;
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

	case CS_PEEVADAPTOR_CONFIG:
		{
			PPVADAPTORCONFIG pPvAdaptorConfig = (PPVADAPTORCONFIG) pUserBuffer;
			if ( 0 != pPvAdaptorConfig )
				pPvAdaptorConfig->u32RegVal = m_PeevAdaptor.GetSwConfig();
		}
		break;

	case CS_READ_GAGE_REGISTER:
		{
			PCS_RD_GAGE_REGISTER_STRUCT		pReadRegister = (PCS_RD_GAGE_REGISTER_STRUCT) pUserBuffer;
			CsRabbitDev*					pDevice = GetCardPointerFromBoardIndex( pReadRegister->u16CardIndex );
			
			if ( NULL != pDevice )
				pReadRegister->u32Data = pDevice->ReadGageRegister( pReadRegister->u16Offset );
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
		{
			uInt32	*pu32Val  = (uInt32	*) pUserBuffer;
			if (  (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
				  BUNNYCALIBINFO_CHECKSUM_VALID != m_pCardState->CalibInfoTable.u32Checksum || 
				  (0 == (m_pCardState->CalibInfoTable.u32Valid & BUNNY_CAL_DAC_VALID )) )
			{
				i32Status = CS_INVALID_DACCALIBTABLE;
			}
			else
				*pu32Val = m_pCardState->bUseDacCalibFromEEprom;
		}
		break;

	case CS_DEFAULTUSE_DACCALIBTABLE:
		{
			uInt32	*pu32Val  = (uInt32	*) pUserBuffer;
			if (  (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
				  BUNNYCALIBINFO_CHECKSUM_VALID != m_pCardState->CalibInfoTable.u32Checksum || 
				  (0 == (m_pCardState->CalibInfoTable.u32Valid & BUNNY_CAL_DAC_VALID )) )
			{
				i32Status = CS_INVALID_DACCALIBTABLE;
			}
			else if ( m_pCardState->CalibInfoTable.u32Active & BUNNY_CAL_DAC_VALID )
				*pu32Val = 1;
			else
				*pu32Val = 0;
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

int32	CsRabbitDev::CsSetParams( uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize )
{
	PVOID			pUserBuffer = pInBuffer;
	int32			i32Status = CS_SUCCESS;
	CsRabbitDev		*pDevice = this;

	UNREFERENCED_PARAMETER(u32InSize);

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


	case CS_WRITE_EEPROM: //----- Write Add-On Flash
		{
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) pUserBuffer;

			PVOID	pAppBuffer = pGenericRw->pBuffer;
			uInt32	u32Offset = pGenericRw->u32Offset;
			uInt32	u32BufferSize = pGenericRw->u32BufferSize;

			if ( u32Offset + u32BufferSize <= RBT_ADDONFLASH_SIZE )
			{
				i32Status = WriteEepromEx(pAppBuffer, u32Offset, u32BufferSize);
				if ( CS_SUCCEEDED(i32Status) )
				{
					i32Status = ReadAddOnHardwareInfoFromEeprom(FALSE);
					if ( CS_SUCCEEDED(i32Status) )
					{
						m_pSystem->SysInfo.u32AddonOptions = GetBoardNode()->u32ActiveAddonOptions;
						// Addon Hw Option may have changed. Update board type
						GetBoardNode()->u32BoardType = 0;
						AssignBoardType();
						ResetCachedState();		
					}
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
				m_PlxData->CsBoard.i64ExpertPermissions = 0;
				RtlZeroMemory( m_PlxData->CsBoard.u32RawFwVersionB, sizeof( m_PlxData->CsBoard.u32RawFwVersionB ) );
				RtlZeroMemory( m_PlxData->CsBoard.u32RawFwVersionA, sizeof( m_PlxData->CsBoard.u32RawFwVersionA ) );
				
				// Force calibration next time 
				ResetCachedState();
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
				i32Status = pDevice->WriteToDac( pSendDAC->u8DacAddress, pSendDAC->u16DacValue, 0, TRUE );
			}
		}
		break;

	case CS_TRIG_OUT:
		{
			PCS_TRIG_OUT_STRUCT	pTrigOut = (PCS_TRIG_OUT_STRUCT) pUserBuffer;
			i32Status = SetTrigOut( pTrigOut );
		}
		break;

	case CS_CLOCK_OUT:
		{
			PCS_CLOCK_OUT_STRUCT	pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;
			i32Status = SetClockOut( m_pCardState->ermRoleMode, (eClkOutMode)pClockOut->u16Value );
		}
		break;

	case CS_TRIG_ENABLE:
		{
				PCS_TRIG_ENABLE_STRUCT	pExtTrigEn = (PCS_TRIG_ENABLE_STRUCT) pUserBuffer;

				i32Status = SendTriggerEnableSetting( (eTrigEnableMode) pExtTrigEn->u16Value );
		}
		break;


#ifdef	DEBUG_MASTERSLAVE
	case CS_DEBUG_MS_PULSE:
		{
			CsRabbitDev* pCard = m_MasterIndependent;
			while( pCard )
			{
				pCard->m_u16DebugMsPulse += 1;
				pCard->m_u16DebugMsPulse = pCard->m_u16DebugMsPulse % 3;

				pCard->m_bPulseTransfer = ! pCard->m_bPulseTransfer;
				pCard = pCard->m_Next;
			}

			if ( 2 == m_u16DebugMsPulse )
			{
				pCard = m_MasterIndependent;

				SendDigitalTriggerSensitivity( CSRB_TRIGENGINE_A1, SINGLEPULSECALIB_SENSITIVE );
				while ( pCard )
				{
					// Switch to Ms calib mode
					pCard->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ), ecmCalModeMs );
					pCard = pCard->m_Next;
				}
			}
			else if ( 0 == m_u16DebugMsPulse )
			{
				pCard = m_MasterIndependent;

				SendDigitalTriggerSensitivity( CSRB_TRIGENGINE_A1 );
				while ( pCard )
				{
					// Switch to Ms calib mode
					pCard->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ) );
					pCard = pCard->m_Next;
				}
			}

		}
		break;

	case CS_DEBUG_MS_OFFSET:
		{
			CsRabbitDev* pCard = m_MasterIndependent;
			while( pCard )
			{
				pCard->m_bMsOffsetAdjust = ! pCard->m_bMsOffsetAdjust;
				pCard = pCard->m_Next;
			}
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
				m_pTriggerMaster->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ), ecmCalModeExtTrig );
				GageCopyMemoryX( m_OldTrigParams, m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams));

				if ( m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Source )
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
				m_pTriggerMaster->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ) );
				uInt8 u8IndexTrig = CSRB_TRIGENGINE_A1;
				GageCopyMemoryX( m_CfgTrigParams, m_OldTrigParams, sizeof(m_CfgTrigParams));

				m_pTriggerMaster->SendExtTriggerSetting( (BOOLEAN) m_CfgTrigParams[CSRB_TRIGENGINE_EXT].i32Source,
									   m_CfgTrigParams[CSRB_TRIGENGINE_EXT].i32Level,
									   m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32Condition,
									   m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange,
									   m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance,
									   m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling,
   									   m_i32ExtTrigSensitivity);

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
				m_pTriggerMaster->SendDigitalTriggerSensitivity( CSRB_TRIGENGINE_A1, m_u8TrigSensitivity );
			}
		}
		break;
#endif

	case CS_SAVE_CALIBTABLE_TO_EEPROM:
		{
			uInt32 u32Valid = *((uInt32 *)	pUserBuffer);
			
			// Update the Calib Table with all latest Dac values
			GageCopyMemoryX( &m_pCardState->CalibInfoTable.ChanCalibInfo, &m_pCardState->DacInfo, sizeof(m_pCardState->DacInfo) );
			// Wtite updated Calib table into EEPROM
			i32Status = WriteCalibTableToEeprom( m_CalibTableId, u32Valid );
		}
		break;

	case CS_USE_DACCALIBTABLE:
		{
			if (  (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
						  BUNNYCALIBINFO_CHECKSUM_VALID != m_pCardState->CalibInfoTable.u32Checksum || 
						  (0 == (m_pCardState->CalibInfoTable.u32Valid & BUNNY_CAL_DAC_VALID )) )
			{
				i32Status = CS_INVALID_DACCALIBTABLE;
			}
			else 
			{
				uInt32	*pValue = (uInt32 *) pUserBuffer;
				if ( *pValue )
				{
					// Use the Calib table from EEPROM. Update the current table with the one saved from EEPROM
					m_pCardState->bUseDacCalibFromEEprom = true;
					RtlCopyMemory( &m_pCardState->DacInfo, &m_pCardState->CalibInfoTable.ChanCalibInfo, sizeof(m_pCardState->DacInfo) );
				}
				else
				{
					m_pCardState->bUseDacCalibFromEEprom = false;
					ResetCachedState();		// Force calibration next time
				}
			}
		}
		break;


	case CS_DEFAULTUSE_DACCALIBTABLE:
		{
			if (  (-1 == m_pCardState->CalibInfoTable.u32Valid ) ||
				  BUNNYCALIBINFO_CHECKSUM_VALID != m_pCardState->CalibInfoTable.u32Checksum || 
				  (0 == (m_pCardState->CalibInfoTable.u32Valid & BUNNY_CAL_DAC_VALID )) )
			{
				i32Status = CS_INVALID_DACCALIBTABLE;
			}
			else
			{
				if ( *((uInt32 *) pUserBuffer) )
				{
					// Default use Cal table: Set Valid bit and clear invalid bit
					m_pCardState->CalibInfoTable.u32Active |= BUNNY_CAL_DAC_VALID;
					m_pCardState->CalibInfoTable.u32Active &= ~BUNNY_CAL_DAC_INVALID;
				}
				else
				{
					// Default NOT use Cal table: Clear Valid bit and set Invalid bit
					m_pCardState->CalibInfoTable.u32Active &= ~BUNNY_CAL_DAC_VALID;
					m_pCardState->CalibInfoTable.u32Active |= BUNNY_CAL_DAC_INVALID;
				}

				i32Status = WriteCalibTableToEeprom( m_CalibTableId, m_pCardState->CalibInfoTable.u32Valid );
				ResetCachedState();
			}
		}
		break;


	case CS_SAVE_OUT_CFG:
		{
			RABBIT_FLASH_FOOTER		RabbitFshFooter;

			uInt32	u32Offset = FIELD_OFFSET(RBT_ADDONFLASH_LAYOUT, Footer);
			i32Status = ReadEeprom( &RabbitFshFooter, u32Offset, sizeof(RabbitFshFooter) );
			if ( CS_FAILED(i32Status) )
				break;

			if ( 0 != *((uInt32 *) pUserBuffer)  )
			{
				// Save out config
				RabbitFshFooter.ClockOutCfg			= m_pCardState->eclkoClockOut;
				RabbitFshFooter.TrigEnableCfg		= m_pCardState->eteTrigEnable;
				RabbitFshFooter.u16TrigOutCfg		= m_pCardState->u16TrigOutMode;
				RabbitFshFooter.u16TrigOutDisabled	= m_pCardState->bDisableTrigOut;
			}
			else
			{
				// Clear out config
				RabbitFshFooter.ClockOutCfg			= (eClkOutMode) -1;
				RabbitFshFooter.TrigEnableCfg		= (eTrigEnableMode) -1;
				RabbitFshFooter.u16TrigOutCfg		= (uInt16) -1;
				RabbitFshFooter.u16TrigOutDisabled	= (uInt16) -1;
				SetDefaultAuxIoCfg();
			}

			i32Status = WriteEepromEx( &RabbitFshFooter, u32Offset, sizeof(RabbitFshFooter) );
		}
		break;

	case CS_TIMESTAMP_RESET:
		ResetTimeStamp();

		// We have seen in some exteme cases the card stop triggering if an acquisition is performed right away after
		// the timestamp reset. Put this delay solve the problem
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
						i32Status = pDevice->WriteRegisterThruNios( NiosCmd->u32DataIn, NiosCmd->u32Command, -1, &NiosCmd->u32DataOut );
					else
						i32Status = pDevice->WriteRegisterThruNios( NiosCmd->u32DataIn, NiosCmd->u32Command );
				}

				pDevice = pDevice->m_Next;
			}
		}
		break;

	case CS_PEEVADAPTOR_CONFIG:
		{
			PPVADAPTORCONFIG pPvAdaptorConfig = (PPVADAPTORCONFIG) pUserBuffer;
			i32Status = m_PeevAdaptor.SetConfig(pPvAdaptorConfig->u32RegVal);
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

	case CS_HISTOGRAM_RESET:
		if ( IsHistogramEnabled() )
		{
			while ( pDevice )
			{
				pDevice->HistogramReset();
				pDevice = pDevice->m_Next;
			}
		}
		else
			i32Status = CS_FUNCTION_NOT_SUPPORTED;

		break;

	case CS_ADDON_RESET:
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

int32 CsRabbitDev::CsStartAcquisition()
{
	int32	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;

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

int32	CsRabbitDev::CsAbort()
{
	CsRabbitDev *pDevice = m_MasterIndependent;

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
					if ( m_bNucleon )
						m_MasterIndependent->m_pCardState->u32ActualSegmentCount = ReadRegisterFromNios( 0, CSPLXBASE_READ_REC_COUNT_CMD ) & PLX_REC_COUNT_MASK;	
					GetBoardNode()->bTriggered = true;
					GetBoardNode()->bBusy = false;
					MsAbort();
					m_pTriggerMaster->SignalEvents();
				}
				break;

			default:
			{
				if ( m_pSystem->BusyTransferCount > 0 )
				{
					// Abort all pending asynchronous data transfers
					CsRabbitDev *pDevice = this;
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
	CsRabbitDev* pCard = m_MasterIndependent;

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

void	CsRabbitDev::CsForceTrigger()
{

	m_pTriggerMaster->ForceTrigger();

}

////////////////////////////////////////////////////////////////////////
//

int32	CsRabbitDev::CsDataTransfer( in_DATA_TRANSFER *InDataXfer )
{
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;
	int32						i32Status = CS_SUCCESS;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;

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
			pOutParams->i32LowPart = (uInt32) GetTimeStampFrequency();
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
			 u32Mode == TxMODE_HISTOGRAM )
		{
			if ( u32Mode == TxMODE_HISTOGRAM && !m_bHistogram )
				return CS_INVALID_TRANSFER_MODE;

			if ( TxMODE_DATA_32 & u32Mode )
			{
				if ( !IsExpertAVG() && !m_bSoftwareAveraging )
				{
					return CS_INVALID_TRANSFER_MODE;
				}
			}

			//---- Find the card that we need to setup for transfer
			CsRabbitDev* pDevice = GetCardPointerFromChannelIndex( pInParams->u16Channel );

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
					if ( m_bHistogram &&  (u32Mode == TxMODE_HISTOGRAM) )
						return pDevice->DataTransferModeHistogram(InDataXfer);
					else if ( m_bSoftwareAveraging )
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

int32 CsRabbitDev::CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt )
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

void	CsRabbitDev::CsSystemReset(BOOL bHard_reset)
{
	UNREFERENCED_PARAMETER(bHard_reset);
	CsAbort();

	CsRabbitDev *pCard = m_MasterIndependent;
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

		pCard = pCard->m_Next;
	}

	m_pCardState->u8VerifyTrigOffsetModeIx = (uInt8)-1;
	m_pCardState->u32VerifyTrigOffsetRateIx = (uInt32)-1;

	MsCsResetDefaultParams( TRUE );

}

////////////////////////////////////////////////////////////////////////
//

int32	CsRabbitDev::CsForceCalibration()
{
	int32			i32Status = CS_SUCCESS;

	i32Status = CsCheckPowerState();
	if ( CS_FAILED(i32Status) )
		return i32Status;

	CsRabbitDev		*pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		// Force calibration for all activated channels depending on the current mode (single, dual , quad ...)
		if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
		{
			pDevice->m_pCardState->bCalNeeded[ConvertToHwChannel( CS_CHAN_1 )-1] = true;
			pDevice->m_bForceCal[ConvertToHwChannel( CS_CHAN_1 )-1] = true;
		}
		else
		{
			pDevice->m_pCardState->bCalNeeded[0] = pDevice->m_pCardState->bCalNeeded[1] = true;
			pDevice->m_bForceCal[0] = pDevice->m_bForceCal[1] =  true;
		}
		
		pDevice->m_u32CalibRange_mV = 0;

		i32Status = pDevice->CalibrateAllChannels();
		if ( CS_FAILED(i32Status) )
			break;

		pDevice = pDevice->m_Next;
	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//

int32 CsRabbitDev::CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait )
{
	int32 i32Status = CS_SUCCESS;

	UNREFERENCED_PARAMETER(bWait);

	CsRabbitDev *pDevice = GetCardPointerFromChannelIndex( u16Channel );

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
void * CsRabbitDev::operator new (size_t  size)
{
	PVOID	p = new char[size];

	if ( p )
		memset(p, 0, size);

	return p;
}

#ifdef USE_SHARED_CARDSTATE

int32	CsRabbitDev::InitializeDevIoCtl()
{
	int32 i32Status = CsDevIoCtl::InitializeDevIoCtl();

	if ( CS_SUCCEEDED( i32Status ) )
	{
		i32Status = IoctlGetCardStatePointer( &m_pCardState );
		if ( CS_SUCCEEDED( i32Status ) )
		{
			if ( sizeof(CSRABBIT_CARDSTATE) == m_pCardState->u32Size  )
			{
				m_PlxData	= &m_pCardState->PlxData;
				return CS_SUCCESS;
			}
			else
			{
				::OutputDebugString("\nError RABBIT_CARDSTATE struct size discrepency !!");
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
