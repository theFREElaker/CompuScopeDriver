// RM.cpp : Defines the entry point for the console application.
//
#if defined (_WIN32)
	#include <crtdbg.h>
#else
	#include <signal.h>
	#include <string.h>
	#include <stdio.h> // for printf
	#include <pthread.h>
	#include <unistd.h> // for daemon
#endif

#include "rm.h"
#include "ThreadDispatcher.h"
#include "RawSocketServerWorker.h"
#include "Worker.h"
#include <vector>

#if defined (_WIN32)
	#include <string>
	#include <shlobj.h> // for IsUserAnAdmin
	#include "CsResourceManager.h"
	#include "Debug.h"
#else
	#include "xml.h"
#endif

using namespace std;

extern bool	bDebug;


#if defined (_WIN32)

#include "Service.h"
#include "RMmsglog.h"
#include "CsSupport.h"

#endif

#include "DllStruct.h"
typedef vector <PSHAREDDLLMEM> VECSHAREDDLLMEM; // move somewhere else


#if defined (_WIN32)

HANDLE g_hCloseEvent = NULL;
PSHAREDDLL_CRITICALSECTION g_DriverDllCriticalSec;


DWORD GetNumberOfRegEntries(HKEY hKey)
{
    CHAR     achClass[MAX_PATH] = "";	// buffer for class name
    DWORD    cchClassName = MAX_PATH;	// length of class string
    DWORD    cSubKeys;					// number of subkeys
    DWORD    cbMaxSubKey;				// longest subkey size
    DWORD    cchMaxClass;				// longest class string
    DWORD    cchMaxValue;				// longest value name
    DWORD    cbMaxValueData;			// longest value data
    DWORD    cbSecurityDescriptor;		// size of security descriptor
    FILETIME ftLastWriteTime;			// last write time
    DWORD    cValues = 0;				// number of values for key

	// Get the class name and the value count.
	if (ERROR_SUCCESS != ::RegQueryInfoKey(hKey, achClass, &cchClassName, NULL, &cSubKeys, &cbMaxSubKey, &cchMaxClass, &cValues, &cchMaxValue, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime))
	{
		cValues = 0; // just to make sure that it's 0 in case of error
	}
	return cValues;
}


BOOL MapDriverDlls(VECSHAREDDLLMEM& vecMemMapFiles, PREMOTESYSTEM pRemoteSystem)
{
	TCHAR	Subkey[128] = GAGE_SW_DRIVERS_REG;
	HKEY	hKey;
	
	DWORD dwFlags = KEY_READ;

	if (Is32on64())
		dwFlags |= KEY_WOW64_64KEY;

	unsigned long ulStatus = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, Subkey, 0, dwFlags, &hKey);

	if (ERROR_SUCCESS != ulStatus)
	{
		::AddToMessageLog(MSG_RM_OTHER_ERROR, TEXT("Could not open Gage drivers registry key."));
		TRACE(RM_TRACE_LEVEL, "Error opening Gage Drivers in registry (WinError = %x)\n", ulStatus );

		return FALSE;
	}

	DWORD dwCount = GetNumberOfRegEntries(hKey);
	if (0 == dwCount)
	{
		::AddToMessageLog(MSG_RM_OTHER_ERROR, TEXT("No Gage drivers in the registry."));
		::RegCloseKey(hKey);
		return FALSE;
	}

	// Get the DLL file names from registry
	for(DWORD i = 0; i < dwCount; i++)
	{
		TCHAR	strValueBuffer[_MAX_PATH];
		unsigned long ulDataBuffer;
		unsigned long ulValueBuffSize = sizeof(strValueBuffer);
		unsigned long ulDataBuffSize = sizeof(ulDataBuffer);
		DWORD	dwDataType;

		int nRetVal = ::RegEnumValue(hKey, i, strValueBuffer, &ulValueBuffSize, NULL, &dwDataType, (LPBYTE)&ulDataBuffer, &ulDataBuffSize);
		if (nRetVal == ERROR_NO_MORE_ITEMS)	break;
		if (dwDataType != REG_DWORD) continue;

		// Convert dll name to lower case
		::_strlwr_s(strValueBuffer, _countof(strValueBuffer));

		string str(strValueBuffer);
		size_t nPos = str.find_first_of(_T("."));
		if ( -1 == nPos ) // proper name is not there so skip over that one
		{
			continue;
		}
		str.erase(str.begin() + nPos, str.end());
		str.append(_T("-shared"));
		PSHAREDDLLMEM pDllSharedMem = new SHAREDDLLMEM(str.c_str());

		if (!pDllSharedMem->IsMapped())
		{
			TCHAR strError[100];
			_stprintf_s(strError, _countof(strError),
				pDllSharedMem->IsCreated()? _T("Could not map shared memory with %s.") : _T("Could not create shared memory with %s."),
				strValueBuffer);
			::AddToMessageLog(MSG_RM_SHARED_MEM, strError);
			continue;
		}
		// if we've mapped the DLL for remote connections, we also need to map the structure that
		// it will use for socket connections because the structure is shared between CsRm.exe and whatever
		// application is called
		if ( _T("csdevremote-shared") == str ) 
		{
			pRemoteSystem = new REMOTESYSTEM(CS_FCI_SHARED_MEM);
			if (!pRemoteSystem->IsMapped())
			{
				TCHAR strError[100];
				_stprintf_s(strError, _countof(strError),
						_T("%s"), pRemoteSystem->IsCreated()? _T("Could not map shared memory for CsFCI.") : _T("Could not create shared memory for CsFCI."));
				::AddToMessageLog(MSG_RM_SHARED_MEM, strError);
			}
		}
		vecMemMapFiles.push_back(pDllSharedMem);
	}
	::RegCloseKey(hKey);

	// Create Driver Dll critical section.
	// The global critical section will be used by all driver Dlls
	// The critical section is required to prevent race condition between Rm64 and Rm32 when loading and unloading driver Dlls
	g_DriverDllCriticalSec = new SHAREDDLL_CRITICALSECTION(SHAREDDLL_CRITICALSECTION_NAME, false, true);
	if (!g_DriverDllCriticalSec->IsMapped())
	{
		TCHAR strError[100];
		_stprintf_s(strError, _countof(strError),
			g_DriverDllCriticalSec->IsCreated()? _T("Could not map shared memory with %s.") : _T("Could not create shared memory with %s."),
			SHAREDDLL_CRITICALSECTION_NAME);
		::AddToMessageLog(MSG_RM_SHARED_MEM, strError);
	}

	return TRUE;
}

