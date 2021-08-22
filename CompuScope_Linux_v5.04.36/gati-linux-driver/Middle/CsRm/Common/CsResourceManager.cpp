#include "rm.h"

#if defined (_WIN32)

	#include <windows.h>
	#include <crtdbg.h>
	#include <xutility>
	#include <process.h>
	#include <userenv.h>

	#include "Debug.h"
	#include "service.h"
	#include "RMmsglog.h"
	#include "resource.h"
	#include "CsSupport.h"

#else

	#include <string.h>
	#include <assert.h>
	#include <signal.h>
	#include <algorithm>

#endif // _WIN32

#include <stdlib.h>

#include "misc.h"

#include "CsStruct.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsResourceManager.h"
#include "CsPrivatePrototypes.h"
#include "CsDisp.h"
#include "CsFirmwareDefs.h"

#ifdef __linux__
	#include "CsLinuxPort.h"
#endif


#if defined(_WIN32)

typedef BOOL  (WINAPI *WTSQUERYUSERTOKEN)			(ULONG, PHANDLE);
typedef DWORD (WINAPI *WTSGETACTIVECONSOLESESSIONID)(void);
typedef UINT  (WINAPI *GETSYSTEMWOW64DIRECTORY)		(LPTSTR, UINT);
LRESULT CALLBACK GetUserFeedBack(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

unsigned __stdcall UpdateFirmwareProc(void *arg)
{
	CsResourceManager*  pCsRm = (CsResourceManager *)arg;

	int32 i32Status = pCsRm->m_pCsDrvAutoFirmwareUpdate();
	pCsRm->SetUpdateStatus(i32Status);

	// It doesn't matter if the call fails, once we exit the thread
	// routine we'll try it again so we'll catch the failure there.
	::_endthreadex(0);
	return 0;
}


unsigned __stdcall RefreshProc(void *arg)
{
	REFRESHSTRUCT *pRefresh = (REFRESHSTRUCT *)arg;
	TIMER_HANDLE handleTimer = pRefresh->hTimer;

	CsResourceManager *pCsRm = (CsResourceManager *)pRefresh->pThis;
	::VirtualFree(arg, 0, MEM_RELEASE);
	HANDLE pEvents[3];
	pEvents[0] = pCsRm->GetStopTimerThreadEvent();
	pEvents[1] = handleTimer;
	pEvents[2] = pCsRm->GetRefreshEvent();
	for (;;)
	{
		// Wait for either the timer event or GageResourceManagerRefreshEvent, which
		// signals that an application has closed without calling FreeSystem
		DWORD dwRet = ::WaitForMultipleObjects(3, pEvents, FALSE, INFINITE);
		if (dwRet == WAIT_OBJECT_0)
			break;
		// Give the process that sent the event some time to close
		::Sleep(200);
		pCsRm->CsRmRefreshAllSystemHandles();
	}
	::_endthreadex(0);
	return 0;
}


int CreateAndSetTimer(int nTimeInSeconds, TIMER_HANDLE& hTimerHandle, TIMER_HANDLE& hTimerThreadHandle, void *arg)
{
	LARGE_INTEGER liDueTime;

	const int unitsPerSecond = 10 * 1000 * 1000;
	liDueTime.QuadPart = -(nTimeInSeconds * unitsPerSecond);

    // Create a waitable timer.
	hTimerHandle = ::CreateWaitableTimer(NULL, FALSE, 0);
	if (!hTimerHandle)
	{
		TRACE(RM_TRACE_LEVEL, "RM::CreateWaitableTimer failed\n");
		DisplayErrorMessage(GetLastError());
		return -1;
	}
	int nTimeInMilliseconds = nTimeInSeconds * 1000;
	if (!::SetWaitableTimer(hTimerHandle, &liDueTime, nTimeInMilliseconds, 0, NULL, 0))
	{
		TRACE(RM_TRACE_LEVEL, "RM::SetWaitableTimer failed\n");
		DisplayErrorMessage(GetLastError());
		return -1;
	}

	REFRESHSTRUCT* pRefresh = static_cast<REFRESHSTRUCT*>(::VirtualAlloc(NULL, sizeof(REFRESHSTRUCT), MEM_COMMIT, PAGE_READWRITE));
	pRefresh->hTimer = hTimerHandle;
	pRefresh->pThis = arg;
	hTimerThreadHandle = (TIMER_HANDLE)::_beginthreadex(NULL, 0, RefreshProc, (void *)pRefresh, 0, 0);
	return 0;
}

int CreateRefreshTimer(TIMER_HANDLE& hTimerHandle, TIMER_HANDLE& hTimerThreadHandle, void *arg)
{
    // Create a waitable timer.
	hTimerHandle = ::CreateWaitableTimer(NULL, FALSE, 0);
	if (!hTimerHandle)
	{
		TRACE(RM_TRACE_LEVEL, "RM::CreateWaitableTimer failed\n");
		DisplayErrorMessage(GetLastError());
		return -1;
	}
	// Create the thread
	REFRESHSTRUCT* pRefresh = static_cast<REFRESHSTRUCT*>(::VirtualAlloc(NULL, sizeof(REFRESHSTRUCT), MEM_COMMIT, PAGE_READWRITE));
	pRefresh->hTimer = hTimerHandle;
	pRefresh->pThis = arg;
	hTimerThreadHandle = (TIMER_HANDLE)::_beginthreadex(NULL, 0, RefreshProc, (void *)pRefresh, 0, 0);
	return 0;
}

int SetRefreshTimer(int nTimeInSeconds, TIMER_HANDLE& hTimerHandle)
{
	LARGE_INTEGER liDueTime;

	const int unitsPerSecond = 10 * 1000 * 1000;
	liDueTime.QuadPart = -(nTimeInSeconds * unitsPerSecond);

	int nTimeInMilliseconds = nTimeInSeconds * 1000;
	if (!::SetWaitableTimer(hTimerHandle, &liDueTime, nTimeInMilliseconds, 0, NULL, 0))
	{
		TRACE(RM_TRACE_LEVEL, "RM::SetWaitableTimer failed\n");
		DisplayErrorMessage(GetLastError());
		return -1;
	}
	return 0;
}

void StopTimerThread(TIMER_HANDLE hTimerThreadHandle, TIMER_HANDLE hStopTimerThreadEvent)
{
	if (NULL != hTimerThreadHandle && NULL != hStopTimerThreadEvent)
	{
		::SetEvent(hStopTimerThreadEvent);
		//Wait only for 10 sec.
		// if thead is not stopped it is dead anyway
		::WaitForSingleObject(hTimerThreadHandle, 10000);
		::CloseHandle(hTimerThreadHandle);
	}
}

void FreeTimer(TIMER_HANDLE hTimer)
{
	if (NULL != hTimer)
	{
		::CancelWaitableTimer(hTimer);
		::CloseHandle(hTimer);
	}
}

int GetRequestedRefreshTimeInSeconds(void)
{
	char Subkey[50] = RESOURCE_MANAGER_KEY;
	int		nRefreshTimeInSeconds = 300; // 5 minutes	
	HKEY	hKey;
	char	ValueBuffer[50];
	uInt32 	ValueBuffSize;
	DWORD	DataBuffer;
	uInt32 	DataBuffSize;
	DWORD	DataType;

	DWORD dwFlags = KEY_QUERY_VALUE;

	if (Is32on64())
		dwFlags |= KEY_WOW64_64KEY;
	
	long lRetVal = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, Subkey, 0, dwFlags, &hKey);

	if (ERROR_SUCCESS == lRetVal)
	{
		ValueBuffSize = sizeof(ValueBuffer);
		DataBuffSize = sizeof(DWORD);

		long lRetVal = ::RegQueryValueEx(hKey, REFRESH_VALUE, NULL, &DataType, 
								(LPBYTE) &DataBuffer, &DataBuffSize);
		
		if ((ERROR_SUCCESS == lRetVal) && (DataType == REG_DWORD))
		{
			nRefreshTimeInSeconds = DataBuffer;
		}
		::RegCloseKey(hKey);
	}
	return nRefreshTimeInSeconds;
}

uInt8 GetAutoFirmwareUpdateCount(void)
{
	char Subkey[50] = RESOURCE_MANAGER_KEY;
	uInt8	u8AutoUpdateCount = 2;	
	HKEY	hKey;
	char	ValueBuffer[50];
	uInt32 	ValueBuffSize;
	DWORD	DataBuffer;
	uInt32 	DataBuffSize;
	DWORD	DataType;

	DWORD dwFlags = KEY_QUERY_VALUE;

	if (Is32on64())
		dwFlags |= KEY_WOW64_64KEY;

	long lRetVal = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, Subkey, 0, dwFlags, &hKey);

	if (ERROR_SUCCESS == lRetVal)
	{
		ValueBuffSize = sizeof(ValueBuffer);
		DataBuffSize = sizeof(DWORD);
		lRetVal = ::RegQueryValueEx(hKey, FIRMWARE_UPDATE_COUNT, NULL, &DataType, 
								(LPBYTE) &DataBuffer, &DataBuffSize);
		if ((ERROR_SUCCESS == lRetVal) && (DataType == REG_DWORD))
		{
			u8AutoUpdateCount = static_cast<uInt8>(DataBuffer);
		}
		::RegCloseKey(hKey);
	}
	return u8AutoUpdateCount;
}


void CenterWindowOnWindow(HWND hMainWnd, HWND hWndToCenterOn)
{
    RECT rectToCenterOn;

	::GetWindowRect(hWndToCenterOn, &rectToCenterOn);

    // Get this window's area
    RECT rectSubDialog;
	::GetWindowRect(hMainWnd, &rectSubDialog);

    // Now rectWndToCenterOn contains the screen rectangle of the window 
    // pointed to by pWndToCenterOn.  Next, we apply the same centering 
    // algorithm as does CenterWindow()

    // find the upper left of where we should center to
    int xLeft = (rectToCenterOn.left + rectToCenterOn.right) / 2 - 
		        (rectSubDialog.right-rectSubDialog.left) / 2;
    int yTop = (rectToCenterOn.top + rectToCenterOn.bottom) / 2 - 
				(rectSubDialog.bottom-rectSubDialog.top) / 2;

    // Move the window to the correct coordinates with SetWindowPos()
	::SetWindowPos(hMainWnd, NULL, xLeft, yTop, -1, -1,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


HWND DisplayUpdateMessage(void)
{
	TCHAR strBuffer[256];
	HMODULE hModule = ::GetModuleHandle(_T("CsRm.exe"));
	HWND hDlg = ::CreateDialog(hModule, MAKEINTRESOURCE(IDD_UPDATE_FIRMWARE), NULL, NULL);
	if (NULL != hDlg)
	{
		::CenterWindowOnWindow(hDlg, ::GetDesktopWindow());
		::ShowWindow(hDlg, SW_SHOW);

		::LoadString(hModule, IDS_MESSAGE_1, strBuffer, 256);
		::SetDlgItemText(hDlg, IDC_STATIC_1, strBuffer);

		::LoadString(hModule, IDS_MESSAGE_2, strBuffer, 256);
		::SetDlgItemText(hDlg, IDC_STATIC_2, strBuffer);
	}
	return hDlg;
}


void DestroyUpdateMessage(HWND hDlg)
{
	::ShowWindow(hDlg, SW_HIDE);
	::DestroyWindow(hDlg);
}

LRESULT CALLBACK GetUserFeedBack(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (msg) 
	{
    	case WM_INITDIALOG:
			{
				HMODULE hModule = ::GetModuleHandle(_T("CsRm.exe"));
				TCHAR strBuffer[256];

				::CenterWindowOnWindow(hWnd, ::GetDesktopWindow());
				::LoadString(hModule, IDS_MESSAGE_3, strBuffer, 256);
				::SetDlgItemText(hWnd, IDC_STATIC_1, strBuffer);
				::LoadString(hModule, IDS_MESSAGE_4, strBuffer, 256);
				::SetDlgItemText(hWnd, IDC_STATIC_2, strBuffer);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, (LOWORD(wParam) == IDOK));
					return (TRUE);
				default:
					break;
			}
		default:
			break;
	}
	return (FALSE);
}


BOOL CheckMessageQueue (void)
{
    MSG msg;

    while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) 
	{
        if ( msg.message == WM_COMMAND || 
			 msg.message == WM_QUIT  )
		{
            return FALSE;
		}
        else 
		{
            GetMessage (&msg, NULL, 0, 0);
            TranslateMessage (&msg);
            DispatchMessage (&msg);
		}
    }
    return TRUE;
}

