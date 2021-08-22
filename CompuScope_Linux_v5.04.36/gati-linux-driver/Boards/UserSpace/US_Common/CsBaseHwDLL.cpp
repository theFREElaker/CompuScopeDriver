#include "StdAfx.h"
#include "CsBaseHwDLL.h"
#include "CsIoctl.h"
#include "CsDriverTypes.h"
#include "CsErrors.h"
#include "CsPrivateStruct.h"
#include "CsMarvinOptions.h"
#include "CsDeviceApi.h"
#include "CsSymLinks.h"

extern SHAREDDLLMEM			Globals;
extern GLOBAL_PERPROCESS	g_GlobalPerProcess;
bool   bSystemDetectDone = FALSE;      //System Detection should be done at least one, Event bChange has already been set in driver 

#define _PROTECTACCESS

//-----------------------------------------------------------------------------
//  This function will be call when HW driver DLL is load (Process attach)
//-----------------------------------------------------------------------------
CsBaseHwDLL::CsBaseHwDLL(char *szSymbolicLink, PHW_DLL_LOOKUPTABLE pHwDllLookupTable )
{
	char szDrvNameBase[MAX_STR_LENGTH];

	m_pHwLookupTable = pHwDllLookupTable;
	m_Status = CS_FALSE;

#ifdef _WINDOWS_
	// Build the driver name for CreateFile
	strcpy_s(szDrvNameBase , sizeof(szDrvNameBase), _T("\\\\.\\"));
	strcat_s(szDrvNameBase, sizeof(szDrvNameBase), szSymbolicLink);
#else
	// Build the driver name for CreateFile
	strcpy_s(szDrvNameBase, sizeof(szDrvNameBase), _T("/proc/driver/"));
	strcat(szDrvNameBase, szSymbolicLink);
#endif

	strcpy_s( m_DriverName, sizeof(m_DriverName), szDrvNameBase );


	// For applications, we have to create card objects on process attacth
	if ( SearchForActiveDriver() )
		m_Status = CsDrvAllocateCardObjectForProcess();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOL CsBaseHwDLL::SearchForActiveDriver()
{
	BOOL	bDriverActive = FALSE;
	char	szDriverName[MAX_STR_LENGTH];

	// Attemp to open a connection to a compuscope card
	for (int i = 0; i < MAX_ACQ_SYSTEMS; i ++)
	{
		sprintf_s(szDriverName, sizeof(szDriverName), _T("%s%02d"), m_DriverName, i);

#ifdef _WINDOWS_
		// Open kernel driver by using its generic symbolic link
		m_hDriver = CreateFile( (LPCSTR)szDriverName,
								GENERIC_READ|GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								0,
								(void *)NULL);
#else
		m_hDriver = open (szDriverName, 0);;
#endif

		if ( m_hDriver != (FILE_HANDLE) -1  )
		{
			// Found a generic link to kernel driver
			strcpy_s( m_LinkName, sizeof(m_LinkName), szDriverName );
			CloseDrvGenericLink();
			bDriverActive = TRUE;
			break;
		}
		else
			m_hDriver = 0;
	}

	return bDriverActive;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsBaseHwDLL::~CsBaseHwDLL()
{
	::DeleteProcessDeviceCardList();

	// Close Drivers Handle
	if (( m_hDriver ) && (m_hDriver != (FILE_HANDLE)-1))
	{
#ifdef _WINDOWS_
		CloseHandle(m_hDriver);
#else
		close(m_hDriver);
#endif
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::GetStatus()
{
	// return the creation status of the constructor
	return m_Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

FILE_HANDLE CsBaseHwDLL::OpenDrvGenericLink()
{
#ifdef _WINDOWS_
	// Open kernel driver by using its generic symbolic link
	m_hDriver = CreateFile( (LPCSTR)m_LinkName,
							GENERIC_READ|GENERIC_WRITE,
							0,
							NULL,
							OPEN_EXISTING,
							0,
							(void *)NULL);

	if (m_hDriver == (FILE_HANDLE)-1)
		m_hDriver = 0;
#else	// Linux
	// Open kernel driver by using its generic symbolic link
	m_hDriver = open ( (LPCSTR)m_LinkName, 0 );

#endif

	return m_hDriver;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void  CsBaseHwDLL::CloseDrvGenericLink()
{
#ifdef _WINDOWS_
	if ( m_hDriver )
	{
		CloseHandle( m_hDriver );
		m_hDriver = NULL;
	}
#else	// Linux
	if ( m_hDriver )
	{
		close( m_hDriver );
		m_hDriver = 0;
	}
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvOpenSystemForRm( DRVHANDLE	DrvHdl )
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	InitializeCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID	pDevice = GetAcqSystemCardPointer(DrvHdl);
	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	::CreateKernelLinkAcqSystem( pDevice );

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvCloseSystemForRm(DRVHANDLE DrvHdl)
{
	PVOID	pDevice = GetAcqSystemCardPointer(DrvHdl);
	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	::DestroyKernelLinkAcqSystem( pDevice );

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//	Allocate card objects for the current process
//-----------------------------------------------------------------------------

int32	CsBaseHwDLL::CsDrvAllocateCardObjectForProcess()
{
	// If ( 0 != g_GlobalPerProcess.hNativeFirstCard ) that means card objects has been allocated
	// for this process.
	if ( 0 == g_GlobalPerProcess.hNativeFirstCard )
	{
		CS_CARD_COUNT		CardCount = {0};
		uInt32				u32CardCount = CsDrvGetNumberOfDevices(&CardCount);
		CSDEVICE_INFO		DeviceInfo[50];

		CsDrvGetDeviceInfo( DeviceInfo, u32CardCount*sizeof(CSDEVICE_INFO) );

		return ::AllocateProcessDeviceCardList( u32CardCount, DeviceInfo );
	}
	else
		return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsBaseHwDLL::GetIndexInLookupTable( DRVHANDLE	DrvHdl )
{
	uInt32 i;


	// Find the index in lookup table
	for ( i = 0; i < m_pHwLookupTable->u16SystemCount; i++ )
	{
		if (m_pHwLookupTable->DrvSystem[i].DrvHandle == DrvHdl)
			break;
	}

	_ASSERT ( i < m_pHwLookupTable->u16SystemCount );

	if ( i < m_pHwLookupTable->u16SystemCount )
	{
		// Return the index found
		return i;
	}
	else
	{
		// The handle was not found in lookup table
		return -1;
	}
}


//-----------------------------------------------------------------------------
//  Get number of device (Compuscope cards) detected by kernel driver
//-----------------------------------------------------------------------------
uInt32	CsBaseHwDLL::CsDrvGetNumberOfDevices(CS_CARD_COUNT *CardCount)
{
	uInt32	u32RetCode;

#ifdef _WINDOWS_

	if ( 0 == OpenDrvGenericLink() )
		return 0;
	
	uInt32 u32OutSize = (CardCount != NULL) ? sizeof( CS_CARD_COUNT ) : 0;
	u32RetCode = GAGE_DEVICE_IOCTL(m_hDriver,
					CS_IOCTL_GET_NUM_OF_DEVICES,
					NULL,
					0,
					CardCount,
					u32OutSize,
					&m_BytesRead,
					NULL,
					NULL );

	CloseDrvGenericLink();

	_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		uInt32 Error = GetLastError();
		TRACE( DRV_TRACE_LEVEL, "%s::IOCTL_GET_ACQUISITION_SYSTEM_COUNT fails. (WinError = %x )\n", m_LinkName, Error );
		return 0;
	}
	else
	{
		_ASSERT( m_BytesRead == u32OutSize );
		if ( CardCount != NULL )
			return CardCount->u32NumOfCards;
		else
			return 0;
	}

#else

	OpenDrvGenericLink();

	u32RetCode = GAGE_DEVICE_IOCTL(m_hDriver,
					CS_IOCTL_GET_NUM_OF_DEVICES,
					CardCount);

	CloseDrvGenericLink();

	_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		uInt32 Error = GetLastError();
		TRACE( DRV_TRACE_LEVEL, "%s::IOCTL_GET_ACQUISITION_SYSTEM_COUNT fails. (WinError = %x )\n", m_LinkName, Error );
		return 0;
	}
	else if ( CardCount != 0 )
		return CardCount->u32NumOfCards;
	else
		return 0;
	
#endif
}



//-----------------------------------------------------------------------------
//  Get the device info array (Symbolic link, native handle... ) from kernel driver
//-----------------------------------------------------------------------------
uInt32 CsBaseHwDLL::CsDrvGetDeviceInfo(PCSDEVICE_INFO pDevInfo, uInt32 u32Size)
{
	uInt32	u32RetCode;

#ifdef _WINDOWS_

	OpenDrvGenericLink();

	u32RetCode = GAGE_DEVICE_IOCTL( m_hDriver,
					 CS_IOCTL_GET_DEVICE_INFO,
					 NULL,
					 0,
					 pDevInfo,
					 u32Size,
					 &m_BytesRead,
					 NULL,
					 NULL );

	CloseDrvGenericLink();

	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		u32RetCode = GetLastError();
		TRACE( DRV_TRACE_LEVEL, "%s::IOCTL_GET_ACQUISITION_SYSTEM_HANDLES fails. (WinError = %x )\n", m_LinkName, u32RetCode );
		return uInt32(CS_DEVIOCTL_ERROR);
	}
	_ASSERT( m_BytesRead != 0 );
	_ASSERT( m_BytesRead == u32Size);

#else
	OpenDrvGenericLink();

	u32RetCode = GAGE_DEVICE_IOCTL( m_hDriver,
						 CS_IOCTL_GET_DEVICE_INFO,
						 pDevInfo);

	CloseDrvGenericLink();

	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		TRACE( DRV_TRACE_LEVEL, "%s::IOCTL_GET_ACQUISITION_SYSTEM_HANDLES fails. (WinError = %x )\n", m_LinkName, u32RetCode );
		return uInt32(CS_DEVIOCTL_ERROR);
	}
#endif

	// How many handles can we copy to the given buffer ?
	u32Size = u32Size / sizeof(CSDEVICE_INFO);

	// return number of handles in buffer
	return (u32Size) ;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsBaseHwDLL::CsDrvGetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32 Size)
{
	uInt32	i = 0;

	// How many handles can we copy to the given buffer ?
	Size = Size / sizeof(DRVHANDLE);
	if ( Size >= m_pHwLookupTable->u16SystemCount)
		Size = m_pHwLookupTable->u16SystemCount;

	// Copy hanlles to the given buffer
	for ( i = 0; i < Size; i++ )
	{
		pDrvHdl[i] = m_pHwLookupTable->DrvSystem[i].DrvHandle;
	}

	// return number of handles in buffer
	return (Size) ;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsBaseHwDLL::CsDrvGetAcquisitionSystemCount( uInt16 bForceIndependentMode, uInt16* pu16SystemFound )
{
	int32	i32Status = CS_SUCCESS;

	if (0 == m_pHwLookupTable->u16SystemCount)
	{
		if ( ! SearchForActiveDriver() )
		{
			// If the kernel driver is not load that means there is no Compuscope cards in system.
			*pu16SystemFound = 0;
			return i32Status;
		}
	}

	CS_CARD_COUNT	CardCount = {0};
	uInt32			u32CardCount = CsDrvGetNumberOfDevices(&CardCount);

	// For Rm, the DLL remains load. The g_GlobalPerProcess.u32CardCount has been initalized before when Rm services started.
	// From that time, the number of cards may have changed becasue user can enable, disable devices from Windows Device Manager.
	// We compare to the newest value from kernel to see if the number of device (Compuscope cards) has changed. 
	// If it is the case then we have to rebuild the card objtects list and perform the whole initialization process
	if ( !bSystemDetectDone || u32CardCount != g_GlobalPerProcess.u32CardCount || CardCount.bChanged )
	{
		// Delete the old card objects list.
		::DeleteProcessDeviceCardList();
		g_GlobalPerProcess.hNativeFirstCard = NULL;
		g_GlobalPerProcess.u32CardCount = u32CardCount;

		if ( 0 == u32CardCount )
		{
			*pu16SystemFound = 0;
			return i32Status;
		}
	}

	i32Status = CsDrvAllocateCardObjectForProcess();
	if ( CS_FAILED(i32Status) )
		return i32Status;

	if ( !bSystemDetectDone || CardCount.bChanged || g_GlobalPerProcess.u16IndMode != bForceIndependentMode )
	{
		g_GlobalPerProcess.pFirstUnCfg = g_GlobalPerProcess.hNativeFirstCard;
		g_GlobalPerProcess.u16IndMode = bForceIndependentMode;

		// Reset all systems into un-initialized state
		for (uInt32 i = 0; i < m_pHwLookupTable->u16SystemCount; i ++ )
		{
			PDRVSYSTEM	pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];
			pSystem->bInitialized = FALSE;
		}

		Globals.it()->g_StaticLookupTable.u16SystemCount = m_pHwLookupTable->u16SystemCount = g_GlobalPerProcess.NbSystem = 0;
	}
	
	if ( ! m_pHwLookupTable->u16SystemCount )
	{
		// Open kernel handle for all of devices
		::CreateKernelLinkDeviceList( g_GlobalPerProcess.hNativeFirstCard );

		i32Status = ::DeviceGetAcqSystemCount( g_GlobalPerProcess.pFirstUnCfg,
												   (BOOL) bForceIndependentMode,
												   pu16SystemFound );

		 m_pHwLookupTable->u16SystemCount		 = *pu16SystemFound;
		 g_GlobalPerProcess.NbSystem			 = *pu16SystemFound;

		 // Close kernel handle for all of devices
		::DestroyKernelLinkDeviceList( g_GlobalPerProcess.hNativeFirstCard );
	}
	else
	{
		*pu16SystemFound = m_pHwLookupTable->u16SystemCount;
	}
	
	// Notify Kernel driver that CsDrvGetAcquisitionSystemCount has completed
	CsDrvGetNumberOfDevices(NULL);

        // Detection has completed
        bSystemDetectDone = TRUE;

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvAcquisitionSystemInit( DRVHANDLE	DrvHdl, BOOL bResetDefaultSetting )
{
	int32	i32Status = CS_SUCCESS;

	// Build the M/S link list
	if ( ! g_GlobalPerProcess.NbSystem || (g_GlobalPerProcess.NbSystem != Globals.it()->g_StaticLookupTable.u16SystemCount) )
	{
		// GetAcquisitionSystemCount has been called and m/s organization has been done done from another process (RM or CsTest /Disp)
		// This process still does not have any information about Compuscope system.
		// Build the CompuScope system link base on global system info
		::DeviceBuildCompuScopeSystemLinks();
	}

	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	InitializeCriticalSection(&pSystem->CsProtectAccess);
#endif
	
	PVOID	pDevice = GetAcqSystemCardPointer(DrvHdl);
	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	uInt32	u32InitFlag = bResetDefaultSetting ? 0 : 1;
	i32Status = ::DeviceAcqSystenInitialize(pDevice, u32InitFlag);
	if ( CS_SUCCESS != i32Status )
		return i32Status;

	// Find the index in lookup table
	int32 i = GetIndexInLookupTable( DrvHdl );

	// Private system info
	// Only Plx base boards have private system Info extended
	if ( m_pHwLookupTable->DrvSystem[i].SysInfo.u32BoardType >= CS14200_BOARDTYPE )
	{
		m_pHwLookupTable->DrvSystem[i].SysInfoEx.u32Size = sizeof( CSSYSTEMINFOEX );
		i32Status = CsDrvGetParams(DrvHdl, CS_SYSTEMINFO_EX, &m_pHwLookupTable->DrvSystem[i].SysInfoEx );
		if ( CS_SUCCESS != i32Status )
			return i32Status;
	}

	m_pHwLookupTable->DrvSystem[i].AcqConfig.u32Size = sizeof( CSACQUISITIONCONFIG );
	i32Status = CsDrvGetParams( DrvHdl, CS_ACQUISITION,  &m_pHwLookupTable->DrvSystem[i].AcqConfig );
	if ( CS_SUCCESS != i32Status )
		return i32Status;

	// Reset variables
	m_pHwLookupTable->DrvSystem[i].pCapsInfo = NULL;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvAcquisitionSystemCleanup(DRVHANDLE DrvHdl)
{
	// Find the index in lookup table
	int32 i = GetIndexInLookupTable( DrvHdl );

	if ( i < 0 )
		return CS_INVALID_HANDLE;


	// Reset the flag for Async serial Data Transfer if there is any
	if ( m_pHwLookupTable->DrvSystem[i].hSerialAsyncTransfer )
	{
		// Make sure that the previous Asynchronous DataTransfer thread is terminated ...
		if ( WAIT_TIMEOUT == WaitForSingleObject( m_pHwLookupTable->DrvSystem[i].hSerialAsyncTransfer, 500 ) )
			return CS_MISC_ERROR;
		else
		{
			CloseHandle( m_pHwLookupTable->DrvSystem[i].hSerialAsyncTransfer );
			m_pHwLookupTable->DrvSystem[i].hSerialAsyncTransfer = NULL;
		}
	}

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	::DeviceAcqSystenCleanup( pDevice );

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetAcquisitionSystemInfo(DRVHANDLE DrvHdl, PCSSYSTEMINFO pSysInfo)
{
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );

	if ( ! pSystem )
		return CS_INVALID_HANDLE;

	if ( pSysInfo->u32Size > sizeof(CSSYSTEMINFO) )
		return CS_INVALID_STRUCT_SIZE;

	memcpy( pSysInfo, &pSystem->SysInfo, sizeof(CSSYSTEMINFO) );

	return CS_SUCCESS;

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsBaseHwDLL::CsDrvGetAcquisitionStatus(DRVHANDLE DrvHdl, uInt32 *u32Status)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	*u32Status =  ::DeviceGetAcqStatus( pDevice );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetParams(DRVHANDLE DrvHdl, uInt32 u32ParamId, void *ParamBuffer)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	int32 i32Status = ::DeviceGetParams( pDevice, u32ParamId,  ParamBuffer );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvSetParams(DRVHANDLE DrvHdl, uInt32 u32ParamId, void *ParamBuffer)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	int32 i32Status = ::DeviceSetParams( pDevice, u32ParamId,  ParamBuffer );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvStartAcquisition(DRVHANDLE DrvHdl)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	int32 i32Status = ::DeviceStartAcquisition( pDevice );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvAbort(DRVHANDLE DrvHdl)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	int32 i32Status = ::DeviceAbortAcquisition( pDevice );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return i32Status;
}




//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvReset(DRVHANDLE DrvHdl, BOOL bHardReset)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	::DeviceResetSystem( pDevice, bHardReset );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvForceTrigger(DRVHANDLE DrvHdl)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif

	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	::DeviceForceTrigger( pDevice );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvForceCalibration(DRVHANDLE DrvHdl)
{
	// Find the system
	PDRVSYSTEM		pSystem = GetAcqSystemPointer( DrvHdl );
	if ( ! pSystem )
		return CS_INVALID_HANDLE;

#ifdef _PROTECTACCESS
	EnterCriticalSection(&pSystem->CsProtectAccess);
#endif
	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);
	int32 i32Status = ::DeviceForceCalibration( pDevice );

#ifdef _PROTECTACCESS
	LeaveCriticalSection(&pSystem->CsProtectAccess);
#endif

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetBoardsInfo(DRVHANDLE DrvHdl, PARRAY_BOARDINFO pABoardInfo )
{
	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);

	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	return ::DeviceGetBoardsInfo( pDevice, pABoardInfo );

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsBaseHwDLL::CsDrvAutoFirmwareUpdate()
{

	PVOID pDevice = g_GlobalPerProcess.hNativeFirstCard;

	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	return ::DeviceAutoUpdateFirmware( pDevice );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvRegisterEventHandle(DRVHANDLE DrvHdl, uInt32 EventType, HANDLE *EHandle)
{
	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);

	EVENT_HANDLE *EventHandle = (EVENT_HANDLE *)EHandle;
	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	if ( EventType != ACQ_EVENT_END_BUSY && EventType != ACQ_EVENT_TRIGGERED )
		return CS_INVALID_EVENT_TYPE;

	in_REGISTER_EVENT_HANDLE	Temp;

	Temp.u32EventType = EventType;
	Temp.bExternalEvent = TRUE;		// Event created by application
	Temp.hUserEventHandle = *EventHandle;

	return ::DeviceRegisterEventHandle( pDevice, Temp );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvValidateParams( DRVHANDLE DrvHdl, uInt32 u32Coerce, PCSSYSTEMCONFIG pSysCfg )
{
	PVOID pDevice = GetAcqSystemCardPointer(DrvHdl);

	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	return ::DeviceValidateAcquisitionConfig( pDevice, pSysCfg->pAcqCfg, u32Coerce );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvTransferData( DRVHANDLE DrvHdl,
							 IN_PARAMS_TRANSFERDATA InParams,
							 POUT_PARAMS_TRANSFERDATA OutParams,
							 BOOL bMulRecRawDataTx )
{
	UNREFERENCED_PARAMETER(bMulRecRawDataTx);

	PVOID	pDevice = GetAcqSystemCardPointer(DrvHdl);

	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	in_DATA_TRANSFER			Cfg = {0};

	Cfg.InParams.u32Mode		= InParams.u32Mode;
	Cfg.InParams.u16Channel		= InParams.u16Channel;
	Cfg.InParams.i64Length		= InParams.i64Length;
	Cfg.InParams.pDataBuffer	= InParams.pDataBuffer;
	Cfg.InParams.u32Segment		= InParams.u32Segment;
	Cfg.InParams.i64StartAddress = InParams.i64StartAddress;
	Cfg.OutParams				= OutParams;

#ifdef _WINDOWS_
	if (InParams.hNotifyEvent)
		Cfg.InParams.hNotifyEvent = InParams.hNotifyEvent;
#endif

	return ::DeviceTransferData( pDevice, &Cfg );

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvTransferDataEx( DRVHANDLE DrvHdl,
							 PIN_PARAMS_TRANSFERDATA_EX InParams,
							 POUT_PARAMS_TRANSFERDATA_EX OutParams )
{
	PVOID	pDevice = GetAcqSystemCardPointer(DrvHdl);

	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	io_DATA_TRANSFER_EX			Cfg = {0};

	Cfg.OutParams				= OutParams;
	memcpy( &Cfg.InParams, InParams, sizeof( IN_PARAMS_TRANSFERDATA_EX ));

#ifdef _WINDOWS_
	if (InParams->hNotifyEvent)
		Cfg.InParams.hNotifyEvent = InParams->hNotifyEvent;
#endif

	return ::DeviceTransferDataEx( pDevice, &Cfg );

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetAsyncTransferDataResult(DRVHANDLE DrvHdl, uInt16 u16Channel,
												   CSTRANSFERDATARESULT *pTxDataResult,
                                                   BOOL bWait )
{
	PVOID	pDevice = GetAcqSystemCardPointer(DrvHdl);

	if ( ! pDevice )
		return CS_INVALID_HANDLE;

	return ::DeviceGetAsynTransDataResult( pDevice, u16Channel, pTxDataResult, bWait );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetAcquisitionSystemCaps(DRVHANDLE , PCSSYSTEMCAPS)
{
	// Should not receive this call
	_ASSERT( FALSE );

	return CS_INVALID_REQUEST;

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsBaseHwDLL::CsValidateBufferForReadWrite(uInt32 ParamID,
										 void *ParamBuffer,
										 BOOL bRead )
{
	BOOL	bTestStructSize = TRUE;
	BOOL	bBadPtr = FALSE;
	uInt32	ParamBuffSize;

	switch (ParamID)
	{
	case CS_SYSTEM:
		{
		PCSSYSTEMCONFIG pSysCfg = (PCSSYSTEMCONFIG) ParamBuffer;

		if (! (pSysCfg->pAcqCfg && pSysCfg->pAChannels && pSysCfg->pATriggers))
			return CS_NULL_POINTER;

		if (bRead)
		{
			// Validate all pointers to application buffer for Read operation
			bBadPtr |= IsBadReadPtr(pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
			bBadPtr |= IsBadReadPtr(pSysCfg->pAChannels, sizeof(ARRAY_CHANNELCONFIG) +
							(pSysCfg->pAChannels->u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
			bBadPtr |= IsBadReadPtr(pSysCfg->pATriggers, sizeof(ARRAY_TRIGGERCONFIG) +
							(pSysCfg->pATriggers->u32TriggerCount-1)* sizeof(CSTRIGGERCONFIG));
		}
		else
		{
			// Validate all pointers to application buffer for Read operation
			bBadPtr |= IsBadWritePtr(pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
			bBadPtr |= IsBadWritePtr(pSysCfg->pAChannels, sizeof(ARRAY_CHANNELCONFIG) +
							(pSysCfg->pAChannels->u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
			bBadPtr |= IsBadWritePtr(pSysCfg->pATriggers, sizeof(ARRAY_TRIGGERCONFIG) +
							(pSysCfg->pATriggers->u32TriggerCount-1)* sizeof(CSTRIGGERCONFIG));
		}

		if ( ! bBadPtr )
		{
			if ( pSysCfg->u32Size != sizeof(CSSYSTEMCONFIG) )
				return CS_INVALID_STRUCT_SIZE;

			for (uInt32 i = 0; i< pSysCfg->pATriggers->u32TriggerCount; i ++)
			{
				if ( pSysCfg->pATriggers->aTrigger[i].u32Size != sizeof(CSTRIGGERCONFIG))
					return CS_INVALID_STRUCT_SIZE;
			}
			for (uInt32 i = 0; i< pSysCfg->pAChannels->u32ChannelCount; i ++)
			{
				if ( pSysCfg->pAChannels->aChannel[i].u32Size != sizeof(CSCHANNELCONFIG))
					return CS_INVALID_STRUCT_SIZE;
			}
			return CS_SUCCESS;
		}
		else
			return CS_INVALID_POINTER_BUFFER;
		}
		break;

	case CS_ACQUISITION:
		{
		PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) ParamBuffer;
		ParamBuffSize = sizeof(CSACQUISITIONCONFIG);

		// Validate pointers to the application buffer for Read/Write operation
		if ( bRead )
			bBadPtr = IsBadReadPtr( ParamBuffer, ParamBuffSize );
		else
			bBadPtr = IsBadWritePtr( ParamBuffer, ParamBuffSize );

		if ( bBadPtr)
			return CS_INVALID_POINTER_BUFFER;
		else
		{
			if ( pAcqCfg->u32Size != sizeof(CSACQUISITIONCONFIG) )
				return CS_INVALID_STRUCT_SIZE;

			return CS_SUCCESS;
		}
		}
		break;

	case CS_TRIGGER:
		{
			PARRAY_TRIGGERCONFIG pATrigCfg = (PARRAY_TRIGGERCONFIG) ParamBuffer;

			ParamBuffSize = (pATrigCfg->u32TriggerCount-1) *
										sizeof(CSTRIGGERCONFIG) + sizeof(ARRAY_TRIGGERCONFIG);
			// Validate pointers to the application buffer for Read/Write operation
			if ( bRead )
				bBadPtr = IsBadReadPtr( pATrigCfg->aTrigger, ParamBuffSize );
			else
				bBadPtr = IsBadWritePtr( pATrigCfg->aTrigger, ParamBuffSize );

			if (  bBadPtr )
				return CS_INVALID_POINTER_BUFFER;
			else
			{
				for (uInt32 i = 0; i< pATrigCfg->u32TriggerCount; i ++)
				{
					if ( pATrigCfg->aTrigger[i].u32Size != sizeof(CSTRIGGERCONFIG))
						return CS_INVALID_STRUCT_SIZE;
				}
				return CS_SUCCESS;
			}
		}
		break;

	case CS_CHANNEL:
		{
			PARRAY_CHANNELCONFIG pAChanCfg = (PARRAY_CHANNELCONFIG) ParamBuffer;
			ParamBuffSize = (pAChanCfg->u32ChannelCount-1) *
										sizeof(CSCHANNELCONFIG) + sizeof(ARRAY_CHANNELCONFIG);
			// Validate pointers to the application buffer for Read/Write operation
			if ( bRead )
				bBadPtr = IsBadReadPtr( pAChanCfg->aChannel, ParamBuffSize );
			else
				bBadPtr = IsBadWritePtr( pAChanCfg->aChannel, ParamBuffSize );

			if (  bBadPtr )
				return CS_INVALID_POINTER_BUFFER;
			else
			{
				for (uInt32 i = 0; i< pAChanCfg->u32ChannelCount; i ++)
				{
					if ( pAChanCfg->aChannel[i].u32Size != sizeof(CSCHANNELCONFIG))
						return CS_INVALID_STRUCT_SIZE;
				}
				return CS_SUCCESS;
			}
		}
		break;

	case CS_BOARD_INFO:
		{
			PARRAY_BOARDINFO pABoardInfo = (PARRAY_BOARDINFO) ParamBuffer;
			ParamBuffSize = (pABoardInfo->u32BoardCount-1) *
										sizeof(CSBOARDINFO) + sizeof(ARRAY_BOARDINFO);
			// Validate pointers to the application buffer for Read/Write operation
			if ( bRead )
				bBadPtr = IsBadReadPtr( pABoardInfo->aBoardInfo, ParamBuffSize );
			else
				bBadPtr = IsBadWritePtr( pABoardInfo->aBoardInfo, ParamBuffSize );

			if (  bBadPtr )
				return CS_INVALID_POINTER_BUFFER;
			else
			{
				for (uInt32 i = 0; i< pABoardInfo->u32BoardCount; i ++)
				{
					if ( pABoardInfo->aBoardInfo[i].u32Size != sizeof(CSBOARDINFO) )
						return CS_INVALID_STRUCT_SIZE;
				}
				return CS_SUCCESS;
			}
		}
		break;

	case CS_READ_NVRAM:
	case CS_WRITE_NVRAM:
	{
		ParamBuffSize = sizeof(NVRAM_IMAGE);
		bTestStructSize = FALSE;
	}
	break;

	case CS_DELAY_LINE_MODE:
	{
		ParamBuffSize = sizeof(CS_PLX_DELAYLINE_STRUCT);
		bTestStructSize = FALSE;
	}
	break;

	case CS_MEM_READ_THRU:
	case CS_MEM_WRITE_THRU:
	{
		ParamBuffSize = sizeof(CS_MEM_TEST_STRUCT);
		bTestStructSize = FALSE;
	}
	break;

	case CS_EXTENDED_BOARD_OPTIONS:
	{
		ParamBuffSize = sizeof(int64);
		bTestStructSize = FALSE;
	}
	break;


	case CS_READ_EEPROM:
	case CS_WRITE_EEPROM:
		// The memory has been checked before getting here. When getting here
		// the pointer and size should be correct
		return CS_SUCCESS;

		break;

	case CS_READ_FLASH:
	case CS_WRITE_FLASH:
	{
		ParamBuffSize = sizeof(CS_GENERIC_EEPROM_READWRITE);
	}
	break;

	case CS_READ_DAC:
	case CS_SEND_DAC:
	{
		ParamBuffSize = sizeof(CS_SENDDAC_STRUCT);
	}
	break;

	case CS_UPDATE_CALIB_TABLE:
	{
		ParamBuffSize = sizeof(CS_UPDATE_CALIBTABLE_STRUCT);
	}
	break;

	case CS_WRITE_DELAY_LINE:
	{
		ParamBuffSize = sizeof(CS_DELAYLINE_STRUCT);
	}
	break;

	case CS_CALIBMODE_CONFIG:
	{
		ParamBuffSize = sizeof(CS_CALIBMODE_PARAMS);
	}
	break;

	case CS_TRIGADDR_OFFSET_ADJUST:
	{
		ParamBuffSize = sizeof(CS_TRIGADDRESS_OFFSET_STRUCT);
	}
	break;

	case CS_TRIGLEVEL_ADJUST:
	{
		ParamBuffSize = sizeof(CS_TRIGLEVEL_OFFSET_STRUCT);
	}
	break;

	case CS_TRIGGAIN_ADJUST:
	{
		ParamBuffSize = sizeof(CS_TRIGGAIN_STRUCT);
	}
	break;

	case CS_FIR_CONFIG:
	{
		ParamBuffSize = sizeof(CS_FIR_CONFIG_PARAMS);
	}
	break;

	case CS_SYSTEMINFO_EX:
	{
		ParamBuffSize = sizeof(CSSYSTEMINFOEX);
	}
	break;

	case CS_SMT_ENVELOPE_CONFIG:
	{
		ParamBuffSize = sizeof(CS_SMT_ENVELOPE_CONFIG_PARAMS);
	}
	break;

	case CS_SMT_HISTOGRAM_CONFIG:
	{
		ParamBuffSize = sizeof(CS_SMT_HISTOGRAM_CONFIG_PARAMS);
	}
	break;

	case CS_FFT_CONFIG:
	{
		ParamBuffSize = sizeof(CS_FFT_CONFIG_PARAMS);
	}
	break;

	case CS_FFTWINDOW_CONFIG:
	{
		if ( bRead )
			bBadPtr = IsBadReadPtr( ParamBuffer, sizeof(uInt32) );
		else
			bBadPtr = IsBadWritePtr( ParamBuffer, sizeof(uInt32) );

		uInt16	u16NbCoeffs = *((uInt16 *) ParamBuffer);
		ParamBuffSize = (u16NbCoeffs-1) * sizeof(int16) +  sizeof(PCS_FFTWINDOW_CONFIG_PARAMS);
		bTestStructSize = FALSE;
	}
	break;

	case CS_SAVE_OUT_CFG:
	case CS_MULREC_AVG_COUNT:
	{
		ParamBuffSize = sizeof(uInt32);
		bTestStructSize = FALSE;
	}
	break;

	case CS_TRIG_OUT_CFG:
	{
		ParamBuffSize = sizeof(CS_TRIG_OUT_CONFIG_PARAMS);
	}
	break;

	case CS_ADC_ALIGN:
	{
		ParamBuffSize = sizeof(CS_ADC_ALIGN_STRUCT);
	}
	break;

	case CS_TRIG_OUT:
	{
		ParamBuffSize = sizeof(CS_TRIG_OUT_STRUCT);
		bTestStructSize = FALSE;
	}
	break;

	case CS_CLOCK_OUT:
	{
		ParamBuffSize = sizeof(CS_CLOCK_OUT_STRUCT);
		bTestStructSize = FALSE;
	}
	break;

	case CS_SEND_CALIB_DC:
	case CS_READ_CALIB_A2D:
	{
		ParamBuffSize = sizeof(CS_CALIBDC_STRUCT);
	}
	break;

	case CS_SEND_CALIB_MODE:
	{
		ParamBuffSize = sizeof(CS_CALIBMODE_STRUCT);
	}
	break;

	case CS_AUX_IN:
	{
		ParamBuffSize = sizeof(CS_AUX_IN_STRUCT);
		bTestStructSize = FALSE;
	}
	break;

	case CS_AUX_OUT:
	{
		ParamBuffSize = sizeof(CS_AUX_OUT_STRUCT);
		bTestStructSize = FALSE;
	}
	break;
	
	case CS_SEND_NIOS_CMD:
	{
		ParamBuffSize = sizeof(CS_SEND_NIOS_CMD_STRUCT);
		bTestStructSize = FALSE;
	}
	break;

	case CS_FLASHING_LED:
	case CS_SELF_TEST_MODE:
	case CS_ADDON_RESET:
	case CS_TRIG_ENABLE:

		//Nothing to validate. Return success
		return CS_SUCCESS;

	default:
		return CS_INVALID_REQUEST;
	}


	if ( bRead )
		bBadPtr = IsBadReadPtr( ParamBuffer, ParamBuffSize );
	else
		bBadPtr = IsBadWritePtr( ParamBuffer, ParamBuffSize );

	if ( bBadPtr )
		return CS_INVALID_POINTER_BUFFER;
	else if ( bTestStructSize )
	{
		// For structtures which have u32Size member variable
		// This variable must be the first member (offset = 0 )
		// It is safe to access and read from this address using *uInt32 pointer
		if ( *((uInt32 *) ParamBuffer) != ParamBuffSize )
			return CS_INVALID_STRUCT_SIZE;
	}

    return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvDo(DRVHANDLE DrvHdl, uInt32 ActionID, void *ActionBuffer )
{
	// Find the index in lookup table
	int32 i = GetIndexInLookupTable( DrvHdl );

	if ( i < 0 )
		return CS_INVALID_HANDLE;

	switch (ActionID)
	{
	case ACTION_START:
		return CsDrvStartAcquisition(DrvHdl);
	case ACTION_FORCE:
		return CsDrvForceTrigger(DrvHdl);
	case ACTION_ABORT:
		return CsDrvAbort(DrvHdl);
	case ACTION_RESET:
	case ACTION_HARD_RESET:
	{
		int32	i32Status;
		i32Status = CsDrvReset(DrvHdl, ACTION_HARD_RESET == ActionID);

		CsDrvGetParams(DrvHdl, CS_ACQUISITION, &m_pHwLookupTable->DrvSystem[i].AcqConfig );
		return i32Status;
	}

	case ACTION_COMMIT:
		{
			int32 Status;

			Status = CsDrvSetParams( DrvHdl, CS_SYSTEM, ActionBuffer );

			// Keep a latest copy of Valid AcquisitionConfig.
			CsDrvGetParams( DrvHdl, CS_ACQUISITION, &m_pHwLookupTable->DrvSystem[i].AcqConfig );

			return Status;
		}
		break;

	case ACTION_TIMESTAMP_RESET:
		return CsDrvSetParams( DrvHdl, CS_TIMESTAMP_RESET, NULL );

	case ACTION_HISTOGRAM_RESET:
		return CsDrvSetParams( DrvHdl, CS_HISTOGRAM_RESET, NULL );

	case ACTION_CALIB:
		return CsDrvForceCalibration(DrvHdl);
		break;

	default:
		return CS_INVALID_REQUEST;
	}

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64	CsBaseHwDLL::CoerceSampleRate( int64 i64AnySampleRate, uInt32 u32Mode, uInt32 CardId,
						  CSSAMPLERATETABLE_EX *pSrTable, uInt32 u32SrSize )
{
	int64 i64UperSampleRate = pSrTable[0].PublicPart.i64SampleRate;
	int64 i64LowerSampleRate = i64UperSampleRate;

	u32SrSize = u32SrSize / sizeof(CSSAMPLERATETABLE_EX);

	uInt32 j;
	for (uInt32 i = 0; i < u32SrSize - 1; i ++)
	{
		bool	bFoundUper = false;
		// find the uper and lower sample rate
		for (j = i; j < u32SrSize ; j++)
		{
			if ( (CardId != 0) && (0 == (CardId & pSrTable[j].u32CardSupport)) )
				continue;
			if ( 0 == ( u32Mode & pSrTable[j].u32Mode ) )
				continue;

			if ( ! bFoundUper )
			{
				i64UperSampleRate = pSrTable[j].PublicPart.i64SampleRate;
				bFoundUper = true;
			}
			else
			{
				i64LowerSampleRate = pSrTable[j].PublicPart.i64SampleRate;
				break;
			}
		}

		// find the closest match
		if ( i64AnySampleRate >= i64UperSampleRate )
		{
			return	i64UperSampleRate;
		}
		else if ( i64AnySampleRate > i64LowerSampleRate )
		{
			int64 i64ToUper = i64UperSampleRate - i64AnySampleRate;
			int64 i64ToLower =  i64AnySampleRate - i64LowerSampleRate ;

			if ( i64ToUper <= i64ToLower )
				return i64UperSampleRate;
			else
				return i64LowerSampleRate;
		}

		i = j-1;
	}

	return i64LowerSampleRate;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsBaseHwDLL::CsDrvValidateTriggerConfig( PCSSYSTEMCONFIG pSysCfg,
												 PCSSYSTEMINFO pSysInfo,
                                                 uInt32 Coerce )
{
	PCSTRIGGERCONFIG	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	int32				i32Status = CS_SUCCESS;


	if ( pSysCfg->pATriggers->u32TriggerCount > pSysInfo->u32TriggerMachineCount )
	{
		if ( Coerce )
		{
			pSysCfg->pATriggers->u32TriggerCount = pSysInfo->u32TriggerMachineCount;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIGGER_COUNT;
	}


	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
	{
		//----- trigger count: must be 0 < count < max_count
		if ( pTrigCfg->u32TriggerIndex <= 0 ||
			pTrigCfg->u32TriggerIndex > pSysInfo->u32TriggerMachineCount )
		{
			if ( Coerce )
			{
				pTrigCfg->u32TriggerIndex = 1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIGGER;
		}

		//----- condition: might be pos, neg or stg else TBD
		if ( pTrigCfg->u32Condition != CS_TRIG_COND_POS_SLOPE &&
			 pTrigCfg->u32Condition != CS_TRIG_COND_NEG_SLOPE  )
		{
			if ( Coerce )
			{
				pTrigCfg->u32Condition = CS_TRIG_COND_POS_SLOPE;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_COND;
		}

		//----- level: must be -100<=level<=100
		if ( pTrigCfg->i32Level > 100 || pTrigCfg->i32Level < -100  )
		{
			if ( Coerce )
			{
				pTrigCfg->i32Level = 0;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_LEVEL;
		}

		//----- coupling for external trigger: must be either AC or DC
		if ( pTrigCfg->i32Source == CS_TRIG_SOURCE_EXT )
		{
			if ( (pTrigCfg->u32ExtCoupling != CS_COUPLING_AC) &&
				(pTrigCfg->u32ExtCoupling != CS_COUPLING_DC ) )
			{
				if ( Coerce )
				{
					pTrigCfg->u32ExtCoupling = CS_COUPLING_DC;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_COUPLING;
			}

			//----- gain for External Trig,
			if ( (pTrigCfg->u32ExtTriggerRange != CS_GAIN_2_V) &&
				(pTrigCfg->u32ExtTriggerRange != CS_GAIN_10_V) )
			{
				if ( Coerce )
				{
					pTrigCfg->u32ExtTriggerRange = CS_GAIN_2_V;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_GAIN;
			}
		}

	}

	return i32Status;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetAvailableTriggerSources(CSHANDLE DrvHdl, uInt32 SubCapsId, int32 *i32ArrayTriggerSources,
													uInt32 *BufferSize )
{
	PCSSYSTEMINFO	pSysInfo;

	// Search for SystemInfo of the current system based on DrvHdl
	// Find the index in lookup table
	int32 j = GetIndexInLookupTable( DrvHdl );
	
	if ( j < 0 )
		return 0;

	pSysInfo = &m_pHwLookupTable->DrvSystem[j].SysInfo;

	uInt32	i;
	uInt32	k;
	uInt32	u32ChannelSkip = 0;

	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	// Calculate number of trigger source available
	uInt32			u32NumberTriggerSources = 0;
	
	u32NumberTriggerSources = 2; 		// External triggers + Disable

	if ( SubCapsId == 0 )
		u32NumberTriggerSources += pSysInfo->u32ChannelCount;   // Channels Trigger
	else
	{
		uInt32	u32ChannelsPerCard = pSysInfo->u32ChannelCount / pSysInfo->u32BoardCount;

		if ( u32ChannelsPerCard == 1 )
		{
			u32NumberTriggerSources += pSysInfo->u32ChannelCount;   // Channels Trigger
			u32ChannelSkip = 0;
		}
		else if ( u32ChannelsPerCard == 2 )
		{
			if ( SubCapsId == 0 || ( SubCapsId & CS_MODE_DUAL ) )
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount;   // Channels Trigger
				u32ChannelSkip = 0;
			}
			else
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount / 2;   // Channels Trigger
				u32ChannelSkip = 1;
			}
		}
		else if ( u32ChannelsPerCard == 4 )
		{
			if ( SubCapsId & CS_MODE_QUAD )
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount;   // Channels Trigger
				u32ChannelSkip = 0;
			}
			if ( SubCapsId & CS_MODE_DUAL )
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount / 2;   // Channels Trigger
				u32ChannelSkip = 1;
			}
			else
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount / 4;   // Channels Trigger
				u32ChannelSkip = 3;
			}
		}
		else if ( u32ChannelsPerCard == 8 )
		{
			if ( SubCapsId & CS_MODE_OCT )
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount;   // Channels Trigger
				u32ChannelSkip = 0;
			}
			if ( SubCapsId & CS_MODE_QUAD )
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount / 2;   // Channels Trigger
				u32ChannelSkip = 1;
			}
			if ( SubCapsId & CS_MODE_DUAL )
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount / 4;   // Channels Trigger
				u32ChannelSkip = 3;
			}
			else
			{
				u32NumberTriggerSources += pSysInfo->u32ChannelCount / 8;   // Channels Trigger
				u32ChannelSkip = 7;
			}
		}

	}

	if ( i32ArrayTriggerSources == NULL )
	{
		*BufferSize = u32NumberTriggerSources * sizeof(uInt32);
		return CS_SUCCESS;
	}
	else 
	{
		if ( *BufferSize < u32NumberTriggerSources * sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			k = 0;
			i32ArrayTriggerSources[k++] = -1;	// External trigger from master
			i32ArrayTriggerSources[k++] = 0;	// Disable
	
			for( i = 0; i < pSysInfo->u32ChannelCount; i ++ )
			{
				i32ArrayTriggerSources[k++] =  (int32) (i+1);
				i += u32ChannelSkip;
			}

			*BufferSize = u32NumberTriggerSources * sizeof(uInt32);
			return CS_SUCCESS;
		}
	}

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetHwOptionText(PCS_HWOPTIONS_CONVERT2TEXT pHwOptionText)
{

	if ( IsBadReadPtr( pHwOptionText,  sizeof(CS_HWOPTIONS_CONVERT2TEXT) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pHwOptionText->u32Size )
		return CS_INVALID_STRUCT_SIZE;

	// Return HW options for Combine base board
	if ( pHwOptionText->bBaseBoard )
	{
		char szBBOptions[][HWOPTIONS_TEXT_LENGTH] = {
									"Slow Nios",
									"No SODIMM"
								  };

		if ( IsBadReadPtr( pHwOptionText,  sizeof(CS_HWOPTIONS_CONVERT2TEXT) ) )
			return CS_INVALID_POINTER_BUFFER;

		if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pHwOptionText->u32Size )
			return CS_INVALID_STRUCT_SIZE;

		// Mask for options that require special BB firmware
		pHwOptionText->u32RevMask = CSMV_BBHW_OPT_FWREQUIRED_MASK;

		// Options text from base board
		uInt32	nSize = sizeof( szBBOptions ) / HWOPTIONS_TEXT_LENGTH;

		for (uInt32 i = 0; i < nSize; i++)
		{
			if ( i == pHwOptionText->u8BitPosition )
			{
				strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), szBBOptions[i] );
				return CS_SUCCESS;
			}
		}

		strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "??" );
		return CS_SUCCESS;
	}
	else
	{
		// HW Addon options for Cs14200, 12400 and Cs14105
		char szAddonOptions[][HWOPTIONS_TEXT_LENGTH] =
									{	"Standard Addon",
										"No Analog Trigger",
										"No Reference Clock"
									};

		// Mask for options that require special AO firmware
		pHwOptionText->u32RevMask = CSMV_AOHW_OPT_FWREQUIRED_MASK;

		// Options text from base board
		uInt32	nSize = sizeof( szAddonOptions ) / HWOPTIONS_TEXT_LENGTH;

		for (uInt32 i = 0; i < nSize; i++)
		{
			if ( i == pHwOptionText->u8BitPosition )
			{
				strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), szAddonOptions[i] );
				return CS_SUCCESS;
			}
		}

		strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "??" );
		return CS_SUCCESS;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsBaseHwDLL::CsDrvGetFwOptionText(PCS_HWOPTIONS_CONVERT2TEXT pFwOptionText)
{
	if ( IsBadReadPtr( pFwOptionText,  sizeof(CS_FWOPTIONS_CONVERT2TEXT) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pFwOptionText->u32Size )
		return CS_INVALID_STRUCT_SIZE;

	// Return HW options for Combine base board
	if ( pFwOptionText->bBaseBoard )
	{
		char szBBOptions[32][HWOPTIONS_TEXT_LENGTH] = {
									"??", //Finite Impulse Response",
									"HW Average",
									"MinMax Detection",
									"Cascade Streaming",
									"MulRec Averaging",
									"Storage media testing",
									"FFT 0512",
									"FFT 1024",
									"FFT 2048",
									"FFT 4096",
									"MulRec Avg TD",
									"Coin filter",
									"Gate Acquisition",
									"Data Streaming",
									"Histogram",
									"AVG Streaming",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"??",
									"Big Trigger Address"
								  };

		// Mask for Expert options
		pFwOptionText->u32RevMask = CS_BBOPTIONS_EXPERT_MASK;

		// Options text from base board
		uInt32	nSize = sizeof( szBBOptions ) / HWOPTIONS_TEXT_LENGTH;

		for (uInt32 i = 0; i < nSize; i++)
		{
			if ( i == pFwOptionText->u8BitPosition )
			{
				strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), szBBOptions[i] );
				return CS_SUCCESS;
			}
		}

		strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "??" );
		return CS_FALSE;
	}
	else
	{
		char szAddonOptions[][HWOPTIONS_TEXT_LENGTH] =
									{	"Digital Trigger"
									};

		// Mask for Expert options
		pFwOptionText->u32RevMask = 0;

		// Options text from base board
		uInt32	nSize = sizeof( szAddonOptions ) / HWOPTIONS_TEXT_LENGTH;

		for (uInt32 i = 0; i < nSize; i++)
		{
			if ( i == pFwOptionText->u8BitPosition )
			{
				strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), szAddonOptions[i] );
				return CS_SUCCESS;
			}
		}

		strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "??" );
		return CS_FALSE;
	}
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsBaseHwDLL::CsDrvGetFlashLayoutInfo(PCS_FLASHLAYOUT_INFO	pCsFlashLayoutInfo )
{
	if ( IsBadWritePtr( pCsFlashLayoutInfo, sizeof(CS_FLASHLAYOUT_INFO) ) )
		return CS_INVALID_POINTER_BUFFER;


	pCsFlashLayoutInfo->u32FlashType = 0;
	pCsFlashLayoutInfo->u32NumOfImages = 3;
	pCsFlashLayoutInfo->u32CalibInfoOffset = (uInt32) -1;	// Not available
	pCsFlashLayoutInfo->u32FwInfoOffset = (uInt32) -1;		// Not available
	pCsFlashLayoutInfo->u32FlashFooterOffset = (uInt32) -1;	// Not available
	pCsFlashLayoutInfo->u32WritableStartOffset = 0;


	if ( pCsFlashLayoutInfo->u32Addon )
	{
		pCsFlashLayoutInfo->u32ImageStructSize = FIELD_OFFSET( AddOnFlashData, Image2 );
		pCsFlashLayoutInfo->u32CalibInfoOffset = FIELD_OFFSET( AddOnFlashData, Calib1 );
		pCsFlashLayoutInfo->u32FwBinarySize = 433*1024;
	}
	else
	{
		pCsFlashLayoutInfo->u32ImageStructSize = BB_FLASHIMAGE_SIZE;
		pCsFlashLayoutInfo->u32ImageStructSize =  FIELD_OFFSET( BaseBoardFlashData, OptionnalImage1 );
		pCsFlashLayoutInfo->u32FwBinarySize = 965*1024;
	}

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
PDRVSYSTEM	CsBaseHwDLL::GetAcqSystemPointer(DRVHANDLE	DrvHdl)
{
	PDRVSYSTEM	pSystem;

	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i ++ )
	{
		pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];
		if (pSystem->DrvHandle == DrvHdl)
			return ( pSystem );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

PVOID	CsBaseHwDLL::GetAcqSystemCardPointer(DRVHANDLE	DrvHdl)
{
	PDRVSYSTEM	pSystem;

	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i ++ )
	{
		pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];

		if (pSystem->DrvHandle == DrvHdl)
			return ::DeviceGetCardPointerFromNativeIndex( g_GlobalPerProcess.hNativeFirstCard,
														  pSystem->u8FirstCardIndex );
	}

	return NULL;
}