void UnmapDriverDlls(VECSHAREDDLLMEM& vecMemMapFiles)
{
	for (VECSHAREDDLLMEM::iterator it = vecMemMapFiles.begin(); it !=  vecMemMapFiles.end(); it++)
		delete (*it);
	vecMemMapFiles.clear();

	delete g_DriverDllCriticalSec;
}

#if defined(_WIN32)
CWorker* g_pWorker(NULL);

void UsbNotifyRm(long lNote)
{ 
	if( NULL != g_pWorker && NULL != g_pWorker->GetRm())
		g_pWorker->GetRm()->CsRmUsbNotify(lNote);
}
#endif

bool RunService()
{
	VECSHAREDDLLMEM vecMemMapFiles;
	SharedMem< SHARED_DISP_GLOBALS > DispGlobal(DISP_SHARED_MEM_FILE);
		
	if (!DispGlobal.IsMapped())
	{
		::AddToMessageLog(MSG_RM_SHARED_MEM, DispGlobal.IsCreated()? TEXT("Could not map shared memory with CsDisp."):TEXT("Could not create shared memory with CsDisp."));
		if (!::IsUserAnAdmin() & bDebug)
		{
			// if we're not admin and we're using CsRm in a console window
			// print out an error message
			printf(_T("CsRm can only be run by an administrator."));
			printf(_T("\n"));
		}
		return false;
	}

	PREMOTESYSTEM pRemoteFci = NULL;
	if (!MapDriverDlls(vecMemMapFiles, pRemoteFci))
	{
		::AddToMessageLog(MSG_RM_SHARED_MEM, TEXT("Error mapping driver dll"));
		return false;
	}

	g_pWorker = new CWorker;
	CWizThreadDispatcher dispatcher (*g_pWorker, 6);
	dispatcher.Start();

	::AddToMessageLog(MSG_RM_STARTED, (LPTSTR)"");
	// Wait infinitely for hCloseEvent to be set.  This
	// will simulate an endless loop.
	// hCloseEvent will be set in StopService, either when control-c
	// is pressed in debug mode, or when a ServiceStop message is sent
	// to the Service Controller.

	// Changed event so it is not named. Named events were causing problems
	// because the 32 bit RM and the 64 bit RM were sharing it and if you
	// stopped one of them the other got confused.
	g_hCloseEvent = ::CreateEvent(NULL, 0, 0, NULL);

	::WaitForSingleObject(g_hCloseEvent, INFINITE);

	// Start to close down the server
	::CloseHandle(g_hCloseEvent);
	dispatcher.SetShutDownEvent();
	dispatcher.WaitShutDown();

	if ( NULL != pRemoteFci )
	{
		delete pRemoteFci;
	}
	UnmapDriverDlls(vecMemMapFiles);
	
	return true;
}