#else  // we're in linux

#include <sys/timerfd.h>
#include <time.h>

static int make_periodic (unsigned int period, int* timer_fd)
{
	int ret;
	unsigned int ns;
	unsigned int sec;
	int fd;
	struct itimerspec itval;

	/* Create the timer */
	fd = timerfd_create (CLOCK_MONOTONIC, 0);
	*timer_fd = fd;
	if (fd == -1)
		return fd;

	/* Make the timer periodic */
	sec = period/1000000;
	ns = (period - (sec * 1000000)) * 1000;
	itval.it_interval.tv_sec = sec;
	itval.it_interval.tv_nsec = ns;
	itval.it_value.tv_sec = sec;
	itval.it_value.tv_nsec = ns;
	ret = timerfd_settime (fd, 0, &itval, NULL);
	return ret;
}

static void wait_period (int* timer_fd)
{
	unsigned long long missed;
	int ret;

	/* Wait for the next timer event. If we have missed any the
	   number is written to "missed" */
	ret = read (*timer_fd, &missed, sizeof (missed));
	if (ret == -1)
	{
		perror ("Error on read timer");
		return;
	}
}


static void* RefreshProc(void *arg)
{
//	struct periodic_info info;
	int timer_fd;
	CsResourceManager *pCsRm = (CsResourceManager *)arg;
	// 2 minutes (300s (5 min.) or 300000000 uSeconds)
	make_periodic(300000000, &timer_fd);  
	while (1)
	{
		wait_period(&timer_fd);
		// maybe put this stuff before wait_period
		// also need to check for GageResourceManagerRefreshEvent
		pCsRm->CsRmRefreshAllSystemHandles();
	}
	return NULL;
}
	
void CreateAndSetTimer(int nTimeInSeconds, void* arg)
{
	pthread_t t_1;
	
	pthread_create(&t_1, NULL, &RefreshProc, arg);
}	

#endif //_WIN32

BOOL UseFileMapping()
{
#ifdef CS_WIN64
	return TRUE;
#elif defined _WIN32
    return Is32on64();
#else // __linux__
    return TRUE;
#endif
}

#ifdef __linux__
bool	bDebug;
CsResourceManager::CsResourceManager(void) : m_pCsDrvAutoFirmwareUpdate(NULL), m_hStopTimerThreadEvent(NULL)
											, m_hRefreshEvent(NULL), m_hRmMutex(NULL), m_hDispHandle(NULL)
											, m_pCsDrvGetAcquisitionSystemCount(NULL), m_pCsDrvGetAcquisitionSystemHandles(NULL)
											, m_pCsDrvGetAcquisitionSystemInfo(NULL), m_pCsDrvUpdateLookupTable(NULL)
											, m_pCsDrvGetBoardsInfo(NULL), m_pCsDrvGetParams(NULL), m_pCsDrvOpenSystemForRm(NULL)
											, m_pCsDrvCloseSystemForRm(NULL), m_pCsDrvSetRemoteSystemInUse(NULL)
											, m_pCsDrvIsRemoteSystemInUse(NULL), m_pSharedMem(NULL), m_ProcessList(NULL)
											, m_pCsDrvGetAcquisitionSystemCaps(NULL)
#else
extern bool	bDebug;
CsResourceManager::CsResourceManager(void) :  m_pSharedMem(NULL)
											, m_hTimer(NULL), m_hTimerThreadHandle(NULL), m_hRefreshEvent(NULL) 
											, m_hStopTimerThreadEvent(NULL), m_hRmMutex(NULL), m_hDispHandle(NULL)
											, m_pCsDrvGetAcquisitionSystemCount(NULL), m_pCsDrvGetAcquisitionSystemHandles(NULL)
											, m_pCsDrvGetAcquisitionSystemInfo(NULL), m_pCsDrvUpdateLookupTable(NULL)
											, m_pCsDrvGetBoardsInfo(NULL), m_pCsDrvGetParams(NULL), m_pCsDrvOpenSystemForRm(NULL)
											, m_pCsDrvCloseSystemForRm(NULL), m_pCsDrvAutoFirmwareUpdate(NULL), m_pCsDrvSetRemoteSystemInUse(NULL)
											, m_pCsDrvIsRemoteSystemInUse(NULL), m_ProcessList(NULL), m_bStartRefreshTimerFlag(TRUE)
											, m_pCsDrvGetAcquisitionSystemCaps(NULL)

#endif
{
	// TODO
#if defined(_WIN32)
	SECURITY_ATTRIBUTES secEventAttributes;
	::ZeroMemory(&secEventAttributes, sizeof(secEventAttributes));
	secEventAttributes.nLength = sizeof(secEventAttributes);
	secEventAttributes.bInheritHandle = FALSE;

	SECURITY_DESCRIPTOR SD;
	BOOL bInitOk = ::InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
	if (bInitOk)
	{
		// Give the security descriptor a NULL Dacl
		BOOL bSetOk = ::SetSecurityDescriptorDacl(&SD, TRUE, (PACL)NULL, FALSE);
		if (bSetOk)
		{
			secEventAttributes.lpSecurityDescriptor = &SD;
			m_hRefreshEvent = ::CreateEvent(&secEventAttributes, FALSE, FALSE, GAGERESOURCEMANAGERREFRESHEVENT_NAME);
			m_hUsbCommEventHandle = ::CreateEvent( &secEventAttributes, TRUE, FALSE, GAGERESOURCEMANAGERUSBCOMMUNICATIONEVENT_NAME);
		}
	}

	m_hStopTimerThreadEvent = ::CreateEvent(NULL, FALSE, FALSE, GAGERESOURCEMANAGERSTOPTIMERTHREADEVENT_NAME);
#endif

	::InitializeCriticalSection(&m_csDriverLoadUnloadSection);

	if (UseFileMapping())
	{
		m_pSharedMem  = new SHAREDRMSYSTEMSTRUCT( RM_SHARED_MEM_FILE, bDebug, true );
		if( m_pSharedMem->IsMapped() )
			m_pSystem = m_pSharedMem->it();
	}
	else
	{
		m_pSystem = static_cast<SYSTEMSTRUCT*>(::VirtualAlloc(NULL, sizeof(SYSTEMSTRUCT), MEM_COMMIT, PAGE_READWRITE));
	}

#if defined(_WIN32)
	try
	{
		m_ProcessList = new CPsapiList();
	}
	catch ( int32 )
	{
		try
		{
			m_ProcessList = new CToolhelpList();
		}
		catch( int32 )
		{
			m_ProcessList = NULL;
		}
	}
#else // we're in linux
	try
	{
		m_ProcessList = new CLinuxList();
	}
	catch ( int32 )
	{
		m_ProcessList = NULL;
	}
#endif // _WIN32

#if defined(_WIN32)

	// By loading driver Dlls, Rm will intialize all CompuScope cards detected in system.
	// This action makes WDF driver's behavior similar to the WDM one (Cs cards is initialized at boot up).
	// Later, any application that use Compuscope cards will start faster because all compuscope cards is already initalized.
	LoadDriverDll();
	// Unload Dlls to keep the alogrithm of GetSystem()unchaged.
	UnLoadDriverDll();

#endif // _WIN32
	m_hRmMutex = ::MakeMutex(m_hRmMutex, FALSE, "");
}

CsResourceManager::~CsResourceManager(void)
{
#if defined(_WIN32)
	::StopTimerThread(m_hTimerThreadHandle, m_hStopTimerThreadEvent);

	m_hTimerThreadHandle = NULL;
	::FreeTimer(m_hTimer);
	m_hTimer = NULL;
#else // we're in linux
//	stop_timer()
#endif // _WIN32

	::DestroyMutex(m_hRmMutex);
	if (NULL != m_hRefreshEvent)
	{
		::CloseHandle(m_hRefreshEvent);
		m_hRefreshEvent = NULL;
	}

	if (NULL != m_hStopTimerThreadEvent)
	{
		::CloseHandle(m_hStopTimerThreadEvent);
		m_hStopTimerThreadEvent = NULL;
	}

	::DeleteCriticalSection(&m_csDriverLoadUnloadSection);
#if defined(_WIN32)
	::CloseHandle( m_hUsbCommEventHandle );
#endif

	if (NULL != m_pSharedMem)
	{
		m_pSystem = NULL;
		delete m_pSharedMem;
	}
	else
	{
		::VirtualFree(m_pSystem, 0, MEM_RELEASE);
	}
	
	if ( m_ProcessList )
	{
		delete m_ProcessList; // this will unload any dll's that were loaded by the class
	}
}

CsResourceManager::CsResourceManager(const CsResourceManager& csRM)
{
#if defined(_WIN32)
	m_hTimer = csRM.m_hTimer;
	m_hTimerThreadHandle = csRM.m_hTimerThreadHandle;
#endif
	m_hStopTimerThreadEvent = csRM.m_hStopTimerThreadEvent;
	m_hRmMutex = csRM.m_hRmMutex;
	m_hRefreshEvent = csRM.m_hRefreshEvent;
	::CopyMemory(m_pSystem, csRM.m_pSystem, sizeof(SYSTEMSTRUCT));

}

