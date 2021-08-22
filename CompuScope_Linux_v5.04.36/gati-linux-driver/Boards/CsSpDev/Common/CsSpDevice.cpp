#include "stdafx.h"
#include "CsBaseHwDLL.h"
#include "CsSpDevice.h"
#include "rm.h"
#include "CsAdqUpdate.h"
#include "CsUsbmsglog.h"
#include "CsSupport.h"

extern SHAREDDLLMEM Globals;

const SpDevDef CsSpDeviceManager::DevType[] = 
{  //ADQType	Name     strCsName    BaseFr     MaxDiv   Res  MemSize Bits #Chan lExtTrigAddrAdj
		112,	"112",  "CS121G11", 2200000000,   28,   -2048,  128,   12,    1,        256,
//		212,	"212",  "CS125502", 1100000000,   28,   -2048,  78,    12,    2,        128,
		114,	"114",  "CS148001", 1600000000,   20,   -8192,  128,   14,    1,        256,
		214,	"214",  "CS144002",  800000000,   20,   -8192,  128,   14,    2,         32,
		108,	"108",  "CS0864G1", 6400000000,   20,    -128,  128,    8,    1,          0,
};

const SpDevSr CsSpDeviceManager::SrTable108_fcs[] = 
{//	i64Sr       nDiv     uSkip
	6400000000,  2,        1, // 6.4 Ghz
};


const SpDevSr CsSpDeviceManager::SrTable112_fcs[] = 
{//	i64Sr       nDiv     uSkip
	1100000000,  2,        1, // 1.1 Ghz
	 550000000,  2,        2, // 550 MHz
	 200000000, 11,        1, // 200 MHz
	 100000000, 11,        2, // 100 MHz
	  50000000, 11,        4, //  50 MHz
	  25000000, 11,        8, //  25 MHz
	  10000000, 11,       20, //  10 MHz
	   5000000, 11,       40, //   5 MHz
	   2000000, 11,      100, //   2 MHz
	   1000000, 11,      200, //   1 MHz
	    500000, 11,      400, // 500 kHz
	    200000, 11,     1000, // 200 kHz
	    100000, 11,     2000, // 100 kHz
		 50000, 11,     4000, //  50 kHz
	     20000, 11,    10000, //  20 kHz
	     10000, 11,    20000, //  10 kHz
};

const SpDevSr CsSpDeviceManager::SrTable114_fcs[] = 
{//	i64Sr       nDiv     uSkip
	 800000000,  2,        1, // 800 Mhz
	 400000000,  2,        2, // 400 MHz
	 200000000,  2,        4, // 200 MHz
	 100000000,  2,        8, // 100 MHz
	  50000000,  2,       16, //  50 MHz
	  25000000,  2,       32, //  25 MHz
	  10000000,  2,       80, //  10 MHz
	   5000000,  2,      160, //   5 MHz
	   2000000,  2,      400, //   2 MHz
	   1000000,  2,      800, //   1 MHz
	    500000,  2,     1600, // 500 kHz
	    200000,  2,     4000, // 200 kHz
	    100000,  2,     8000, // 100 kHz
		 50000,  2,    15000, //  50 kHz
	     20000,  2,    40000, //  20 kHz
	     10000,  2,    80000, //  10 kHz
};

const SpDevSr CsSpDeviceManager::SrTable214_fcs[] = 
{//	i64Sr       nDiv     uSkip
	 400000000,  2,        1, // 400 MHz
	 200000000,  2,        2, // 200 MHz
	 100000000,  2,        4, // 100 MHz
	  50000000,  2,        8, //  50 MHz
	  25000000,  2,       16, //  25 MHz
	  10000000,  2,       40, //  10 MHz
	   5000000,  2,       80, //   5 MHz
	   2000000,  2,      200, //   2 MHz
	   1000000,  2,      400, //   1 MHz
	    500000,  2,      800, // 500 kHz
	    200000,  2,     2000, // 200 kHz
	    100000,  2,     4000, // 100 kHz
		 50000,  2,     8000, //  50 kHz
	     20000,  2,    20000, //  20 kHz
	     10000,  2,    40000, //  10 kHz
};

/////////////////////////////////////////////////////////////

const SpDevSr CsSpDeviceManager::SrTable108_fcd[] = 
{//	i64Sr       nDiv     uSkip
	6400000000,  2,        1, // 6.4 Ghz
};


const SpDevSr CsSpDeviceManager::SrTable112_fcd[] = 
{//	i64Sr       nDiv     uSkip
	1100000000,  2,        1, // 1.1 Ghz
	 550000000,  4,        1, // 550 MHz
	 200000000, 11,        1, // 200 MHz
	 100000000, 11,        2, // 100 MHz
	  50000000, 11,        4, //  50 MHz
	  25000000, 11,        8, //  25 MHz
	  10000000, 11,       20, //  10 MHz
	   5000000, 11,       40, //   5 MHz
	   2000000, 11,      100, //   2 MHz
	   1000000, 11,      200, //   1 MHz
	    500000, 11,      400, // 500 kHz
	    200000, 11,     1000, // 200 kHz
	    100000, 11,     2000, // 100 kHz
		 50000, 11,     4000, //  50 kHz
	     20000, 11,    10000, //  20 kHz
	     10000, 11,    20000, //  10 kHz
};

const SpDevSr CsSpDeviceManager::SrTable114_fcd[] = 
{//	i64Sr       nDiv     uSkip
	 800000000,  2,        1, // 800 Mhz
	 400000000,  4,        1, // 400 MHz
	 200000000,  8,        1, // 200 MHz
	 100000000, 16,        1, // 100 MHz
	  50000000, 16,        2, //  50 MHz
	  25000000, 16,        4, //  25 MHz
	  10000000, 16,       10, //  10 MHz
	   5000000, 16,       20, //   5 MHz
	   2000000, 16,       50, //   2 MHz
	   1000000, 16,      100, //   1 MHz
	    500000, 16,      200, // 500 kHz
	    200000, 16,      500, // 200 kHz
	    100000, 16,     1000, // 100 kHz
		 50000, 16,     2000, //  50 kHz
	     20000, 16,     5000, //  20 kHz
	     10000, 16,    10000, //  10 kHz
};