VOID ServiceStop()
{
    // Wakes the main thread.

	// Don't bother with the return value. At this state we
	// want to set our close event to wake the main thread 
	// regardless of whether or not ReportStatusToSCMgr passes or fails
	ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, RM_WAIT_HINT);
	
	::AddToMessageLog(MSG_RM_STOPPED, (LPTSTR)"");
	::SetEvent(g_hCloseEvent);

}

VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
	WSADATA wsaData;
	int err;

#if defined (_DEBUG) || defined  (RELEASE_TRACE)
	CTraceManager g_TraceMngr;
#endif

	dwArgc = dwArgc;			// Get rid of compiler warnings
	lpszArgv = lpszArgv;

    if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, RM_WAIT_HINT))
	{
        return;
	}

	WORD wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if (err != 0)
	{
		// Tell the user that we could not find a usable 
		// WinSock DLL.       
		::AddToMessageLog(MSG_RM_CANT_START, TEXT("Could not find a usable WinSock DLL."));
		return;
	}

    // Report the status to the service control manager.
    if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0))
	{
        return;
	}

	if (!RunService())
	{
		::MessageBox(NULL, "Unable to start Gage Resource Manager Service", "ERROR", MB_OK);
		// Write to EventViewer
		::AddToMessageLog(MSG_RM_CANT_START, TEXT("Gage Resource Manager failed to start."));
	}
	::WSACleanup();
   
	if (!ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR,0))
	{
        return;
	}

	return;
}

#else // _WIN32

pthread_cond_t finish_cond = PTHREAD_COND_INITIALIZER;

void ServiceStop(int sig)
{

	pthread_cond_broadcast(&finish_cond);
	AddToMessageLog(MSG_RM_STOPPED, "Gage Resource Manager stopped.");
}

//TODO: RG - Combine MapDriverDlls and UnmapDriverDlls with the windows versions of the functions
BOOL MapDriverDlls(VECSHAREDDLLMEM& vecMemMapFiles)
{
	char 	filePath[_MAX_PATH];
	uInt16 	cValues = 0;

/*
	char* homePath = getenv("HOME");
	strncpy(filePath, homePath, 250);
*/
	strncpy(filePath, XML_FILE_PATH, 250);
	strncat(filePath, "/GageDriver.xml", _MAX_PATH);

	XML *pDrivers = new XML(filePath);

	if (!pDrivers)
	{
		fprintf(stderr, "Error reading %s\n", filePath);
		return 0;
	}

	XMLElement* pRoot = pDrivers->GetRootElement();
	if (!pRoot)
	{
		fprintf(stderr, "Error finding drivers in %s\n", filePath);
		delete pDrivers;
		return 0;
	}

	cValues = pRoot->GetChildrenNum();
	if (0 == cValues)
	{
		fprintf(stderr, "Error finding drivers in %s\n", filePath);
		delete pDrivers;
		return 0;
	}
	if ( cValues > MAX_DRIVERS )
	{
		cValues = MAX_DRIVERS;
//		TRACE(DISP_TRACE_LEVEL, "DISP::WARNING : Number of drivers exceeds the maximum number supported.\n");
	}

	XMLElement** pDriverList = pRoot->GetChildren();
	if (!pDriverList)
	{
		fprintf(stderr, "Error finding drivers in %s\n", filePath);
		delete pDrivers;
		return 0;
	}

	cschar strDriverName[MAX_PATH];
	cschar strSharedMemName[MAX_PATH];
	cschar buf[MAX_PATH];
	for(uInt16 i = 0; i< cValues; i++)
	{
		XMLVariable* pDriverName = pDriverList[i]->FindVariableZ((char *)"name");

		if ((NULL == pDriverName) || (-1 == pDriverName->GetValue(strDriverName)))
		{
			pDriverList[i]->GetElementName(strDriverName);
		}
		strncpy(strSharedMemName, strDriverName, _countof(strDriverName));
		strcat(strSharedMemName, _T("-shared"));

		// just in case the shared memory is still there for the dll, let's
		// erase it before we create it again.
		// Note: this will have to be reworked if, for example, we add a 
		// demo system and reinitialize RM (without stopping and restarting)
		sprintf(buf, "%s/%s", "/dev/shm", strSharedMemName); // added Dec 11, 2008
		unlink(buf); // added Dec 11, 2008

		PSHAREDDLLMEM pDllSharedMem = new SHAREDDLLMEM(strSharedMemName);
		if (!pDllSharedMem->IsMapped())
		{
			TCHAR strError[100];
			_stprintf_s(strError, _countof(strError),
			pDllSharedMem->IsCreated()? _T("Could not map shared memory with %s.") : _T("Could not create shared memory with %s."),
										strDriverName);
			fprintf(stderr, "%s\n", strError);
			continue;

		}
		vecMemMapFiles.push_back(pDllSharedMem);
	}

	delete pDrivers;

	return TRUE;
}