BOOL CsResourceManager::LoadDriverDll(void)
{
	// If there's a Combine working directory in the registry, usually
	// Combine/bin_debug, load CsDisp from there. Otherwise get it from
	// System32.

	::EnterCriticalSection(&m_csDriverLoadUnloadSection);

	ChangeToCombineDir();

#ifdef __linux__
	m_hDispHandle = ::LoadLibrary(_T("libCsDisp.so"));
#else
	m_hDispHandle = ::LoadLibrary(_T("CsDisp.dll"));
#endif

	_ASSERT(NULL != m_hDispHandle);
	if (NULL == m_hDispHandle)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvOpenSystemForRm = (LPDRVOPENSYSTEMFORRM)::GetProcAddress(m_hDispHandle, _T("CsDrvOpenSystemForRm"));
	_ASSERT(NULL != m_pCsDrvOpenSystemForRm);
	if (NULL == m_pCsDrvOpenSystemForRm)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvCloseSystemForRm = (LPDRVCLOSESYSTEMFORRM)::GetProcAddress(m_hDispHandle, _T("CsDrvCloseSystemForRm"));
	_ASSERT(NULL != m_pCsDrvCloseSystemForRm);
	if (NULL == m_pCsDrvCloseSystemForRm)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvGetAcquisitionSystemCount = (LPDRVGETACQUISITIONSYSTEMCOUNT)::GetProcAddress(m_hDispHandle, _T("CsDrvGetAcquisitionSystemCount"));
	_ASSERT(NULL != m_pCsDrvGetAcquisitionSystemCount);
	if (NULL == m_pCsDrvGetAcquisitionSystemCount)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvGetAcquisitionSystemHandles = (LPDRVGETACQUISITIONSYSTEMHANDLES)::GetProcAddress(m_hDispHandle, _T("CsDrvGetAcquisitionSystemHandles"));
	_ASSERT(NULL != m_pCsDrvGetAcquisitionSystemHandles);
	if (NULL == m_pCsDrvGetAcquisitionSystemHandles)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvGetAcquisitionSystemInfo = (LPDRVGETACQUISITIONSYSTEMINFO)::GetProcAddress(m_hDispHandle, _T("CsDrvGetAcquisitionSystemInfo"));
	_ASSERT(NULL != m_pCsDrvGetAcquisitionSystemInfo);
	if (NULL == m_pCsDrvGetAcquisitionSystemInfo)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvUpdateLookupTable = (LPDRVUPDATELOOKUPTABLE)::GetProcAddress(m_hDispHandle, _T("CsDrvUpdateLookupTable"));
	_ASSERT(NULL != m_pCsDrvUpdateLookupTable);
	if (NULL == m_pCsDrvUpdateLookupTable)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvGetBoardsInfo = (LPDRVGETBOARDSINFO)::GetProcAddress(m_hDispHandle, _T("CsDrvGetBoardsInfo"));
	_ASSERT(NULL != m_pCsDrvGetBoardsInfo);
	if (NULL == m_pCsDrvGetBoardsInfo)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvGetParams = (LPDRVGETPARAMS)::GetProcAddress(m_hDispHandle, _T("CsDrvGetParams"));
	_ASSERT(NULL != m_pCsDrvGetParams);
	if (NULL == m_pCsDrvGetParams)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}

	m_pCsDrvAutoFirmwareUpdate = (LPDRVAUTOFIRMWAREUPDATE)::GetProcAddress(m_hDispHandle, _T("CsDrvAutoFirmwareUpdate"));
	_ASSERT(NULL != m_pCsDrvAutoFirmwareUpdate);
	if (NULL == m_pCsDrvAutoFirmwareUpdate)
	{
		::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
		return FALSE;
	}


	// Check if this is NULL before using it. If it is we can continue but we won't be able to check if the system supports
	// booting from Image 1 or 2 so we'll assume that it can't
	m_pCsDrvGetAcquisitionSystemCaps = (LPDRVGETACQUISITIONSYSTEMCAPS)::GetProcAddress(m_hDispHandle, _T("CsDrvGetAcquisitionSystemCaps"));

	m_pCsDrvSetRemoteSystemInUse = (LPDRVSETREMOTESYSTEMINUSE)::GetProcAddress(m_hDispHandle, _T("CsDrvSetRemoteSystemInUse"));
	m_pCsDrvIsRemoteSystemInUse = (LPDRVISREMOTESYSTEMINUSE)::GetProcAddress(m_hDispHandle, _T("CsDrvIsRemoteSystemInUse"));
	m_pCsDrvGetRemoteUser = (LPDRVGETREMOTEUSER)::GetProcAddress(m_hDispHandle, _T("CsDrvGetRemoteUser"));
	// Note: only the remote system will have these functions, so we'll always have to check if it's not null before using it

	::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
	return TRUE;
}

BOOL CsResourceManager::UnLoadDriverDll(void)
{
	::EnterCriticalSection(&m_csDriverLoadUnloadSection);

	BOOL bRet = ::FreeLibrary(m_hDispHandle);
	m_hDispHandle = NULL;

	m_pCsDrvGetAcquisitionSystemCount = NULL;
	m_pCsDrvGetAcquisitionSystemHandles = NULL;
	m_pCsDrvGetAcquisitionSystemInfo = NULL;
	m_pCsDrvUpdateLookupTable = NULL;
	m_pCsDrvGetBoardsInfo = NULL;
	m_pCsDrvGetParams = NULL;
	m_pCsDrvOpenSystemForRm = NULL;
	m_pCsDrvCloseSystemForRm = NULL;
	m_pCsDrvSetRemoteSystemInUse = NULL;
	m_pCsDrvIsRemoteSystemInUse = NULL;
	::LeaveCriticalSection(&m_csDriverLoadUnloadSection);
	return bRet;
}


int32 CsResourceManager::FindSystemIndex(RMHANDLE hSystemHandle)
{
	int32 i32FoundIndex = -1;

	for (int32 i = 0; i < m_pSystem->i32NumberofSystems; i++)
	{
		if (m_pSystem->System[i].rmResourceHandle == hSystemHandle)
		{
			i32FoundIndex = i;
			break;
		}
	}
	return i32FoundIndex;
}

#if defined (_WIN32)
BOOL CsResourceManager::GetSessionUserToken(OUT LPHANDLE  lphUserToken)
{
	BOOL   bResult = FALSE;
	HANDLE hImpersonationToken = INVALID_HANDLE_VALUE;
  
	if (lphUserToken != NULL) 
	{
		HMODULE  hmod = ::LoadLibrary("kernel32.dll");
		if( NULL == hmod ) return FALSE;
		WTSGETACTIVECONSOLESESSIONID lpfnWTSGetActiveConsoleSessionId = (WTSGETACTIVECONSOLESESSIONID)::GetProcAddress(hmod, "WTSGetActiveConsoleSessionId"); 
		DWORD dwSessionId(0xFFFFFFFF );
		if( NULL != lpfnWTSGetActiveConsoleSessionId )
		dwSessionId = lpfnWTSGetActiveConsoleSessionId();
		if( 0xFFFFFFFF == dwSessionId)  { ::FreeLibrary(hmod); return FALSE;}
		
		HMODULE  hmodWTS = ::LoadLibrary("Wtsapi32.dll");
		if( NULL == hmod ) {::FreeLibrary(hmod); return FALSE;}
		WTSQUERYUSERTOKEN lpfnWTSQueryUserToken = (WTSQUERYUSERTOKEN)::GetProcAddress(hmodWTS, "WTSQueryUserToken"); 
		if( NULL == lpfnWTSQueryUserToken ) { ::FreeLibrary(hmod); ::FreeLibrary(hmodWTS); return FALSE;}
		
		// Call WTSQueryUserToken to retrieve a handle to the user access token
		// for the current active session. The WTSGetActiveConsoleSessionId function 
		// retrieves the Terminal Services session currently attached to the physical console. 
		// The physical console is the monitor, keyboard, and mouse.
		// This code works for FUS, but it does not work for TS servers where the 
		// current active session is always the console.
		// The thread must be in local system account for WTSQueryUserToken to succeed.
		if (lpfnWTSQueryUserToken (dwSessionId, &hImpersonationToken)) 
		{
			// The token retrieved by the function is an impersonation
			// token. Create a primary token with the same access rights as the original token.
			bResult = ::DuplicateTokenEx( hImpersonationToken, 0,NULL, SecurityImpersonation, TokenPrimary, lphUserToken);
			::CloseHandle(hImpersonationToken); // Close the handle to the impersonation token.
		}            
		::FreeLibrary(hmod); 
		::FreeLibrary(hmodWTS);
	}
	return bResult;
}
#endif