const SpDevSr CsSpDeviceManager::SrTable214_fcd[] = 
{//	i64Sr       nDiv     uSkip
	 400000000,  2,        1, // 400 MHz
	 200000000,  4,        1, // 200 MHz
	 100000000,  8,        1, // 100 MHz
	  50000000, 16,        1, //  50 MHz
	  25000000, 16,        2, //  25 MHz
	  12500000, 16,        4, //12.5 MHz
	   5000000, 16,       10, //   5 MHz
	   2500000, 16,       20, // 2.5 MHz
	   1000000, 16,       50, //   1 MHz
	    500000, 16,      100, // 500 kHz
	    200000, 16,      250, // 200 kHz
	    100000, 16,      500, // 100 kHz
		 50000, 16,     1000, //  50 kHz
	     20000, 16,     2500, //  20 kHz
	     10000, 16,     5000, //  10 kHz
};



#define ADQ_DLL _T("CsUsbApi.dll")
#define EVENT_LOG_NAME _T("CsUsb")

CsEventLog::CsEventLog(void) : m_hEventLog(NULL), m_dwError(0)
{
	m_hEventLog = ::RegisterEventSource(NULL, EVENT_LOG_NAME);

	if ( NULL == m_hEventLog )
	{
		m_dwError = ::GetLastError();
#ifndef CS_WIN64
		TRACE(DRV_TRACE_LEVEL, _T("Could not open event source Error %d"), m_dwError);
#endif
	}
}

CsEventLog::~CsEventLog(void)
{
	if ( NULL != m_hEventLog )
	{
		::DeregisterEventSource(m_hEventLog);
		m_hEventLog = NULL;
	}
}

BOOL CsEventLog::Report(DWORD dwErrorId, LPTSTR lpMsgIn)
{
	BOOL bRet = TRUE;

	LPTSTR lpszStrings[2] = { NULL, NULL };

	WORD wEventType(EVENTLOG_SUCCESS), wMessageCount(1);
	switch ( dwErrorId >> 30 )
	{
			case STATUS_SEVERITY_ERROR:         wEventType = EVENTLOG_ERROR_TYPE; break;
			case STATUS_SEVERITY_WARNING:       wEventType = EVENTLOG_WARNING_TYPE; break;
			case STATUS_SEVERITY_INFORMATIONAL: wEventType = EVENTLOG_INFORMATION_TYPE; break;
			case STATUS_SEVERITY_SUCCESS:       wEventType = EVENTLOG_SUCCESS; wMessageCount = 0; break;
	}


	lpszStrings[0] = lpMsgIn;

	bRet = ::ReportEvent(	m_hEventLog,			// handle of event source
							wEventType,				// event type
							0,						// event category
							dwErrorId,				// event ID
							NULL,					// current user's SID
							wMessageCount,			// strings in lpszStrings
							0,						// no bytes of raw data
							(LPCSTR *)lpszStrings,	// array of error strings
							NULL);					// no raw data
	
	if ( ! bRet )
	{
		// GET THE ERROR, or set m_nError, etc
		m_dwError = ::GetLastError();
#ifndef CS_WIN64
		TRACE(DRV_TRACE_LEVEL, _T("Error %d in ReportEvent"), m_dwError);
#endif
	}
	return bRet;
}


CsSpDeviceManager::CsSpDeviceManager(void): 
	m_nTypeCount(0), m_pControl(NULL), m_bInitOK(true)
{
	HMODULE hDllMod = ::GetModuleHandle( ADQ_DLL );
	if( NULL == hDllMod )
		hDllMod = ::LoadLibrary( ADQ_DLL );

	if( NULL == hDllMod )
	{
		// set variable to false.  if m_bInitOK is false when
		// we call MakeDevicesAndControl we'll just return right away.
		// The map will never be populated and the number of devices will
		// be set to 0
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot load %s"), ADQ_DLL);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szError);

		m_bInitOK = false;
	}

	if ( !ResolveProcAddresses(hDllMod) )
	{
		// Event log reporting done in ResolveProcAddresses
		m_bInitOK = false;
	}

	m_nApiRev = m_pfGetApiRevision();

	m_hCommEventHandle = ::OpenEvent( SYNCHRONIZE|EVENT_MODIFY_STATE,  FALSE, GAGERESOURCEMANAGERUSBCOMMUNICATIONEVENT_NAME);

	if ( CS_FAILED(MakeDevicesAndControl()) )
	{
		// Event log reporting done in MakeDevicesAndControl
		m_bInitOK = false; // may have already been set to false and that's why we returned
	}
}

CsSpDeviceManager::~CsSpDeviceManager(void)
{
	DeleteDevicesAndControl();
	::CloseHandle( m_hCommEventHandle );
	
	HMODULE hDllMod = ::GetModuleHandle( ADQ_DLL );
	if( NULL != hDllMod )
	{
		::FreeLibrary( hDllMod );
	}
}