void UnmapDriverDlls(VECSHAREDDLLMEM& vecMemMapFiles)
{
	for (VECSHAREDDLLMEM::iterator it = vecMemMapFiles.begin(); it !=  vecMemMapFiles.end(); it++)
	{
		// just in case there are still references to the shared memory

		int nReferenceCount = (*it)->GetRefCount();
		for (int i = 0; i < nReferenceCount; i++)
		{
			if (munmap((*it)->it(), sizeof(SHAREDDLLMEM)) == -1)
			{
				perror("munmap failed: ");
			}
		}
		// once all references are unmapped we can unlink the shared memory
		if (shm_unlink((*it)->GetMappedName()) == -1)
		{
			perror("shm_unlink failed: ");
		}
		delete (*it);
	}
}


int main(int argc, char **argv)
{
	openlog("csrmd", LOG_PID | LOG_NDELAY, LOG_USER);

#ifdef _MAKE_DAEMON // added a dash to the front so it's turned off for now
	int err = daemon(1, 1);
	if (err)
	{
		printf("Error in daemon: %d\n", err);
		//AddToMessageLog(MSG_RM_CANT_START, TEXT("Gage Resource Manager failed to start."));
		return 0;
	}
#endif

	// Add a signal handler for control-c
	// so we end gracefully
	struct sigaction sAction;
	memset(&sAction, 0, sizeof(sAction));
	sAction.sa_handler = &ServiceStop;
	sAction.sa_flags = SA_ONESHOT;
	sigaction(SIGINT, &sAction, NULL);

	// Add a signal handler for the terminate signal
	// so if the process is killed we can try to clean up
	struct sigaction sTerminate;
	memset(&sTerminate, 0, sizeof(sTerminate));
	sTerminate.sa_handler = &ServiceStop;
	sTerminate.sa_flags = SA_ONESHOT;
	sigaction(SIGTERM, &sTerminate, NULL);

	// if shared memory files for RM and DISP exist, try to erase them
	// they should have been created with read/write priviliges for
	// everyone.
	// Note: this will have to be reworked if, for example, we add a 
	// demo system and reinitialize RM (without stopping and restarting)
	// We still need to remove the hardware dll's as well
	char buf[MAX_PATH];
	sprintf(buf, "%s/%s", "/dev/shm", DISP_SHARED_MEM_FILE); //added Dec.11
	unlink(buf);
	sprintf(buf, "%s/%s", "/dev/shm", RM_SHARED_MEM_FILE); // added Dec. 11
	unlink(buf);
	

	VECSHAREDDLLMEM vecMemMapFiles;
	SharedMem< SHARED_DISP_GLOBALS > DispGlobal(DISP_SHARED_MEM_FILE);

	if ( !DispGlobal.IsMapped() )
	{
		fprintf(stderr, "Could not create shared memory region with CsDisp\n");
		// check if user is not root and print out message ??
		return false;
	}

	if ( !MapDriverDlls(vecMemMapFiles) )
	{
		return false;
	}

	CWorker worker;
	CWizThreadDispatcher dispatcher (worker, 6);
	dispatcher.Start();

	AddToMessageLog(MSG_RM_STARTED, (LPTSTR)"Gage Resource Manager started.");

	pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mut);
	pthread_cond_wait(&finish_cond, &mut);
	pthread_mutex_unlock(&mut);

	// Start to close down the server
	dispatcher.SetShutDownEvent();
	dispatcher.WaitShutDown();

	UnmapDriverDlls(vecMemMapFiles);

//  This part has to be checked.  I'm not sure that it's
//  working properly. 
	int nReferenceCount = DispGlobal.GetRefCount();

	for (int i = 0; i < nReferenceCount; i++)
	{
		if (munmap(DispGlobal.it(), sizeof(DispGlobal)) == -1)
		{
			perror("munmap failed: ");
		}
	}
	// once all references are unmapped we can unlink the shared memory
	if (shm_unlink( DispGlobal.GetMappedName() ) == -1)
	//if (shm_unlink((*it)->GetMappedName()) == -1)
	{
		perror("shm_unlink failed: ");
	}

	closelog();
	printf("\n");
	return 0;
}

#endif // _WIN32