int32 CsResourceManager::CsRmInitialize(int16 i16HardReset /* = 0*/)
{
	CSHANDLE* pSystemHandles;
	CSSYSTEMHANDLEINFO csHandleInfo;
	int16 i16NumberOfSystems = 0;
	bool bSystemsRecounted(false);

	// If CsRmInitialize has already been called then just return the
	// total number of systems.
	m_pSystem->i16HardReset = i16HardReset;


/*	REMOVED FOR VISTA 64 AND 32 BIT RUNNING TOGETHER (Sept. 28, 2007)
	// Attempting a reset or a full initialization when there are active
	// system(s) is an error, except the first time thru
	if (0 != m_pSystem->i32NumberOfSystemsInUse)
		return CS_RE_INIT_FAILED;
*/
	// mutex is created in the constructor
	::GetMutex(m_hRmMutex);

	// If m_hDispHandle is NULL the driver dll is not loaded so we
	// need to load it.
	if (NULL == m_hDispHandle)
	{
		BOOL bRet = LoadDriverDll();
#ifndef __linux__	
		if ( bRet )
		{
			int nSeconds = ::GetRequestedRefreshTimeInSeconds();
			::CreateAndSetTimer(nSeconds, m_hTimer, m_hTimerThreadHandle, (void *)this); //RG REPLACED MAY 18, 2012
		}
#else // is linux
		if ( bRet )
		{
			int nSeconds = 300; //RG right now this is ignored and we're setting
							    // the time in pthread_create
			CreateAndSetTimer(nSeconds, (void *)this);
		}
#endif // not __linux__

		// If we're able to load the dll, and i16HardReset is not set to 1
		// then set i16HardReset to -1 so we'll do a full reinitialize.
		// This is needed so RM can reconnect to CsDisp and the shared region of memory
		// If we were set to 1 we do a full reinitialize anyways.
		if (bRet && (1 != i16HardReset))
			i16HardReset = -1;
	}

	// i16HardReset is used to reorganize the drivers by calling
	// CsDrvGetAcquisitionSystemCount with a parameter of 1, and also completely reinitializing
	// the drivers.  If i16HardReset is less then 0, we just want to reinitialize the drivers
	// without reorganizing them so we call CsDrvGetAcquisitionSystemCount with a 0.
	if (i16HardReset <= 0)
	{
		i16HardReset = 0;
		m_pSystem->rmReorganizeFlag = 0;
	}
	else
		m_pSystem->rmReorganizeFlag = 1;

Recount:

	BOOL bSystemInUse = FALSE;
	for ( int i = 0; i < m_pSystem->i32NumberofSystems; i++ )
	{
		// CS_RM_ACTIVE is 2 for 64 bit systems and 1 for 32 bit systems.
		// Distinguish between active 32 bit and active 64 bit. For example, if a 32-bit system is active and 
		// this is 64-bit RM we should still call GetAcquisitionSystemCount.
		if ( CS_RM_ACTIVE == m_pSystem->System[i].i8HandleActive ) 
		{
			bSystemInUse = TRUE;
		}
	}
	// if any systems are in use, we don't want to call m_pCsDrvGetAcquisitionSystemCount because it destroys the 
	// lookup table and the system that is running may stop. (Only for systems active within this process i.e. either
	// 32 bit systems or 64 bit systems).
	int32 i32Status;
	if ( !bSystemInUse )
	{
		i32Status = m_pCsDrvGetAcquisitionSystemCount(i16HardReset, (uInt16 *) &i16NumberOfSystems);
	}
	else 
	{
		i32Status = CS_SUCCESS;
		i16NumberOfSystems = (int16)m_pSystem->i32NumberofSystems;
	}

	if ( CS_INVALID_FW_VERSION == i32Status )
	{
		i16NumberOfSystems = AutoUpdateHandler(i16HardReset);
	}
	else if (CS_FAILED(i32Status))
	{
		::ReleaseMutex(m_hRmMutex);
		return i32Status;
	}

	// If the number of systems has changed report it as an event
	if (i16NumberOfSystems != m_pSystem->i32NumberofSystems)
	{
#if defined (_WIN32)	// Check if AddMessageToLog is in Linux
		TCHAR strReport[20];
		wsprintf(strReport, "%d syste%s found.", i16NumberOfSystems, 1 < i16NumberOfSystems? _T("ms") : _T("m"));
		::AddToMessageLog(MSG_RM_SYSTEMS_FOUND, TEXT(strReport));
#endif
		m_pSystem->i32NumberofSystems = i16NumberOfSystems;
	}

	if (0 == m_pSystem->i32NumberofSystems)
	{
		::ReleaseMutex(m_hRmMutex);
		return CS_NO_SYSTEMS_FOUND;
	}

	uInt32 u32SizeOfHandleBuffer = m_pSystem->i32NumberofSystems * sizeof(CSHANDLE);
	pSystemHandles = (CSHANDLE *)::VirtualAlloc(NULL, u32SizeOfHandleBuffer, MEM_COMMIT, PAGE_READWRITE);

	_ASSERT(pSystemHandles != NULL);

	if (NULL == pSystemHandles)
	{
		::ReleaseMutex(m_hRmMutex);
		return CS_MEMORY_ERROR;
	}

	uInt32 u32RetVal = m_pCsDrvGetAcquisitionSystemHandles(pSystemHandles, u32SizeOfHandleBuffer);
	_ASSERT(u32RetVal == (uInt32)m_pSystem->i32NumberofSystems);
	if (u32RetVal != (uInt32)m_pSystem->i32NumberofSystems)
	{
#if defined (_WIN32)
		m_pSystem->i32NumberofSystems = (int32)_cpp_min(u32RetVal, (uInt32)m_pSystem->i32NumberofSystems);
#else
		m_pSystem->i32NumberofSystems = (int32)MIN(u32RetVal, (uInt32)m_pSystem->i32NumberofSystems);
#endif
	}

	int32 i32SystemsActuallyFound = 0;
	for (int32 i = 0; i < m_pSystem->i32NumberofSystems; i++)
	{
		if (CS_RM_INACTIVE != m_pSystem->System[i].i8HandleActive) 
		{
			i32SystemsActuallyFound++;
			continue;
		}

		csHandleInfo.csDriverHandle = pSystemHandles[i];
		csHandleInfo.rmResourceHandle = (RM_BASE_HANDLE | (i + 11)); //FIX THIS make unique handle now

		// If the high bit in the driver handle is NOT set it's a real handle.
		// If it is then set the high bit in the Resource Manager handle to
		// denote that it's a demo
		uInt32 u32Mask = csHandleInfo.csDriverHandle >> 24;
		if ( (u32Mask & 0xff) != 0 )
		{
			csHandleInfo.rmResourceHandle |= RM_REMOTE_HANDLE;
		}
		else
		{
			u32Mask = csHandleInfo.csDriverHandle >> 8;
			if ((u32Mask & 0xffff) == 0x1234)
			{
				csHandleInfo.rmResourceHandle |= RM_DEMO_HANDLE;
			}
		}
		i32Status = m_pCsDrvUpdateLookupTable(pSystemHandles[i], csHandleInfo.rmResourceHandle);
		if (CS_FAILED(i32Status)) continue; // If we fail, continue on to the next system.

		// if it'a a remote system and we don't have the handle, check if anyone does
		if ( (csHandleInfo.rmResourceHandle & RM_REMOTE_HANDLE) && (m_pSystem->System[i].i8HandleActive != CS_RM_ACTIVE) )
		{
			BOOL bSet = FALSE;
			i32Status = m_pCsDrvIsRemoteSystemInUse(csHandleInfo.rmResourceHandle, &bSet);
			bSet = CS_SUCCEEDED(i32Status) ? bSet : FALSE;

			m_pSystem->System[i].i8HandleActive = bSet ? CS_RM_ACTIVE_REMOTE : CS_RM_INACTIVE;
		}

		csHandleInfo.i8HandleActive = m_pSystem->System[i].i8HandleActive;

//		if (CS_RM_INACTIVE == m_pSystem->System[i].i8HandleActive || CS_RM_ACTIVE_REMOTE == m_pSystem->System[i].i8HandleActive)
		if ( CS_RM_ACTIVE != m_pSystem->System[i].i8HandleActive )
		{
			// if the system is remote active we don't have to call OpenSystemForRm ( or CloseSystemForRm ).
			// The information will already be there because the system that is using it will have called these functions
			// and the information ( they calls in between the Open and Close ) is already available.
			if ( CS_RM_ACTIVE_REMOTE != m_pSystem->System[i].i8HandleActive )
			{
				i32Status = m_pCsDrvOpenSystemForRm(csHandleInfo.rmResourceHandle);
				if (CS_FAILED(i32Status)) continue; // If the call failed ignore that system
			}

			csHandleInfo.u32ProcessID = 0; // Will add the process id in GetSystem
			ZeroMemory(csHandleInfo.strProcessName, sizeof(csHandleInfo.strProcessName)/sizeof(csHandleInfo.strProcessName[0]));

			::ZeroMemory(&csHandleInfo.csSystemInfo, sizeof(CSSYSTEMINFO));
			csHandleInfo.csSystemInfo.u32Size = sizeof(CSSYSTEMINFO);

			i32Status = m_pCsDrvGetAcquisitionSystemInfo(csHandleInfo.rmResourceHandle, &csHandleInfo.csSystemInfo);
			if (CS_FAILED(i32Status)) continue;  // If the call failed ignore that system

			csHandleInfo.i16SystemNumber = (int16)(i + 1);
			csHandleInfo.dwTimeActive = 0;
			// Get the serial numbers for master and slaves of each system
			// The max number of slaves is arbitrarily set at 16
			ARRAY_BOARDINFO* pArrayBoardInfo = static_cast<ARRAY_BOARDINFO*>(::VirtualAlloc (NULL, ((csHandleInfo.csSystemInfo.u32BoardCount - 1) * sizeof(CSBOARDINFO)) + sizeof(ARRAY_BOARDINFO), MEM_COMMIT, PAGE_READWRITE));
			if (NULL != pArrayBoardInfo)
			{
				pArrayBoardInfo->u32BoardCount = csHandleInfo.csSystemInfo.u32BoardCount;

				for (uInt32 u32Index = 0; u32Index < pArrayBoardInfo->u32BoardCount; u32Index++)
				{
					pArrayBoardInfo->aBoardInfo[u32Index].u32BoardIndex = u32Index + 1;
					pArrayBoardInfo->aBoardInfo[u32Index].u32Size = sizeof(CSBOARDINFO);
				}
				int32 i32RetValue = m_pCsDrvGetBoardsInfo(csHandleInfo.rmResourceHandle, pArrayBoardInfo);
				if (CS_SUCCEEDED(i32RetValue))
				{
					for (uInt32 u32Index = 0; u32Index < pArrayBoardInfo->u32BoardCount; u32Index++)
					{
						lstrcpyn(csHandleInfo.strSerialNumber[u32Index], pArrayBoardInfo->aBoardInfo[u32Index].strSerialNumber, _countof(csHandleInfo.strSerialNumber[u32Index]));
					}
					// Just use the master's version numbers
					csHandleInfo.u32BaseBoardVersion = pArrayBoardInfo->aBoardInfo[0].u32BaseBoardVersion;
					csHandleInfo.u32AddOnVersion = pArrayBoardInfo->aBoardInfo[0].u32AddonBoardVersion;
				}
				else
				{
					for (uInt32 u32Index = 0; u32Index < pArrayBoardInfo->u32BoardCount; u32Index++)
						lstrcpyn((LPSTR)csHandleInfo.strSerialNumber[u32Index], "Unknown", _countof(csHandleInfo.strSerialNumber[u32Index]));

					csHandleInfo.u32BaseBoardVersion = 0;
					csHandleInfo.u32AddOnVersion = 0;
				}
				::VirtualFree(pArrayBoardInfo, 0, MEM_RELEASE);
			}
			else
			{
				for (uInt32 u32Index = 0; u32Index < pArrayBoardInfo->u32BoardCount; u32Index++)
					lstrcpy((LPSTR)csHandleInfo.strSerialNumber[u32Index], "Unknown");

				csHandleInfo.u32BaseBoardVersion = 0;
				csHandleInfo.u32AddOnVersion = 0;
			}

			::ZeroMemory(&csHandleInfo.csVersionInfo, sizeof(CS_FW_VER_INFO));
			csHandleInfo.csVersionInfo.u32Size = sizeof(CS_FW_VER_INFO);
			int32 i32RetValue = m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_READ_VER_INFO, &csHandleInfo.csVersionInfo);
			if (CS_FAILED(i32RetValue))
			{
				// If we fail, probably because this is not supported on legacy boards,
				// zero out the structure
				::ZeroMemory(&csHandleInfo.csVersionInfo, sizeof(CS_FW_VER_INFO));
			}

			// Zero out structure first so unused fields will be 0. This might
			// be the case for legacy boards.

			::ZeroMemory(&csHandleInfo.csSystemInfoEx, sizeof(CSSYSTEMINFOEX));
			csHandleInfo.csSystemInfoEx.u32Size = sizeof(CSSYSTEMINFOEX);
			m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_SYSTEMINFO_EX, &csHandleInfo.csSystemInfoEx);
			// If the board is a legacy board, we'll get the eeprom options from
			// systeminfo and put them into the AddonOptions field.
			if ((CsIsLegacyBoard(csHandleInfo.csSystemInfo.u32BoardType)) || (CsIsDemoBoard(csHandleInfo.csSystemInfo.u32BoardType)))
			{
				csHandleInfo.csSystemInfoEx.u32AddonHwOptions = csHandleInfo.csSystemInfo.u32AddonOptions;
				csHandleInfo.csSystemInfoEx.u32BaseBoardHwOptions = 0;
			}

			// Zero out structure. Needed for non PCIe boards

			::ZeroMemory(&csHandleInfo.csPCIeInfo, sizeof(PCIeLINK_INFO) * MAX_BOARD_INSYSTEM);

			for (uInt32 u32Index = 0; u32Index < csHandleInfo.csSystemInfo.u32BoardCount; u32Index++)
			{
				csHandleInfo.csPCIeInfo[u32Index].u16CardIndex = uInt16(u32Index) + 1;
				m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_READ_PCIeLINK_INFO, &csHandleInfo.csPCIeInfo[u32Index]);
			}
			// if the call succeeds CAPS_FWCHANGE_REBOOT is supported, otherwise it's not

			csHandleInfo.bCanChangeBootFw = CS_SUCCEEDED(m_pCsDrvGetAcquisitionSystemCaps(csHandleInfo.rmResourceHandle, CAPS_FWCHANGE_REBOOT, NULL, NULL, NULL)) ? true : false;

			// get which image this system is booting from, normal (0) or one of the expert images
			i32RetValue = m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_CURRENT_BOOT_LOCATION, &csHandleInfo.nBootLocation);
			// if the call fails assume the boot location is image 0
			if (CS_FAILED(i32RetValue))
			{
				csHandleInfo.nBootLocation = 0;
			}

	// adding June 26, 2015			
			i32RetValue = m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_NEXT_BOOT_LOCATION, &csHandleInfo.nNextBootLocation);
			// if the call fails assume the next boot location is the same as the current boot location
			if (CS_FAILED(i32RetValue))
			{
				csHandleInfo.nNextBootLocation = csHandleInfo.nBootLocation;
			}			
			FIRMWARE_VERSION_STRUCT fwStruct;
			i32RetValue = m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_CURRENT_FW_VERSION, &fwStruct);
			// if the call fails assume the next boot location is the same as the current boot location
			csHandleInfo.fwVersion.u32Reg = CS_FAILED(i32RetValue) ? 0 : fwStruct.BaseBoard.u32Reg;
					
	// end of adding June 26, 2015


			// RG - Check to see if the baseboard flash checksum is valid (check for each of the images ??)
			CS_FLASHLAYOUT_INFO csFlashLayout;
			csFlashLayout.u32Addon = 0;
			i32RetValue = m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_FLASHINFO, &csFlashLayout);
			// total images should always be at least 1
			int nImageCount = CS_SUCCEEDED(i32RetValue) ? csFlashLayout.u32NumOfImages : 1; 
			csHandleInfo.nImageCount = nImageCount;

			CS_GENERIC_EEPROM_READWRITE csFlash;
			csFlash.u32Size = sizeof(CS_GENERIC_EEPROM_READWRITE);
			csFlash.u32BufferSize = sizeof(uInt32);

			if (!CsIsDecadeBoard(csHandleInfo.csSystemInfo.u32BoardType))
			{
				// non-Decade boards should always have the checksum ok. If it's not it will be 
				// handled by auto-update
				::memset(csHandleInfo.bIsChecksumValid, true, sizeof(csHandleInfo.bIsChecksumValid));
			}
			else // only Decade boards
			{
				::memset(csHandleInfo.bIsChecksumValid, false, sizeof(csHandleInfo.bIsChecksumValid));

	// RG FOR NOW ONLY DO FOR 1 BOARD - NEED TO CHECK THIS
	//			for (uInt32 u32Board = 0; u32Board < csHandleInfo.csSystemInfo.u32BoardCount; u32Board++)
	//			{
					for (int nImageIndex = 0; nImageIndex < nImageCount; nImageIndex++)
					{
						if (0 == csFlashLayout.u32FlashType) // old style flash structure
						{
							csFlash.u32Offset = csFlashLayout.u32WritableStartOffset + (csFlashLayout.u32ImageStructSize * csFlashLayout.u32NumOfImages)
													+ sizeof(BaseBoardFlashHeader) + (nImageIndex * sizeof(uInt32));
						}
						else
						{
							csFlash.u32Offset = csFlashLayout.u32WritableStartOffset + (nImageIndex * csFlashLayout.u32ImageStructSize) 
													+ csFlashLayout.u32FwInfoOffset + sizeof(uInt32);
						}
						uInt32 u32Checksum;
						csFlash.pBuffer = &u32Checksum;
						i32RetValue = m_pCsDrvGetParams(csHandleInfo.rmResourceHandle, CS_READ_EEPROM, &csFlash);
						csHandleInfo.bIsChecksumValid[nImageIndex] = (FWL_FWCHECKSUM_VALID == u32Checksum);
					}
	//			}
			}

			if ( CS_RM_ACTIVE_REMOTE != m_pSystem->System[i].i8HandleActive )
			{
				i32RetValue = m_pCsDrvCloseSystemForRm(csHandleInfo.rmResourceHandle);
				if (CS_FAILED(i32RetValue))	continue;  // If the call failed ignore that system
			}

			::CopyMemory(&m_pSystem->System[i32SystemsActuallyFound], &csHandleInfo, sizeof(CSSYSTEMHANDLEINFO));
		}
		i32SystemsActuallyFound++;
	}

	if( !bSystemsRecounted && //Have not done already
		1 != i16HardReset &&  //Was not Reinitilised anyway
		i32SystemsActuallyFound != m_pSystem->i32NumberofSystems ) //A descrepancy found
	{
		bSystemsRecounted = true;
		m_pCsDrvGetAcquisitionSystemCount(1, (uInt16 *)&i16NumberOfSystems);
		goto Recount;
	}

	::VirtualFree(pSystemHandles, 0, MEM_RELEASE);

	// this next part added May 18, 2012 - Now that we have a system in use, start the refresh timer
	// we'll cancel it in CsRmFreeSystem if there's no more systems being used