int CsSpDeviceManager::MakeDevicesAndControl(void)
{
	if ( !m_bInitOK ) // in this case we won't write to the eventlog because we already have in the constructor
	{
		return CS_MISC_ERROR;
	}
	m_pControl = m_pfCreateControl();

	int nFoundDev = m_pfFindDevices(m_pControl); 
	DRVHANDLE drvHandle = DRVHANDLE(CSUSB_BASE_HANDLE);
	for ( int i = 1; i <= nFoundDev; i++ )
	{
		try 
		{
			CsSpDevice* pDev = new CsSpDevice( this,  i );
			m_mapDev.insert(USBDEVMAP::value_type(drvHandle, pDev));
			drvHandle++;
		}
		catch (CsSpdevException ex)
		{
			// Report execption to event log and / or TRACE statement
#ifndef CS_WIN64
			TRACE(DRV_TRACE_LEVEL, _T("%s\n"), ex._cause);
#endif
			// report to event log
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_TEXT, ex._cause);
			return CS_MISC_ERROR;
		}
	}
	m_nFailedNum = m_pfGetFailedDevCount( m_pControl );
	if( 0 != m_nFailedNum )
	{
		TCHAR szMessage[MAX_PATH];
		::_stprintf_s(szMessage, MAX_PATH, _T("%d failed USB devices"), m_nFailedNum);
#ifndef CS_WIN64
		TRACE(DRV_TRACE_LEVEL, _T("%s\n"), szMessage);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szMessage);
		return CS_MISC_ERROR;
	}
	return nFoundDev;
}

void CsSpDeviceManager::DeleteDevicesAndControl(void)
{
	USBDEVMAP::iterator theMapIterator;
	if ( !m_mapDev.empty() )
	{
		for (theMapIterator = m_mapDev.begin(); theMapIterator  != m_mapDev.end(); theMapIterator++)
		{
			delete ((*theMapIterator).second);
		}
	}
	m_mapDev.clear();

	if( NULL != m_pControl ) m_pfDeleteControl(m_pControl);
}

const SpDevDef* CsSpDeviceManager::GetDevTypeTable(size_t* pSize)
{
	if( NULL != pSize && !::IsBadWritePtr(pSize, sizeof(size_t)) )
		*pSize = sizeof(CsSpDeviceManager::DevType)/sizeof(SpDevDef); 
	return DevType; 
}

int CsSpDeviceManager::ResolveProcAddresses(HMODULE _hDllMod)
{
	m_pfCreateControl = (_AdqCreateControlUnit*)::GetProcAddress(_hDllMod, _T("CreateADQControlUnit") );
	if ( !m_pfCreateControl )
	{
		::FreeLibrary(_hDllMod);
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot find CreateADQControlUnit in %s"), ADQ_DLL);
#ifndef CS_WIN64
		TRACE( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szError);
		return 0;
	}
	m_pfDeleteControl = (_AdqDeleteControlUnit*)::GetProcAddress(_hDllMod, _T("DeleteADQControlUnit") );
	if ( !m_pfDeleteControl )
	{
		::FreeLibrary(_hDllMod);
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot find DeleteADQControlUnit in %s"), ADQ_DLL);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szError);
		return 0;
	}
	m_pfGetApiRevision = (_AdqGetApiRevision*)::GetProcAddress(_hDllMod, _T("ADQAPI_GetRevision"));
	if ( !m_pfGetApiRevision )
	{
		::FreeLibrary(_hDllMod);
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot find ADQAPI_GetRevision in %s"), ADQ_DLL);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szError);
		return 0;
	}
	m_pfFindDevices = (_AdqFindDevices*)::GetProcAddress(_hDllMod, _T("ADQControlUnit_FindDevices"));
	if ( !m_pfFindDevices )
	{
		::FreeLibrary(_hDllMod);
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot find ADQControlUnit_FindDevices in %s"), ADQ_DLL);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szError);
		return 0;
	}
	m_pfGetFailedDevCount = (_AdqGetFailedDeviceCount*)::GetProcAddress(_hDllMod, _T("ADQControlUnit_GetFailedDeviceCount"));
	if ( !m_pfGetFailedDevCount )
	{
		::FreeLibrary(_hDllMod);
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot find ADQControlUnit_GetFailedDeviceCount in %s"), ADQ_DLL);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szError);
		return 0;
	}
	m_pfGetAdqInterface = (_AdqGetADQInterface*)::GetProcAddress(_hDllMod, _T("ADQControlUnit_GetADQ"));
	if ( !m_pfGetAdqInterface )
	{
		::FreeLibrary(_hDllMod);
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot find ADQControlUnit_GetADQ in %s"), ADQ_DLL);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, szError);
		return 0;
	}
	return CS_SUCCESS;
}


int32 CsSpDeviceManager::GetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32 )
{
	USBDEVMAP::iterator theMapIterator;
	if (!m_mapDev.empty())
	{
		uInt32 u32Index = 0;
		for (theMapIterator = m_mapDev.begin(); theMapIterator  != m_mapDev.end(); theMapIterator++)
		{
			pDrvHdl[u32Index] = Globals.it()->g_StaticLookupTable.DrvSystem[u32Index].DrvHandle = (*theMapIterator).first;
			u32Index++;
		}
	}
	return static_cast<uInt32>(m_mapDev.size());
}