//	if ( m_bStartRefreshTimerFlag )
//	{
//		if ( i32SystemsActuallyFound > 0 )
//		{
//			int nSeconds = ::GetRequestedRefreshTimeInSeconds();
//			::SetRefreshTimer(nSeconds, m_hTimer);
//			m_bStartRefreshTimerFlag = FALSE;
//		}
//	}
	::ReleaseMutex(m_hRmMutex);

	return i32SystemsActuallyFound;
}


BOOL CsResourceManager::ChangeToCombineDir(void)
{
#if defined (_WIN32)
	BOOL bReturn;
	TCHAR systemStr[MAX_PATH + 1];
#ifdef _DEBUG
	TCHAR* ptCh;  

	errno_t err = ::_tdupenv_s(&ptCh, NULL, _T("CombineWDF")); // Feb 2, 2010 - use new environment variable
	if (err)
		ptCh = NULL;

	if(NULL != ptCh)
	{
		_tcsncpy_s(systemStr, _countof(systemStr), ptCh, _countof(systemStr));
		::free(ptCh);
		// if last character is no \, then add it
		if( systemStr[_tcslen(systemStr)-1] != _T('\\') )
		{
			::_tcscat_s(systemStr, MAX_PATH,  _T("\\") );
		}
#ifdef CS_WIN64
		TCHAR strName[] = _T("Bin_Debug64");
#else
		TCHAR strName[] = _T("Bin_Debug");
#endif
		::_tcsncat_s(systemStr, _countof(systemStr), strName, _countof(strName));
	}
	else
	{
		::GetSystemDirectory(systemStr, MAX_PATH);
	}
#else // if not debug

	::GetSystemDirectory(systemStr, MAX_PATH);

#endif

	bReturn = ::SetCurrentDirectory(systemStr);
	return bReturn;
#else
	return TRUE;
#endif
}


int32 CsResourceManager::CsRmGetAvailableSystems(void)
{
	int32 i32NumberOfSystems = m_pSystem->i32NumberofSystems;
	::GetMutex(m_hRmMutex);

	for (int32 i = 0; i < m_pSystem->i32NumberofSystems; i++)
	{
		if (CS_RM_INACTIVE != m_pSystem->System[i].i8HandleActive)
		{
			i32NumberOfSystems--;
		}
	}
	::ReleaseMutex(m_hRmMutex);

	return i32NumberOfSystems;
}


int32 CsResourceManager::CsRmLockSystem(RMHANDLE hSystemHandle, uInt32 u32ProcessID)
{
	int32 i32RetVal = CS_INVALID_HANDLE;

	::GetMutex(m_hRmMutex);
	int32 i32FoundIndex = FindSystemIndex(hSystemHandle);

	if (-1 != i32FoundIndex)
	{
		// We've found the handle in the table
		// Now check to see if is active (locked) or not
		if (CS_RM_INACTIVE == m_pSystem->System[i32FoundIndex].i8HandleActive)
		{
      		// It's not locked, so mark it as active, increase the NumberOfSystemsInUse
			// and remember the process ID.
			m_pSystem->System[i32FoundIndex].i8HandleActive = CS_RM_ACTIVE;
			m_pSystem->System[i32FoundIndex].u32ProcessID = u32ProcessID;

//			::GetProcessName(u32ProcessID, m_pSystem->System[i32FoundIndex].strProcessName);
			if ( m_ProcessList )
			{
				m_ProcessList->GetName(u32ProcessID, m_pSystem->System[i32FoundIndex].strProcessName);
			}
			else
			{
				::GetProcessName(u32ProcessID, m_pSystem->System[i32FoundIndex].strProcessName);
			}

			i32RetVal = CS_SUCCESS;
			m_pSystem->System[i32FoundIndex].dwTimeActive = ::GetTickCount();
			m_pSystem->i32NumberOfSystemsInUse++;
		}
		else
		{
			// It's in the table but is already marked as locked. Either the same process is
			// trying to lock it again or another process is trying to lock it.
			// Regardless, return an error that the handle is already in use.

			// If for some reason the processID is 0, then mark it with the current processID.
			if (0 == m_pSystem->System[i32FoundIndex].u32ProcessID)
				m_pSystem->System[i32FoundIndex].u32ProcessID = u32ProcessID;

			i32RetVal = CS_HANDLE_IN_USE;
		}
	}
	::ReleaseMutex(m_hRmMutex);

	// If we break with no match, the handle is not in the table and we return i32RetVal as 0 (INVALID_HANDLE).
	return i32RetVal;
}


int32 CsResourceManager::CsRmFreeSystem(RMHANDLE hSystemHandle)
{
	CSHANDLE csHandle = 0;
	BOOL bHandleWasNotActive = FALSE;

	::GetMutex(m_hRmMutex);

	int32 i32FoundIndex = FindSystemIndex(hSystemHandle);

	if (-1 != i32FoundIndex)
	{
		csHandle = m_pSystem->System[i32FoundIndex].csDriverHandle;

#ifdef _DEBUG

		DWORD dwHours, dwMinutes, dwSeconds, dwMilliseconds;
		DWORD dwTimeStarted = m_pSystem->System[i32FoundIndex].dwTimeActive;
		::CalculateTimeActive(dwTimeStarted, &dwHours, &dwMinutes, &dwSeconds, &dwMilliseconds);

		TRACE(RM_TRACE_LEVEL, "RM::Removing process %d\n", m_pSystem->System[i32FoundIndex].u32ProcessID);
		TRACE(RM_TRACE_LEVEL, "RM::System active for: %d hrs, %d minutes, %d seconds, %d milliseconds\n", dwHours, dwMinutes, dwSeconds, dwMilliseconds);

#endif

		if (CS_RM_INACTIVE == m_pSystem->System[i32FoundIndex].i8HandleActive)
		{
			bHandleWasNotActive = TRUE;
		}
		// Even if the handle wasn't active we can still do this part.
		// mark handle as not in use, etc.

		m_pSystem->System[i32FoundIndex].i8HandleActive = CS_RM_INACTIVE;
		m_pSystem->System[i32FoundIndex].u32ProcessID = 0;

		ZeroMemory(m_pSystem->System[i32FoundIndex].strProcessName, sizeof(m_pSystem->System[i32FoundIndex].strProcessName)/sizeof(m_pSystem->System[i32FoundIndex].strProcessName[0]));

		m_pSystem->System[i32FoundIndex].dwTimeActive = 0;
		// if we're using remote systems RG ??? TESTING
		if ( hSystemHandle & RM_REMOTE_HANDLE )
		{
			//  have to send a message to GageServer so it knows to mark it's system as in use	
			m_pCsDrvSetRemoteSystemInUse(hSystemHandle, FALSE); // ??? RG CHECK IF I'M USING THE RIGHT HANDLE
		}
	}
	::ReleaseMutex(m_hRmMutex);
	int32 i32Ret;

	if (0 == csHandle)
	{
		return CS_INVALID_HANDLE;
	}
	else
	{
		if ( !bHandleWasNotActive )
		{
			if ( m_pSystem->i32NumberOfSystemsInUse > 0 )
				m_pSystem->i32NumberOfSystemsInUse--;
			i32Ret = CS_SUCCESS;
		}
		else
		{
			// If we try to free a system that has already been freed, return
			// CS_FALSE as a warning rather than an error
			i32Ret = CS_FALSE;
		}
	}
/* RG ???
	// if there are no systems and the refresh timer is running, stop it
	if ( m_pSystem->i32NumberofSystems <= 0 && !m_bStartRefreshTimerFlag )
	{
		::CancelWaitableTimer(m_hTimer);
		m_bStartRefreshTimerFlag = TRUE;
	}
*/
	return i32Ret;
}

int32 CsResourceManager::CsRmIsSystemHandleValid(RMHANDLE hSystemHandle, uInt32 u32ProcessID)
{
	// If hSystemHandle is not in the table, we set *pSystemHandle to 0
	// If it is in the table but is in use by someone else we set *pSystemHandle to -1
	// Else we set *pSystemHandle to hSystemHandle, which means it's valid.

	_ASSERT(0 != hSystemHandle);
	int32 i32RetVal = CS_INVALID_HANDLE;

	::GetMutex(m_hRmMutex);

	int32 i32FoundIndex = FindSystemIndex(hSystemHandle);
	if (-1 != i32FoundIndex)
	{
		if (m_pSystem->System[i32FoundIndex].u32ProcessID == u32ProcessID)
			i32RetVal = CS_SUCCESS;
		else
		{
			if (CS_RM_INACTIVE != m_pSystem->System[i32FoundIndex].i8HandleActive)
				i32RetVal = CS_HANDLE_IN_USE;
			else // Handle is in the table but it's not active
				i32RetVal = CS_SUCCESS;
		}
	}
	::ReleaseMutex(m_hRmMutex);

	return i32RetVal;
}

int32 CsResourceManager::CsRmRefreshAllSystemHandles(void)
{
#if defined(_WIN32)
	TRACE(RM_TRACE_LEVEL, "============RM::In Refresh");
#endif

	// Oct. 25, 2016 - Removed mutex from this routine
	// CsRmRefreshAllHandles doesn't need a mutex because
	// it never accesses the hardware

	for (int32 i = 0; i < m_pSystem->i32NumberofSystems; i++)
	{
		// if the system is remote ( and we're not using it ), check to see if it's still in use 
		// we need to check if it's active as a 32 or a 64 bit process
		if ( (m_pSystem->System[i].rmResourceHandle & RM_REMOTE_HANDLE) && 
					( (CS_RM_ACTIVE32 != m_pSystem->System[i].i8HandleActive) && 
					  (CS_RM_ACTIVE64 != m_pSystem->System[i].i8HandleActive) )  )
		{
			BOOL bSet = FALSE;
			
			if (m_pCsDrvIsRemoteSystemInUse)
			{
				int32 i32Ret = m_pCsDrvIsRemoteSystemInUse(m_pSystem->System[i].rmResourceHandle, &bSet);
				bSet = CS_SUCCEEDED(i32Ret) ? bSet : FALSE;

				m_pSystem->System[i].i8HandleActive = bSet ? CS_RM_ACTIVE_REMOTE : CS_RM_INACTIVE;
			}
		}
		else
		{
			uInt32 u32ProcessID = (uInt32)-1;
			if ( m_ProcessList )
			{
				u32ProcessID = m_ProcessList->GetProcessFromList(m_pSystem->System[i].u32ProcessID);
			}
			// If < 0 we couldn't walk the process list or couldn't even call the function
			if ((int32)u32ProcessID < 0)
			{
				ReleaseMutex(m_hRmMutex); //RG
				return CS_FALSE;
			}
			// If the process is not active then clean up that handle
			if (0 == u32ProcessID && CS_RM_ACTIVE_REMOTE != m_pSystem->System[i].i8HandleActive)
			{
				// Check for errors from SystemCleanup
				m_pSystem->System[i].i8HandleActive = CS_RM_INACTIVE;
				m_pSystem->System[i].dwTimeActive = 0;
#if defined(_WIN32)
				TRACE(RM_TRACE_LEVEL, "RM::Removing process %d\n", m_pSystem->System[i].u32ProcessID);
#endif
				m_pSystem->System[i].u32ProcessID = 0;
				if ( m_pSystem->i32NumberOfSystemsInUse > 0 )
					m_pSystem->i32NumberOfSystemsInUse--;
			}
		}
	}
/*	RG ???
	// if there are no systems and the refresh timer is running, stop it
	if ( m_pSystem->i32NumberofSystems <= 0 && !m_bStartRefreshTimerFlag )
	{
		::CancelWaitableTimer(m_hTimer);
		m_bStartRefreshTimerFlag = TRUE;
	}
*/
	return CS_SUCCESS;
}

int32 CsResourceManager::CsRmGetNumberOfSystems(void)
{
	return m_pSystem->i32NumberofSystems;
}

int32 CsResourceManager::CsRmGetSystemStateByHandle(RMHANDLE hSystemHandle, CSSYSTEMSTATE* pSystemState)
{
	DWORD dwHours, dwMinutes, dwSeconds, dwMilliseconds;

	_ASSERT(sizeof(CSSYSTEMSTATE) == pSystemState->u32Size);

	if ( sizeof(CSSYSTEMSTATE) != pSystemState->u32Size )
	{
		return CS_INVALID_STRUCT_SIZE;
	}
	_ASSERT(0 != hSystemHandle);
	int32 i32RetVal = CS_INVALID_HANDLE;

	_ASSERT(pSystemState != NULL);
	if (NULL == pSystemState)
		return CS_NULL_POINTER;

	::GetMutex(m_hRmMutex);

	int32 i32FoundIndex = FindSystemIndex(hSystemHandle);

	if (-1 != i32FoundIndex)
	{
		strcpy_s(pSystemState->strBoardName, sizeof(pSystemState->strBoardName)/sizeof(pSystemState->strBoardName[0]), m_pSystem->System[i32FoundIndex].csSystemInfo.strBoardName);
		pSystemState->u32BoardType = m_pSystem->System[i32FoundIndex].csSystemInfo.u32BoardType;
		pSystemState->u32NumberOfBoards = m_pSystem->System[i32FoundIndex].csSystemInfo.u32BoardCount;
		pSystemState->i64MemorySize = m_pSystem->System[i32FoundIndex].csSystemInfo.i64MaxMemory;
		pSystemState->u32NumberOfChannels = m_pSystem->System[i32FoundIndex].csSystemInfo.u32ChannelCount;
		pSystemState->u32NumberOfBits = m_pSystem->System[i32FoundIndex].csSystemInfo.u32SampleBits;
		pSystemState->rmHandle = m_pSystem->System[i32FoundIndex].rmResourceHandle;

#if defined (_WIN32)
		for (uInt32 u32Index = 0; u32Index < min(pSystemState->u32NumberOfBoards,MAX_BOARD_INSYSTEM); u32Index++)
#else
		for (uInt32 u32Index = 0; u32Index < MIN(pSystemState->u32NumberOfBoards,MAX_BOARD_INSYSTEM); u32Index++)
#endif
		{
			lstrcpyn(pSystemState->strSerialNumber[u32Index], m_pSystem->System[i32FoundIndex].strSerialNumber[u32Index], _countof(pSystemState->strSerialNumber[u32Index]));
		}

		::CopyMemory(&pSystemState->csVersionInfo, &m_pSystem->System[i32FoundIndex].csVersionInfo, sizeof(CS_FW_VER_INFO));
		::CopyMemory(&pSystemState->csSystemInfoEx, &m_pSystem->System[i32FoundIndex].csSystemInfoEx, sizeof(CSSYSTEMINFOEX));
		::CopyMemory(&pSystemState->csPCIeInfo, &m_pSystem->System[i32FoundIndex].csPCIeInfo, sizeof(PCIeLINK_INFO) * MAX_BOARD_INSYSTEM);
		pSystemState->u32BaseBoardVersion = m_pSystem->System[i32FoundIndex].u32BaseBoardVersion;
		pSystemState->u32AddOnVersion = m_pSystem->System[i32FoundIndex].u32AddOnVersion;
		pSystemState->i8Active = m_pSystem->System[i32FoundIndex].i8HandleActive;
		if (CS_RM_INACTIVE == pSystemState->i8Active)
		{
			dwHours = dwMinutes = dwSeconds = dwMilliseconds = 0;
		}
		else if ( CS_RM_ACTIVE_REMOTE == pSystemState->i8Active )
		{
			dwHours = dwMinutes = dwSeconds = dwMilliseconds = 0;
			DRVHANDLE drvHandle = m_pSystem->System[i32FoundIndex].csDriverHandle;
			TCHAR strIPAddress[80];
			sprintf_s(strIPAddress, sizeof(strIPAddress), _T("Remote process on %d.%d.%d.%d"), (drvHandle >> 24) & 0xff, (drvHandle >> 16) & 0xff, (drvHandle >> 8) & 0xff, drvHandle & 0xff);
			strcpy_s(pSystemState->lpOwner, sizeof(pSystemState->lpOwner)/sizeof(pSystemState->lpOwner[0]), strIPAddress);

			TCHAR szUser[20];
			if ( CS_SUCCEEDED(m_pCsDrvGetRemoteUser(hSystemHandle, szUser)) )
			{
				{
					// Might have to add strcat_s to CsLinuxPort.h
//					strcat_s(pSystemState->lpOwner, _countof(pSystemState->lpOwner) , "  In use by: ");
//					strcat_s(pSystemState->lpOwner, _countof(pSystemState->lpOwner) , szUser);
					lstrcat(pSystemState->lpOwner, "  In use by: ");
					lstrcat(pSystemState->lpOwner, szUser);
				}
			}
		}
		else	// CS_RM_ACTIVE
		{
			::CalculateTimeActive(m_pSystem->System[i32FoundIndex].dwTimeActive, &dwHours, &dwMinutes, &dwSeconds, &dwMilliseconds);
			strcpy_s(pSystemState->lpOwner, sizeof(pSystemState->lpOwner)/sizeof(pSystemState->lpOwner[0]), m_pSystem->System[i32FoundIndex].strProcessName);
		}
		pSystemState->u32Hours = dwHours;
		pSystemState->u32Minutes = dwMinutes;
		pSystemState->u32Seconds = dwSeconds;
		pSystemState->u32Milliseconds = dwMilliseconds;
		pSystemState->u32ProcessID = m_pSystem->System[i32FoundIndex].u32ProcessID;

	
		if ( (pSystemState->rmHandle & RM_REMOTE_HANDLE) != 0 )
		{
			DRVHANDLE drvHandle = m_pSystem->System[i32FoundIndex].csDriverHandle;
			::sprintf_s( pSystemState->lpIpAddress, 
						_countof(pSystemState->lpIpAddress), 
						_T("%d.%d.%d.%d"), 
						(drvHandle >> 24) & 0xff, (drvHandle >> 16) & 0xff, (drvHandle >> 8) & 0xff, drvHandle & 0xff);
		}
		else
		{
			::strcpy_s(pSystemState->lpIpAddress, _countof(pSystemState->lpIpAddress), _T(""));
		}

		// if the call succeeds CAPS_FWCHANGE_REBOOT is supported, otherwise it's not
		pSystemState->bCanChangeBootFw = m_pSystem->System[i32FoundIndex].bCanChangeBootFw;
		pSystemState->nBootLocation = m_pSystem->System[i32FoundIndex].nBootLocation;
		pSystemState->nNextBootLocation = m_pSystem->System[i32FoundIndex].nNextBootLocation;
		pSystemState->fwVersion.u32Reg = m_pSystem->System[i32FoundIndex].fwVersion.u32Reg;

		pSystemState->nImageCount = m_pSystem->System[i32FoundIndex].nImageCount;

		for (int imageIndex = 0; imageIndex < 3; imageIndex++)
		{
			pSystemState->bIsChecksumValid[imageIndex] = m_pSystem->System[i32FoundIndex].bIsChecksumValid[imageIndex];
		}
//		CopyMemory(pSystemState->bIsChecksumValid, m_pSystem->System[i32FoundIndex].bIsChecksumValid, 3 * sizeof(bool));

		i32RetVal = CS_SUCCESS;
	}
	::ReleaseMutex(m_hRmMutex);

	return i32RetVal;
}