int32 CsSpDeviceManager::GetAcquisitionSystemCount(uInt16* pu16SystemFound)
{ 
	int32 i32Ret = CS_SUCCESS;
	*pu16SystemFound = (uInt16)GetDevNum(); 
	TCHAR szText[128];
	CsEventLog EventLog;
	USBDEVMAP::iterator theMapIterator;
	if (!m_mapDev.empty())
	{
		::_stprintf_s(szText, _countof(szText), _T("Found %d Usb CompuScope system(s)"), *pu16SystemFound );
		EventLog.Report(CS_INFO_TEXT,szText);
		
		PVOID	OldValue = NULL;
		BOOL	bRet = TRUE;

		if ( Is32on64() )
		{
			// Disable file system direction so we'll find the csi files in system32 on a 64 bit system when running 32 bit
			LPFN_WOW64DISABLEWOW64FSREDIRECTION pfnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64DisableWow64FsRedirection"));
			if ( NULL != pfnWow64DisableWow64FsRedirection )
			{
				bRet = pfnWow64DisableWow64FsRedirection (&OldValue);
			}
		}
		for (theMapIterator = m_mapDev.begin(); theMapIterator  != m_mapDev.end(); theMapIterator++)
		{
			if ( CS_SUCCEEDED((*theMapIterator).second->ReadAndValidateCsiFile()) )
			{
				// compare the version in the csi file to the version on the board
				i32Ret = (*theMapIterator).second->CompareFirmwareVersions();	
			}
		}
		if ( Is32on64() & bRet)
		{
			// Restore the previous WOW64 file system redirection value.
			LPFN_WOW64REVERTWOW64FSREDIRECTION pfnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64RevertWow64FsRedirection"));
			if ( (NULL != pfnWow64RevertWow64FsRedirection) )
			{
				pfnWow64RevertWow64FsRedirection(OldValue);
			}
		}
	}
	if( 0 != GetFailedNum() )
	{
		::_stprintf_s(szText, _countof(szText), _T("%d Usb CompuScope system(s) failed detection"), GetFailedNum() );
		EventLog.Report(CS_INFO_TEXT,szText);
		*pu16SystemFound += (uInt16)GetFailedNum();
	}
	int32 i32RetValue =  (0 == GetFailedNum()) ? i32Ret: CS_INVALID_FW_VERSION; 
	if ( CS_SUCCEEDED(i32RetValue) )
	{
		// report to event log
		for (theMapIterator = m_mapDev.begin(); theMapIterator  != m_mapDev.end(); theMapIterator++)
		{
			::_stprintf_s(szText, _countof(szText), _T("%s system %s initialized"), (*theMapIterator).second->GetDeviceName(), (*theMapIterator).second->GetSerialNumber());
			EventLog.Report(CS_INFO_TEXT, szText);
		}
	}
	return i32RetValue;
}


int32 CsSpDeviceManager::AutoFirmwareUpdate(void)
{
	USBDEVMAP::iterator theMapIterator;
	int32 i32Ret = CS_SUCCESS;
	int nOriginalCount = (int)GetFailedNum() + (int)GetDevNum();

	if ( nOriginalCount != 0)
	{
		bool bForceUpdateComm = false;
		bool bForceUpdateAlgo = false;
		HKEY hKey = NULL;
		//Need to read it again from RM
		if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, USB_PARAMS_KEY, 0, KEY_QUERY_VALUE, &hKey) )
		{
			DWORD dw = 0;
			DWORD dwDataSize = sizeof(DWORD);
			::RegQueryValueEx( hKey, _T("ForceFirmwareUpdateComm"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
			bForceUpdateComm = dw != 0;

			dw = 0;
			dwDataSize = sizeof(DWORD);
			::RegQueryValueEx( hKey, _T("ForceFirmwareUpdateAlgo"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
			bForceUpdateAlgo = dw != 0;

			::RegCloseKey( hKey );
			hKey = NULL;
		}
		
		DeleteDevicesAndControl();

		CsAdqUpdate* pAdq = NULL;

		try
		{
			pAdq = new CsAdqUpdate( bForceUpdateAlgo, bForceUpdateComm );
		}
		catch ( CsSpdevException exSpDev )
		{
			MakeDevicesAndControl();
			return CS_MISC_ERROR;
		}

		if ( (NULL == pAdq) || (::IsBadReadPtr(pAdq, sizeof(pAdq))) )
		{
			MakeDevicesAndControl();
			return CS_NULL_POINTER;
		}

		int nFoundDev = pAdq->Initialize();
		
		if ( nFoundDev !=  nOriginalCount )
		{
			MakeDevicesAndControl();
			return CS_MISC_ERROR;
		}

		PVOID	OldValue = NULL;
		BOOL	bRet = TRUE;
		if ( Is32on64() )
		{
			// Disable file system direction so we'll find the csi files in system32 on a 64 bit system when running 32 bit			
			LPFN_WOW64DISABLEWOW64FSREDIRECTION pfnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64DisableWow64FsRedirection"));
			if ( NULL != pfnWow64DisableWow64FsRedirection )
			{
				bRet = pfnWow64DisableWow64FsRedirection (&OldValue);
			}
		}
		for ( int i = 1; i <= nFoundDev; i++ )
		{
			i32Ret = pAdq->UpdateFirmware(i);
			if ( CS_FAILED(i32Ret) )
			{
				break;
			}
		}
		if ( Is32on64() && bRet)
		{
			// Restore the previous WOW64 file system redirection value.
			LPFN_WOW64REVERTWOW64FSREDIRECTION pfnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64RevertWow64FsRedirection"));
			if ( (NULL != pfnWow64RevertWow64FsRedirection) )
			{
				pfnWow64RevertWow64FsRedirection(OldValue);
			}
		}
		delete pAdq;
		::Sleep(SPD_INIT_DELAY);  // give the flash a chance to settle

		if ( CS_FAILED(MakeDevicesAndControl()) )
		{
			// Report to event log done in MakeDevicesAndControl
			i32Ret = CS_MISC_ERROR;
		}
	}
	return i32Ret;
}


CsSpDevice::CsSpDevice(CsSpDeviceManager* pMngr, int nNum):
m_pManager(pMngr), m_nNum(nNum), m_bStarted (false), m_i64TickFrequency(1),
m_uBusAddr(0), m_bSkipFwUpdate(false), m_pSrTable(NULL), m_szSrTableSize(0),
m_bForceFwUpdateAlgo(false), m_bForceFwUpdateComm(false), m_i64TrigAddrAdj(0),
m_nTimeStampMode (0),m_bDisplayTrace (false), m_bDisplayXferTrace(false), m_bUsb(false), m_bPxi(false),
m_bFavourClockSkip(true), m_nTrigSense(CS_USB_DEFAULT_TRIG_SENSE), m_bDcInput(false), m_bDcAcInput(false), m_u32AFE(0)
{
	::ZeroMemory(&m_BaseBoardCsiEntry, sizeof(CSI_ENTRY));
	::ZeroMemory(&m_AddonCsiEntry, sizeof(CSI_ENTRY));
	::ZeroMemory(&m_UsbFpgaEntry, sizeof(CSI_ENTRY));

	ReadRegistryInfo();

	m_pAdq = m_pManager->m_pfGetAdqInterface(m_pManager->GetControl(), m_nNum);
	if ( NULL == m_pAdq )
	{
		Sleep(10);
		// Report to event viewer will be done in the catch
		throw ( CsSpdevException(_T("Could not obtain Adq Interface")) );
	}
	
	::ZeroMemory( &m_Info, sizeof(m_Info) );
	
	m_nType = m_pAdq->GetADQType();
	if(m_bDisplayTrace) 
	{
		char str[128];
		::_stprintf_s( str, 128,  "m_pAdq->GetADQType() returned %d\n", m_nType );
		::OutputDebugString(str);
	}

	int nTypeCount = sizeof(CsSpDeviceManager::DevType)/sizeof(CsSpDeviceManager::DevType[0]);
	
	for ( int i = 0; i < nTypeCount; i++ )
	{
		if ( m_nType == CsSpDeviceManager::DevType[i].nAdqType )
		{
			::CopyMemory( &m_Info, &CsSpDeviceManager::DevType[i], sizeof(m_Info) );
			m_u32MemSize = m_Info.lMemSize;
			m_u32MemSize *= 0x100000; //Convert to Mega samples
			m_u32MemSize -= 512; //Last page is bad
			m_u32MemSize /= m_Info.usChanNumber;
		}
	}
	if ( 0 == m_Info.nAdqType )
	{
		TCHAR szMessage[MAX_PATH];
		::_stprintf_s(szMessage, MAX_PATH, _T("Device %d not found in table"), m_nType);
		throw ( CsSpdevException(szMessage) );
	}

	BuildSrTable();

	// for now set it to the highest sampling rate
	// Note: the formula will be different if we ever use Sync on mode
	int64 i64TopSampleRate = GetMaxSr();
	m_i64TickFrequency = (214 == m_Info.nAdqType) ? (2 * i64TopSampleRate) : i64TopSampleRate;

	int* pnRev = NULL;

	pnRev = m_pAdq->GetRevision();
	if(m_bDisplayTrace) 
		::OutputDebugString("m_pAdq->GetRevision()\n");

	if( NULL != pnRev && !IsBadReadPtr(pnRev, sizeof(int)*4 ) )
	{
		m_nRevFpga1 = pnRev[3];
		m_nRevFpga2 = pnRev[0];
	}
	else
	{
		m_nRevFpga1 = 0;
		m_nRevFpga2 = 0;
	}

	PVOID	OldValue = NULL;
	BOOL	bRet = TRUE;

	if ( Is32on64() )
	{
		// Disable file system direction so we'll find the csi files in system32 on a 64 bit system when running 32 bit
		LPFN_WOW64DISABLEWOW64FSREDIRECTION pfnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64DisableWow64FsRedirection"));
		if ( NULL != pfnWow64DisableWow64FsRedirection )
		{
			bRet = pfnWow64DisableWow64FsRedirection (&OldValue);
		}
	}
	
	ReadAndValidateCsiFile();
	if ( Is32on64() && bRet)
	{
		// Restore the previous WOW64 file system redirection value.
		LPFN_WOW64REVERTWOW64FSREDIRECTION pfnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64RevertWow64FsRedirection"));
		if ( (NULL != pfnWow64RevertWow64FsRedirection) )
		{
			pfnWow64RevertWow64FsRedirection(OldValue);
		}
	}

	m_bUsb = 0 != m_pAdq->IsUSBDevice();
	m_bPxi = 0 != m_pAdq->IsPCIeDevice();
	if(m_bDisplayTrace) 
	{
		char str[128];
		::_stprintf_s( str, 128,  "m_pAdq->IsUSBDevice() returned %s\n", m_bUsb ? "true":"false" );
		::OutputDebugString(str);
		::_stprintf_s( str, 128,  "m_pAdq->IsPCIeDevice() returned %s\n", m_bPxi ? "true":"false" );
		::OutputDebugString(str);
	}

	if( m_bUsb )
	{
		m_uBusAddr = m_pAdq->GetUSBAddress();
		if(m_bDisplayTrace) 
		{
			char str[128];
			::_stprintf_s( str, 128,  "m_pAdq->GetUSBAddress() returned %d\n", m_uBusAddr );
			::OutputDebugString(str);
		}
	}
	if( m_bPxi )
	{
		m_uBusAddr = m_pAdq->IsPCIeDevice();
		if(m_bDisplayTrace) 
		{
			char str[128];
			::_stprintf_s( str, 128,  "m_pAdq->IsPCIeDevice() returned %d\n", m_uBusAddr );
			::OutputDebugString(str);
		}
	}
	char* str = NULL;
	str = m_pAdq->GetBoardSerialNumber();
	if(m_bDisplayTrace) 
		::OutputDebugString("m_pAdq->GetBoardSerialNumber()\n");
	if( NULL != str )
		strcpy_s( m_strSerNum, _countof(m_strSerNum), str );

	m_u32PcbRev = 0x100;
	m_u32PcbRev = m_pAdq->GetBcdDevice();
	if(m_bDisplayTrace) 
		::OutputDebugString("m_pAdq->GetBcdDevice()\n");

#ifndef CS_WIN64
	TRACE( DRV_TRACE_LEVEL, _T("Constructed %s SN %s PcbRev %x  UsbAddr %x\n"), m_Info.strCsName, m_strSerNum, m_u32PcbRev, m_uBusAddr );
#endif

	char strSig[SDP_DEV_SIG_SIZE+1], strDc[SDP_DEV_DC_SIG_SIZE + 1];
	for( int i = SDP_DEV_SIG_ADDR, j = 0; i < SDP_DEV_SIG_ADDR+SDP_DEV_SIG_SIZE; i++, j++ )
	{
		char str[128];
		strSig[j] = (char)m_pAdq->ReadEEPROM(i); 
		if(m_bDisplayTrace) 
		{
			::_stprintf_s( str, 128,  "m_pAdq->ReadEEPROM(%d) returned %c\n", i, strSig[j] );
			::OutputDebugString(str);
		}
	}

	strSig[SDP_DEV_SIG_SIZE] = 0;
	strncpy_s( strDc, _countof(strDc), strSig+8, SDP_DEV_DC_SIG_SIZE );
	if( 0 == strcmp(SDP_DEV_DC_SIG, strDc ) )
		m_bDcInput = true;
	else if( 0 == strcmp(SDP_DEV_DC_AC_SIG, strDc ) )
		m_bDcAcInput = true;

	m_pAdq->ResetTrigTimer(0);
	m_pAdq->SetTrigTimeMode(m_nTimeStampMode);
	m_pAdq->ResetTrigTimer(1);
	if(m_bDisplayTrace) 
	{
		char str[128];
		::OutputDebugString("m_pAdq->ResetTrigTimer(0);\n");
		::_stprintf_s( str, 128,  "m_pAdq->SetTrigTimeMode(%d);\n", m_nTimeStampMode );
		::OutputDebugString(str);
		::OutputDebugString("m_pAdq->ResetTrigTimer(1);\n");
	}

	::ZeroMemory( &m_Acq, sizeof(m_Acq) );
	m_Acq.u32Size = sizeof(CSACQUISITIONCONFIG);
	m_ReqAcq = m_Acq;

	::ZeroMemory( &m_aChan, sizeof(m_aChan) );
	m_aChan[0].u32Size = m_aChan[1].u32Size = sizeof(CSCHANNELCONFIG);
	m_aChan[0].u32ChannelIndex = CS_CHAN_1; m_aChan[1].u32ChannelIndex= CS_CHAN_2;
	m_aReqChan[0] = m_aChan[0]; m_aReqChan[1] = m_aChan[1];

	::ZeroMemory( &m_Trig, sizeof(m_Trig) );
	m_Trig.u32Size = sizeof(CSTRIGGERCONFIG);
	m_ReqTrig = m_Trig;
}

CsSpDevice::~CsSpDevice()
{
}

void CsSpDevice::ReadRegistryInfo(void)
{
	HKEY hKey = NULL;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, USB_PARAMS_KEY, 0, KEY_QUERY_VALUE, &hKey) )
	{
		DWORD dwDataSize = sizeof(DWORD);
		DWORD dw = m_nTimeStampMode != 0 ? CS_AUXIN_TS_RESET : 0;
		::RegQueryValueEx( hKey, _T("AuxIn"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
		m_nTimeStampMode = (0 != (dw& CS_AUXIN_TS_RESET)) ? 1: 0;

		dw = DWORD(m_nTrigSense);
		::RegQueryValueEx( hKey, _T("TriggerSense"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
		m_nTrigSense = (int)dw;

		dwDataSize = sizeof(DWORD);
		dw = (m_bDisplayXferTrace ? 1:0)| (m_bDisplayTrace ? 2:0);
		::RegQueryValueEx( hKey, _T("Trace"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
		m_bDisplayTrace = (dw & 0x2) != 0;
		m_bDisplayXferTrace = (dw & 0x1) != 0;
		
		dw = m_bSkipFwUpdate? 1 : 0;
		dwDataSize = sizeof(DWORD);
		::RegQueryValueEx( hKey, _T("SkipCheckFirmware"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
		m_bSkipFwUpdate = dw != 0;

		dw = m_bForceFwUpdateComm ? 1 : 0;
		dwDataSize = sizeof(DWORD);
		::RegQueryValueEx( hKey, _T("ForceFirmwareUpdateComm"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
		m_bForceFwUpdateComm = dw != 0;

		dw = m_bForceFwUpdateAlgo ? 1 : 0;
		dwDataSize = sizeof(DWORD);
		::RegQueryValueEx( hKey, _T("ForceFirmwareUpdateAlgo"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
		m_bForceFwUpdateAlgo = dw != 0;

		dw = m_bFavourClockSkip ? 1 : 0;
		dwDataSize = sizeof(DWORD);
		::RegQueryValueEx( hKey, _T("FavourClockSkip"), NULL, NULL, (LPBYTE)&dw, &dwDataSize );
		m_bFavourClockSkip = dw != 0;

		::RegCloseKey( hKey );
		hKey = NULL;
	}	
}
	
	

int32 CsSpDevice::ReadAndValidateCsiFile(void)
{
	int32			i32RetCode = CS_SUCCESS;
	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry;
	TCHAR			strCsiFileName[MAX_PATH] = _T("");
	TCHAR			strError[MAX_PATH] = _T("");

	if( m_bPxi )
	{
		if(m_bDisplayTrace) 
			::OutputDebugString("CSI files are not supported by PXI version");
		return CS_FALSE; // not success but we don't want to abort the program
	}

	if ( ::GetSystemDirectory(strCsiFileName, MAX_PATH) )
	{
		::lstrcat(strCsiFileName, _T("\\drivers\\"));
		::lstrcat(strCsiFileName, m_Info.strCsName);
		::lstrcat(strCsiFileName, _T("U")); //only USB board are currently supported
		::lstrcat(strCsiFileName, _T(".csi"));
	}
	else
	{ 
		if(m_bDisplayTrace) 
			::OutputDebugString("GetSystemDirectory failed\n");

#ifndef CS_WIN64		
		TRACE(DRV_TRACE_LEVEL, _T("GetSystemDirectory failed\n"));
#endif
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, _T("GetSystemDirectory failed"));

		return CS_FALSE; // not success but we don't want to abort the program
	}

	
	if ( 0 == lstrcmp(strCsiFileName, _T("")) )
	{
		if(m_bDisplayTrace) 
			::OutputDebugString("Csi filename  is empty\n" );

		TCHAR szMessage[MAX_PATH];
		::_stprintf_s(szMessage, MAX_PATH, _T("Cs file %s not found"), strCsiFileName);
#ifndef CS_WIN64
		TRACE( DRV_TRACE_LEVEL, _T("%s\n"), szMessage );
#endif
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CSI_FILE, szMessage );
		return CS_FALSE; // not success but we don't want to abort the program
	}

	HANDLE hCsiFile = ::CreateFile(strCsiFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( INVALID_HANDLE_VALUE == hCsiFile )
	{
		if(m_bDisplayTrace) 
		{
			char str[128];
			::_stprintf_s( str, 128,  "Csi file %s cannot be opened\n", strCsiFileName );
			::OutputDebugString(str);
		}
		TCHAR szMessage[MAX_PATH];
		::_stprintf_s(szMessage, MAX_PATH, _T("Csi file %s cannot be opened"), strCsiFileName);
#ifndef CS_WIN64
		TRACE( DRV_TRACE_LEVEL, _T("%s\n"), szMessage );
#endif
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CSI_FILE, szMessage );
		return CS_FALSE; // not success but we don't want to abort the program
	}

	uInt32	u32BBTarget = CSI_USB_BB_TARGET;
	
	DWORD dwBytesRead = 0;

	if ( !::ReadFile(hCsiFile, &header, sizeof(header), &dwBytesRead, NULL) )
		return CS_FALSE; // not success but we don't want to abort the program
	
	if ( ( header.u32Size == sizeof(CSI_FILE_HEADER) ) &&
		 ( header.u32Checksum == CSI_FWCHECKSUM_VALID ) &&
		 (( header.u32Version == 0 ) || ( header.u32Version == CSIFILE_VERSION )) )
	{
		for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
		{
			if ( 0 == ::ReadFile( hCsiFile, &CsiEntry, sizeof(CsiEntry), &dwBytesRead, NULL) )
			{
				::_stprintf_s(strError, MAX_PATH, _T("Cannot read %s"), strCsiFileName);
				i32RetCode = CS_CSI_FILE_ERROR;
				break;
			}
	
			if( CsiEntry.u32Target == u32BBTarget )
			{
				// Found Base board firmware info
				if ( (CsiEntry.u32FwOptions & CS_OPTIONS_USB_FX70T_MASK) == CS_OPTIONS_USB_FX70T_MASK )
				{
					if ( CsiEntry.u32ImageSize != CS_USB_ALG_FX70T_IMAGE_SIZE )
					{
						::_stprintf_s(strError, MAX_PATH, _T("Alg image size %d in %s is incorrect"), CsiEntry.u32ImageSize, strCsiFileName);
						i32RetCode = CS_CSI_FILE_ERROR;
						break;
					}
					else
						::CopyMemory(&m_BaseBoardCsiEntry, &CsiEntry, sizeof(CSI_ENTRY));
				}
				else // it's an SX50T ( the default )
				{
					if ( CsiEntry.u32ImageSize != CS_USB_ALG_IMAGE_SIZE )
					{
						::_stprintf_s(strError, MAX_PATH, _T("Alg image size %d in %s is incorrect"), CsiEntry.u32ImageSize, strCsiFileName);
						i32RetCode = CS_CSI_FILE_ERROR;
						break;
					}
					else
						::CopyMemory(&m_BaseBoardCsiEntry, &CsiEntry, sizeof(CSI_ENTRY));
				} //end of bb/algo target
			}
			else
			{
				if ( CsAdqUpdate::GetAddOnCsiTarget( m_nType ) != CsiEntry.u32Target )
				{
					::_stprintf_s(strError, MAX_PATH, _T("Error in %s: unknown target"), strCsiFileName);
					i32RetCode = CS_CSI_FILE_ERROR;
					break;
				}
				else
				{
					// Found Add-on board firmware info
					// check that both current options aren't available at the same time
					if ( ((CsiEntry.u32FwOptions & CS_OPTIONS_USB_FIRMWARE_MASK) == CS_OPTIONS_USB_FIRMWARE_MASK) &&
						 ((CsiEntry.u32FwOptions & CS_OPTIONS_USB_SX50T_MASK) == CS_OPTIONS_USB_SX50T_MASK) )
					{
						::_stprintf_s(strError, MAX_PATH, _T("Error in %: tncorrect options in Comm firmware"), strCsiFileName);
						i32RetCode = CS_CSI_FILE_ERROR;
						break;
					}
					if ( (CsiEntry.u32FwOptions & CS_OPTIONS_USB_SX50T_MASK) ==  CS_OPTIONS_USB_SX50T_MASK)
					{
						if ( CsiEntry.u32ImageSize != CS_USB_COMM_SX50T_IMAGE_SIZE )
						{
							::_stprintf_s(strError, MAX_PATH, _T("Comm image size %d in %s is incorrect"), CsiEntry.u32ImageSize, strCsiFileName);
							i32RetCode = CS_CSI_FILE_ERROR;
							break;
						}
						else
							::CopyMemory(&m_AddonCsiEntry, &CsiEntry, sizeof(CSI_ENTRY));
					}
					else if ( (CsiEntry.u32FwOptions & CS_OPTIONS_USB_FIRMWARE_MASK) ==  CS_OPTIONS_USB_FIRMWARE_MASK )
					{
						// USB firmware size might change often so we don't check it
						::CopyMemory(&m_UsbFpgaEntry, &CsiEntry, sizeof(CSI_ENTRY));
					}
					else // must be the LX-30T ???
						if ( CsiEntry.u32ImageSize != CS_USB_COMM_LX30T_IMAGE_SIZE )
						{
							::_stprintf_s(strError, MAX_PATH, _T("Comm image size %d in %s is incorrect"), CsiEntry.u32ImageSize, strCsiFileName);
							i32RetCode = CS_CSI_FILE_ERROR;
							break;
						}
						else
						{
							::CopyMemory(&m_AddonCsiEntry, &CsiEntry, sizeof(CSI_ENTRY));
						}
				} //end of AO/Comm target
			} // wrong target
		} //CsiEntry loop
	} //header check
	else
	{
		::_stprintf_s(strError, MAX_PATH, _T("Error in %s: Invalid header"), strCsiFileName);
		i32RetCode = CS_CSI_FILE_ERROR;
	}

	// Additional validation...
	if ( CS_SUCCESS == i32RetCode )
	{
		if ( m_BaseBoardCsiEntry.u32ImageOffset == 0 || 
			 m_AddonCsiEntry.u32ImageOffset == 0  ||
			 m_UsbFpgaEntry.u32ImageOffset == 0 )
		{
			::_stprintf_s(strError, MAX_PATH, _T("Error in %s: Invalid image offset"), strCsiFileName);
			i32RetCode = CS_CSI_FILE_ERROR;
		}
		else
		{
			uInt8	u8Buffer[CSISEPARATOR_LENGTH];
			uInt32	u32ReadPosition = m_BaseBoardCsiEntry.u32ImageOffset - CSISEPARATOR_LENGTH;
			::SetFilePointer( hCsiFile, u32ReadPosition, 0, FILE_BEGIN);	//??? OR CURRENT
			if ( 0 == ::ReadFile( hCsiFile, u8Buffer, sizeof(u8Buffer), &dwBytesRead, NULL) )
				i32RetCode = CS_CSI_FILE_ERROR;
			else if ( 0 != ::memcmp( CsiSeparator, u8Buffer, sizeof(u8Buffer) ) )
				i32RetCode = CS_CSI_FILE_ERROR;
			
			u32ReadPosition = m_AddonCsiEntry.u32ImageOffset - CSISEPARATOR_LENGTH;
			::SetFilePointer( hCsiFile, u32ReadPosition, 0, FILE_BEGIN);
			if ( 0 == ::ReadFile( hCsiFile, u8Buffer, sizeof(u8Buffer), &dwBytesRead, NULL) )
				i32RetCode = CS_CSI_FILE_ERROR;
			else if ( 0 != ::memcmp( CsiSeparator, u8Buffer, sizeof(u8Buffer) ) )
				i32RetCode = CS_CSI_FILE_ERROR;
		}
	}
	::CloseHandle( hCsiFile );

	if ( CS_CSI_FILE_ERROR == i32RetCode )
	{
		if(m_bDisplayTrace) 
		{
			char str[128];
			::_stprintf_s( str, 128,  "Csi file %s cannot be read\n", strCsiFileName );
			::OutputDebugString(str);
		}
		if ( 0 == ::lstrcmp(strError, _T("")) )
		{
			::_stprintf_s(strError, MAX_PATH, _T("Csi file %s cannot be read"), strCsiFileName);
		}
#ifndef CS_WIN64
		TRACE( DRV_TRACE_LEVEL, _T("%s\n"), strError );
#endif
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CSI_FILE, strError );				
		return CS_FALSE; // not success but we don't want to abort the program
	}
	return i32RetCode;
}

int32 CsSpDevice::CompareFirmwareVersions(void)
{
	// convert ald fpga version to our numbering scheme and compare it to
	// the csi file's base board versions
	FPGA_FWVERSION fwBbVersion = {0};
	
	fwBbVersion.Version.uMinor = (m_nRevFpga1 & 0xff00)>>8;
	fwBbVersion.Version.uRev = m_nRevFpga1 & 0x00ff;;

	bool bBadBbFirmware = m_bForceFwUpdateAlgo || ( (m_BaseBoardCsiEntry.u32ImageOffset != 0) && (fwBbVersion.u32Reg != m_BaseBoardCsiEntry.u32FwVersion) );

	// do the same with the add on and the comm fpga
	FPGA_FWVERSION fwAddonVersion = {0};
	
	fwAddonVersion.Version.uMinor = (m_nRevFpga2 & 0xff00)>>8;
	fwAddonVersion.Version.uRev = m_nRevFpga2 & 0x00ff;;

	bool bBadAddonFirmware = m_bForceFwUpdateComm || ( (m_AddonCsiEntry.u32ImageOffset != 0) && (fwAddonVersion.u32Reg != m_AddonCsiEntry.u32FwVersion) );

	if ( !m_bSkipFwUpdate && (bBadBbFirmware || bBadAddonFirmware) )
	{
		// set the force variables to false so we can continue thru next time we call CsDrvGetAcquisitionCount
		m_bForceFwUpdateAlgo = false;
		m_bForceFwUpdateComm = false;
		return CS_INVALID_FW_VERSION;
	}
	else
	{
		return CS_SUCCESS;
	}
}

void  CsSpDevice::BuildSrTable(void)
{
	// First determine the Divider limit
	if( m_bFavourClockSkip )
	{
		if(m_bDisplayTrace)
			::OutputDebugString("---- Favour clock skip ---- \n");
		switch( m_Info.nAdqType )
		{
			case 108: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable108_fcs; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable108_fcs)/sizeof(SpDevSr); break;
			case 112: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable112_fcs; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable112_fcs)/sizeof(SpDevSr); break;
			case 114: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable114_fcs; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable114_fcs)/sizeof(SpDevSr); break;
			case 214: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable214_fcs; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable214_fcs)/sizeof(SpDevSr); break;
			default: throw ( CsSpdevException(_T("Unknown Adq Type")) );
		}
	}
	else
	{
		if(m_bDisplayTrace)
			::OutputDebugString("++++ Favour clock decimation ++++ \n");
		switch( m_Info.nAdqType )
		{
			case 108: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable108_fcd; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable108_fcd)/sizeof(SpDevSr); break;
			case 112: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable112_fcd; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable112_fcd)/sizeof(SpDevSr); break;
			case 114: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable114_fcd; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable114_fcd)/sizeof(SpDevSr); break;
			case 214: m_pSrTable = (SpDevSr*)CsSpDeviceManager::SrTable214_fcd; m_szSrTableSize = sizeof(CsSpDeviceManager::SrTable214_fcd)/sizeof(SpDevSr); break;
			default: throw ( CsSpdevException(_T("Unknown Adq Type")) );
		}
	}

}