int32 CsResourceManager::CsRmGetSystemStateByIndex(int16 i16Index, CSSYSTEMSTATE* pSystemState)
{
	DWORD dwHours, dwMinutes, dwSeconds, dwMilliseconds;

	_ASSERT(sizeof(CSSYSTEMSTATE) == pSystemState->u32Size);

	if ( sizeof(CSSYSTEMSTATE) != pSystemState->u32Size )
	{
		return CS_INVALID_STRUCT_SIZE;
	}

	_ASSERT(i16Index >= 0);	// Index can be 0, which means don't care
	if ((i16Index > m_pSystem->i32NumberofSystems) || (i16Index < 0))
		return CS_NO_AVAILABLE_SYSTEM;

	_ASSERT(pSystemState != NULL);
	if (NULL == pSystemState)
		return CS_NULL_POINTER;

	::GetMutex(m_hRmMutex);

	int32 i32Ret = CS_NO_AVAILABLE_SYSTEM;

	for (int32 i = 0; i < m_pSystem->i32NumberofSystems; i++)
	{
		if (m_pSystem->System[i].i16SystemNumber != i16Index)
		{
			continue;
		}
		strcpy_s(pSystemState->strBoardName, sizeof(pSystemState->strBoardName)/sizeof(pSystemState->strBoardName[0]),m_pSystem->System[i].csSystemInfo.strBoardName);
		pSystemState->u32BoardType = m_pSystem->System[i].csSystemInfo.u32BoardType;
		pSystemState->u32NumberOfBoards = m_pSystem->System[i].csSystemInfo.u32BoardCount;
		pSystemState->i64MemorySize = m_pSystem->System[i].csSystemInfo.i64MaxMemory;
		pSystemState->u32NumberOfChannels = m_pSystem->System[i].csSystemInfo.u32ChannelCount;
		pSystemState->u32NumberOfBits = m_pSystem->System[i].csSystemInfo.u32SampleBits;
		pSystemState->rmHandle = m_pSystem->System[i].rmResourceHandle;

#if defined (_WIN32)
		for (uInt32 u32Index = 0; u32Index < min(pSystemState->u32NumberOfBoards,MAX_BOARD_INSYSTEM); u32Index++)
#else
		for (uInt32 u32Index = 0; u32Index < MIN(pSystemState->u32NumberOfBoards,MAX_BOARD_INSYSTEM); u32Index++)
#endif
		{
			lstrcpyn(pSystemState->strSerialNumber[u32Index], m_pSystem->System[i].strSerialNumber[u32Index], _countof(pSystemState->strSerialNumber[u32Index]));
		}

		::CopyMemory(&pSystemState->csVersionInfo, &m_pSystem->System[i].csVersionInfo, sizeof(CS_FW_VER_INFO));
		::CopyMemory(&pSystemState->csSystemInfoEx, &m_pSystem->System[i].csSystemInfoEx, sizeof(CSSYSTEMINFOEX));
		::CopyMemory(&pSystemState->csPCIeInfo, &m_pSystem->System[i].csPCIeInfo, sizeof(PCIeLINK_INFO) * MAX_BOARD_INSYSTEM);
		pSystemState->u32BaseBoardVersion = m_pSystem->System[i].u32BaseBoardVersion;
		pSystemState->u32AddOnVersion = m_pSystem->System[i].u32AddOnVersion;

		pSystemState->i8Active = m_pSystem->System[i].i8HandleActive;
		if (CS_RM_INACTIVE == pSystemState->i8Active)
		{
			dwHours = dwMinutes = dwSeconds = dwMilliseconds = 0;
		}
		else if ( CS_RM_ACTIVE_REMOTE == pSystemState->i8Active )
		{
			dwHours = dwMinutes = dwSeconds = dwMilliseconds = 0;
			DRVHANDLE drvHandle = m_pSystem->System[i].csDriverHandle;
			TCHAR strIPAddress[80];
			sprintf_s(strIPAddress, sizeof(strIPAddress), _T("Remote process on %d.%d.%d.%d"), (drvHandle >> 24) & 0xff, (drvHandle >> 16) & 0xff, (drvHandle >> 8) & 0xff, drvHandle & 0xff);
			strcpy_s(pSystemState->lpOwner, sizeof(pSystemState->lpOwner)/sizeof(pSystemState->lpOwner[0]), strIPAddress);
			if ( m_pCsDrvGetRemoteUser )
			{
				TCHAR szUser[20];
				if ( CS_SUCCEEDED(m_pCsDrvGetRemoteUser(m_pSystem->System[i].rmResourceHandle, szUser)) )
				{
					// Add strcat_s to CsLinuxPort.h
//					strcat_s(pSystemState->lpOwner, _countof(pSystemState->lpOwner), "  In use by: ");
//					strcat_s(pSystemState->lpOwner, _countof(pSystemState->lpOwner), szUser);
					lstrcat(pSystemState->lpOwner, "  In use by: ");
					lstrcat(pSystemState->lpOwner, szUser);

				}
			}
			::sprintf_s( pSystemState->lpIpAddress, 
						_countof(pSystemState->lpIpAddress), 
						_T("%d.%d.%d.%d"), 
						(drvHandle >> 24) & 0xff, (drvHandle >> 16) & 0xff, (drvHandle >> 8) & 0xff, drvHandle & 0xff);
		}
		else	// CS_RM_ACTIVE
		{
			::CalculateTimeActive(m_pSystem->System[i].dwTimeActive, &dwHours, &dwMinutes, &dwSeconds, &dwMilliseconds);
			strcpy_s(pSystemState->lpOwner, sizeof(pSystemState->lpOwner)/sizeof(pSystemState->lpOwner[0]), m_pSystem->System[i].strProcessName);
		}	
		pSystemState->u32Hours = dwHours;
		pSystemState->u32Minutes = dwMinutes;
		pSystemState->u32Seconds = dwSeconds;
		pSystemState->u32Milliseconds = dwMilliseconds;
		pSystemState->u32ProcessID = m_pSystem->System[i].u32ProcessID;

		if ( (pSystemState->rmHandle & RM_REMOTE_HANDLE) != 0 )
		{
			DRVHANDLE drvHandle = m_pSystem->System[i].csDriverHandle;
			::sprintf_s( pSystemState->lpIpAddress, 
						_countof(pSystemState->lpIpAddress), 
						_T("%d.%d.%d.%d"), 
						(drvHandle >> 24) & 0xff, (drvHandle >> 16) & 0xff, (drvHandle >> 8) & 0xff, drvHandle & 0xff);
		}
		else
		{
			::strcpy_s(pSystemState->lpIpAddress, _countof(pSystemState->lpIpAddress), _T(""));
		}

		// if the call succeeds CAPS_FWCHANGE_REBOOT is supported, otherwise it's not
		pSystemState->bCanChangeBootFw = m_pSystem->System[i].bCanChangeBootFw;
		pSystemState->nBootLocation = m_pSystem->System[i].nBootLocation;
		pSystemState->nNextBootLocation = m_pSystem->System[i].nNextBootLocation;
		pSystemState->fwVersion.u32Reg = m_pSystem->System[i].fwVersion.u32Reg;

		pSystemState->nImageCount = m_pSystem->System[i].nImageCount;

		for (int imageIndex = 0; imageIndex < 3; imageIndex++)
		{
			pSystemState->bIsChecksumValid[imageIndex] = m_pSystem->System[i].bIsChecksumValid[imageIndex];
		}
//		CopyMemory(pSystemState->bIsChecksumValid, m_pSystem->System[i].bIsChecksumValid, 3 * sizeof(bool));
		
		i32Ret = CS_SUCCESS;
	}
	::ReleaseMutex(m_hRmMutex);
	return i32Ret;
}

int32 CsResourceManager::CsRmGetSystem(uInt32 u32BoardType, uInt32 u32ChannelCount, uInt32 u32SampleBits, RMHANDLE *rmHandle, uInt32 u32ProcessID, int16 i16Index /* = 0 */)
{
	int32 i32RetVal = CS_NO_AVAILABLE_SYSTEM;
	*rmHandle = 0;
	::GetMutex(m_hRmMutex);

    // Just in case a system has crashed or not been freed
	// and is still marked as in use, we'll refresh all the handles

	if (0 < m_pSystem->i32NumberOfSystemsInUse)
		CsRmRefreshAllSystemHandles();
	int nNumberOfTries = 2;
	// This for loop is in case the program crashes or is stopped by the debugger. If
	// we can't get a system on the first try, call CsRefreshAllSystemHandles and try
	// one more time
	for (int nAttempts = 0; nAttempts < nNumberOfTries; nAttempts++)
	{
		for (int32 i = 0; i < m_pSystem->i32NumberofSystems; i++)
		{
			if (CS_RM_INACTIVE == m_pSystem->System[i].i8HandleActive)
			{
				if (u32BoardType)
				{
					if (u32BoardType != m_pSystem->System[i].csSystemInfo.u32BoardType)
						continue;
				}
				if (u32ChannelCount)
				{
					if (u32ChannelCount != m_pSystem->System[i].csSystemInfo.u32ChannelCount)
						continue;
				}
				if (u32SampleBits)
				{
					if (u32SampleBits != m_pSystem->System[i].csSystemInfo.u32SampleBits)
						continue;
				}

				// If we match everything else and i16Index is 0, mark the board as in use and
				// return the handle. If i16Index is non-zero then we don't want the first board that
				// matches the criteria, we want i16Index (ie.2nd if i16Index = 1) board that matches
				// it.  So if i16BoardNumber and i16Index don't match we'll continue looking. i16Index
				// = 0 means get the first board that matches.

				if (i16Index)
				{
					// i16Index starts at 0 (which means get the first)
					// the actual system numbers in the map start at 1.
					// if i16Index == 0 we skip this section because we're
					// at the first iteration
					if (i16Index != m_pSystem->System[i].i16SystemNumber)
						continue;
				}

				*rmHandle = m_pSystem->System[i].rmResourceHandle;

				m_pSystem->System[i].i8HandleActive = CS_RM_ACTIVE; // Mark handle as being in use
				m_pSystem->System[i].u32ProcessID = u32ProcessID;

				if ( m_ProcessList )
				{
					m_ProcessList->GetName(u32ProcessID, m_pSystem->System[i].strProcessName);
				}
				else
				{
					::GetProcessName(u32ProcessID, m_pSystem->System[i].strProcessName);
				}
				// if we're using remote systems RG ??? TESTING
				if ( *rmHandle & RM_REMOTE_HANDLE )
				{
					//  have to send a message to GageServer so it knows to mark it's system as in use	
						m_pCsDrvSetRemoteSystemInUse(*rmHandle, TRUE); // RG - CAN ID JUST DO IT IN AcquisitionSystemInit and AcquisitionSystemCleanup 
																	// in CsRemoteDev.dll ????
				}

				m_pSystem->System[i].dwTimeActive = ::GetTickCount();
				m_pSystem->i32NumberOfSystemsInUse++;
				break;
			}
		}
		// If this is our first try and we can't get a handle, try one more time.
		if (0 == nAttempts)
		{
			if (0 == *rmHandle)
				CsRmRefreshAllSystemHandles();
			else
				break;
		}
	}
	::ReleaseMutex(m_hRmMutex);

	if (*rmHandle > 0)
		i32RetVal = CS_SUCCESS;

	return i32RetVal;
}


int32 CsResourceManager::CsRmGetSystemInfo(RMHANDLE hSystemHandle, CSSYSTEMINFO* pSystemInfo)
{
	_ASSERT(NULL != pSystemInfo);
	if (NULL == pSystemInfo)
		return CS_NULL_POINTER;

	int16 i16Result = 0;

	::GetMutex(m_hRmMutex);

	int32 i32FoundIndex = FindSystemIndex(hSystemHandle);
	
	if (-1 != i32FoundIndex)
	{
		i16Result = CS_SUCCESS;
		::CopyMemory(pSystemInfo, &m_pSystem->System[i32FoundIndex].csSystemInfo, pSystemInfo->u32Size);
	}
	else
		i16Result = CS_INVALID_HANDLE;

	::ReleaseMutex(m_hRmMutex);

	return i16Result;
}


int16 CsResourceManager::AutoUpdateHandler(int16 i16HardReset)
{
	bool	bUpdateFw = true;
	int16 i16NumberOfSystems(0);

#ifdef __linux__

	m_pCsDrvGetAcquisitionSystemCount(i16HardReset, (uInt16 *) &i16NumberOfSystems);
/* this part was in the Savoir Faire code, the above line wasn't

	if (m_pCsDrvAutoFirmwareUpdate && m_pCsDrvGetAcquisitionSystemCount)
	{	::AddToMessageLog(MSG_RM_AUTOUPDATE, _T("AutoUpdateHandler: before CsDrvAutoFirmwareUpdate"));
		int32 i32Status = m_pCsDrvAutoFirmwareUpdate();
		if ( CS_FAILED (i32Status) ) {
			::AddToMessageLog(MSG_RM_AUTOUPDATE, _T("AutoUpdateHandler: CsDrvAutoFirmwareUpdate failed"));
			return 0;
		}
		i32Status = m_pCsDrvGetAcquisitionSystemCount(i16HardReset, (uInt16 *) &i16NumberOfSystems);
		if ( CS_FAILED (i32Status) ) {
			::AddToMessageLog(MSG_RM_AUTOUPDATE, _T("AutoUpdateHandler: CsDrvGetAcquisitionSystemCount failed"));
			return 0;
		}
	}
*/
#else

	DWORD dwError = NULL;

	// Check to see that the AutoFirmwareUpdate function exists in these drivers
	// If any of the calls to CsDrvGetAcqcuisitionSystemCount fails, m_i16NumberOfSystems will
	// be set to 0.
	if (m_pCsDrvAutoFirmwareUpdate)
	{	
		HANDLE hProcess(NULL);
		HANDLE hThread(NULL);
		HWND   hDlg(NULL); 
		TCHAR pathSystemDirectory[MAX_PATH];
		BOOL bOpenShutDownDlg = TRUE;
		HANDLE hToken(NULL);

		if (bDebug)
		{
			FARPROC lpfnDIALOGAMsgProc;
			INT_PTR nRc = 0;
			HMODULE hModule = ::GetModuleHandle(_T("CsRm.exe"));

			// Get user confirmation about auto update firmware
			lpfnDIALOGAMsgProc = MakeProcInstance((FARPROC)GetUserFeedBack, hModule);
			nRc = DialogBox(hModule, MAKEINTRESOURCE(IDD_USER_FEEDBACK), NULL, (DLGPROC)lpfnDIALOGAMsgProc);
			FreeProcInstance(lpfnDIALOGAMsgProc);
			
			bUpdateFw = ( IDOK == nRc );

			// Show message
			if ( bUpdateFw )
				hDlg = ::DisplayUpdateMessage();
		}
		else
		{
			// Vista Code for User Elevation and Security
			// Always get the 32Bit emulation system folder on a 64-Bit OS.
			// On 32-Bit, simply get the System folder.

			HMODULE hmod = GetModuleHandle(_T("kernel32.dll"));
			GETSYSTEMWOW64DIRECTORY lpfnGetSystemWow64Directory = (GETSYSTEMWOW64DIRECTORY)::GetProcAddress(hmod, "GetSystemWow64DirectoryA"); 
			if (NULL != lpfnGetSystemWow64Directory)
			{
				if (0 == lpfnGetSystemWow64Directory(pathSystemDirectory, MAX_PATH))
				{
					// Vista 32-Bit returns
					::GetSystemDirectory(pathSystemDirectory, MAX_PATH);
				}
			}
			else
				::GetSystemDirectory(pathSystemDirectory, MAX_PATH);

			lstrcat(pathSystemDirectory, "\\CsAutoUpdatePopup.exe");

			if ( GetSessionUserToken(&hToken) )
			{
				PVOID lpEnvironment(NULL);
				TCHAR  strReport[50];
				PROCESS_INFORMATION	pi;
				ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
				if ( ::CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE) ) 
				{
					TCHAR str[MAX_PATH];
					lstrcpy(str, pathSystemDirectory);
					lstrcat(str, " update");
					DWORD dwFlags = BELOW_NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
					STARTUPINFO	si;
					ZeroMemory( &si, sizeof( STARTUPINFO ) );
					si.cb = sizeof( STARTUPINFO );
					si.lpDesktop = "winsta0\\default";
					if (!::CreateProcessAsUser(hToken, NULL, TEXT(str), NULL, NULL, FALSE, dwFlags, lpEnvironment, NULL, &si, &pi))
					{
						wsprintf(strReport, "Error: (%d) on auto-update %s", GetLastError(), pathSystemDirectory);
						::AddToMessageLog(MSG_RM_AUTOUPDATE, TEXT(strReport));
						bOpenShutDownDlg = FALSE;
					}
					else
					{
						hProcess = pi.hProcess;
						hThread = pi.hThread;
					}
					if (lpEnvironment) 
						::DestroyEnvironmentBlock(lpEnvironment);
				}
				else
				{
					// Error on CreateEnvironmentBlock 
					dwError = GetLastError();
					wsprintf(strReport, "Error: (%d) on CreateEnvironmentBlock", dwError);
					::AddToMessageLog(MSG_RM_AUTOUPDATE, TEXT(strReport));
					bOpenShutDownDlg = FALSE;
				}
			}
			else
			{
				// Error on GetToken 
				::AddToMessageLog(MSG_RM_AUTOUPDATE, TEXT("GetSessionUserToken failed"));
				hDlg = ::DisplayUpdateMessage();
			}
		}

		int32 i32Status = CS_SUCCESS;
		if ( bUpdateFw )
		{
			// Only get the Retry Count from the registry the first
			// time we initialize

			uInt8	u8RetryCount = 1;	// This should be configurable from registry
			u8RetryCount = ::GetAutoFirmwareUpdateCount();
					
			while ( u8RetryCount > 0 )
			{
				// Begin autoupdate thread.  This thread autoupdates the driver firmware while
				// we check the message queue so the message can be refreshed.
				HANDLE hFirmwareUpdateThread = (HANDLE)::_beginthreadex(NULL, 0, UpdateFirmwareProc, (void *)this, 0, 0);

				while (WAIT_OBJECT_0 != WaitForSingleObject(hFirmwareUpdateThread, 50))
				{
					::CheckMessageQueue();
				}
				::CloseHandle(hFirmwareUpdateThread);

				// if the error status was CS_FWUPDATED_SHUTDOWN_REQUIRED we
				// were successful but we need to reboot so we'll break the loop 
				if ( GetFirmwareUpdateStatus() == CS_FWUPDATED_SHUTDOWN_REQUIRED )
				{
					break;
				}
				i32Status = m_pCsDrvGetAcquisitionSystemCount(i16HardReset, (uInt16 *) &i16NumberOfSystems);
				if ( CS_SUCCEEDED (i32Status) ) 	break;
				u8RetryCount--;
			}

			// Kill message
			if (NULL != hDlg)
				::DestroyUpdateMessage(hDlg);

			if (NULL != hProcess)
			{
				::TerminateProcess(hProcess, 1);
				if ( NULL != hThread )
				{
					::CloseHandle(hThread);
					hThread = NULL;
				}
				::CloseHandle(hProcess);
				hProcess = NULL;
			}
		}
		if ( bOpenShutDownDlg && GetFirmwareUpdateStatus() == CS_FWUPDATED_SHUTDOWN_REQUIRED )
		{
			PVOID lpEnvironment(NULL);
			TCHAR  strReport[50];
			PROCESS_INFORMATION	pi;
			ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
			if ( ::CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE) ) 
			{
				TCHAR str[MAX_PATH];
				lstrcpy(str, pathSystemDirectory);
				lstrcat(str, " restart");
				DWORD dwFlags = BELOW_NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
				STARTUPINFO	si;
				ZeroMemory( &si, sizeof( STARTUPINFO ) );
				si.cb = sizeof( STARTUPINFO );
				si.lpDesktop = "winsta0\\default";
				if (!::CreateProcessAsUser(hToken, NULL, TEXT(str), NULL, NULL, FALSE, dwFlags, lpEnvironment, NULL, &si, &pi))
				{
					wsprintf(strReport, "Error: (%d) on auto-update %s", GetLastError(), pathSystemDirectory);
					::AddToMessageLog(MSG_RM_AUTOUPDATE, TEXT(strReport));
				}
				else
				{	
					hProcess = pi.hProcess;
					hThread = pi.hThread;					
				}
				if (lpEnvironment) 
					::DestroyEnvironmentBlock(lpEnvironment);
			}
			else
			{
				// Error on CreateEnvironmentBlock 
				dwError = GetLastError();
				wsprintf(strReport, "Error: (%d) on CreateEnvironmentBlock", dwError);
				::AddToMessageLog(MSG_RM_AUTOUPDATE, TEXT(strReport));
			}
			if ( NULL != hProcess )
			{
				// if we've reached here CsAutoUpdate should be closing. We'll wait
				// for it to close so we don't start the applicatin that brought us here
				DWORD dwRet = ::WaitForSingleObject(hProcess, INFINITE);
				if ( NULL != hThread )
				{
					::CloseHandle(hThread);
				}
				::CloseHandle(hProcess);
			}
		}
		::CloseHandle(hToken);
		// make number of systems 0 so no systems are found unless the user reboots
		// only if the return value was SHUTDOWN_REQUIRED
		if ( GetFirmwareUpdateStatus() == CS_FWUPDATED_SHUTDOWN_REQUIRED )
		{
			i16NumberOfSystems = 0;
		}
	}

#endif  // the else part of #ifdef __linux__

	return i16NumberOfSystems;
}

#if defined (_WIN32)
void  CsResourceManager::CsRmUsbNotify(long lNote)
{

	switch( lNote )
	{
		case  1: 
//			CsRmInitialize(1); //RG
			::SetEvent( m_hUsbCommEventHandle ); 
			OutputDebugString( " Notify Addition of USB CompuScope" ); 
			break;

		case -1: 
//			CsRmInitialize(1); //RG
			OutputDebugString( " Notify Removal of USB CompuScope" ); break;
		default: break;
	}
}

#endif
